/* 
 * Copyright (c) 1995-1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *      Author: Bryan Ford, University of Utah CSL
 */

/* This just generates 16-bit versions of the routines in bzero.S.  */

	.code16

#define memset		i16_memset
#define blkclr		i16_blkclr
#define bzero		i16_bzero

#include <mach/machine/asm.h>

#undef B_ARG0
#define B_ARG0	 8(%bp)
#undef B_ARG1
#define B_ARG1	12(%bp)
#undef B_ARG2
#define B_ARG2	16(%bp)
#undef B_ARG3
#define B_ARG3	20(%bp)

/*
 * memset(char *addr, int ch, unsigned int length)
 */
ENTRY(memset)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	movl	B_ARG0,%edi
	movb	B_ARG1,%al
	movb	%al,%ah
	movl	%eax,%edx
	shll	$16,%eax
	movw	%dx,%ax
	movl	B_ARG2,%edx
bzero_common:
	cld
/* zero longs */
	movl	%edx,%ecx
	shrl	$2,%ecx
	rep
	stosl
/* zero bytes */
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	stosb
/* return pointer to dest */
	movl	B_ARG0,%eax
	popl	%edi
	leave
	ret

/*
 * bzero(char * addr, unsigned int length)
 */
ENTRY(blkclr)
ENTRY(bzero)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	movl	B_ARG0,%edi
	movl	B_ARG1,%edx
	xorl	%eax,%eax
	jmp	bzero_common



