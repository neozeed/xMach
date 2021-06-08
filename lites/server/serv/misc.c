/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * 18-Oct-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Remove test on bfreelist from boot() and use get_proc() to find
 *	the first argument for sync.
 *
 * 06-Oct-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *    Enabled the sync on boot code.
 *
 * 15-Jan-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Support for multi-threaded programs.
 *
 * $Log: misc.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/03/23  01:44:05  law
 * Update to 950323 snapshot + utah changes
 *
 * Revision 1.1.1.2  1995/03/22  23:26:32  law
 * Pure lites-950316 snapshot.
 *
 * Revision 2.6  93/05/14  15:19:00  rvb
 * 	Make Gcc happy -> less warnings
 * 
 * Revision 2.5  93/04/27  12:37:39  rwd
 * 	Fix MAP_UAREA code.
 * 	[93/03/16            rwd]
 * 
 * Revision 2.4  93/02/26  12:56:11  rwd
 * 	Include sys/systm.h for printf prototypes.
 * 	[92/12/09            rwd]
 * 	Fixed typo in sysctrace.
 * 	[92/07/09            rwd]
 * 
 * Revision 2.3  92/05/25  14:46:29  rwd
 * 	Added syscalltrace().
 * 	[92/05/25            rwd]
 * 
 * Revision 2.2  92/04/22  14:01:15  rwd
 * 	Remove some obsolete code.  Fix includes.
 * 	[92/04/22            rwd]
 * 
 * Revision 2.1  92/04/21  17:10:55  rwd
 * BSDSS
 * 
 *
 */
#include "osfmach3.h"
#include "map_time.h"
#include "second_server.h"
#include "map_uarea.h"

#include <serv/server_defs.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/synch.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <serv/device.h>
#include <serv/device_utils.h>
#include <serv/import_mach.h>
#include <serv/syscalltrace.h>

#if OSFMACH3
extern security_token_t security_id;
#endif

#if	SECOND_SERVER
extern int	second_server;
#endif	/* SECOND_SERVER */

mach_error_t swapon(){
    printf("swapon called");
    return EOPNOTSUPP;
}

/*
 * New kernel interfaces.
 */

#if MAP_TIME
volatile struct mapped_time_value *mapped_time_memory = NULL;

void init_mapped_time()
{
	kern_return_t rc;
	mach_port_t device_port, pager = MACH_PORT_NULL;

	rc = device_open(device_server_port,
#if OSF_LEDGERS
			 MACH_PORT_NULL,
#endif
			 D_READ|D_WRITE,
#if OSFMACH3
			 security_id,
#endif
			 "time",
			 &device_port);
	if (rc != D_SUCCESS) panic("unable to open device time %s",
				   mach_error_string(rc));

	rc = device_map(device_port, VM_PROT_READ,
			0, sizeof(time_value_t), &pager, FALSE);
	if (rc != D_SUCCESS) panic("unable to map device time %s",
				   mach_error_string(rc));
	if (pager == MACH_PORT_NULL) panic("unable to map device time %s",
					   mach_error_string(rc));
	
	rc = vm_map(mach_task_self(), (vm_address_t *)&mapped_time_memory,
		    sizeof(time_value_t), 0, TRUE,
		    pager, 0, 0, VM_PROT_READ, 
		    VM_PROT_READ, VM_INHERIT_NONE);
	if (rc != D_SUCCESS) panic("unable to vm_map device time %s",
				   mach_error_string(rc));

	rc = mach_port_deallocate(mach_task_self(), pager);
	if (rc != KERN_SUCCESS) panic("unable to deallocate pager %s",
				      mach_error_string(rc));
}

/* XXX move to sys/kernel.h */
void microtime(tvp)
	struct timeval *tvp;
{
	*tvp = *(struct timeval *)mapped_time_memory;
}

#else MAP_TIME
init_mapped_time(){}

void get_time(tvp)
	struct timeval *tvp;
{
	time_value_t	time_value;

	kern_timestamp(&time_value);
	tvp->tv_sec = time_value.seconds;
	tvp->tv_usec = time_value.microseconds;
}

void microtime(tvp)
	struct timeval *tvp;
{
	get_time(tvp);
}

#endif MAP_TIME

void set_time(tvp)
	struct timeval *tvp;
{
	struct timeval delta, now;
	time_value_t	time_value;

	get_time(&now);
	delta = *tvp;
	timevalsub(&delta, &now);
	timevalfix(&delta);

	time_value.seconds = tvp->tv_sec;
	time_value.microseconds = tvp->tv_usec;

	(void) host_set_time(privileged_host_port, time_value);
	timer_fix_timeouts_delta(&delta);
}

int	waittime = -1;

void Debugger()
{
#if	SECOND_SERVER
	if (second_server) {
		printf("Debugger\n");
#if defined(i386)
		asm("int3");
#elif defined(mips)
		asm("break 0");	/* User level breakpoint */
#else
		task_suspend(mach_task_self());
#endif
		return;
	}
#endif	/* SECOND_SERVER */

	boot(FALSE, RB_NOSYNC | RB_DEBUGGER);

}

/* Checked by proc_died() */
extern volatile boolean_t server_is_going_down;

#define DELAY(n) { int i; for (i = (n); i > 0; i--); }

void boot(
	boolean_t	paniced,
	int		flags)
{
	register struct buf *bp;
	int	iter, nbusy, obusy;
	struct proc *p;
   
#if 0 /* Does not work if ETHER_AS_SYSCALL */
	(void) splnet();
#endif
	/* 
	 * Zap all remaining tasks.  This is nice when running as
	 * second server as this way no junk will be left over.
	 */
	server_is_going_down = TRUE;

	for (p = (struct proc *) allproc; p != NULL; p = p->p_next)
	  if (!paniced && !(flags & (RB_KDB | RB_DEBUGGER)))
	    if ((p->p_flag & P_SYSTEM) == 0)
		proc_zap(p, 0);

	if (((flags & RB_NOSYNC) == 0)
	    && (waittime < 0))
	{
		waittime = 0;
		printf("syncing disks... ");

		sync(get_proc(), 0, 0);
		obusy = 0;

		for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
			    if ((bp->b_flags & (B_BUSY | B_INVAL)) == B_BUSY)
				nbusy++;
			if (nbusy == 0)
			    break;
			printf("%d ", nbusy);

			if (nbusy != obusy)
			    iter = 0;
			obusy = nbusy;

			DELAY(40000 * iter);
		}
		if (nbusy)
		    printf("giving up\n");
		else
		    printf("done\n");
		DELAY(10000);			/* wait for printf to finish */

	}

#if	SECOND_SERVER
	if (second_server) {
		second__exit();
		server_is_going_down = FALSE;
		return;
	}
#endif	/* SECOND_SERVER */
	(void) host_reboot(privileged_host_port, flags);
	server_is_going_down = FALSE;
}

void thread_read_times(thread, utv, stv)
	thread_t	thread;
	time_value_t	*utv;
	time_value_t	*stv;
{
	struct thread_basic_info	bi;
	mach_msg_type_number_t		bi_count;

	bi_count = THREAD_BASIC_INFO_COUNT;
	(void) thread_info(thread,
			   THREAD_BASIC_INFO,
			   (thread_info_t)&bi,
			   &bi_count);

	*utv = bi.user_time;
	*stv = bi.system_time;
}

/*
 *	Priorities run from 0 (high) to 31 (low).
 *	The user base priority is 12.
 *	priority = 12 + nice / 2.
 */

void set_thread_priority(thread, pri)
	thread_t	thread;
	int		pri;
{
#if OSFMACH3
	kern_return_t kr;
	struct policy_timeshare_base base = { pri };
	struct policy_timeshare_limit limit = { pri };

	kr = thread_set_policy(thread, default_processor_set, POLICY_TIMESHARE,
			       (policy_base_t)&base,
			       POLICY_TIMESHARE_BASE_COUNT,
			       (policy_limit_t)&limit,
			       POLICY_TIMESHARE_LIMIT_COUNT);
	assert(kr == KERN_SUCCESS);
#else /* OSFMACH3 */
	(void) thread_max_priority(thread, default_processor_set, pri);
	(void) thread_priority(thread, pri, FALSE);
#endif /* OSFMACH3 */
}

#if MAP_UAREA
#include <sys/proc.h>
#include <sys/parallel.h>
#define BACKOFF_SECS 5
#define SHARED_PRIORITY 0

extern int hz;

boolean_t share_try_lock(struct shared_lock *lock, struct proc *p)
{
	if (spin_try_lock(&lock->lock)) {
		if (p->p_shared_rw->us_inuse)
			lock->who = (vm_offset_t)p | KERNEL_USER;
		return TRUE;
	} else
		return FALSE;
}

void share_lock(struct shared_lock *x, struct proc *p)
{
    if (p->p_shared_rw->us_inuse) {
      while (!spin_try_lock(&(x)->lock) && !share_lock_solid(x, p));
      (x)->who = (vm_offset_t)(p) | KERNEL_USER;
    } else {
      spin_lock(&(x)->lock);
    }
}

void share_unlock(struct shared_lock *x, struct proc *p)
{
	proc_invocation_t pk = get_proc_invocation();

    if (p->p_shared_rw->us_inuse) {
      (x)->who = 0;
      spin_unlock(&(x)->lock);
      if ((x)->need_wakeup) {
	unix_master();
	wakeup((caddr_t)x);
	unix_release();
      }
    } else {
      spin_unlock(&(x)->lock);
    }
}

int share_lock_solid(struct shared_lock *x, struct proc *p)
{
	proc_invocation_t pk = get_proc_invocation();

	unix_master();
	x->need_wakeup++;
	if (spin_try_lock(&x->lock)) {
		x->need_wakeup--;
		unix_release();
		return 1;
	}
/*	printf("[%lx]Share_lock_solid %lx\n",(vm_offset_t)x, x->who);*/
	if (tsleep((caddr_t)x, SHARED_PRIORITY,"share lock solid",
		   BACKOFF_SECS * hz) == EWOULDBLOCK) {
	    printf("[%lx]Taking scribbled share_lock\n",(vm_offset_t)x);
	    share_lock_init(x);
	    x->need_wakeup++;
	}
	(x)->need_wakeup--; /* This protected by the master lock */
/*	printf("[%lx]Share_lock_solid\n",(vm_offset_t)x);*/
	unix_release();
	return 0;
}

#endif MAP_UAREA

mach_error_t sysctrace(p, _uap_, retval)
    register struct proc *p;
    void *_uap_;
    int *retval;
{
    register struct args {
	int pid;
    } *uap = (struct args *)_uap_;
#if SYSCALLTRACE
    syscalltrace = uap->pid;
#endif
    return 0;
}
