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
 * $Log: timer.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:59  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	serv/timer.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Timeout handling for Lites.
 */

#include <serv/server_defs.h>
#include <sys/synch.h>
#include <sys/kernel.h>

void timer_wakeup(void);

queue_head_t timer_queue;
struct mutex timer_lock = MUTEX_NAMED_INITIALIZER("timer_lock");
mach_port_t timer_port = MACH_PORT_NULL;

/* XXX mach_msg (MK83) has a bug that overflows timeouts above about xfffff */
const struct timeval infinite_time = { 0xfffff, 0x7fffffff };
volatile struct timeval timer_next_wakeup;

void time_diff_to_time(
	struct timeval *diff,
	struct timeval *abs)	/* OUT space allocated by caller */
{
	get_time(abs);
	timevaladd(abs, diff);
	timevalfix(abs);
}

void timer_element_initialize(
	timer_element_t telt,
	void (*function)(void *, timer_element_t),
	void *context,
	struct timeval *timeout)
{
	queue_chain_init(&telt->chain);
	telt->active = FALSE;
	telt->function = function;
	telt->kludge_function = 0;
	telt->context = context;
	telt->timeout = *timeout;
}

timer_element_t timer_element_allocate(
	void (*function)(void *, timer_element_t),
	void *context,
	struct timeval *timeout)
{
	/* XXX use zalloc instead */
	timer_element_t telt = (timer_element_t) malloc(sizeof(*telt));
	queue_chain_init(&telt->chain);
	telt->active = FALSE;
	telt->function = function;
	telt->kludge_function = 0;
	telt->context = context;
	telt->timeout = *timeout;
	return telt;
}

void timer_element_deallocate(timer_element_t telt)
{
	queue_chain_init(&telt->chain);	/* XXX maybe catches bugs if any */
	free(telt);
}

/* 
 * Activate (absolute) timer element.
 * Returns TRUE if it already expired.
 */
boolean_t timer_element_activate(timer_element_t telt)
{
	struct timeval now;
	boolean_t should_wakeup = FALSE;
	timer_element_t iter;
	queue_t head = &timer_queue;

	/* Did it already expire? */
	get_time(&now);
	if (timercmp(&telt->timeout, &now, <)) {
		return TRUE;
	}
	mutex_lock(&timer_lock);
	assert(telt->active == FALSE);
	telt->active = TRUE;
	/* Sorted insert. */
	if (queue_empty(head)) {
		queue_enter(head, telt, timer_element_t, chain);
		should_wakeup = TRUE;
	} else {
		queue_iterate(head, iter, timer_element_t, chain) {
			if (timercmp(&iter->timeout, &telt->timeout, >)) {
				queue_enter_before(head, iter, telt,
						   timer_element_t, chain);
				goto inserted;
			}
		}
		/* At tail */
		queue_enter(head, telt, timer_element_t, chain);
	}
inserted:
	if (timercmp(&telt->timeout, &timer_next_wakeup, <))
	    should_wakeup = TRUE;
	mutex_unlock(&timer_lock);
	if (should_wakeup)
	    timer_wakeup();
	return FALSE;
}

/* 
 * Deactivate timeout.
 * Return TRUE if it was deactivated (that is, previously was active).
 */
boolean_t timer_element_deactivate(timer_element_t telt)
{
	queue_t head = &timer_queue;

	mutex_lock(&timer_lock);
	if (telt->active) {
		telt->active = FALSE;
		queue_assert_member(head, telt, timer_element_t, chain);
		queue_remove(head, telt, timer_element_t, chain);
		queue_chain_init(&telt->chain);
		mutex_unlock(&timer_lock);
		return TRUE;
	}
	mutex_unlock(&timer_lock);
	return FALSE;
}

/* Mach msg timeouts are relative and coarse (milliseconds) */
mach_msg_timeout_t timeval_to_msg_timeout(struct timeval *timeout)
{
	struct timeval now, tmp;
	int ticks;

	get_time(&now);
	tmp = *timeout;
	timevalsub(&tmp, &now);
	ticks = time_to_hz(&tmp);
	assert(tick > 0);
	return ticks;
}

unsigned int timer_wakeups = 0;	/* for debugging only */

/* 
 * The timer thread blocks until a timeout expires and then calls
 * ws_signal_with_locking on the wstate.
 */
noreturn timer_thread()
{
	struct timeval now, diff;
	timer_element_t telt;
	mach_msg_timeout_t msg_timeo;
	mach_msg_header_t msgh;
	struct proc *p;
	kern_return_t kr;
	queue_t head = &timer_queue;

	/* Make this thread high priority. */
	cthread_wire();
	set_thread_priority(mach_thread_self(), 1);
	system_proc(&p, "Timeout");

	mutex_lock(&timer_lock);

	while(TRUE) {
		get_time(&now);
		if (queue_empty(head)) {
			/* infinite_time is relative as well as diff */
			diff = infinite_time;
			timevaladd(&now, &diff);
			/* timer_next_wakeup is absolute */
			timer_next_wakeup = now;
		} else {
			telt = (timer_element_t) queue_first(&timer_queue);
			if (timercmp(&telt->timeout, &now, >)) {
				/* telt->timeout is absolute */
				timer_next_wakeup = diff = telt->timeout;
				timevalsub(&diff, &now);
			} else {
				/* Already expired or now */
				queue_assert_member(&timer_queue, telt,
						    timer_element_t, chain);
				queue_remove(&timer_queue, telt,
					     timer_element_t, chain);
				telt->active = FALSE;
				mutex_unlock(&timer_lock);
				telt->function(telt->context, telt);
				/* (telt may now have been deallocated) */
				mutex_lock(&timer_lock);
				continue;
			}
		}
		assert((diff.tv_sec > 0) || (diff.tv_sec == 0 && diff.tv_usec > 0));
		msg_timeo = diff.tv_sec * 1000 + diff.tv_usec / 1000;
		mutex_unlock(&timer_lock);
		/* receive from timer_port */
		kr = mach_msg(&msgh,
			      MACH_RCV_MSG|MACH_RCV_TIMEOUT|MACH_RCV_INTERRUPT,
			      0, sizeof(msgh), timer_port,
			      msg_timeo, MACH_PORT_NULL);
		mutex_lock(&timer_lock);
		timer_wakeups++; /* for debugging only */
	}
}

/* 
 * Someone called settimeofday. Adjust timeouts as most are relative
 * XXX Settimeofday should not affect the actual kernel clock.
 */
void timer_fix_timeouts_delta(struct timeval *delta)
{
	timer_element_t telt;
	queue_t head = &timer_queue;
	int count = 0;

	mutex_lock(&timer_lock);
	queue_iterate(head, telt, timer_element_t, chain) {
		timevaladd(&telt->timeout, delta);
		timevalfix(&telt->timeout);
		count++;
	};
	mutex_unlock(&timer_lock);
	if (delta->tv_sec < 0)
	    printf("timer: %d timeouts adjusted by %d.%06u seconds\n",
		   count, delta->tv_sec + 1, 1000000 - delta->tv_usec);
	else
	    printf("timer: %d timeouts adjusted by %d.%06u seconds\n",
		   count, delta->tv_sec, delta->tv_usec);
	timer_wakeup();
}

/* 
 * As timer_thread uses mach_msg to block it must be woken up by
 * sending a message in case a timeout should be taken before the
 * scheduled message timeout time.
 */
mach_msg_header_t timer_wakeup_msg;

void timer_wakeup()
{
	mach_msg(&timer_wakeup_msg, MACH_SEND_MSG|MACH_SEND_TIMEOUT,
		 sizeof(timer_wakeup_msg), 0, MACH_PORT_NULL,
		 0, MACH_PORT_NULL);
}

void timer_init()
{
	queue_init(&timer_queue);
	mutex_init(&timer_lock);

	mutex_lock(&timer_lock);

	mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
			   &timer_port);
	mach_port_insert_right(mach_task_self(), timer_port, timer_port,
			       MACH_MSG_TYPE_MAKE_SEND);

	ux_create_thread(timer_thread);

	/* Set up message once. */
	timer_wakeup_msg.msgh_bits
	    = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
	timer_wakeup_msg.msgh_size = 0;
	timer_wakeup_msg.msgh_remote_port = timer_port;
	timer_wakeup_msg.msgh_local_port = MACH_PORT_NULL;
	timer_wakeup_msg.msgh_id = 0;

	mutex_unlock(&timer_lock);
}

/* Compatibilty interfaces */

/* For internal use only! */
void timeout_did_timeout(void *context, timer_element_t telt)
{
	assert(telt && telt->function && telt->kludge_function);

	interrupt_enter(SPLSOFTCLOCK);
	telt->kludge_function(telt->context);
	interrupt_exit(SPLSOFTCLOCK);

	timer_element_deallocate(telt);
}

void timeout(void (*f)(void *), void *context, int ticks)
{
	struct timeval when, diff;
	timer_element_t telt;

	/* Ticks are actually milliseconds */
	diff.tv_sec = ticks / 1000;
	diff.tv_usec = (ticks % 1000) * 1000;
	time_diff_to_time(&diff, &when);
	telt = timer_element_allocate(timeout_did_timeout, context, &when);
	telt->kludge_function = f;
	if (timer_element_activate(telt)) {
		/* already expired. Call function manually */
		timeout_did_timeout(context, telt);
	}
}

void untimeout(void (*f)(void *), void *context)
{
	timer_element_t telt;
	queue_t head = &timer_queue;
	int s;

	/* Search for the correct telt and deactivate it */
	s = splclock();		/* XXX avoid race with realitexpire */
	mutex_lock(&timer_lock);
	queue_iterate(head, telt, timer_element_t, chain) {
		if (telt->function == timeout_did_timeout
		    && telt->kludge_function == f
		    && telt->context == context)
		{
			goto found;
		}
	}
	/* Not around */
	mutex_unlock(&timer_lock);
	splx(s);
	return;
      found:
	/* Duplicate most of timer_element_deactivate. Lock must be held... */
	if (telt->active) {
		telt->active = FALSE;
		queue_assert_member(head, telt, timer_element_t,chain);
		queue_remove(head, telt, timer_element_t, chain);
		queue_chain_init(&telt->chain);
	}
	mutex_unlock(&timer_lock);
	splx(s);
	timer_element_deallocate(telt);
}
