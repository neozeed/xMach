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
 * $Log: queue.c,v $
 * Revision 1.2  2000/10/27 01:58:46  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:46  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.1  92/04/21  17:19:06  rwd
 * BSDSS
 * 
 *
 */

/*
 *	Routines to implement queue package.
 */

#include <sys/cmu_queue.h>

/*
 *	Insert element at tail of queue.
 */
void enqueue_tail(que,elt)
	register queue_t	que;
	register queue_entry_t	elt;
{
	elt->next = que;
	elt->prev = que->prev;
	elt->prev->next = elt;
	que->prev = elt;
}

_insque(entry, pred)
	register struct queue_entry *entry, *pred;
{
	entry->next = pred->next;
	entry->prev = pred;
	(pred->next)->prev = entry;
	pred->next = entry;
}

void _remque(elt)
	register struct queue_entry *elt;
{
	(elt->next)->prev = elt->prev;
	(elt->prev)->next = elt->next;
}





#if 0
/*
 *	Insert element at head of queue.
 */
void enqueue_head(que, elt)
	register queue_t	que;
	register queue_entry_t	elt;
{
	elt->next = que->next;
	elt->prev = que;
	elt->next->prev = elt;
	que->next = elt;
}

/*
 *	Remove and return element at head of queue.
 */
queue_entry_t dequeue_head(que)
	register queue_t	que;
{
	register queue_entry_t	elt;

	if (que->next == que)
		return((queue_entry_t)0);

	elt = que->next;
	elt->next->prev = que;
	que->next = elt->next;
	return(elt);
}

/*
 *	Remove and return element at tail of queue.
 */
queue_entry_t dequeue_tail(que)
	register queue_t	que;
{
	register queue_entry_t	elt;

	if (que->prev == que)
		return((queue_entry_t)0);

	elt = que->prev;
	elt->prev->next = que;
	que->prev = elt->prev;
	return(elt);
}

/*
 *	Remove arbitrary element from queue.
 *	Does not check whether element is on queue - the world
 *	will go haywire if it isn't.
 */

/*ARGSUSED*/
void remqueue(que, elt)
	queue_t			que;
	register queue_entry_t	elt;
{
	elt->next->prev = elt->prev;
	elt->prev->next = elt->next;
}
#endif
