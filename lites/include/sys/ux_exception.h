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
 * $Log: ux_exception.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:36  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.1  92/04/21  17:17:55  rwd
 * BSDSS
 * 
 *
 */

#ifndef	_SYS_UX_EXCEPTION_H_
#define	_SYS_UX_EXCEPTION_H_

/*
 *	Codes for Unix software exceptions under EXC_SOFTWARE.
 */


#define EXC_UNIX_BAD_SYSCALL	0x10000		/* SIGSYS */

#define EXC_UNIX_BAD_PIPE	0x10001		/* SIGPIPE */

#define EXC_UNIX_ABORT		0x10002		/* SIGABRT */

#ifdef	KERNEL
#include <serv/import_mach.h>

/*
 *	Kernel data structures for Unix exception handler.
 */

struct mutex		ux_handler_init_lock;
mach_port_t		ux_exception_port;

#endif	KERNEL
#endif	_SYS_UX_EXCEPTION_H_
