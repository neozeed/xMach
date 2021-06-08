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
 * $Log: shared_lock.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:34  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	sys/shared_lock.h
 *	Author:	Randall W. Dean
 *	Date:	1992
 *
 */

#ifndef	_SYS_SHARED_LOCK_H_
#define	_SYS_SHARED_LOCK_H_

#include <sys/proc.h>		/* for struct proc */

#define share_lock_init(x)\
    do {\
      spin_lock_init(&(x)->lock);\
      (x)->who = 0;\
      (x)->need_wakeup = 0;\
    } while (0)

typedef struct shared_lock {
	spin_lock_t lock;
	vm_offset_t who;
	int need_wakeup;
} shared_lock_t;

extern void		share_lock(struct shared_lock *, struct proc *);
extern int		share_lock_solid(struct shared_lock *, struct proc *);
extern void		share_unlock(struct shared_lock *, struct proc *);
extern boolean_t	share_try_lock(struct shared_lock *, struct proc *);

#endif	_SYS_SHARED_LOCK_H_
