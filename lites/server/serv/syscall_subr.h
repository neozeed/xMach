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
 * $Log: syscall_subr.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:17:04  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File: serv/syscall_subr.h
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Glue to make MiG procedure interfaces look like old UX system
 *	calls.
 */

extern void server_thread_register(struct proc *p);
extern void server_thread_deregister(struct proc *p);
extern int start_server_op(struct proc *, int syscode);
extern int end_server_op(struct proc *p, int error, boolean_t *interrupt);
#define proc_receive_lookup(port, seqno) \
    ((struct proc *) port_object_receive_lookup(port, seqno, POT_PROCESS))

#define	START_SERVER(syscode, nargs)			\
	mach_error_t	error;				\
	integer_t	arg[nargs];			\
        struct proc *p = proc_receive_lookup(proc_port, seqno); \
	proc_invocation_t pk = get_proc_invocation();	\
        int was_serial = TRUE;				\
	pk->k_p = p;					\
							\
	error = start_server_op(p, syscode);		\
	if (error)					\
	    return (error);				\
	pk->k_reply_msg = 0;				\
	if (was_serial)					\
	    unix_master();				\
	{

#define	END_SERVER(syscode)				\
	}						\
	if (was_serial)					\
	    unix_release();				\
	error = end_server_op(p, error, (boolean_t *)0);\
	pk->k_p = NULL;					\
	return error;

#define	END_SERVER_DEALLOC(syscode, data, size)		\
	}						\
        unix_release();		 		        \
       (void) vm_deallocate(mach_task_self(), 		\
                           (vm_address_t)data, (vm_size_t)size); \
	return (end_server_op(p, error, (boolean_t *)0));

#define	END_SERVER_DEALLOC_ON_ERROR(syscode, data, size)\
	}						\
        unix_release();		 		        \
	error = end_server_op(p, error, (boolean_t *)0);\
	if (error) {					\
		(void) vm_deallocate(mach_task_self(),	\
			(vm_address_t)data, (vm_size_t)size); \
	} \
	return error;
