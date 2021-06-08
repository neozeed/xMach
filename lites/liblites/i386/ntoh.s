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
 * $Log: ntoh.s,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
# Revision 1.1.1.1  1995/03/02  21:49:39  mike
# Initial Lites release from hut.fi
#
 * Revision 2.2  93/05/11  11:59:59  rvb
 * 	Fix comment leader.
 * 	[93/05/03  13:06:10  rvb]
 * 
 * Revision 2.1  92/04/21  17:19:04  rwd
 * BSDSS
 *
 */

#include <i386/asm.h>

Entry(ntohl)
ENTRY(htonl)
	movl	4(%esp), %eax
	rorw	$8, %ax
	ror	$16,%eax
	rorw	$8, %ax
	ret


Entry(ntohs)
ENTRY(htons)
	movzwl	4(%esp), %eax
	rorw	$8, %ax
	ret
