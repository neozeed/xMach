/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994 Johannes Helander
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND JOHANNES HELANDER ALLOW FREE USE OF THIS
 * SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND JOHANNES
 * HELANDER DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: serv_fork.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.3  1995/03/28  22:47:54  law
 * Import lites-1.1 release.
 *
 */
/* 
 *	File:	serv/serv_fork.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 * 	fork.
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_fork.c	8.6 (Berkeley) 4/8/94
 */

#include "ktrace.h"
#include "map_uarea.h"
#include "machid_register.h"
#include "second_server.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/ktrace.h>
#include <sys/synch.h>
#include <serv/import_mach.h>
#include <sys/assert.h>

int max_pid = 0;

void proc_set_condition_names(struct proc *p);

#if MAP_UAREA
void alloc_mapped_uarea(struct proc *p);
#endif

#if MACHID_REGISTER
extern int machid_register_process(struct proc *p); /* kern_exec.c XXX tri */
#endif

struct proc *newproc(struct proc *, boolean_t, boolean_t);

fork()
{
	panic("fork called");
}

vfork()
{
	panic("vfork called");
}

int	nprocs = 1;		/* process 0 */

s_fork(
	struct proc	*p1,
	thread_state_t	new_state,
	unsigned int	new_state_count,
	int		*child_pid,
	boolean_t	isvfork)
{
	struct proc *p2;
	int count;
	uid_t uid;

	/*
	 * Although process entries are dynamically created, we still keep
	 * a global limit on the maximum number we will create.  Don't allow
	 * a nonprivileged user to use the last process; don't let root
	 * exceed the limit. The variable nprocs is the current number of
	 * processes, maxproc is the limit.
	 */
	uid = p1->p_cred->p_ruid;
	if ((nprocs >= maxproc - 1 && uid != 0) || nprocs >= maxproc) {
		tablefull("proc");
		return EAGAIN;
	}
	/*
	 * Increment the count of procs running with this uid. Don't allow
	 * a nonprivileged user to exceed their current limit.
	 */
	count = chgproccnt(uid, 1);
	if (uid != 0 && count > p1->p_rlimit[RLIMIT_NPROC].rlim_cur) {
		(void)chgproccnt(uid, -1);
		return EAGAIN;
	}

	/*
	 * Create new user process, with task.
	 */
	p2 = newproc(p1, FALSE, isvfork);
	if (p2 == 0) {
	    /*
	     * No tasks or threads available.
	     */
	    return EAGAIN;
	}

	/*
	 * Clone the parent's registers, but mark it as the child
	 * process.
	 */
	if (!thread_dup(p2->p_thread,
			new_state, new_state_count,
			p1->p_pid, 1))	{
	    task_terminate(p2->p_task);
	    return EFAULT;
	}

	/*
	 * Child process.  Set start time and get to work.
	 */
	get_time(&p2->p_stats->p_start);
	p2->p_acflag = AFORK;

	/*
	 * And start it.
	 */
	(void) thread_resume(p2->p_thread);

	/*
	 * Preserve synchronization semantics of vfork.  If waiting for
	 * child to exec or exit, set P_PPWAIT on child, and sleep on our
	 * proc (in case of exit).
	 */
	if (isvfork)
		while (p2->p_flag & P_PPWAIT)
			tsleep(p1, PWAIT, "ppwait", 0);

	*child_pid = p2->p_pid;
	return KERN_SUCCESS;
}

struct proc *proc_free_list = 0;

void proc_free(struct proc *p)
{
    if (p->p_ref != 0)
	panic("proc_free");

    mutex_lock(&allproc_lock);
    p->p_next = proc_free_list;
    proc_free_list = p;
    mutex_unlock(&allproc_lock);
}

int nprocs_allocated = 0;

void proc_allocate(np)
    struct proc **np;
{
	mutex_lock(&allproc_lock);

	nprocs++;
	if (proc_free_list) {
		*np = proc_free_list;
		proc_free_list = proc_free_list->p_next;
	} else {
		nprocs_allocated++;
		if (nprocs_allocated >= maxproc)
		    panic("proc_allocate");
		*np = (struct proc *) malloc(sizeof(struct proc));
		assert(*np);
		bzero(*np, sizeof(struct proc));
#if MAP_UAREA
		alloc_mapped_uarea(*np);
#endif

	}
	(*np)->p_next = allproc;
	(*np)->p_next->p_prev = &(*np)->p_next;	/* allproc is never NULL */
	(*np)->p_prev = &allproc;
	allproc = (*np);
	(*np)->p_forw = NULL;			/* shouldn't be necessary */
	(*np)->p_back = NULL;			/* shouldn't be necessary */

	mutex_unlock(&allproc_lock);
}


#if	MAP_UAREA
kern_return_t
mapin_user(p)
	register struct proc *p;
{
	vm_address_t	user_addr = EMULATOR_END - vm_page_size;
	kern_return_t	kr;
	extern mach_port_t	shared_memory_port;
	
	assert(p->p_task != mach_task_self());

	kr = vm_map(p->p_task, &user_addr, vm_page_size, 0,
		    FALSE, shared_memory_port,
		    p->p_shared_off + 3*vm_page_size, FALSE, VM_PROT_READ,
		    VM_PROT_READ, VM_INHERIT_NONE);
	if (kr != KERN_SUCCESS) {
		panic("mapin_user ro: ua=x%x off=x%x %s", user_addr,
		      p->p_shared_off + 3*vm_page_size, mach_error_string(kr));
		return kr;
	}
	user_addr -= 3*vm_page_size;
	kr = vm_map(p->p_task, &user_addr, 3*vm_page_size, 0,
		    FALSE, shared_memory_port, p->p_shared_off,
		    FALSE, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE, VM_INHERIT_NONE);
	if (kr != KERN_SUCCESS) {
		panic("mapin_user rw: ua=x%x off=x%x %s", user_addr,
		      p->p_shared_off, mach_error_string(kr));
	}
	return kr;
}
#endif	MAP_UAREA

struct proc *
newproc(struct proc *p1, boolean_t is_sys_proc, boolean_t isvfork)
{
	static int nextpid, pidchecked = 0;
	register struct proc *p2;
#if MAP_UAREA
	kern_return_t result;
#endif	

	/*
	 * Find an unused process ID.
	 * We remember a range of unused IDs ready to use
	 * (from nextpid+1 through pidchecked-1).
	 */
	nextpid++;
retry:
	/*
	 * If the process ID prototype has wrapped around,
	 * restart somewhat above 0, as the low-numbered procs
	 * tend to include daemons that don't exit.
	 */
	if (nextpid >= PID_MAX) {
		nextpid = 100;
		pidchecked = 0;
	}
	if (nextpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = PID_MAX;
		/*
		 * Scan the active and zombie procs to check whether this pid
		 * is in use.  Remember the lowest pid that's greater
		 * than nextpid, so we can avoid checking for a while.
		 */
		mutex_lock(&allproc_lock);
		p2 = allproc;
again:
		for (; p2 != NULL; p2 = p2->p_next) {
			while (p2->p_pid == nextpid ||
			    p2->p_pgrp->pg_id == nextpid) {
				nextpid++;
				if (nextpid >= pidchecked) {
				    mutex_unlock(&allproc_lock);
					goto retry;
				    }
			}
			if (p2->p_pid > nextpid && pidchecked > p2->p_pid)
				pidchecked = p2->p_pid;
			if (p2->p_pgrp->pg_id > nextpid && 
			    pidchecked > p2->p_pgrp->pg_id)
				pidchecked = p2->p_pgrp->pg_id;
		}
		if (!doingzomb) {
			doingzomb = 1;
			p2 = zombproc;
			goto again;
		}
		mutex_unlock(&allproc_lock);
	}

	max_pid = (max_pid>nextpid?max_pid:nextpid);

	/*
	 * Allocate new proc.
	 * Link onto allproc (this should probably be delayed).
	 */

    {
	struct proc *np;
	proc_allocate(&np);
	p2 = np;
    }

	/*
	 * Make a proc table entry for the new process.
	 * Start by zeroing the section of proc that is zero-initialized,
	 * then copy the section that is copied directly from the parent.
	 */
	bzero(&p2->p_startzero,
	    (unsigned) ((caddr_t)&p2->p_endzero - (caddr_t)&p2->p_startzero));
	bcopy(&p1->p_startcopy, &p2->p_startcopy,
	    (unsigned) ((caddr_t)&p2->p_endcopy - (caddr_t)&p2->p_startcopy));

	p2->p_ref = 0;
	p2->p_servers_count = 0;
	p2->p_pid = nextpid;
    {
	kern_return_t	result;
	mach_port_t		new_req_port, previous;

	result = task_create(p1->p_task,
#if OSF_LEDGERS
			     0, 0,
#endif
			     TRUE,
			     &p2->p_task);
	if (result != KERN_SUCCESS) {
	    printf("kern_fork:task create failure %x\n",result);
	    return 0;
	}
	result = thread_create(p2->p_task, &p2->p_thread);
	if (result != KERN_SUCCESS) {
	    (void) task_terminate(p2->p_task);
	    (void) mach_port_deallocate(mach_task_self(), p2->p_task);
	    printf("mach_fork:thread create failure %x\n",result);
	    return 0;
	}
	new_req_port = task_to_proc_enter(p2->p_task, p2);
	
	/*
	 * Insert the BSD request port for the task as
	 * its bootstrap port.
	 */
	result = task_set_bootstrap_port(p2->p_task,
					 new_req_port);
	if (result != KERN_SUCCESS)
	    panic("set bootstrap port on fork");

	/* XXX task_set_bootstrap_port port should be polymorphic! */
	/* XXX emulate MACH_MSG_TYPE_MOVE_SEND */
	result = mach_port_deallocate(mach_task_self(), new_req_port);
	assert(result == KERN_SUCCESS);

    }

#if	MAP_UAREA
	if ((result=mapin_user(p2)) != KERN_SUCCESS) {
	    printf("kern_fork:mapin_user %x\n",result);
	    panic("kern_fork");
	    return 0;
	}
	bcopy(p1->p_shared_ro,p2->p_shared_ro,sizeof(struct ushared_ro));
	bcopy(p1->p_shared_rw,p2->p_shared_rw,sizeof(struct ushared_rw));
	p2->p_shared_rw->us_inuse = 0;
	p2->p_shared_rw->us_debug = 0;
	share_lock_init(&p2->p_shared_rw->us_lock);
	p2->p_shared_ro->us_version = USHARED_VERSION;
	p2->p_shared_ro->us_proc_pointer = (vm_offset_t)p2;
	p2->p_sigmask = p1->p_sigmask;
	p2->p_sigignore = p1->p_sigignore;
	p2->p_siglist = 0;
	share_lock_init(&p2->p_shared_rw->us_siglock);
#endif	MAP_UAREA
	p2->p_spare[0] = 0;	/* XXX - should be in zero range */
	p2->p_spare[1] = 0;	/* XXX - should be in zero range */
	p2->p_spare[2] = 0;	/* XXX - should be in zero range */
	p2->p_spare[3] = 0;	/* XXX - should be in zero range */

	/*
	 * Duplicate sub-structures as needed.
	 * Increase reference counts on shared objects.
	 * The p_stats and p_sigacts substructs are set in vm_fork.
	 */
	MALLOC(p2->p_cred, struct pcred *, sizeof(struct pcred),
	    M_SUBPROC, M_WAITOK);
	bcopy(p1->p_cred, p2->p_cred, sizeof(*p2->p_cred));
	p2->p_cred->p_refcnt = 1;
	crhold(p1->p_ucred);

#if 0	/* XXX in original Lite code. Might be needed later */
	/* bump references to the text vnode (for procfs) */
	p2->p_textvp = p1->p_textvp;
	if (p2->p_textvp)
		VREF(p2->p_textvp);
#endif

	p2->p_fd = fdcopy(p1);
	/*
	 * If p_limit is still copy-on-write, bump refcnt,
	 * otherwise get a copy that won't be modified.
	 * (If PL_SHAREMOD is clear, the structure is shared
	 * copy-on-write.)
	 */
#if	MAP_UAREA
	p2->p_limit = &p2->p_shared_ro->us_limit;
	bcopy(p1->p_limit, p2->p_limit, sizeof(*p2->p_limit));
#else
	if (p1->p_limit->p_lflags & PL_SHAREMOD)
		p2->p_limit = limcopy(p1->p_limit);
	else {
		p2->p_limit = p1->p_limit;
		p2->p_limit->p_refcnt++;
	}
#endif	MAP_UAREA

	p2->p_flag = 0;
	if (p1->p_session->s_ttyvp != NULL && p1->p_flag & P_CONTROLT)
		p2->p_flag |= P_CONTROLT;
	p2->p_stat = SIDL;
	if (isvfork)
		p2->p_flag |= P_PPWAIT;
	p2->p_pid = nextpid;
	{
	struct proc **hash = &pidhash[PIDHASH(p2->p_pid)];

	p2->p_hash = *hash;
	*hash = p2;
	}
	p2->p_pgrpnxt = p1->p_pgrpnxt;
	p1->p_pgrpnxt = p2;
	p2->p_pptr = p1;
	p2->p_osptr = p1->p_cptr;
	if (p1->p_cptr)
		p1->p_cptr->p_ysptr = p2;
	p1->p_cptr = p2;
#if KTRACE
	/*
	 * Copy traceflag and tracefile if enabled.
	 * If not inherited, these were zeroed above.
	 */
	if (p1->p_traceflag&KTRFAC_INHERIT) {
		p2->p_traceflag = p1->p_traceflag;
		if ((p2->p_tracep = p1->p_tracep) != NULL)
			VREF(p2->p_tracep);
	}
#endif

	vm_fork(p1, p2, isvfork);

	p2->p_stat = SRUN;

	p2->p_servers_count = 0;
	queue_init(&p2->p_servers);

	condition_init(&p2->p_condition);
	mutex_init(&p2->p_lock);
	/* give condition and mutex names for debugging */
	proc_set_condition_names(p2);

#if MACHID_REGISTER
	machid_register_process(p2);
#endif /* MACHID_REGISTER */

	return p2;
}

void
proc_set_condition_names(struct proc *p)
{
	sprintf(p->p_lock_name, "p%d", p->p_pid);
	condition_set_name(&p->p_condition, p->p_lock_name);
	mutex_set_name(&p->p_lock, p->p_lock_name);
}
