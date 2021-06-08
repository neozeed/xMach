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
 * $Log: kern_exit.c,v $
 * Revision 1.2  2000/10/27 01:58:45  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/03/23  01:44:39  law
 * Update to 950323 snapshot + utah changes
 *
 * Revision 1.1.1.2  1995/03/22  23:26:13  law
 * Pure lites-950316 snapshot.
 *
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
 *	@(#)kern_exit.c	8.7 (Berkeley) 2/12/94
 */

#include "compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <sys/ptrace.h>
#include <sys/assert.h>

#include <machine/cpu.h>

#include <vm/vm.h>

boolean_t proc_died_debug = FALSE;

/*
 * exit --
 *	Death of process.
 */
struct rexit_args {
	int	rval;
};
int rexit(p, uap, retval)
	struct proc *p;
	struct rexit_args *uap;
	int *retval;
{

	proc_zap(p, W_EXITCODE(uap->rval, 0));
	return EJUSTRETURN;
}

/* boot() sets this to indicate init is going to be zapped */
volatile boolean_t server_is_going_down = FALSE;

/*
 * Exit: deallocate address space and other resources, change proc state
 * to zombie, and unlink proc from allproc and parent's lists.  Save exit
 * status and rusage for wait().  Check for child processes and orphan them.
 */
/* p is *unlocked* but has one ref. proc_died consumes the ref */
void proc_died(struct proc *p)
{
	struct proc *q, *nq;
	struct proc **pp;
	struct vmspace *vm;
	mach_error_t kr;

	if (proc_died_debug)
	    printf ("proc_died: pid=%d\n", p->p_pid);

	if (p->p_pid == 1) {
		if (server_is_going_down)
		    return;		/* never mind */
		else
		    panic("init died");
	}

#ifdef PGINPROF
	vmsizmon();
#endif
#ifndef LITES
	if (p->p_flag & P_PROFIL)
		stopprofclock(p);
#endif
	MALLOC(p->p_ru, struct rusage *, sizeof(struct rusage),
		M_ZOMBIE, M_WAITOK);

#ifdef LITES
	assert(p->p_task == MACH_PORT_NULL && (p->p_flag & P_WEXIT));
	assert(queue_empty(&p->p_servers));
	assert(p->p_ref == 1);
#endif
	/*
	 * If parent is waiting for us to exit or exec,
	 * P_PPWAIT is set; we will wakeup the parent below.
	 */
	/* XXX p is unlocked (to avoid deadlock in untimeout and fdfree) */
	p->p_flag &= ~(P_TRACED | P_PPWAIT);
	p->p_flag |= P_WEXIT;
	p->p_sigignore = ~0;
	p->p_siglist = 0;
	untimeout(realitexpire, (caddr_t)p);

	/*
	 * Close open files and release open-file table.
	 * This may block!
	 */
	fdfree(p);

	/* The next two chunks should probably be moved to vmspace_exit. */
	vm = p->p_vmspace;
#ifdef SYSVSHM
	if (vm->vm_shm)
		shmexit(p);
#endif
	/*
	 * Release user portion of address space.
	 * This releases references to vnodes,
	 * which could cause I/O if the file has been unlinked.
	 * Need to do this early enough that we can still sleep.
	 * Can't free the entire vmspace as the kernel stack
	 * may be mapped within that space also.
	 */
#ifndef LITES
	if (vm->vm_refcnt == 1)
		(void) vm_map_remove(&vm->vm_map, VM_MIN_ADDRESS,
		    VM_MAXUSER_ADDRESS);
#endif
	if (SESS_LEADER(p)) {
		register struct session *sp = p->p_session;

		if (sp->s_ttyvp) {
			/*
			 * Controlling process.
			 * Signal foreground pgrp,
			 * drain controlling terminal
			 * and revoke access to controlling terminal.
			 */
			if (sp->s_ttyp->t_session == sp) {
				if (sp->s_ttyp->t_pgrp)
					pgsignal(sp->s_ttyp->t_pgrp, SIGHUP, 1);
				(void) ttywait(sp->s_ttyp);
				/*
				 * The tty could have been revoked
				 * if we blocked.
				 */
				if (sp->s_ttyvp)
					vgoneall(sp->s_ttyvp);
			}
			if (sp->s_ttyvp)
				vrele(sp->s_ttyvp);
			sp->s_ttyvp = NULL;
			/*
			 * s_ttyp is not zero'd; we use this to indicate
			 * that the session once had a controlling terminal.
			 * (for logging and informational purposes)
			 */
		}
		sp->s_leader = NULL;
	}
#if 1
	mutex_lock(&p->p_lock);
	assert(p->p_ref == 1);
	p->p_ref--;
#endif
	fixjobc(p, p->p_pgrp, 0);
	p->p_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	(void)acct_process(p);
#ifdef KTRACE
	/* 
	 * release trace file
	 */
	p->p_traceflag = 0;	/* don't trace the vrele() */
	if (p->p_tracep)
		vrele(p->p_tracep);
#endif
	/*
	 * Remove proc from allproc queue and pidhash chain.
	 * Place onto zombproc.  Unlink from parent's child list.
	 */
	/* XXX locks! */
	if (*p->p_prev = p->p_next)
		p->p_next->p_prev = p->p_prev;
	if (p->p_next = zombproc)
		p->p_next->p_prev = &p->p_next;
	p->p_prev = &zombproc;
	zombproc = p;
	p->p_stat = SZOMB;

	for (pp = &pidhash[PIDHASH(p->p_pid)]; *pp; pp = &(*pp)->p_hash)
		if (*pp == p) {
			*pp = p->p_hash;
			goto done;
		}
	panic("proc_died: not on pidhash");
done:

	if (p->p_cptr) {		/* only need this if any child is S_ZOMB */
		int s = splhigh();
		wakeup((caddr_t) initproc);
		splx(s);
	}
	for (q = p->p_cptr; q != NULL; q = nq) {
		nq = q->p_osptr;
		if (nq != NULL)
			nq->p_ysptr = NULL;
		if (initproc->p_cptr)
			initproc->p_cptr->p_ysptr = q;
		q->p_osptr = initproc->p_cptr;
		q->p_ysptr = NULL;
		initproc->p_cptr = q;

		q->p_pptr = initproc;
		/*
		 * Traced processes are killed
		 * since their existence means someone is screwing up.
		 */
		if (q->p_flag & P_TRACED) {
			q->p_flag &= ~P_TRACED;
			psignal(q, SIGKILL);
		}
	}
	p->p_cptr = NULL;

	/*
	 * Save exit status and final rusage info, adding in child rusage
	 * info and self times.
	 */
	/* p->p_xstat = rv; XXX most of the time done by proc_zap */
	*p->p_ru = p->p_stats->p_ru;
	calcru(p, &p->p_ru->ru_utime, &p->p_ru->ru_stime, NULL);
	ruadd(p->p_ru, &p->p_stats->p_cru);

	/*
	 * Notify parent that we're gone.
	 */
	psignal(p->p_pptr, SIGCHLD);
	{
		int s = splhigh();
		wakeup((caddr_t)p->p_pptr);
		splx(s);
	}
#if defined(tahoe)
	/* move this to cpu_exit */
	p->p_addr->u_pcb.pcb_savacc.faddr = (float *)NULL;
#endif
#ifdef LITES

	vm_exit(p);

	/*
	 * Yes, it can happen.  The user-reference counts on
	 * the task/thread port names might be greater than one
	 * because other threads in the Unix server were doing
	 * things with the ports.
	 */

#if 0 /* These are all handled by notifications and must have occurred */
	/* One for the notification and one from newproc */
	if (MACH_PORT_VALID(p->p_task)) {
		kr = mach_port_deallocate(mach_task_self(), p->p_task);
		assert(kr == KERN_SUCCESS);
		kr = mach_port_deallocate(mach_task_self(), p->p_task);
		assert(kr == KERN_SUCCESS);
		p->p_task = MACH_PORT_NULL;
	}
	if (MACH_PORT_VALID(p->p_thread)) {
		kr = mach_port_deallocate(mach_task_self(), p->p_thread);
		assert(kr == KERN_SUCCESS);
		p->p_thread = MACH_PORT_NULL;
	}

	if (MACH_PORT_VALID(p->p_sigport)) {
		kr = mach_port_deallocate(mach_task_self(), p->p_sigport);
		assert(kr == KERN_SUCCESS);
		p->p_sigport = MACH_PORT_NULL;
	}
#else
	assert(p->p_task == MACH_PORT_NULL);
	assert(p->p_thread == MACH_PORT_NULL);
	assert(p->p_sigport == MACH_PORT_NULL);
#endif
	mutex_unlock(&p->p_lock);
#else /* LITES */
	/*
	 * Clear curproc after we've done all operations
	 * that could block, and before tearing down the rest
	 * of the process state that might be used from clock, etc.
	 * Also, can't clear curproc while we're still runnable,
	 * as we're not on a run queue (we are current, just not
	 * a proper proc any longer!).
	 *
	 * Other substructures are freed from wait().
	 */

	curproc = NULL;
	if (--p->p_limit->p_refcnt == 0)
		FREE(p->p_limit, M_SUBPROC);

	/*
	 * Finally, call machine-dependent code to release the remaining
	 * resources including address space, the kernel stack and pcb.
	 * The address space is released by "vmspace_free(p->p_vmspace)";
	 * This is machine-dependent, as we may have to change stacks
	 * or ensure that the current one isn't reallocated before we
	 * finish.  cpu_exit will end with a call to cpu_swtch(), finishing
	 * our execution (pun intended).
	 */
	cpu_exit(p);
#endif
}

struct wait_args {
	int	pid;
	int	*status;
	int	options;
	struct	rusage *rusage;
};

int wait4(q, uap, retval)
	register struct proc *q;
	register struct wait_args *uap;
	int retval[];
{
	register int nfound;
	register struct proc *p, *t;
	int status, error;

	if (uap->pid == 0)
		uap->pid = -q->p_pgid;
#ifdef notyet
	if (uap->options &~ (WUNTRACED|WNOHANG))
		return (EINVAL);
#endif
   
loop:
	nfound = 0;
	for (p = q->p_cptr; p; p = p->p_osptr) {
		if (uap->pid != WAIT_ANY &&
		    p->p_pid != uap->pid && p->p_pgid != -uap->pid)
			continue;
		nfound++;
		if (p->p_stat == SZOMB) {
			assert(p->p_ref == 0);
			retval[0] = p->p_pid;
			if (uap->status) {
				status = p->p_xstat;	/* convert to int */
				if (error = copyout((caddr_t)&status,
				    (caddr_t)uap->status, sizeof(status)))
					return (error);
			}
			if (uap->rusage && (error = copyout((caddr_t)p->p_ru,
			    (caddr_t)uap->rusage, sizeof (struct rusage))))
				return (error);
			/*
			 * If we got the child via a ptrace 'attach',
			 * we need to give it back to the old parent.
			 */
			if (p->p_oppid && (t = pfind(p->p_oppid))) {
				p->p_oppid = 0;
				proc_reparent(p, t);
				psignal(t, SIGCHLD);
				wakeup((caddr_t)t);
				return (0);
			}
			p->p_xstat = 0;
			ruadd(&q->p_stats->p_cru, p->p_ru);
			FREE(p->p_ru, M_ZOMBIE);

			/*
			 * Decrement the count of procs running with this uid.
			 */
			(void)chgproccnt(p->p_cred->p_ruid, -1);

			/*
			 * Free up credentials.
			 */
			if (--p->p_cred->p_refcnt == 0) {
				crfree(p->p_cred->pc_ucred);
				FREE(p->p_cred, M_SUBPROC);
			}

			/*
			 * Release reference to text vnode
			 */
			if (p->p_textvp)
				vrele(p->p_textvp);

			/*
			 * Finally finished with old proc entry.
			 * Unlink it from its process group and free it.
			 */
			leavepgrp(p);
			if (*p->p_prev = p->p_next)	/* off zombproc */
				p->p_next->p_prev = p->p_prev;
			if (q = p->p_ysptr)
				q->p_osptr = p->p_osptr;
			if (q = p->p_osptr)
				q->p_ysptr = p->p_ysptr;
			if ((q = p->p_pptr)->p_cptr == p)
				q->p_cptr = p->p_osptr;

#ifndef LITES
			/*
			 * Give machine-dependent layer a chance
			 * to free anything that cpu_exit couldn't
			 * release while still running in process context.
			 */
			cpu_wait(p);
#endif
			proc_free(p);
			nprocs--;
			return (0);
		}
		if (p->p_stat == SSTOP && (p->p_flag & P_WAITED) == 0 &&
		    (p->p_flag & P_TRACED || uap->options & WUNTRACED)) {
			p->p_flag |= P_WAITED;
			retval[0] = p->p_pid;
			if (uap->status) {
				status = W_STOPCODE(p->p_xstat);
				error = copyout((caddr_t)&status,
					(caddr_t)uap->status, sizeof(status));
			} else
				error = 0;
			return (error);
		}
#if defined(LITES)
		/* 
		 * XXX major hack for BSD init compatibility.
		 *
		 * Under lites, mach_init (pid 2) is a child of BSD init (1).
		 * This will delay init when going single-user since it will
		 * wait for all its children to die (which mach_init won't)
		 * and eventually timeout after 30 seconds.  For now we just
		 * pretend mach_init isn't there (unless it has died or is
		 * traced and stopped).
		 *
		 * XXX mach_init does an init_process system call.  It
		 * should cause this effect among other things.
		 */
		if (q->p_pid == 1 && p->p_pid == 2) {
			nfound--;
			continue;
		}
#endif
	}
	if (nfound == 0)
		return (ECHILD);
	if (uap->options & WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	{
		int s = splhigh();
		error = tsleep((caddr_t)q, PWAIT | PCATCH, "wait", 0);
		splx(s);
	}
	/* wait4 is not restarted after signals... */
	if (error == ERESTART)
	    return EINTR;
	if (error)
	    return (error);
	goto loop;
}

/*
 * make process 'parent' the new parent of process 'child'.
 */
void
proc_reparent(child, parent)
	register struct proc *child;
	register struct proc *parent;
{
	register struct proc *o;
	register struct proc *y;

	if (child->p_pptr == parent)
		return;

	/* fix up the child linkage for the old parent */
	o = child->p_osptr;
	y = child->p_ysptr;
	if (y)
		y->p_osptr = o;
	if (o)
		o->p_ysptr = y;
	if (child->p_pptr->p_cptr == child)
		child->p_pptr->p_cptr = o;

	/* fix up child linkage for new parent */
	o = parent->p_cptr;
	if (o)
		o->p_ysptr = child;
	child->p_osptr = o;
	child->p_ysptr = NULL;
	parent->p_cptr = child;
	child->p_pptr = parent;
}
