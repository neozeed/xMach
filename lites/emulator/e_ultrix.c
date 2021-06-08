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
 * $Log: e_ultrix.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:28  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_ultrix.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Ultrix (R3000) system call handler functions.
 */

#include <e_defs.h>

/* 
 * sbrk requires a0 and v1 to remain intact. Nothing special is needed
 * here though.
 */
errno_t e_ultrix_obreak(const char *addr, char **retval)
{
	errno_t err;

	return e_obreak(addr, retval);
}
