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
 * $Log: proc.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:01  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	include/sys/proc.h
 *	Origin:	Adapted to Lites from 4.4 BSD Lite.
 */
/*-
 * Copyright (c) 1986, 1989, 1991, 1993
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
 *	@(#)proc.h	8.8 (Berkeley) 1/21/94
 */

#ifndef _SYS_PROC_H_
#define	_SYS_PROC_H_

#ifdef KERNEL
#include "map_uarea.h"
#endif

#include <serv/import_mach.h>
#include <sys/cmu_queue.h>
#ifdef KERNEL
#if MAP_UAREA
#include <sys/ushared.h>
#endif
#endif

#include <sys/select.h>			/* For struct selinfo. */

/*
 * One structure allocated per session.
 */
struct	session {
	int	s_count;		/* Ref cnt; pgrps in session. */
	struct	proc *s_leader;		/* Session leader. */
	struct	vnode *s_ttyvp;		/* Vnode of controlling terminal. */
	struct	tty *s_ttyp;		/* Controlling terminal. */
	char	s_login[MAXLOGNAME];	/* Setlogin() name. */
};

/*
 * One structure allocated per process group.
 */
struct	pgrp {
	struct	pgrp *pg_hforw;		/* Forward link in hash bucket. */
	struct	proc *pg_mem;		/* Pointer to pgrp members. */
	struct	session *pg_session;	/* Pointer to session. */
	pid_t	pg_id;			/* Pgrp id. */
	int	pg_jobc;	/* # procs qualifying pgrp for job control */
};

/*
 * Description of a process.
 *
 * This structure contains the information needed to manage a thread of
 * control, known in UN*X as a process; it has references to substructures
 * containing descriptions of things that the process uses, but may share
 * with related processes.  The process structure and the substructures
 * are always addressible except for those marked "(PROC ONLY)" below,
 * which might be addressible only on a processor on which the process
 * is running.
 */
struct	proc {
	struct	proc *p_forw;		/* Doubly-linked run/sleep queue. */
	struct	proc *p_back;
	struct	proc *p_next;		/* Linked list of active procs */
	struct	proc **p_prev;		/*    and zombies. */

	/* substructures: */
	struct	pcred *p_cred;		/* Process owner's identity. */
	struct	filedesc *p_fd;		/* Ptr to open files structure. */
	struct	pstats *p_stats;	/* Accounting/statistics (PROC ONLY). */
	struct	plimit *p_limit;	/* Process limits. */
	struct	vmspace *p_vmspace;	/* Address space. */
	struct	sigacts *p_sigacts;	/* Signal actions, state (PROC ONLY). */

#define	p_ucred		p_cred->pc_ucred
#define	p_rlimit	p_limit->pl_rlimit

#if MAP_UAREA
#define p_flag p_shared_ro->us_flag
#else
	int	p_flag;			/* P_* flags. */
#endif
	char	p_stat;			/* S* process status. */
	char	p_pad1[3];

	pid_t	p_pid;			/* Process identifier. */
	struct	proc *p_hash;	 /* Hashed based on p_pid for kill+exit+... */
	struct	proc *p_pgrpnxt; /* Pointer to next process in process group. */
	struct	proc *p_pptr;	 /* Pointer to process structure of parent. */
	struct	proc *p_osptr;	 /* Pointer to older sibling processes. */

/* The following fields are all zeroed upon creation in fork. */
#define	p_startzero	p_ysptr
	struct	proc *p_ysptr;	 /* Pointer to younger siblings. */
	struct	proc *p_cptr;	 /* Pointer to youngest living child. */
	pid_t	p_oppid;	 /* Save parent pid during ptrace. XXX */
	int	p_dupfd;	 /* Sideways return value from fdopen. XXX */

	/* Mach stuff */
	mach_port_t p_sigport;	/* signal notification port for process */
	mach_port_t p_task;
	mach_port_t p_req_port;	/* for reverse mapping (ie. p -> port) */
	mach_port_t p_fork_req_port; /* original proc port. Used by wait. */
	mach_port_t p_thread;	/* XXX Move to invocation XXX */
	void	*p_after_exec_state;	/* state kept over exec (XXX type) */
	/* chain of active system call invocations in this process */
	queue_head_t p_servers;
	/* count of the above */
	int p_servers_count;
	int p_ref;		/* ref count */
	struct mutex p_lock;
	struct condition p_condition;
	char p_lock_name[12];	/* lock and cond name. max "p4294967296\0" */

	/* For namei @bin expansion */
	char *p_atbin;

	/* scheduling */
	u_int	p_estcpu;	 /* Time averaged value of p_cpticks. */
	int	p_cpticks;	 /* Ticks of cpu time. */
	fixpt_t	p_pctcpu;	 /* %cpu for this process during p_swtime */
	u_int	p_swtime;	 /* Time swapped in or out. */
	u_int	p_slptime;	 /* Time since last blocked. */

	struct	itimerval p_realtimer;	/* Alarm timer. */
	struct	timeval p_rtime;	/* Real time. */
	u_quad_t p_uticks;		/* Statclock hits in user mode. */
	u_quad_t p_sticks;		/* Statclock hits in system mode. */
	u_quad_t p_iticks;		/* Statclock hits processing intr. */

	int	p_traceflag;		/* Kernel trace points. */
	struct	vnode *p_tracep;	/* Trace to vnode. */

#if MAP_UAREA
#define p_siglist p_shared_rw->us_sig
#else
	int	p_siglist;		/* Signals arrived but not delivered.*/
#endif

	struct	vnode *p_textvp;	/* Vnode of executable. */

	long	p_spare[5];		/* pad to 256, avoid shifting eproc. */

/* End area that is zeroed on creation. */
#define	p_endzero	p_startcopy

/* The following fields are all copied upon creation in fork. */
#if MAP_UAREA
#define p_sigmask p_shared_rw->us_sigmask
#define p_sigignore p_shared_rw->us_sigignore
#define	p_startcopy	p_sigcatch
#else
#define	p_startcopy	p_sigmask
	sigset_t p_sigmask;	/* Current signal mask. */
	sigset_t p_sigignore;	/* Signals being ignored. */
#endif
	sigset_t p_sigcatch;	/* Signals being caught by user. */

	u_char	p_priority;	/* Process priority. */
	u_char	p_usrpri;	/* User-priority based on p_cpu and p_nice. */
	char	p_nice;		/* Process "nice" value. */
	char	p_comm[MAXCOMLEN+1];
	struct 	pgrp *p_pgrp;	/* Pointer to process group. */

/* End area that is copied on creation. (the next item is not copied). */
#define	p_endcopy	p_addr
	struct	user *p_addr;	/* Kernel virtual addr of u-area (PROC ONLY).*/

	u_short	p_xstat;	/* Exit status for wait; also stop signal. */
	u_short	p_acflag;	/* Accounting flags. */
	struct	rusage *p_ru;	/* Exit information. XXX */

#if	MAP_UAREA
	struct ushared_rw
			*p_shared_rw;	/* shared memory */
	struct ushared_ro
			*p_shared_ro;	/* shared memory */
	char *		p_readwrite;	/* shared read/write buffer */
	vm_offset_t	p_shared_off;
#else
	struct	vmspace p_realvmspace;	/* address space */
#endif
};

#define	p_session	p_pgrp->pg_session
#define	p_pgid		p_pgrp->pg_id

/* Status values. */
#define	SIDL	1		/* Process being created by fork. */
#define	SRUN	2		/* Currently runnable. */
#define	SSLEEP	3		/* Sleeping on an address. */
#define	SSTOP	4		/* Process debugging or suspension. */
#define	SZOMB	5		/* Awaiting collection by parent. */

/* These flags are kept in p_flags. */
#define	P_ADVLOCK	0x00001	/* Process may hold a POSIX advisory lock. */
#define	P_CONTROLT	0x00002	/* Has a controlling terminal. */
#define	P_INMEM		0x00004	/* Loaded into memory. */
#define	P_NOCLDSTOP	0x00008	/* No SIGCHLD when children stop. */
#define	P_PPWAIT	0x00010	/* Parent is waiting for child to exec/exit. */
#define	P_PROFIL	0x00020	/* Has started profiling. */
#define	P_SINTR		0x00080	/* Sleep is interruptible. */
#define	P_SUGID		0x00100	/* Had set id privileges since last exec. */
#define	P_SYSTEM	0x00200	/* System proc: no sigs, stats or swapping. */
#define	P_TIMEOUT	0x00400	/* Timing out during sleep. */
#define	P_TRACED	0x00800	/* Debugged process being traced. */
#define	P_WAITED	0x01000	/* Debugging process has waited for child. */
#define	P_WEXIT		0x02000	/* Working on exiting. */
#define P_EXEC		0x04000	/* Process called exec. */

/* These flags are kept in k_flags */
#define	PK_SELECT	0x00040	/* Selecting; wakeup/waiting danger. */

/* Should probably be changed into a hold count. */
#define	P_NOSWAP	0x08000	/* Another flag to prevent swap out. */
#define	P_PHYSIO	0x10000	/* Doing physical I/O. */

/* Should be moved to machine-dependent areas. */
#define	P_OWEUPC	0x20000	/* Owe process an addupc() call at next ast. */

/*
 * MOVE TO ucred.h?
 *
 * Shareable process credentials (always resident).  This includes a reference
 * to the current user credentials as well as real and saved ids that may be
 * used to change ids.
 */
struct	pcred {
	struct	ucred *pc_ucred;	/* Current credentials. */
	uid_t	p_ruid;			/* Real user id. */
	uid_t	p_svuid;		/* Saved effective user id. */
	gid_t	p_rgid;			/* Real group id. */
	gid_t	p_svgid;		/* Saved effective group id. */
	int	p_refcnt;		/* Number of references. */
};

#ifdef KERNEL
/*
 * We use process IDs <= PID_MAX; PID_MAX + 1 must also fit in a pid_t,
 * as it is used to represent "no process group".
 */
#define	PID_MAX		30000
#define	NO_PID		30001
#define	PIDHASH(pid)	((pid) & pidhashmask)

#define SESS_LEADER(p)	((p)->p_session->s_leader == (p))
#define	SESSHOLD(s)	((s)->s_count++)
#define	SESSRELE(s) {							\
	if (--(s)->s_count == 0)					\
		FREE(s, M_SESSION);					\
}

extern struct proc *pidhash[];		/* In param.c. */
extern struct pgrp *pgrphash[];		/* In param.c. */
extern struct proc proc0;		/* Process slot for swapper. */
extern int nprocs, maxproc;		/* Current and max number of procs. */
extern int pidhashmask;			/* In param.c. */

extern struct proc *allproc; 		/* List of active procs. */
extern struct proc *zombproc;		/* List of zombie procs. */
extern struct proc *initproc, *pageproc; /* Process slots for init, pager. */
extern struct mutex allproc_lock;

#if 0
#define	NQS	32			/* 32 run queues. */
int	whichqs;			/* Bit mask summary of non-empty Q's. */
struct	prochd {
	struct	proc *ph_link;		/* Linked list of running processes. */
	struct	proc *ph_rlink;
} qs[NQS];
#endif

void proc_allocate(struct proc **np);

/* State for active invoKation of concurrent system call of a single process */
typedef struct proc_invocation {
	queue_chain_t k_servers_chain;
	struct proc *k_p;	/* my process */
	char	*k_wmesg;	/* reason for sleep */
	caddr_t k_wchan;	/* event process is awaiting */
	mach_port_t k_thread;
	int k_master_lock;
	mach_msg_header_t *k_reply_msg;
	int k_current_size;
	int k_ool_count;	/* untyped ipc: OOL descriptor count  */
	char *k_ndr;		/* untyped ipc: pointer to NDR record */
	int k_ipl;
	queue_chain_t k_sleep_link;
	boolean_t k_timedout;
	int k_flag;		/* PK_* flags */
	cthread_t cthread;	/* server thread this data belongs to */
	/* New experimental sleep mechanism */
	struct mutex lock;	/* protects event and condition */
	int event;
	condition_t condition;
} *proc_invocation_t;

#define get_proc_invocation() cthread_data(cthread_self())
#define get_proc() (((proc_invocation_t)get_proc_invocation())->k_p)

void    proc_exit(struct proc *p, int);

struct proc *pfind __P((pid_t));	/* Find process by id. */
struct pgrp *pgfind __P((pid_t));	/* Find process group by id. */

void	mi_switch __P((void));
void	resetpriority __P((struct proc *));
void	setrunnable __P((struct proc *));
void	setrunqueue __P((struct proc *));
void	sleep __P((void *chan, int pri));
int	tsleep __P((void *chan, int pri, char *wmesg, int timo));
void	unsleep __P((struct proc *));
void	wakeup __P((void *chan));
#endif	/* KERNEL */
#endif	/* !_SYS_PROC_H_ */
