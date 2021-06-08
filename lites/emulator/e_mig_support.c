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
 * $Log: e_mig_support.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:29  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/e_mig_support.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Private version of MiG reply port allocator.
 *
 *	We don't know if cthreads is in use or not but still must cope
 *	with multiple threads. Also signals etc. must be taken into
 *	account.
 */


#include <mach.h>
#include "osfmach3.h"
#if !OSFMACH3
#include <mach/mig_support.h>
#endif
#include <cthreads.h>

spin_lock_t mig_port_allocator_lock;
#define MAX_E_REPLY_PORTS 32

mach_port_t e_reply_ports[MAX_E_REPLY_PORTS];

void mig_init(void *first)
{
	int i;
	for (i = 0; i < MAX_E_REPLY_PORTS; i++)
	    e_reply_ports[i] = MACH_PORT_NULL;
	spin_lock_init(&mig_port_allocator_lock);
}

mach_port_t mig_get_reply_port()
{
	mach_port_t port = MACH_PORT_NULL;
	int i;

	spin_lock(&mig_port_allocator_lock);
	for (i = 0; i < MAX_E_REPLY_PORTS; i++) {
		port = e_reply_ports[i];
		if (MACH_PORT_VALID(port)) {
			/* Cached port found */
			e_reply_ports[i] = MACH_PORT_NULL;
			spin_unlock(&mig_port_allocator_lock);
			return port;
		}
	}
	spin_unlock(&mig_port_allocator_lock);
	/* No cached port available */
	return mach_reply_port();
}

/* An error occurred. Zap the port. Called by MiG generated code */
void mig_dealloc_reply_port(mach_port_t port)
{
	/* 
	 * No infinite recursion unless mach_port_mod_refs fails with
	 * MACH_SEND_INVALID_REPLY or MACH_RCV_INVALID_NAME
	 * continuously in which case we are dead anyway and can as
	 * well take the address exception inevitably coming when the
	 * stack is finished. Alternatively we could call
	 * task_terminate(mach_task_self()) right away with a fresh
	 * reply port.
	 */
	mach_port_mod_refs(mach_task_self(), port,
			   MACH_PORT_RIGHT_RECEIVE, -1);
}

/* MiG is now done with a reply port */
void mig_put_reply_port(mach_port_t port)
{
	int i;

	spin_lock(&mig_port_allocator_lock);
	for (i = 0; i < MAX_E_REPLY_PORTS; i++) {
		if (!MACH_PORT_VALID(e_reply_ports[i])) {
			e_reply_ports[i] = port;
			port = MACH_PORT_NULL;
			break;
		}
	}
	spin_unlock(&mig_port_allocator_lock);

	/* 
	 * We are not getting into an infinite recursion here as
	 * eventually there will be a free slot.
	 */
	if (MACH_PORT_VALID(port))
	    mig_dealloc_reply_port(port);
}

/* Export locking for signal code in case it needs it */
void e_mig_lock()
{
	spin_lock(&mig_port_allocator_lock);
}

void e_mig_unlock()
{
	spin_unlock(&mig_port_allocator_lock);
}

/* Global symbol to identify */
void emul_mig_allocator_in_use()
{}
