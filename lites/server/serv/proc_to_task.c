/* 
 * Mach Operating System
 * Copyright (c) 1994 Johannes Helander
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * JOHANNES HELANDER ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  JOHANNES HELANDER DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: proc_to_task.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/03/23  01:44:05  law
 * Update to 950323 snapshot + utah changes
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	proc_to_task.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	Map tasks and request ports to processes.
 */

#include "data_synch.h"

#include <serv/server_defs.h>
#include <serv/synch_prim.h>
#include <sys/file.h>

/*
 * Enter task and process in hash table.
 * Allocate request port for process, and return it.
 */
mach_port_t task_to_proc_enter(
	task_t		task,
	struct proc *	p)
{
	mach_port_t	req_port;
	kern_return_t	kr;

	proc_ref(p);		/* XXX should be in p_o_a_r ? */
	kr = port_object_allocate_receive(&req_port, POT_PROCESS, p);
	assert(kr == KERN_SUCCESS);

	kr = port_object_make_send(req_port);
	assert(kr == KERN_SUCCESS);
	p->p_req_port = req_port;

	proc_ref(p);		/* XXX should be in function */
	kr = port_object_enter_send(&task, POT_TASK, p);
	assert(kr == KERN_SUCCESS);
	p->p_task = task;

	proc_ref(p);		/* XXX should be in function */
	kr = port_object_enter_send(&p->p_thread, POT_THREAD, p);
	assert(kr == KERN_SUCCESS);

	ux_server_add_port(req_port);
	return req_port;
}

/* Port Object methods for processes */

/* Lock p */
void proc_lock(struct proc *p)
{
	mutex_lock(&p->p_lock);
}

/* Add a ref. p must be locked */
void proc_ref(struct proc *p)
{
	p->p_ref++;
}

/* Release a ref. Consumes lock. */
void proc_deref(struct proc *p, mach_port_t port)
{
	kern_return_t kr;

	if (p->p_ref == 1) {
		/* Destroy p */
		proc_invocation_t pk = get_proc_invocation();

		mutex_unlock(&p->p_lock);
		unix_master();
#if 0
		mutex_lock(&p->p_lock);
		p->p_ref--;
#endif
		/* 
		 * proc_died has to close file descriptors etc.
		 * Sometimes that results in tsleep being called and
		 * in that case there must be a process to sleep on.
		 * We don't want to sleep on the died proc though as
		 * that would make server_thread_deregister a total
		 * mess. So sleep on proc0 instead. proc0 never dies.
		 */
		server_thread_register_locked(&proc0);
		proc_died(p); /* consumes p and last ref */
		server_thread_deregister(&proc0);
		unix_release();
	} else {
		p->p_ref--;
		assert(p->p_ref >= 1);
		condition_broadcast(&p->p_condition);
		mutex_unlock(&p->p_lock);
	}
}

/* Task dereferensing kills the process regardless of counts. Consumes lock */
void task_deref(struct proc *p, mach_port_t port)
{
	proc_deref(p, port);
}

/* Remove proc to process port mapping */
void proc_remove_reverse(struct proc *p, mach_port_t port)
{
	mutex_lock(&p->p_lock);
	if (p->p_req_port ==  port)
	    p->p_req_port = MACH_PORT_NULL;
	mutex_unlock(&p->p_lock);
}

/* Remove task to process mapping */
void task_remove_reverse(struct proc *p, mach_port_t port)
{
	kern_return_t kr;

	mutex_lock(&p->p_lock);
	if (p->p_task == port) {
		/* Prepare for termination */
		p->p_flag &= ~(P_TRACED|P_PPWAIT);
		p->p_flag |= P_WEXIT;
		p->p_sigignore = ~0;
		p->p_siglist = 0;
		p->p_task = MACH_PORT_NULL;
		/* Wake up all server threads to make them go away */
#if DATA_SYNCH
		proc_wakeup_unlocked(p, PKW_EXIT);
#else
		condition_broadcast(&p->p_condition);
		proc_wakeup(p);
#endif
		mutex_unlock(&p->p_lock);
	} else {
		mutex_unlock(&p->p_lock);
	}
	/* Remove reference from newproc */
	kr = mach_port_deallocate(mach_task_self(), port);
	assert(kr == KERN_SUCCESS);
	/* and from the dead name notification.  XXX -> po module */
	kr = mach_port_deallocate(mach_task_self(), port);
	assert(kr == KERN_SUCCESS);
}

void thread_remove_reverse(struct proc *p, mach_port_t port)
{
	kern_return_t kr;

	mutex_lock(&p->p_lock);
	if (p->p_thread == port)
	    p->p_thread = MACH_PORT_NULL;

	/* Remove reference that came from newproc */
	kr = mach_port_deallocate(mach_task_self(), port);
	assert(kr == KERN_SUCCESS);
	/* and from the dead name notification.  XXX -> po module */
	kr = mach_port_deallocate(mach_task_self(), port);
	assert(kr == KERN_SUCCESS);

	mutex_unlock(&p->p_lock);
}

void sigport_remove_reverse(struct proc *p, mach_port_t port)
{
	kern_return_t kr;

	mutex_lock(&p->p_lock);
	/* One port by user checkin and one from the notification */
	kr = mach_port_deallocate(mach_task_self(), port);
	assert(kr == KERN_SUCCESS);
	kr = mach_port_deallocate(mach_task_self(), port); /*XXX -> po module*/
	assert(kr == KERN_SUCCESS);
	p->p_sigport = MACH_PORT_NULL;
	mutex_unlock(&p->p_lock);
}

/* PO methods for file handles */

void file_ref(struct file *f)
{
/* 	p->p_ref++; */
}

void file_deref(struct file *f, mach_port_t port)
{
	boolean_t registered = FALSE;
	proc_invocation_t pk = get_proc_invocation();

	if (pk->k_p == 0) {
		server_thread_register_locked(&proc0);
		unix_master();
		registered = TRUE;
	}
	
	closef(f, &proc0);
	if (registered) {
		unix_release();
		server_thread_deregister(&proc0);
	}
}

void file_lock(struct file *f)
{
/* 	mutex_lock(&f->f_lock); */
}

void file_remove_reverse(struct file *f, mach_port_t port)
{
	/* XXX this is at the moment the only place the lock is taken */
	mutex_lock(&f->f_lock);
	if (f->f_port == port)
	    f->f_port = MACH_PORT_NULL;
	mutex_unlock(&f->f_lock);
}
