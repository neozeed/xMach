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
 * 12-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Make catch_exception_raise return unsuccessful status if the
 *	exception is handled by the task (caught signal).
 *
 * 20-Jan-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Default to signal zero on unknown exceptions and EXC_SOFTWARE codes.
 *
 * $Log: ux_exception.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1996/03/14  21:08:38  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.1.1.2  1995/03/23  01:17:00  law
 * lites-950323 from jvh.
 *
 * Revision 2.2  93/02/26  12:56:35  rwd
 * 	Include sys/systm.h for printf prototypes.
 * 	[92/12/09            rwd]
 * 
 * Revision 2.1  92/04/21  17:11:14  rwd
 * BSDSS
 * 
 *
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <mach/exception.h>
#include <sys/ux_exception.h>

#include <serv/import_mach.h>

/*
 *	Unix exception handler.
 */

/*
 * Returns exception port to map exceptions to signals.
 */
mach_port_t ux_handler_setup()
{
  	kern_return_t	r;
	mach_port_t	ux_local_port;

	/*
	 *	Allocate the exception port.
	 */
	r = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
			       &ux_local_port);
	if (r != KERN_SUCCESS)
		panic("ux_handler_setup: can't allocate");

	r = mach_port_insert_right(mach_task_self(),
				   ux_local_port, ux_local_port,
				   MACH_MSG_TYPE_MAKE_SEND);
	if (r != KERN_SUCCESS)
		panic("ux_handler_setup: can't acquire send right");

	/*
	 * Add it to the server port set.
	 */
	ux_server_add_port(ux_local_port);

	/*
	 * Return the exception port.
	 */

	return (ux_local_port);
}

/* forward */
void ux_exception(integer_t exception, integer_t code, integer_t subcode,
		  int *ux_signal, int *ux_code);

kern_return_t catch_exception_raise(
	mach_port_t		exception_port,
	thread_t		thread,
	task_t			task,
	integer_t		exception,
	integer_t		code,
	integer_t		subcode)
{
	int	signal = 0, ux_code = 0;
	int	ret = KERN_SUCCESS;

	/*
	 *	Catch bogus ports
	 */
	if (MACH_PORT_VALID(task) && MACH_PORT_VALID(thread)) {

	    /*
	     *	Convert exception to unix signal and code.
	     */
	    ux_exception(exception, code, subcode, &signal, &ux_code);

	    /*
	     *	Send signal.
	     */
	    if (signal != 0) {
		    if (thread_psignal(task, thread, signal, ux_code)) {
		      /* The tasks signal thread has been notified and
		       * it will unblock the thread which got the exception
		       * so we don't want exc_server() reply.
		       */
		      ret = MIG_NO_REPLY;
		    }
	    }
	} else {
	    printf("catch_exception_raise: task %x thread %x\n",
		   task, thread);
	    ret = KERN_INVALID_ARGUMENT;
	}

	if (MACH_PORT_VALID(task))
		(void) mach_port_deallocate(mach_task_self(), task);

	if (MACH_PORT_VALID(thread))
		(void) mach_port_deallocate(mach_task_self(), thread);

	return(ret);
}

#if OSFMACH3
kern_return_t catch_exception_raise_state(
	mach_port_t		port,
	exception_type_t	exception,
	exception_type_t	code,
	exception_data_t	subcode,
	thread_state_flavor_t	*flavor,
	thread_state_t		in_state,
	thread_state_t		out_state)
{
	panic("catch_exception_raise_state");
}

kern_return_t catch_exception_raise_state_identity(
	mach_port_t		port,
	mach_port_t		thread,
	mach_port_t		task,
	exception_type_t	exception,
	exception_type_t	code,
	exception_data_t	subcode,
	thread_state_flavor_t	*flavor,
	thread_state_t		in_state,
	thread_state_t		out_state)
{
	panic("catch_exception_raise_state_identity");
}
#endif /* OSFMACH3 */

boolean_t machine_exception(integer_t, integer_t, integer_t, int *, int *);

/*
 *	ux_exception translates a mach exception, code and subcode to
 *	a signal and u.u_code.  Calls machine_exception (machine dependent)
 *	to attempt translation first.
 */

void ux_exception(integer_t exception, integer_t code, integer_t subcode,
		  int *ux_signal, int *ux_code)
{
	/*
	 *	Try machine-dependent translation first.
	 */
	if (machine_exception(exception, code, subcode, ux_signal, 
	    ux_code))
		return;
	
	switch(exception) {

	    case EXC_BAD_ACCESS:
		if (code == KERN_INVALID_ADDRESS)
		    *ux_signal = SIGSEGV;
		else
		    *ux_signal = SIGBUS;
		break;

	    case EXC_BAD_INSTRUCTION:
	        *ux_signal = SIGILL;
		break;

	    case EXC_ARITHMETIC:
	        *ux_signal = SIGFPE;
		break;

	    case EXC_EMULATION:
		*ux_signal = SIGEMT;
		break;

	    case EXC_SOFTWARE:
		switch (code) {
		    case EXC_UNIX_BAD_SYSCALL:
			*ux_signal = SIGSYS;
			break;
		    case EXC_UNIX_BAD_PIPE:
		    	*ux_signal = SIGPIPE;
			break;
		    case EXC_UNIX_ABORT:
			*ux_signal = SIGABRT;
			break;
		    default:
			printf("ux_exception: unknown EXC_SOFTWARE code x%x\n",
			       code);
			*ux_signal = 0;
			break;
		}
		break;

	    case EXC_BREAKPOINT:
		*ux_signal = SIGTRAP;
		break;
	    default:
		printf("ux_exception: unknown exception x%x code x%x\n",
		       exception, code);
		*ux_signal = 0;
		break;
	}
}
