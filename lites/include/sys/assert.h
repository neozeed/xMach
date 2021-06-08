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
 * $Log: assert.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:33  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	sys/assert.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	assert macro for the Lites server
 */

#ifndef _SYS_ASSERT_H_
#define _SYS_ASSERT_H_

#include "assertions.h"

#if ASSERTIONS
#define assert(expr) \
	((void) ((expr) ? 0 : assert_internal(#expr, __FILE__, __LINE__)))

#define assert_internal(exprstr, file, line) \
  (panic("Assertion `%s' failed at %s:%u\n", exprstr, file, line), 0)
#else /* ASSERTIONS */
#define assert(expr)
#endif /* ASSERTIONS */

#endif /* !_SYS_ASSERT_H_ */
