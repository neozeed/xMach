/* 
 * Mach Operating System
 * Copyright (c) 1994 Ian Dall
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * IAN DALL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * IAN DALL DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: mach_error.c,v $
 * Revision 1.2  2000/10/27 01:58:46  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:46  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	libkern/mach_error.c
 *	Author:	Ian Dall
 *	Date:	September 1994
 *
 *	Replacement for mach_error which is not available in libmach_sa
 */

#include <serv/import_mach.h>

void mach_error(
	char		str[],
	mach_error_t	err)
{
	panic("mach_error %s: %s (x%x)\n", str, mach_error_string(err), err);
}
