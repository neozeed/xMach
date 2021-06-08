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

/* This just generates 16-bit versions of the routines in bcopy.S.  */

	.code16

#define l2rbcopy	i16_l2rbcopy
#define bcopy16		i16_bcopy16
#define bcopy		i16_bcopy
#define memcpy		i16_memcpy

#include <mach/machine/asm.h>

#undef B_ARG0
#define B_ARG0	 8(%bp)
#undef B_ARG1
#define B_ARG1	12(%bp)
#undef B_ARG2
#define B_ARG2	16(%bp)
#undef B_ARG3
#define B_ARG3	20(%bp)

#if 0 /* is this useful? */

/* l2rbcopy(from, to, bcount) */

ENTRY(l2rbcopy)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushl	%esi
	movl	B_ARG2,%edx
	movl	B_ARG1,%edi
	movl	B_ARG0,%esi
/* move longs */
0:	cld
	movl	%edx,%ecx
	sarl	$2,%ecx
	js	1f
	rep
	movsl
/* move bytes */
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	movsb
1:
	popl	%esi
	popl	%edi
	leave
	ret	
#endif



/* 
 * bcopy - like l2rbcopy, but recognizes overlapping ranges and handles 
 *           them correctly.
 *	  bcopy(from, to, bytes)
 *		char *from, *to;
 *		int bytes;
 */
ENTRY(bcopy)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushl	%esi
	movl	B_ARG0,%esi
	movl	B_ARG1,%edi
bcopy_common:
	movl	B_ARG2,%edx
	cld
/*	leal	(%esi,%edx), %eax */	/* from + bytes */
        movl    %esi, %eax
	addl    %edx, %eax
	cmpl	%eax, %edi		/* <= to 	*/
	jae	0f
/*	leal	(%edi,%edx), %eax */	/* to + bytes	*/
        movl    %edi, %eax
	addl    %edx, %eax
	cmpl	%eax, %esi		/* <= from	*/
	jae	0f
	cmpl	%esi, %edi		/* from > to	*/
	jb	0f
	je 	1f

	addl	%edx, %esi; decl %esi
	addl	%edx, %edi; decl %edi
	std
	movl	%edx,%ecx
/* move bytes backwards */
	rep
	cld
	movsb
1:
/* memcpy: return dest pointer */
	movl	B_ARG0,%eax
	popl	%esi
	popl	%edi
	leave
	ret	
/* move words forwards */
0:	cld
	movl	%edx,%ecx
	sarl	$2,%ecx
	js	2f
	rep
	movsl
/* move bytes forwards */
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	movsb
2:
/* memcpy: return dest pointer */
	movl	B_ARG0,%eax
	popl	%esi
	popl	%edi
	leave
	ret	

/* memcpy(to, from, count) */

ENTRY(memcpy)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushl	%esi
	movl	B_ARG0,%edi
	movl	B_ARG1,%esi
	jmp	bcopy_common



