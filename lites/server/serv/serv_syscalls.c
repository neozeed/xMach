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
 * $Log: serv_syscalls.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.4  1996/03/14  21:08:37  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.3  1995/09/08  16:45:58  mike
 * variant of Ian Dall's signal mask fix (with some explanation)
 *
 * Revision 1.2  1995/08/18  18:04:54  mike
 * port_object_send_lookup consume a task reference.  In a couple of places
 * we weren't expecting this, so take an extra reference.
 *
 * Revision 1.1.1.2  1995/03/23  01:16:51  law
 * lites-950323 from jvh.
 *
 * 12-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Allow for sendsig to return a "task notified" status.
 *
 * 11-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Use HAVE_SIGNALS and CURSIG instead of testing p_siglist
 *	directly.
 *
 * 30-Sep-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Use SAS_NOTIFY_DONE flag to indicate that the process has already been
 *	told there are signals waiting.
 *
 * 30-Sep-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Removed definition for SIG_CATCH and SIG_HOLD and SA_OLDMASK.
 *	These are defined in sigvar.h
 *
 */
/* 
 *	File: 	serv/serv_syscalls.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 * 	System calls unique to the server and related code.
 */

#include "map_uarea.h"

#include <serv/server_defs.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/namei.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/acct.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <bsd_1_server.h>

#include <serv/syscall_subr.h>

void proc_zap(struct proc *p, int rv);

/* XXX */
extern int sigprop[NSIG+1];

#define	ps_onstack	ps_sigstk.ss_flags
/* end XXX */


/* PROCESSES */
extern mach_port_t privileged_host_port;
extern mach_port_t device_server_port;

/*
 *	Routine:	bsd_task_by_pid
 *	Purpose:
 *		Get the task port for another "process", named by its
 *		process ID on the same host as "target_task".
 *
 *		Only permitted to privileged processes, or processes
 *		with the same user ID.
 */
int
bsd_task_by_pid(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			pid,
	task_t			*t,
	mach_msg_type_name_t	*tType)
{
	proc_invocation_t pk = get_proc_invocation();

	struct proc	*p, *pf;
	register int	error;
	extern struct proc proc0;

	p = proc_receive_lookup(proc_port, seqno);
	if (error = start_server_op(p, 1001))
	    return (error);

	p = pk->k_p;

	unix_master();

	/* XXX groupmember(2) */

	if (pid == 0) {
		if (groupmember(2,p->p_ucred)||!suser(p->p_ucred,&p->p_acflag))
			*t = proc0.p_task;	/* UX server */
		else
			error = EACCES;
	} else if (pid == -1) {
		if (groupmember(2,p->p_ucred)||!suser(p->p_ucred,&p->p_acflag))
			*t = privileged_host_port;
		else
			error = EACCES;
	} else if (pid == -2) {
		if (groupmember(2,p->p_ucred)||!suser(p->p_ucred,&p->p_acflag))
			*t = device_server_port;
		else
			error = EACCES;
	} else if (((pf = pfind(pid)) != (struct proc *) 0) &&
		   (pf->p_stat != SZOMB)) {
		if ((pf->p_ucred->cr_uid == p->p_ucred->cr_uid) ||
		    groupmember(2,p->p_ucred) || !suser(p->p_ucred, 0))
			*t = pf->p_task;
		else
			error = EACCES;
	} else
		error = ESRCH;
	/* 
	 * XXX Give port only if not suid/setgid and binary is
	 * XXX readable or caller is root.
	 */

	if (!error && MACH_PORT_VALID(*t)) {
		kern_return_t kr;

		/*
		 * Give ourself another send right for the task port,
		 * and specify that the send right should be moved
		 * into the reply message.  This way there is no problem
		 * if the task port should be destroyed before the
		 * the reply message is sent.
		 */

		kr = mach_port_mod_refs(mach_task_self(), *t,
					MACH_PORT_RIGHT_SEND, 1);
		if (kr == KERN_SUCCESS) {
			*tType = MACH_MSG_TYPE_MOVE_SEND;
		} else if (kr == KERN_INVALID_RIGHT) {
#if 0
/* XXX But then it must already be dead? */
			/* Probably a terminated task, so kill the proc. */
			proc_zap(pf, SIGKILL);
#endif
			error = ESRCH;
		} else {
			printf("pid %d -> task 0x%x; mod_refs returned %d\n",
				pid, *t, kr);
			panic("bsd_task_by_pid");
		}
	}

	unix_release();

	if (error) {
		/*
		 * If we can't produce a real send right,
		 * then give our caller MACH_PORT_NULL instead
		 * of a unix error code which he would probably
		 * mistake for a port.
		 */

		*t = MACH_PORT_NULL;
		*tType = MACH_MSG_TYPE_MOVE_SEND;
		error = 0;
	}

	return (end_server_op(p, error, (boolean_t *)0));
}

int
bsd_pid_by_task(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	task_t			task,
	int			*pidp,
	char			*comm,
	mach_msg_type_number_t	*commlen)
{
	register struct proc *p;
	register struct proc *sp;
	int len;
	extern struct proc proc0;
	proc_invocation_t pk = get_proc_invocation();
	mach_error_t ret = KERN_SUCCESS;
	p = pk->k_p;

	/* Gotta keep seqno in sync */
	p = proc_receive_lookup(proc_port, seqno);
	mutex_unlock(&p->p_lock);

	unix_master();
	if (task == mach_task_self()) {
		sp = &proc0;
	} else {
		/*
		 * XXX port_object_send_lookup will consume the task reference
		 * but our caller expects it to still exist, so take an extra
		 * reference.
		 */
		(void) port_object_copy_send(task);

		sp = (struct proc *) port_object_send_lookup(task, POT_TASK);
		if (sp)
		    mutex_unlock(&sp->p_lock);
	}
	if ((sp == 0) || (sp->p_flag & P_WEXIT)) {
	    *pidp = 0;
	    *commlen = 0;
	    ret = ESRCH;
	} else {
	    *pidp = sp->p_pid;
	    len = strlen(sp->p_comm);
	    if (*commlen < len)
		len = *commlen;
	    bcopy(sp->p_comm, comm, len);
	    *commlen = len;
	    /* get rid of the send right (consume on success) */
	    (void) mach_port_deallocate(mach_task_self(), task);
	}
	unix_release();

	return ret;
}

/*
 *	Make the current process an "init" process, meaning
 *	that it doesn't have a parent, and that it won't be
 *	gunned down by kill(-1, 0).
 */

kern_return_t	init_process(struct proc *p)
{
	proc_invocation_t pk = get_proc_invocation();

	if (!suser(p->p_ucred, &p->p_acflag))
		return(KERN_NO_ACCESS);

	unix_master();

	/*
	 *	Take us out of the sibling chain, and
	 *	out of our parent's child chain.
	 */

	if (p->p_osptr)
		p->p_osptr->p_ysptr = p->p_ysptr;
	if (p->p_ysptr)
		p->p_ysptr->p_osptr = p->p_osptr;
	if (p->p_pptr->p_cptr == p)
		p->p_pptr->p_cptr = p->p_osptr;
	p->p_pptr = p;
	p->p_ysptr = p->p_osptr = 0;
	leavepgrp(p);
	p->p_pptr = 0;

	unix_release();
	return(KERN_SUCCESS);
}

/* 
 * Terminate process's task.
 * The notify handler will take care of cleaning up.
 */
void proc_zap(struct proc *p, int rv)
{
	/* mutex_lock(&p->p_lock); XX p is sometimes locked by self */
	p->p_xstat = rv;
	task_suspend(p->p_task);
	proc_get_stats(p);
	task_terminate(p->p_task);
	/* mutex_unlock(&p->p_lock); */
}

/* ARGSUSED */
int
bsd_getrusage(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			which,
	struct rusage		*rusage)	/* OUT */
{
        register int error;
	register struct rusage *rup;
	register struct proc *p;
	proc_invocation_t pk = get_proc_invocation();

	p = proc_receive_lookup(proc_port, seqno);
	error = start_server_op(p, SYS_getrusage);
	if (error)
	    return (error);

	switch (which) {

	case RUSAGE_SELF: {
		rup = &p->p_stats->p_ru;
		unix_master();
		proc_get_stats(p);
		unix_release();
		calcru(p, &rup->ru_utime, &rup->ru_stime, NULL);
		break;
	}

	case RUSAGE_CHILDREN:
		rup = &p->p_stats->p_cru;
		break;

	default:
		return (end_server_op(p, EINVAL, (boolean_t *)0));
	}
	*rusage = *rup;
	return (end_server_op(p, 0, (boolean_t *)0));
}

mach_error_t getrusage()
{
	panic("getrusage called");
	return EOPNOTSUPP;
}

/* SIGNALS */

#if MAP_UAREA
#define siglock(p) share_lock(&(p)->p_shared_rw->us_siglock,(p))
#define sigunlock(p) share_unlock(&(p)->p_shared_rw->us_siglock,(p))
#else
#define siglock(p)
#define sigunlock(p)
#endif

#define SIGNAL_DEBUG 1
#if SIGNAL_DEBUG
int signal_debug = 0;
#define SD(statement) if (signal_debug) statement
#else
#define SD(x)
#endif SIGNAL_DEBUG

/*
 * Send the signal to a thread.  The thread must not be executing
 * any kernel calls.
 */
boolean_t send_signal(
	struct proc 	*p,
	thread_t	thread,
	int		sig,
	int		code)
{
	register int	mask;
	int	returnmask;
	boolean_t notified = FALSE;
	register struct sigacts *ps = p->p_sigacts;
	register sig_t action;

	SD(printf("%8x[%d]: send_signal %d\n", p, p->p_pid, sig));
	action = ps->ps_sigact[sig];

	mask = sigmask(sig);

	/*
	 * Signal action may have changed while we blocked for
	 * task_suspend or thread_abort.  Check it again.
	 */

	if ((p->p_sigcatch & mask) == 0) {
	    /*
	     * No longer catching signal.  Put it back if
	     * not ignoring it, and start up the task again.
	     */
	    psignal(p, sig);
	    (void) task_resume(p->p_task);
	    return FALSE;
	}

	if (ps->ps_flags & SAS_NOTIFY_DONE)
	    return FALSE;

	/*
	 * At this point, the thread may be in exception_raise,
	 * waiting for a reply message.  A reply message should resume
	 * it. We don't want a reply if the task is handling it its self.
	 */
	SD(printf("%8x[%d]: really send_signal %d\n", p, p->p_pid, sig));
	notified = sendsig(p, thread, action, sig, code, /* unused */ 0);
	if (notified)
	    ps->ps_flags |= SAS_NOTIFY_DONE;
	return notified;
}

/*
 * Take default action on signal.
 */
void sig_default(struct proc *p, int sig)
{
	boolean_t	dump;

	SD(printf("%8x[%d]: sig_default %x\n",p,p->p_pid, sig));
	switch (sig) {
	    /*
	     *	The new signal code for multiple threads makes it possible
	     *	for a multi-threaded task to get here (a thread that didn`t
	     *	originally process a "stop" signal notices that cursig is
	     *	set), therefore, we must handle this.
	     */
	    case SIGIO:
	    case SIGURG:
	    case SIGCHLD:
	    case SIGCONT:
	    case SIGWINCH:
	    case SIGTSTP:
	    case SIGTTIN:
	    case SIGTTOU:
	    case SIGSTOP:
		return;

	    case SIGILL:
	    case SIGIOT:
	    case SIGBUS:
	    case SIGQUIT:
	    case SIGTRAP:
	    case SIGEMT:
	    case SIGFPE:
	    case SIGSEGV:
	    case SIGSYS:
		/*
		 * Kill the process with a core dump.
		 */
		dump = TRUE;
		break;
	
	    default:
		dump = FALSE;
		break;
	}

	sigexit(p,sig);
}

void psig(int sig)
{
	register struct proc *p = get_proc();
	register struct sigacts *ps = p->p_sigacts;
	register sig_t action;

	SD(printf("%8x[%d]: psig %x\n",p,p->p_pid, sig));
	action = ps->ps_sigact[sig];
	if (action != SIG_DFL) {

	    /*
	     * User handles sending himself signals.
	     */
	    return;
	}

	p->p_siglist &= ~sigmask(sig);
	sig_default(p, sig);
}

boolean_t thread_signal(struct proc *p, thread_t thread, int sig, int code)
{
  	int mask;
	boolean_t notified = FALSE;

	mask = sigmask(sig);

	SD(printf("%8x[%d]: thread_signal %x\n",p,p->p_pid, sig));
	for (;;) {
	    if (p->p_sigmask & mask) {
		/*
		 * Save the signal in p_sig.
		 */
		p->p_siglist |= mask;
		return FALSE;
	    }

	    while (p->p_stat == SSTOP || (p->p_flag & P_WEXIT)) {
		if (p->p_flag & P_WEXIT) {
		    return FALSE;
		}
		sleep((caddr_t)&p->p_stat, 0);
	    }
	    if (p->p_sigmask & mask) {
		/* could have changed */
		continue;
	    }

	    if (p->p_flag & P_TRACED) {
		/*
		 * Stop for trace.
		 */
		psignal(p->p_pptr, SIGCHLD);
		stop(p);
		sleep((caddr_t)&p->p_stat, 0);

		if (p->p_flag & P_WEXIT) {
		    return FALSE;
		}

		if ((p->p_flag & P_TRACED) == 0) {
		    continue;
		}

		if (!HAVE_SIGNALS(p)) {
		    return FALSE;
		}

		sig = CURSIG(p);
		mask = sigmask(sig);
		if (p->p_sigmask & mask) {
		    continue;
		}
	    }
	    break;
	}

	switch ((vm_offset_t)p->p_sigacts->ps_sigact[sig]) {
	    case SIG_IGN:
	    case SIG_HOLD:
		/*
		 * Should not get here unless traced.
		 */
		break;

	    case SIG_DFL:
		sig_default(p, sig);
		break;

	    default:
		/*
		 * Send signal to user thread.
		 */
		siglock(p);
		p->p_siglist |= mask;
		sigunlock(p);
		notified = send_signal(p, thread, sig, code);
		break;
	}
	return notified;
}

/*
 * New thread_psignal.  Runs as part of the normal service - thus
 * we have a thread per user exception, so it can wait.
 */
boolean_t thread_psignal(
	task_t		task,
	thread_t	thread,
	register int	sig,
	int		code)
{
	register struct proc *p;
	proc_invocation_t pk = get_proc_invocation();
	boolean_t notified = FALSE;

	/*
	 * XXX port_object_send_lookup will consume the task reference
	 * but our caller expects it to still exist, so take an extra
	 * reference.
	 */
	(void) port_object_copy_send(task);

	p = (struct proc *) port_object_send_lookup(task, POT_TASK);
	
	SD(printf("%8x[%d]: thread_psignal %x\n",p,p->p_pid, sig));
	if (p == 0) {
	    (void) task_terminate(task);
	    return FALSE;
	}
	if (sig < 0 || sig > NSIG) {
		mutex_unlock(&p->p_lock);
		return FALSE;
	}

	/*
	 * Register thread as service thread
	 */
	server_thread_register_internal(p);
	mutex_unlock(&p->p_lock);
	unix_master();

	notified = thread_signal(p, thread, sig, code);

	unix_release();

	server_thread_deregister(p);

	if (p->p_flag & P_WEXIT)
	    wakeup((caddr_t)&p->p_flag);
	return notified;
}

/*
 * Unblock thread signal if it was masked off.
 */
void check_proc_signals(struct proc *p)
{
	wakeup((caddr_t)&p->p_sigmask);
}

/*
 * Called by user to take a pending signal.
 */
int
bsd_take_signal(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	sigset_t		*old_mask,	/* out */
	int			*old_onstack,	/* out */
	int			*o_sig,		/* out */
	integer_t		*o_code,	/* out */
	vm_offset_t		*o_handler,	/* out */
	vm_offset_t		*new_sp)	/* out */
{
	proc_invocation_t pk = get_proc_invocation();
	register struct proc *p;
	register int	error;
	register int	sig;
	sigset_t mask;
	register struct sigacts *ps;
	register sig_t action;

	int		returnmask;
	int		oonstack;

	p = proc_receive_lookup(proc_port, seqno);

	error = start_server_op(p,1000);
		/* XXX code for take-signal */
	if (error)
	    return (error);

	p = pk->k_p;
	SD(printf("%8lx[%d]: bsd_take_signal\n",p,p->p_pid));
	ps = p->p_sigacts;
	unix_master();

	/*
	 * Process should be running.
	 * Should not get here if process is stopped.
	 */
	if (p->p_stat != SRUN || (p->p_flag & P_WEXIT)) {
	    unix_release();
	    return (end_server_op(p, ESRCH, (boolean_t *)0));
	}

	/*
	 * Set up return values in case no signals pending.
	 */
	*old_mask = 0;
	*old_onstack = 0;
	*o_sig = 0;
	*o_code = 0;
	*o_handler = 0;
	*new_sp = 0;

	/*
	 * Get pending signal.
	 */
	mutex_lock(&p->p_lock);	/* XXX Entire funct should be locked */
	sig = CURSIG(p);
	mutex_unlock(&p->p_lock);
	if (sig == 0) {
	    /*
	     * No signals - return to user.
	     */
	    ps->ps_flags &= ~SAS_NOTIFY_DONE;
	    unix_release();
	    return (end_server_op(p, 0, (boolean_t *)0));
	}

	p->p_siglist &= ~sigmask(sig);
	action = ps->ps_sigact[sig];
	switch ((vm_offset_t)action) {
	    case SIG_IGN:
	    case SIG_HOLD:
		/*
		 * Should not get here.
		 */
		sig = 0;
		break;

	    case SIG_DFL:
		/*
		 * take default action
		 */
		sig_default(p, sig);
		sig = 0;
		break;

	    default:
		/*
		 * user gets signal
		 */
		mask = sigmask(sig);

		if ((p->p_sigcatch & mask) == 0) {
		    psignal(p, sig);
		    sig = 0;
		    break;
		}
		if (ps->ps_flags & SAS_OLDMASK) {
		    returnmask = ps->ps_oldmask;
		    ps->ps_flags &= ~SAS_OLDMASK;
		}
		else
		    returnmask = p->p_sigmask;
		siglock(p);
		p->p_sigmask |= mask;
		sigunlock(p);
		oonstack = ps->ps_onstack;
		if (!oonstack && (ps->ps_sigonstack & mask)) {
		    *new_sp = (vm_offset_t) ps->ps_sigstk.ss_base;
		    ps->ps_sigstk.ss_flags |= SA_ONSTACK;
		}
		else
		    *new_sp = 0;	/* use existing stack */

		*old_mask = returnmask;
		*old_onstack = oonstack;
		*o_sig = sig;
		*o_code = 0;
		*o_handler = (vm_offset_t)action;
		break;
	}

	if (*o_sig == 0)
	  ps->ps_flags &= ~SAS_NOTIFY_DONE;
	SD(printf("%x[%d] got signal %d\n", p, p->p_pid, sig));
	unix_release();
	return (end_server_op(p, 0, (boolean_t *)0));
}

int
bsd_sigreturn(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			old_on_stack,
	sigset_t		old_sigmask)
{
	proc_invocation_t pk = get_proc_invocation();
	register int error;
	register struct proc *p;

	p = proc_receive_lookup(proc_port, seqno);

	error = start_server_op(p, SYS_sigreturn);
	if (error)
	    return (error);

	/* Can not get proc before start_server_op! */
	p = pk->k_p;

	SD(printf("%8x[%d]: bsd_sigreturn\n",p,p->p_pid));
	unix_master();

	p->p_sigacts->ps_onstack = old_on_stack & 01;

	siglock(p);
	p->p_sigmask = old_sigmask & ~sigcantmask;
	sigunlock(p);

	check_proc_signals(p);

	/* Statistics: add to signal count */
	p->p_stats->p_ru.ru_nsignals++;

	unix_release();

	return (end_server_op(p, 0, (boolean_t *)0));
}

/* FILES */
/*
 * Set attributes given a file descriptor.
 * Code based on Lite chown/fchown.
 */
/* ARGSUSED */
mach_error_t setattr(
	struct proc *p,
	int fileno,
	struct vattr *va)
{
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	if (error = getvnode(p->p_fd, fileno, &fp))
		return (error);
	vp = (struct vnode *)fp->f_data;
	LEASE_CHECK(vp, p, p->p_ucred, LEASE_WRITE);
	VOP_LOCK(vp);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
	    error = EROFS;
	else 
	    error = VOP_SETATTR(vp, va, p->p_ucred, p);
	VOP_UNLOCK(vp);
	return (error);
}

/*
 * Set attributes given a path name.
 */
mach_error_t path_setattr(
	struct proc	*p,
	char		*path,
	boolean_t	follow,	/* follow symlinks? */
	struct vattr	*va)
{
	register struct vnode *vp;
	int error;
	struct nameidata nd;

	/* XXX chown actually does a NOFOLLOW */
	NDINIT(&nd, LOOKUP, (follow ? FOLLOW : NOFOLLOW), UIO_SYSSPACE,
	       path, p);
	if (error = namei(&nd))
		return (error);
	vp = nd.ni_vp;
	LEASE_CHECK(vp, p, p->p_ucred, LEASE_WRITE);
	VOP_LOCK(vp);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
	    error = EROFS;
	else
	    error = VOP_SETATTR(vp, va, p->p_ucred, p);
	vput(vp);
	return (error);
}

/*
 * Get attributes given a file descriptor.
 */
mach_error_t getattr(
	struct proc	*p,
	int		fileno,
	struct vattr	*va)	/* OUT */
{
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	mach_error_t err;

	if (fileno >= fdp->fd_nfiles || (fp = fdp->fd_ofiles[fileno]) == NULL)
	    return EBADF;

	VATTR_NULL(va);
	switch (fp->f_type) {
	      case DTYPE_VNODE:
		return VOP_GETATTR((struct vnode *) fp->f_data, va,
				   p->p_ucred, p);
	      case DTYPE_SOCKET:
		return soo_getattr((struct socket *)fp->f_data, va);
	      default:
		panic("getattr: bad file type");
		return EFTYPE;
	}
	/*NOTREACHED*/
}

/*
 * Get attributes given a path name.
 */
mach_error_t path_getattr(
	struct proc	*p,
	char		*path,
	boolean_t	follow,
	struct vattr	*va)	/* OUT */
{
	struct nameidata nd;
	mach_error_t err;

	NDINIT(&nd, LOOKUP, (follow ? FOLLOW : NOFOLLOW) | LOCKLEAF,
	       UIO_SYSSPACE, path, p);
	if (err = namei(&nd))
		return err;
	err = VOP_GETATTR(nd.ni_vp, va, p->p_ucred, p);
	vput(nd.ni_vp);
	return err;
}
