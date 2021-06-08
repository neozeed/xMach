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
 * $Log: synch_prim.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	serv/synch_prim.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *
 */

#ifndef _SERV_SYNCH_PRIM_H_
#define _SERV_SYNCH_PRIM_H_

#define PKW_TIMEOUT	1
#define PKW_SIGNAL	2
#define PKW_EXIT	4

mach_error_t pk_condition_wait(proc_invocation_t pk, condition_t c,
			       mutex_t m, struct timeval *timeout, int mask);
void pkwakeup(proc_invocation_t pk, int event);

#endif /* !_SERV_SYNCH_PRIM_H_ */
