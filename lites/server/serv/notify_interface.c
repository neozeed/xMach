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
 * $Log: notify_interface.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	notify_interface.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	January 1994.
 *
 * notify_server functions.
 * Dead name notifications are requested (in newproc) for each
 * process's task to the same process's request port (ie. bootstrap port).
 */

#include <serv/server_defs.h>
#include <serv/syscall_subr.h>

#define DEBUG_PRINT(s)

#define XMM_START_SERVER()				\
	struct proc *p = &proc0;			\
	proc_invocation_t pk;				\
	server_thread_register_locked(p);			\
	pk = get_proc_invocation();			\
	pk->k_reply_msg = 0;				\
	unix_master();					\
	{

#define XMM_END_SERVER()				\
	}						\
	server_thread_deregister(p);			\
	unix_release();					\
	return KERN_SUCCESS;

kern_return_t do_seqnos_mach_notify_port_deleted (
	mach_port_t		notify,
	mach_msg_seqno_t	seqno,
	mach_port_t		name)
{
	panic("do_seqnos_mach_notify_port_deleted called");
}

kern_return_t do_seqnos_mach_notify_msg_accepted (
	mach_port_t		notify,
	mach_msg_seqno_t	seqno,
	mach_port_t		name)
{
	panic("do_seqnos_mach_notify_msg_accepted called");
}

kern_return_t do_seqnos_mach_notify_port_destroyed (
	mach_port_t		notify,
	mach_msg_seqno_t	seqno,
	mach_port_t		rights,
	mach_msg_type_name_t	rightsPoly)
{
	panic("do_seqnos_mach_notify_port_destroyed called");
}

kern_return_t do_seqnos_mach_notify_no_senders (
	mach_port_t		notify,
	mach_msg_seqno_t	seqno,
	mach_port_mscount_t	mscount)
{
	proc_invocation_t pk = get_proc_invocation();
#if 0	/* Now set in proc_deref */
	/* p must be set or soo_close will crash when doing proc_died */
	struct proc *p = &proc0;
	server_thread_register_locked(p);
	/* XXX master_lock ! */
	/* master_lock is set when needed in task_deref. */
#endif
	pk->k_reply_msg = 0;
	port_object_no_senders(notify, seqno, mscount);
#if 0
	server_thread_deregister(p);
#endif
	return KERN_SUCCESS;
}

kern_return_t do_seqnos_mach_notify_send_once (
	mach_msg_seqno_t	seqno,
	mach_port_t		notify)
{
	panic("do_seqnos_mach_notify_send_once called");
}

/* A task died */
kern_return_t do_seqnos_mach_notify_dead_name (
	mach_port_t		notify,
	mach_msg_seqno_t	seqno,
	mach_port_t		name)
{
	proc_invocation_t pk = get_proc_invocation();
#if 0
	struct proc *died_proc;
	struct proc *p = &proc0;
	server_thread_register_locked(p);
	pk = get_proc_invocation();
	pk->k_reply_msg = 0;

	died_proc = proc_receive_lookup(notify, seqno);
	DEBUG_PRINT(("do_seqnos_mach_notify_dead_name(x%x x%x) proc=x%x\n",
		     notify, name, died_proc));
	if (died_proc) {
		mutex_unlock(&died_proc->p_lock); /* XXX */
		unix_master();	/* XXX */
		proc_died(died_proc, name);
	} else {
		panic("do_seqnos_mach_notify_dead_name: no proc");
	}

	server_thread_deregister(p);
	unix_release();
	return KERN_SUCCESS;
#else
#if 0
	/* p must be set or soo_close will crash when doing proc_died */
	struct proc *p = &proc0;
	server_thread_register_locked(p);
	/* XXX master_lock ! */
	/* master_lock is set when needed in task_deref. */
#endif
	pk->k_reply_msg = 0;
	port_object_dead_name(notify, seqno, name);

#if 0
	server_thread_deregister(p);
#endif
#endif
	return KERN_SUCCESS;
}
