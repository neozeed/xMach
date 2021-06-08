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
 * $Log: synch_prim.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:47  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	serv/synch_prim.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Sleep/wakeup mechanisms for Lites.  This file implements a
 *	model of synchronization where locks protect data instead of
 *	execution (as was the case with the old tsleep code).  The
 *	implementation is based on the cthreads library but should not
 *	be impossible to adapt to RT threads and locks.
 */

#include <serv/server_defs.h>
#include <serv/synch_prim.h>

void pk_wakeup_from_timer(void *context, timer_element_t telt);
void condition_wait_unlock(condition_t c, mutex_t m, mutex_t m_unlock);
void condition_signal_cthread(condition_t c, cthread_t cthread);

/* 
 * Waiting is always done on a resource.  The condition that is waited
 * on is associated with the resource.  The resource is protected by a
 * mutex that is associated with the resource and the condition.  If
 * the resource doesn't have its own lock it is associated with the
 * master lock which is a default lock.
 *
 * Wakeups may occur because of four reasons:
 * 1) the resource changed state [condition_signal or condition_broadcast]
 * 2) a timeout expired [pk_wakeup(event = PKW_TIMEOUT)]
 * 3) a signal arrived [pk_wakeup(event = PKW_SIGNAL)]
 * 4) the process exited [pk_wakeup(event = PKW_EXIT)]
 *
 * Wakeups from the resource are simple.  The condition is signalled
 * as usual and the resource state is updated. Atomicity is guaranteed
 * by keeping the resource lock.
 *	mutex_lock(resource->lock);
 *	resource->state = new_state;
 *	condition_broadcast(resource->condition, resource->lock);
 *	mutex_unlock(resource->lock);
 *
 * Wakeups for the other reasons is harder as the condition is not
 * known. Only the thread is known and only the thread should be woken
 * up.  This is handled by having a per thread lock that protects an
 * event field and a condition pointer field.  This spin lock does
 * lock coupling with the spin lock protecting the condition itself.
 *
 * On sleep the thread acquires its lock and checks the event field.
 * If it should sleep it adds itself to the condition waiter list and
 * releases the locks.  It is important that the thread lock is still
 * held while acquiring the condition lock.
 *
 * On wakeup the thread is known.  The thread lock is taken, the event
 * field is updated, the condition pointer is read.  If there is a
 * condition the condition is signalled while still keeping the
 * thread lock.  Again there is lock coupling between the thread and
 * condition locks.  Now the thread is woken up from the condition.
 * If there was no condition pointer in the thread, then the thread
 * was not waiting on a condition but the thread will find the updated
 * event when attempting to sleep.  If there was a condition pointer
 * but the thread was not found in the condition then it is known that
 * the thread was already woken up from the condition but didn't yet
 * have a chance to clear its condition pointer.  This is again ok
 * because the thread will check the event field before going to sleep
 * again.
 *
 * In some unusual cases the thread will wish to sleep regardless of
 * termination or signal events occurring.  This is the case for
 * example when a process exits but its TTY must be flushed.  During
 * the flush wait the thread should sleep even if it is being exiting.
 * This kind of a situation is analogous to what used to be called
 * uninterruptible waits.  The waiter makes this situation known by
 * giving a mask to pk_condition_wait with the event it wishes to
 * ignore.
 *
 * TTY flush wait:
 *	err = KERN_SUCCESS;
 *	mutex_lock(tty->lock);
 *	while (tty->state != desired_state && err == KERN_SUCCESS)
 *		err = pk_condition_wait(pk, tty->condition, tty->lock,
 *					0, PKW_EXIT);
 *	mutex_unlock(tty->lock);
 */

mach_error_t pk_condition_wait(
	proc_invocation_t pk,
	condition_t c,
	mutex_t m,
	struct timeval *timeout,
	int mask)
{
	int events;
	mach_error_t err = KERN_SUCCESS;
	struct timer_element telt;

	if (timeout) {
		timer_element_initialize(&telt, pk_wakeup_from_timer, pk,
					 timeout);
		if (timer_element_activate(&telt))
		    return EWOULDBLOCK;	/* expired already */
	}
	mutex_lock(&pk->lock);
	events = pk->event & ~mask;
	if (events) {
		mutex_unlock(&pk->lock);
		if (events & PKW_TIMEOUT) {
			err = EWOULDBLOCK; /* the old tsleep returned this */
			pk->event &= ~PKW_TIMEOUT;
		} else if (events & PKW_EXIT) {
			err = EJUSTRETURN;
			/* pk->event &= ~PKW_EXIT; */
		} else if (events & PKW_SIGNAL) {
			/* ERESTART gets converted to EINTR in some cases */
			err = ERESTART;
			pk->event &= ~PKW_SIGNAL;
		} else
		    panic("pk_condition_wait: event = x%x", events);
	} else {
		pk->condition = c;
		ux_server_thread_busy(); /* get something to do while waiting*/
		condition_wait_unlock(c, m, &pk->lock);
		/* Here pk->condition is !0 but the thread is not in the cond*/
		pk->condition = 0;
		ux_server_thread_active(); /* keep counts up to date */
		/* 
		 * Now we might have woken up because a PKW event
		 * occurred.  The caller will see it when calling this
		 * function again.
		 */
		err = KERN_SUCCESS;
	}
	if (timeout)
	    (void) timer_element_deactivate(&telt);
	return err;
}

/* Wakeup by thread */
void pk_wakeup(proc_invocation_t pk, int event)
{
	cthread_t cthread;
	condition_t condition;

	mutex_lock(&pk->lock);
	cthread = pk->cthread;
	condition = pk->condition;

	pk->event |= event;
	if (condition)
	    condition_signal_cthread(condition, cthread);
	mutex_unlock(&pk->lock);
}

/* Timeout expiry */
void pk_wakeup_from_timer(void *context, timer_element_t telt)
{
	proc_invocation_t pk = context;

	pk_wakeup(pk, PKW_TIMEOUT);
}

/* wakeup by process. Wakes up all threads. */
void proc_wakeup_locked(struct proc *p, int event)
{
	proc_invocation_t tpk;
	queue_t	q;

	mutex_lock(&p->p_lock);
	queue_iterate(&p->p_servers, tpk, proc_invocation_t, k_servers_chain)
	{
		pk_wakeup(tpk, event);
	}
	mutex_unlock(&p->p_lock);
}

void proc_wakeup_unlocked(struct proc *p, int event)
{
	proc_invocation_t tpk;
	queue_t	q;

	queue_iterate(&p->p_servers, tpk, proc_invocation_t, k_servers_chain)
	{
		pk_wakeup(tpk, event);
	}
}

/*************************** Temporary compat interfaces. *******************/
#include <sys/kernel.h>
#include "ether_as_syscall.h"

/* Replaces serv_synch.c */

struct mutex	master_mutex = MUTEX_NAMED_INITIALIZER("master_mutex");

#define	SLEEP_QUEUE_SIZE	64	/* power of 2 */
#define	SLEEP_HASH(x)	(((integer_t)(x)>>16) & (SLEEP_QUEUE_SIZE - 1))

static queue_head_t	sleep_queue[SLEEP_QUEUE_SIZE];
static struct condition sleep_conditions[SLEEP_QUEUE_SIZE];
static char sleep_names[SLEEP_QUEUE_SIZE][10];

void sleep_init(void)
{
	int i;

	for (i = 0; i < SLEEP_QUEUE_SIZE; i++) {
		queue_init(&sleep_queue[i]);
		condition_init(&sleep_conditions[i]);
		sprintf(sleep_names[i], "sleep%d", i);
		condition_set_name(&sleep_conditions[i], sleep_names[i]);
	}
}

/* tsleep is always called with the combined spl and master lock taken */
/* Master must be held */
mach_error_t
tsleep_abs(void *chan, int pri, char *wmesg, struct timeval *timeo)
{
	proc_invocation_t pk = get_proc_invocation();
	mach_error_t kr = KERN_SUCCESS;
	int mask = 0;
	queue_t q;

	/* ALL data protected by master lock! */
	assert(chan);
	assert(pk->k_wchan == 0);
	if ((pri & PRIMASK) != PPAUSE && (pri & PCATCH) == 0)
	    mask |= PKW_EXIT;
	if ((pri & PCATCH) == 0)
	    mask |= PKW_SIGNAL;

	pk->k_wchan = chan;
	pk->k_wmesg = wmesg;
	q = &sleep_queue[SLEEP_HASH(chan)];
	queue_enter(q, pk, proc_invocation_t, k_sleep_link);

	while(kr == KERN_SUCCESS && pk->k_wchan) {
		kr = pk_condition_wait(pk,
				       &sleep_conditions[SLEEP_HASH(chan)],
				       &master_mutex,
				       timeo, mask);
	}

	if (pk->k_wchan) {
		queue_assert_member(q, pk, proc_invocation_t, k_sleep_link);
		queue_remove(q, pk, proc_invocation_t, k_sleep_link);
		queue_chain_init(&pk->k_sleep_link);
		pk->k_wchan = NULL;
		pk->k_wmesg = NULL;
	}
	if (kr == EJUSTRETURN)
	    kr = KERN_SUCCESS;
	return kr;
}

/* Master must be held */
void wakeup(void *chan)
{
	proc_invocation_t pk = get_proc_invocation();
	queue_t q;
	proc_invocation_t otherpk;

	q = &sleep_queue[SLEEP_HASH(chan)];
	queue_iterate(q, otherpk, proc_invocation_t, k_sleep_link)
	{
		if (otherpk->k_wchan == chan) {
			queue_assert_member(q, otherpk, proc_invocation_t,
					    k_sleep_link);
			queue_remove(q, otherpk, proc_invocation_t,
				     k_sleep_link);
			otherpk->k_wchan = NULL;
			otherpk->k_wmesg = NULL;
			condition_broadcast(&sleep_conditions[SLEEP_HASH(chan)]);
		}
	}
}

/* Wakeup on a single thread.  Called by select.  Master must be held */
void thread_unsleep(proc_invocation_t tpk)
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

void master_lock()
{
	mutex_lock(&master_mutex);
}

void master_unlock()
{
	mutex_unlock(&master_mutex);
}

void spl_init()
{
	/* nothing to do */
}

/* 
 * The combined master and spl lock is locked when it is previously
 * not locked and either spl is raised above zero or unix_master is
 * called.  It is released when both the master lock is released and
 * the spl level goes to zero.
 */
int spl_n(int new_level)
{
	extern int	netisr;
	extern void	dosoftnet();

	proc_invocation_t pk = get_proc_invocation();
	int old_level = pk->k_ipl;
	boolean_t has_master = pk->k_master_lock;

	if (new_level > 0
	    && old_level <= 0
	    && has_master == FALSE)
	{
		master_lock();
	} else if (new_level <= 0
		   && old_level > 0
		   && has_master == FALSE)
	{
		master_unlock();
#if !ETHER_AS_SYSCALL
		if (netisr) {
			int s = splnet();
			dosoftnet();
			splx(s);
		}
#endif
	}
	pk->k_ipl = new_level;
	return old_level;
}

void interrupt_enter(int level)
{
	spl_n(level);
}

void interrupt_exit(int level)
{
	proc_invocation_t pk = get_proc_invocation();

	spl_n(0);
	pk->k_ipl = -1;
}

void unix_master()
{
	proc_invocation_t pk = get_proc_invocation();
	int ipl = pk->k_ipl;
	boolean_t has_master = pk->k_master_lock;

	assert (has_master == FALSE); /* unix_master doesn't nest */
	if (ipl == 0)
	    master_lock();
	pk->k_master_lock = TRUE;
}

void unix_release()
{
	proc_invocation_t pk = get_proc_invocation();
	int ipl = pk->k_ipl;
	boolean_t has_master = pk->k_master_lock;

	assert (has_master == TRUE);
	if (ipl == 0)
	    master_unlock();
	pk->k_master_lock = FALSE;
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
