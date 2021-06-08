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
 * $Log: import_mach.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/08/18  18:01:34  mike
 * add mutex_init call to ensure holder field is initialized
 * thought this might be the problem with mutex assertion triggering, but it wasn't
 *
 * Revision 1.1.1.2  1995/03/23  01:16:49  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	 serv/import_mach.h
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	MACH interface definitions and data for out-of-kernel kernel.
 */

#ifndef SERV_IMPORT_MACH_H
#define SERV_IMPORT_MACH_H

#ifndef __INLINE__
#define __INLINE__ extern inline
#endif

/* 
 * OSF headers include string.h [sic].
 * It needs KERNEL to be defined.
 * So we include it in advance here.
 */
#include <string.h>
#include <sys/types.h>

/*
 * <mach/mach.h> must be included with 'KERNEL' off
 */
#ifdef	KERNEL
#include <sys/assert.h>
#define	KERNEL__
#undef	KERNEL
#endif	KERNEL

#ifdef DEBUG
#define DEBUG__ DEBUG
#undef DEBUG
#endif

#define MACH_IPC_COMPAT 0

#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <mach/mig_errors.h>
#include <mach/mach_host.h>
#include <mach/error.h>
#include <mach/machine.h>
#include <cthreads.h>
#include <device/device.h>

#include "mutex_holder_assert.h"
#if MUTEX_HOLDER_ASSERT
void	panic(const char *, ...);

#undef mutex_init
#define	mutex_init(m) \
	MACRO_BEGIN \
	spin_lock_init(&(m)->lock); \
	cthread_queue_init(&(m)->queue); \
	spin_lock_init(&(m)->held); \
	(m)->holder = 0; \
	MACRO_END

#undef mutex_try_lock
__INLINE__ boolean_t mutex_try_lock(mutex_t m)
{
	if (spin_try_lock(&m->held)) {
		assert(m->holder == 0);
		m->holder = cthread_self();
		return TRUE;
	}
	return FALSE;
}

#undef mutex_lock
#define mutex_lock(m) \
	MACRO_BEGIN \
	if (!spin_try_lock(&(m)->held)) { \
		mutex_lock_solid(m); \
	} \
	assert((m)->holder == 0); \
	(m)->holder = cthread_self(); \
	MACRO_END

#undef mutex_unlock
#define mutex_unlock(m) \
	MACRO_BEGIN \
	assert((m)->holder == cthread_self()); \
	(m)->holder = 0; \
	spin_unlock(&(m)->held); \
	if (cthread_queue_head(&(m)->queue, vm_offset_t) != 0) { \
		mutex_unlock_solid(m); \
	} \
	MACRO_END
#endif	/* MUTEX_HOLDER_ASSERT */

#ifdef MACHID_REGISTER
#if MACHID_REGISTER
#include <servers/machid_lib.h>
#endif
#endif

#ifdef DEBUG__
#define DEBUG DEBUG__
#undef DEBUG__
#endif

#ifdef	KERNEL__
#undef	KERNEL__
#define	KERNEL	1
#endif	KERNEL__

#include "osfmach3.h"
#if OSFMACH3

#include <mach/machine/vm_param.h>
#include <sys/macro_help.h>
#include <mach/bootstrap.h>

typedef mach_port_seqno_t mach_msg_seqno_t;
typedef thread_port_array_t thread_array_t;
typedef mach_port_t thread_t;
typedef mach_port_t task_t;

#define MUTEX_NAMED_INITIALIZER(Name) MUTEX_INITIALIZER
#define CONDITION_NAMED_INITIALIZER(Name) CONDITION_INITIALIZER

#endif /* OSFMACH3 */

#endif /* SERV_IMPORT_MACH_H */
