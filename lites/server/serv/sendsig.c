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
 * $Log: sendsig.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1996/03/14  21:08:35  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.1.1.2  1995/03/23  01:16:58  law
 * lites-950323 from jvh.
 *
 * 12-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Make sendsig return notified/failed.
 *
 *
 */
/* 
 *	File:	serv/sendsig.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Send signal by waking up signal thread in the emulator. This
 *	is a first step on moving most signal management to the
 *	emulator.
 */

#include <serv/server_defs.h>
#include <signal_user.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

/* Return TRUE if the message was successfully sent to the task.
 * This is needed at a higer level by catch_exception_raise()
 * so it can know not to reply if the task will unblock the
 * exception its self.
 */
boolean_t sendsig(
	struct proc	*p,
	thread_t	thread,
	sig_t		action,
        int		sig,
	unsigned	code,
	int		mask)
{
	kern_return_t kr;

	if (!MACH_PORT_VALID(p->p_sigport) || !MACH_PORT_VALID(thread))
	    return FALSE;

	/* Add a send right. signal_notify consumes it. */
	kr = port_object_copy_send(thread);
	assert(kr == KERN_SUCCESS);

	kr = signal_notify(p->p_sigport, thread);
	if (kr) {
	  printf("signal_notify failed pid=%d: %s\n",
		 p->p_pid, mach_error_string(kr));
	  return FALSE;
	}
	return TRUE;
}
