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
 * $Log: syscall_subr.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.3  1996/03/14  21:08:37  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.2  1995/08/30  22:16:00  mike
 * hack rpc for GDB support
 *
 * Revision 1.1.1.2  1995/03/23  01:16:59  law
 * lites-950323 from jvh.
 *
 * 11-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Rely on HAVE_SIGNALS and CURSIG instead of using p_siglist
 *	directly.
 *
 */
/* 
 *	File: 	serv/syscall_subr.h
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Server thread service routines.
 */

#include "map_uarea.h"
#include "mutex_holder_assert.h"

#include <serv/server_defs.h>

#include <sys/signalvar.h>
#include <sys/ptrace.h>
#include <sys/systm.h>

#include <serv/bsd_msg.h>
#include <serv/syscalltrace.h>

#if	SYSCALLTRACE
extern int	nsysent;
int		syscalltrace = 0;
extern char	*syscallnames[];
#endif


/*
 * Register a server thread as serving a process.
 */
void
server_thread_register_locked(struct proc *p)
{
	proc_invocation_t pk = get_proc_invocation();

	mutex_lock(&p->p_lock);
	pk->k_p = p;
	pk->event = 0;
	queue_enter(&p->p_servers,
		    pk,
		    proc_invocation_t,
		    k_servers_chain);
	p->p_servers_count++;
	proc_ref(p);
	mutex_unlock(&p->p_lock);
}

void
server_thread_register_internal(struct proc *p)
{
	proc_invocation_t pk = get_proc_invocation();

#if MUTEX_HOLDER_ASSERT
	assert(p->p_lock.holder == cthread_self());
#endif

	pk->k_p = p;
	pk->event = 0;
	queue_enter(&p->p_servers,
		    pk,
		    proc_invocation_t,
		    k_servers_chain);
	p->p_servers_count++;
	proc_ref(p);
}

/*
 * Unregister a server thread.
 */
void
server_thread_deregister(struct proc *p)
{
	proc_invocation_t pk = get_proc_invocation();

	mutex_lock(&p->p_lock);
	assert (pk && (p->p_ref >= 1) || !queue_empty(&p->p_servers));
	assert(!pk->k_wchan);
	queue_assert_member(&p->p_servers, pk, proc_invocation_t,
			    k_servers_chain);
	queue_remove(&p->p_servers, pk, proc_invocation_t, k_servers_chain);
	p->p_servers_count--;
	pk->k_p = (struct proc *) 0;
	proc_deref(p, MACH_PORT_NULL);		/* consumes lock */
}

int
start_server_op(
	struct proc	*p,
	int		syscode)
{
	proc_invocation_t pk = get_proc_invocation();

	int error = 0;

	if (p == (struct proc *)0) {
		return ESRCH;
	}
	if (p->p_flag & P_WEXIT) {
		mutex_unlock(&p->p_lock);	/* XXX */
		return (MIG_NO_REPLY);
	}
	if (HAVE_SIGNALS(p) && syscode < 1000) {
		mutex_unlock(&p->p_lock);	/* XXX */
		return (ERESTART);
	}

	server_thread_register_internal(p);
	mutex_unlock(&p->p_lock);	/* XXX */

#if	SYSCALLTRACE
	if (syscalltrace &&
		(syscalltrace == p->p_pid || syscalltrace < 0)) {

	    char *s;
	    char num[10];
	    static char * extra_syscallnames[] = {
		    "1000take_signal",		/* 1000 */
		    "1001task_by_pid",		/* 1001 */
		    "1002init_process",		/* 1002 */
		    "1003exec_args_set",	/* 1003 */
		    "1004",			/* 1004 */
		    "1005maprw_request",	/* 1005 */
		    "1006maprw_release_it",	/* 1006 */
		    "1007maprw_remap",		/* 1007 */
		    "1008setattr",
		    "1009getattr",
		    "1010path_setattr",
		    "1011path_getattr",
		    "1012secure_execve",
		    "1013after_exec",
		    "1014file_vm_map",
		    "1015fd_to_file_port",
		    "1016file_port_open",
		    "1017",
		    "1018signal_port_register",
		    "1019exec_done",
		    "1020", "1021", "1022"
	    };
	    static int extra_nsysent = sizeof(extra_syscallnames) / 
		    sizeof(extra_syscallnames[0]);

	    if (syscode >= nsysent || syscode < 0) {

		if (syscode - 1000 >= extra_nsysent || syscode < 0) {
			sprintf(num, "%d", syscode);
			s = num;
		} else {
			s = extra_syscallnames[syscode - 1000];
		}
	    }
	    else
		s = syscallnames[syscode];
	    printf("\n[%d]%s", p->p_pid, s);
	}
#endif	SYSCALLTRACE
	return(error);
}

int
end_server_op(p, error, interrupt)
    register int	error;
    boolean_t	*interrupt;
    register struct proc *p;
{
	proc_invocation_t pk;
	if (p == 0) {
	    return (MIG_NO_REPLY);
	}
	pk = get_proc_invocation();

	if (error) {
	    switch (error) {
		case EDQUOT:
		case ENOSPC:
		    /* XXX resource pause code here */
		    break;

		case EJUSTRETURN:
		    error = MIG_NO_REPLY;
		    break;
	    }
	}

	if (interrupt)
	  *interrupt = FALSE;

	if (p->p_flag & P_WEXIT) {
	    int sig;
	    if (HAVE_SIGNALS(p)) {
		unix_master();
		mutex_lock(&p->p_lock);
		if ((sig = CURSIG(p)))
		    psig(sig);
		if (interrupt && sig)	/* user should take signal */
		    *interrupt = TRUE;
		mutex_unlock(&p->p_lock);
		unix_release();
	    }
	}

#if	SYSCALLTRACE
	if (syscalltrace &&
		(syscalltrace == p->p_pid || syscalltrace < 0)) {

	    printf("    [%d] returns %d%s\n",
		p->p_pid,
		error,
		(interrupt && *interrupt) ? " Interrupt" : "");
	}
#endif

	server_thread_deregister(p);

	if (p->p_flag & P_WEXIT)
	    wakeup((caddr_t)&p->p_flag);

	if (pk->k_master_lock)
	    panic("Master still held", pk->k_master_lock);

	if (pk->k_ipl)
	    panic("IPL above 0", pk->k_ipl);

	return (error);
}

