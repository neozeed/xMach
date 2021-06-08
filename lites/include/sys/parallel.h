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
 * $Log: parallel.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:34  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File: 	include/sys/parallel.h
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Master lock serialization for threads within the server.
 */

#ifndef SYS_PARALLEL_H
#define SYS_PARALLEL_H

#include "data_synch.h"

#include <serv/import_mach.h>

extern struct mutex master_mutex;
extern cthread_t master_holder;

#if DATA_SYNCH
/* yes, they should still be inlined */
#else /* DATA_SYNCH */
#define master_lock() \
MACRO_BEGIN \
	mutex_lock(&master_mutex); \
	master_holder = cthread_self(); \
MACRO_END

#define master_unlock() \
MACRO_BEGIN \
	master_holder = NULL; \
	mutex_unlock(&master_mutex); \
MACRO_END

#define	unix_master() \
MACRO_BEGIN \
	if (pk->k_master_lock != 0) panic("unix_master"); \
	if (pk->k_master_lock++ == 0) \
	    master_lock(); \
MACRO_END

#define	unix_release() \
MACRO_BEGIN \
	if (pk->k_master_lock != 1) panic("unix_release"); \
	if (--pk->k_master_lock == 0) \
	    master_unlock(); \
MACRO_END

#define	unix_reset() \
MACRO_BEGIN \
	if (pk->k_master_lock > 0) \
	    pk->k_master_lock = 1; \
MACRO_END
#endif /* DATA_SYNCH */

#endif /* SYS_PARALLEL_H */
