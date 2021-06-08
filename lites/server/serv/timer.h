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
 * $Log: timer.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	serv/timer.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	Timeout handling.
 */

#ifndef _TIMER_H_
#define _TIMER_H_

typedef struct timer_element {
	queue_chain_t chain;	/* protected by timer_lock */
	boolean_t active;	/* protected by timer_lock */
	struct timeval timeout;
	void (*function)(void *, struct timer_element *);
	void (*kludge_function)(void *); /* XXX internal for compat timeout */
	void *context;
} *timer_element_t;

#define TIMER_ELEMENT_NULL ((timer_element_t) 0)

/* Initialize a timer element with absolute time */
void timer_element_initialize(timer_element_t telt,
			      void (*function)(void *, timer_element_t),
			      void *context, struct timeval *timeout);

/* Allocate a timer element with absolute time */
timer_element_t timer_element_allocate(void (*func)(void *, timer_element_t),
				       void *context,
				       struct timeval *timeout);

/* Get rid of timer elements allocated by timer_element_allocate */
void timer_element_deallocate(timer_element_t);

/* Convert relative time to absolute time */
void time_diff_to_time(struct timeval *, struct timeval *);

/* Add and remove timeout to/from timeout queue */
boolean_t timer_element_activate(timer_element_t telt);
boolean_t timer_element_deactivate(timer_element_t telt);

/* Fix timeouts after the kernel clock was changed */
void timer_fix_timeouts_delta(struct timeval *delta);

/* Compat */
void timeout(void (*f)(void *), void *arg, int ticks);
void untimeout(void (*f)(void *), void *arg);

#endif /* _TIMER_H_ */
