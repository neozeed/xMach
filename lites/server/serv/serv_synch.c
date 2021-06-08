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
 * $Log: serv_synch.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1996/03/14  21:08:36  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.1.1.2  1995/03/23  01:16:51  law
 * lites-950323 from jvh.
 *
 * 09-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Use CURSIG and HAVE_SIGNALS rather than test siglist directly.
 *
 *
 */
/* 
 *	File: 	serv/serv_synch.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 */

#include "diagnostic.h"

#include <serv/server_defs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/parallel.h>
#include <sys/synch.h>
#include <sys/queue.h>
#include <sys/assert.h>
#include <serv/import_mach.h>

extern void	ux_server_thread_busy();
extern void	ux_server_thread_active();

/*
 * Sleeping threads are hashed by p_wchan onto sleep queues.
 */
 
#ifndef CTHREAD_NULL
#define CTHREAD_NULL ((cthread_t) 0)
#endif

#define	SLEEP_QUEUE_SIZE	64	/* power of 2 */
#define	SLEEP_HASH(x)	(((integer_t)(x)>>16) & (SLEEP_QUEUE_SIZE - 1))

static queue_head_t	sleep_queue[SLEEP_QUEUE_SIZE];

struct mutex	master_mutex = MUTEX_NAMED_INITIALIZER("master_mutex");
cthread_t	master_holder = 0;
struct mutex	sleep_lock = MUTEX_NAMED_INITIALIZER("sleep_lock");

/* to call from debugger */
cthread_t get_thread(void)
{
	return cthread_self();
}

/* to call from debugger */
proc_invocation_t get_pk(void)
{
	return get_proc_invocation();
}

/*
 * Initialize the sleep queues.
 */
void sleep_init(void)
{
	register int i;

	for (i = 0; i < SLEEP_QUEUE_SIZE; i++)
	    queue_init(&sleep_queue[i]);
}

/* Internal. Sleep and proc locks must be held. Wakeup is done by caller. */
void thread_unsleep_internal(proc_invocation_t tpk)
{
	queue_t	q;

	if (tpk->k_wchan) {
		q = &sleep_queue[SLEEP_HASH(tpk->k_wchan)];
		queue_assert_member(q, tpk, proc_invocation_t, k_sleep_link);
		queue_remove(q, tpk, proc_invocation_t, k_sleep_link);
		queue_chain_init(&tpk->k_sleep_link);
		tpk->k_wchan = NULL;
		tpk->k_wmesg = NULL;
	}
}

/* called from selwakeup. Many locks held. p->lock must be held */
void thread_unsleep(proc_invocation_t tpk)
{
	struct proc *p = tpk->k_p;

	assert(tpk && p);

	mutex_lock(&sleep_lock);
	thread_unsleep_internal(tpk);
	condition_broadcast(&p->p_condition);
	mutex_unlock(&sleep_lock);
}

/* No locks held by caller */
void thread_wakeup_from_timer(void *context, timer_element_t telt)
{
	proc_invocation_t tpk = context;
	struct proc *p = tpk->k_p;

	assert(tpk && p);

	mutex_lock(&p->p_lock);
	mutex_lock(&sleep_lock);
	tpk->k_timedout = TRUE;
	thread_unsleep_internal(tpk);
	condition_broadcast(&p->p_condition);
	mutex_unlock(&sleep_lock);
	mutex_unlock(&p->p_lock);

}

/*
 * Wake up all server threads in a process.
 */
unsleep_process(struct proc *p)
{
	proc_invocation_t tpk;
	queue_t	q;

	mutex_lock(&p->p_lock);
	mutex_lock(&sleep_lock);
	queue_iterate(&p->p_servers, tpk, proc_invocation_t, k_servers_chain)
	{
		assert_pk_in_p(tpk, p);
		thread_unsleep_internal(tpk);
	}
	condition_broadcast(&p->p_condition);
	mutex_unlock(&sleep_lock);
	mutex_unlock(&p->p_lock);
}

assert_pk_in_p(proc_invocation_t pk, struct proc *p)
{
	if (!p)
	    panic("assert_pk_in_p: !p");

	if (!pk)
	    panic("assert_pk_in_p: !pk");
	    
	queue_assert_member(&p->p_servers, pk, proc_invocation_t,
			    k_servers_chain);
	if (p != pk->k_p)
	    panic("assert_pk_in_p: p not in pk");
}

/*
 * Exiting, wakeup all threads servicing a process.
 * Called from task_remove_reverse.
 *
 * p is locked and protected by a ref.
 * unix_master is held (this might change).
 */
proc_wakeup(struct proc *p)
{
	proc_invocation_t otherpk;

	mutex_lock(&sleep_lock);

	queue_iterate(&p->p_servers, otherpk, proc_invocation_t,
		      k_servers_chain)
	{
		/* wakeup each thread */
		thread_unsleep_internal(otherpk);
	}
	mutex_unlock(&sleep_lock);
	condition_broadcast(&p->p_condition);
}

void tsleep_enter(void *chan, char *wmesg)
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	register queue_t	q;

	/*
	 * Zero is a reserved value, used to indicate
	 * that we have been woken up and are no longer on
	 * the sleep queues.
	 */

	assert(chan);

	/*
	 * The sleep_lock protects the sleep queues and
	 * the uu_wchan/uu_interruptible fields in threads.
	 */

	mutex_lock(&p->p_lock);
	mutex_lock(&sleep_lock);

	if (pk->k_wchan)
	    panic("tsleep_enter: pk already has wchan");
	pk->k_wchan = chan;
	pk->k_wmesg = wmesg;
	assert_pk_in_p(pk, p);
	q = &sleep_queue[SLEEP_HASH(chan)];
	{			/* sanity checking */
		proc_invocation_t tmppk;
		queue_iterate(q, tmppk, proc_invocation_t, k_sleep_link) {
			if (pk == tmppk)
			    panic("tsleep_enter: pk already in sleep queue");
		}
	}
	queue_enter(q, pk, proc_invocation_t, k_sleep_link);
	mutex_unlock(&sleep_lock);
	mutex_unlock(&p->p_lock);
}

/* 
 * The control structure and locking in this function evolved into a
 * hopeless mess. Gotta clean it up sometimes.
 */

/*
 * p is locked and stays locked throughout this function
 * sleep_lock is consumed.
 */
int tsleep_main(int pri, timer_element_t telt, boolean_t catch)
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	boolean_t timed_out = FALSE;
	int sig = 0;
	boolean_t already_busy = FALSE;

rewait:
	if (pk->k_wchan == 0) {
	    mutex_unlock(&sleep_lock);
	    goto resume;
	}

	assert_pk_in_p(pk, p);
	if (catch) {
	    p->p_flag |= P_SINTR;
	    /* must ttysleep but not sigpause on exit */
	    if (((p->p_flag&P_WEXIT) && ((pri & PRIMASK) == PPAUSE))
		|| HAVE_SIGNALS(p)) {
		    int ret = 0;
		    thread_unsleep_internal(pk);
		    mutex_unlock(&sleep_lock);

		    if (p->p_stat == SSLEEP)
			p->p_stat = SRUN;
		    p->p_flag &= ~P_SINTR;
		    if ((sig = CURSIG(p))) {
			    if (p->p_sigacts->ps_sigintr & sigmask(sig))
				ret = EINTR;
			    else
				ret = ERESTART;
		    }
		    return ret;
	    }
	}
	mutex_unlock(&sleep_lock);

	if (!already_busy && (pri & PCATCH)) {
		already_busy = TRUE; /* kludge upon kludge. busy only once! */
		ux_server_thread_busy();
	}

	pk->k_timedout = FALSE;
	if (telt && timer_element_activate(telt)) {
		/* Already timed out */
		timed_out = TRUE;
		goto after_wait;
	}

	if (p->p_stat == SRUN) /* May be using tsleep for SSTOP */
	    p->p_stat = SSLEEP;

	condition_wait(&p->p_condition, &p->p_lock);

	timed_out = pk->k_timedout;
	if (telt)
	    timer_element_deactivate(telt);

      after_wait:
	if (pk->k_wchan) {
		mutex_lock(&sleep_lock);
		if (timed_out)
		    thread_unsleep_internal(pk);
		else
		    goto rewait;
		mutex_unlock(&sleep_lock);
	}

	if (p->p_stat == SSLEEP)
	    p->p_stat = SRUN;

	if (pri & PCATCH) ux_server_thread_active();

      resume:
      { int ret = 0;
	p->p_flag &= ~P_SINTR;
	if (timed_out) {
		ret = EWOULDBLOCK;
	} else if (catch && (sig = CURSIG(p))) {
		if (p->p_sigacts->ps_sigintr & sigmask(sig))
		    ret = EINTR;
		else
		    ret = ERESTART;
	}
	return ret;
      }
}

unsigned int  tsleep_lock_mixup_count = 0;
unsigned int  tsleep_lock_ok_count = 0;

int tsleep_abs(void *chan, int pri, char *wmesg, struct timeval *timeo)
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	int			old_spl;
	int			result;
	boolean_t 		catch = pri & PCATCH;
	int sig;
	struct timer_element telt;

	tsleep_enter(chan, wmesg);

	if (timeo)
	    timer_element_initialize(&telt, thread_wakeup_from_timer,
				     pk, timeo);

	/*
	 * Network interrupt code might be called from spl_n.
	 * This code might try to wake us up.  Hence we can't
	 * hold p->p_lock or sleep_lock across spl_n,
	 * or we could deadlock with ourself.
	 */
	old_spl = spl_n(0);
	/* <--- wakeup may proceed here */
	mutex_lock(&p->p_lock);
	mutex_lock(&sleep_lock);
	unix_release();

	result = tsleep_main(pri,
			     timeo ? &telt : 0,
			     catch);

	mutex_unlock(&p->p_lock);
	/* <--- wakeup may proceed here */
	unix_master();
	splx(old_spl);

	return result;
#if 0	
	/* 
	 * XXX This complication is unnecessary as we are already on
	 * XXX the way out from tsleep
	 */
	/* 
	 * Must held some lock all the time to avoid race but locking
	 * order gets mixed up so attempt a lock all and backoff on
	 * failure strategy.
	 */
	if (mutex_try_lock(&master_mutex)) {
		/* success. Duplicate unix_master() */
		master_holder = cthread_self();
		pk->k_master_lock++;
		assert(pk->k_master_lock == 1);

		mutex_unlock(&p->p_lock);
		tsleep_lock_ok_count++;
	} else {
		/* 
		 * We ran into a locking order problem.
		 * Restart from user.
		 */
		mutex_unlock(&p->p_lock);
		if (result == 0)
		    result = ERESTART;
		unix_master();
		tsleep_lock_mixup_count++;
	}

	(void) splx(old_spl);

	return (result);
#endif
}

extern const struct timeval infinite_time; /* XXX */

int tsleep(void *chan, int pri, char *wmesg, int time_out)
{
	struct timeval diff, when;
	if (time_out) {
		diff.tv_sec = time_out / 1000;
		diff.tv_usec = (time_out % 1000) * 1000;
		if (timercmp(&diff, &infinite_time, >))
		    diff = infinite_time; /* XXX */
		time_diff_to_time(&diff, &when);
	}

	return tsleep_abs(chan, pri, wmesg, time_out ? &when : 0);
}

/*
 * Sleep on chan at pri.
 *
 * We make a special check for lbolt (the once-per-second timer)
 * to avoid keeping a separate lbolt thread or overloading the
 * timeout queue.
 */
void sleep(void *chan, int pri)
{
	tsleep(chan, pri, "sleep",(chan == (caddr_t)&lbolt) ? hz : 0);
}

unsigned int wakeup_lock_mixup_count = 0;
unsigned int wakeup_lock_ok_count = 0;

/* wakeup all threads waiting on chan */
void wakeup(void *chan)
{
	register queue_t q;
	struct proc *p;
	proc_invocation_t otherpk;

	if (!chan) {
		panic("wakeup: no chan");
		return;
	}
	q = &sleep_queue[SLEEP_HASH(chan)];
restart:
	mutex_lock(&sleep_lock);

	queue_iterate(q, otherpk, proc_invocation_t, k_sleep_link)
	{
		p = otherpk->k_p;
		if (!p || !mutex_try_lock(&p->p_lock)) {
			/* locking order screw up or p went away */
			wakeup_lock_mixup_count++;
			mutex_unlock(&sleep_lock);
			goto restart;
		}
		wakeup_lock_ok_count++;
		if (otherpk->k_wchan == chan) {
			queue_assert_member(q, otherpk, proc_invocation_t,
					    k_sleep_link);
			queue_remove(q, otherpk, proc_invocation_t,
				     k_sleep_link);
			otherpk->k_wchan = NULL;
			otherpk->k_wmesg = NULL;
			/*
			 * wakeup thread
			 */
			condition_broadcast(&p->p_condition);
		}
		mutex_unlock(&p->p_lock);
	}

	mutex_unlock(&sleep_lock);
}
/*
 * Stack of spl locks - one for each priority level.
 */
struct spl_lock_struct {
    cthread_t		holder;
    struct condition	condition;
};

struct spl_lock_struct	spl_locks[SPL_COUNT];
struct mutex spl_lock = MUTEX_NAMED_INITIALIZER("spl_lock");

char * spl_condition_names[SPL_COUNT] = {"spl0", "spl1 SOFTCLOCK", "spl2 NET",
					     "spl3 TTY/BIO", "spl4 IMP",
					     "spl5 HIGH"};
void
spl_init()
{
    register int i;
    register struct spl_lock_struct *sp;

    for (i = SPL_COUNT-1; i > 0; i--) {
	sp = &spl_locks[i];
	sp->holder = CTHREAD_NULL;
	condition_init(&sp->condition);
	condition_set_name(&sp->condition, spl_condition_names[i]);
    }
}

int
spl_n(int new_level)
{
    int	old_level;
    register int	i;
    register struct spl_lock_struct *sp;
    register cthread_t	self = cthread_self();
    proc_invocation_t pk = get_proc_invocation();
    struct proc *p = pk->k_p;

    if (pk->k_ipl < 0) {
	panic("current ipl < 0",pk->k_ipl);
	pk->k_ipl = 0;
    }
    
    if (new_level < 0) {
	panic("new_level < 0");
	new_level = 0;
    }
    
    old_level = pk->k_ipl;
    
    if (new_level > old_level) {
	/*
	 * Raising priority
	 */
	mutex_lock(&spl_lock);
	for (i = old_level + 1; i <= new_level; i++) {
	    sp = &spl_locks[i];
	    
	    while (sp->holder != self && sp->holder != CTHREAD_NULL)
		condition_wait(&sp->condition, &spl_lock);
	    sp->holder = self;
	}
	mutex_unlock(&spl_lock);
    }
    else if (new_level < old_level) {
	/*
	 * Lowering priority
	 */
	mutex_lock(&spl_lock);
	for (i = old_level; i > new_level; i--) {
	    sp = &spl_locks[i];
	    
	    assert(sp->holder == self);
	    sp->holder = CTHREAD_NULL;
	    condition_signal(&sp->condition);
	}
	mutex_unlock(&spl_lock);
    }
    pk->k_ipl = new_level;
    
	/*
	 * Simulate software interrupts for network.
	 */
	{
	    extern int	netisr;
	    extern void	dosoftnet();

	    if (new_level < SPLNET && netisr) {
		register int s = splnet();
		dosoftnet();
		splx(s);
	    }
	}

    return (old_level);
}

/*
 * Interrupt routines start at a given priority.  They may interrupt other
 * threads with a lower priority (unlike non-interrupt threads, which must
 * wait).
 */
void interrupt_enter(int level)
{
    register cthread_t	self = cthread_self();
    register struct spl_lock_struct *sp = &spl_locks[level];
    proc_invocation_t pk = get_proc_invocation();
    struct proc *p = pk->k_p;

    /*
     * Grab the lock for the interrupt priority.
     */
    
    mutex_lock(&spl_lock);
    while (sp->holder != self && sp->holder != CTHREAD_NULL)
	condition_wait(&sp->condition, &spl_lock);
    sp->holder = self;
    mutex_unlock(&spl_lock);
    
    pk->k_ipl = level;
}

void interrupt_exit(int level)
{
    register cthread_t	self = cthread_self();
    register struct spl_lock_struct *sp;
    register int		i;
    proc_invocation_t pk = get_proc_invocation();
    struct proc *p = pk->k_p;

    /*
     * Release the lock for the interrupt priority.
     */
    
    mutex_lock(&spl_lock);
    for (i = pk->k_ipl; i >= level; i--) {
	sp = &spl_locks[i];
	assert(sp->holder == self);
	sp->holder = CTHREAD_NULL;
	condition_signal(&sp->condition);
    }
    mutex_unlock(&spl_lock);
    
    /*
     * Simulate software interrupts for network.
     */
    {
	extern int	netisr;
	extern void	dosoftnet();

	if (netisr && level >= SPLNET) {
	    /*
	     * Check SPL levels down to splnet.  If none held,
	     * take a softnet interrupt.
	     */
	    for (i = level; i >= SPLNET; i--)
		if (spl_locks[i].holder != CTHREAD_NULL)
		    goto exit;

	    interrupt_enter(SPLNET);
	    dosoftnet();
	    interrupt_exit(SPLNET);

	  exit:;
	}
    }

    pk->k_ipl = -1;
}

/* XXX remains from BSDSS kern_clock.c */
/*
 * Compute number of hz until specified time.
 * Used to compute third argument to timeout() from an
 * absolute time.
 */
int hzto(tv)
	struct timeval *tv;
{
	struct timeval time, desired_time;

	desired_time = *tv;
	get_time(&time);
	if (!timercmp(&time, &desired_time, <))
	    return (0);
	timevalsub(&desired_time, &time);
	return (time_to_hz(&desired_time));
}

int
time_to_hz(tv)
	struct timeval *tv;
{
	register long	ticks, sec;

	/*
	 * If number of milliseconds will fit in 32 bit arithmetic,
	 * then compute number of milliseconds to time and scale to
	 * ticks.  Otherwise just compute number of hz in time, rounding
	 * times greater than representible to maximum value.
	 *
	 * Delta times less than 25 days can be computed ``exactly''.
	 * Maximum value for any timeout in 10ms ticks is 250 days.
	 */
	sec = tv->tv_sec;
	if (sec <= 0x7fffffff / 1000 - 1000)
		ticks = (tv->tv_sec * 1000 +
			 tv->tv_usec / 1000) / (tick / 1000);
	else if (sec <= 0x7fffffff / hz)
		ticks = sec * hz;
	else
		ticks = 0x7fffffff;

	return (ticks);
}

