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
 * $Log: emul_misc_asm.s,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
# Revision 1.1.1.2  1995/03/23  01:15:39  law
# lites-950323 from jvh.
#
 */
/* 
 *	File:	emulator/i386/misc_asm.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	save/load state & random stuff.
 */

/* XXX */
	.globl _mcount
_mcount:
	.globl mcount
mcount:
	ret

/* 
 * int emul_save_state(struct i386_thread_state *state)
 *
 * Saves current registers into struct pointed to by arg.
 * "setjmp".
 *
 * Caller saved registers eax, ecx, and edx are not saved but zeroed.
 * Returns zero normally but may return anything when "longjmped" to.
 */
	.globl _emul_save_state
	.globl emul_save_state
_emul_save_state:
emul_save_state:
	movl	4(%esp),%edx	/* point to register buffer */
	xorl	%eax,%eax	/* clear high 16 bits */
	mov	%gs,%ax
	movl	%eax,0(%edx)	/* gs to state[0] */
	mov	%fs,%ax
	movl	%eax,4(%edx)	/* fs to state[1] */
	mov	%es,%ax
	movl	%eax,8(%edx)	/* es to state[2] */
	mov	%ds,%ax
	movl	%eax,12(%edx)	/* ds to state[3] */
	movl	%edi,16(%edx)	/* edi to state[4] */
	movl	%esi,20(%edx)	/* esi to state[5] */
	movl	%ebp,24(%edx)	/* fp to state[6] */
	movl	$0,28(%edx)	/* ksp to state[7] */
	movl	%ebx,32(%edx)	/* ebx to state[8] */
	movl	$0,36(%edx)	/* edx to state[9] */
	movl	$0,40(%edx)	/* ecx to state[10] */
	movl	$0,44(%edx)	/* eax to state[11] */
	movl	0(%esp),%ecx
	movl	%ecx,48(%edx)	/* pc to state[12] */
	mov	%cs,%ax
	movl	%eax,52(%edx)	/* cs to state[13] */
	pushf			/* flags via stack */
	popl	%ecx		/* ... */
	movl	%ecx,56(%edx)	/* ... flags to state[14] */
	movl	%esp,%ecx
	movl	%ecx,60(%edx)	/* sp to state[15] */
	mov	%ss,%ax
	movl	%eax,64(%edx)	/* ss to state[16] */
	xorl	%eax,%eax	/* return zero */
	ret

/* 
 * noreturn emul_load_state(struct i386_thread_state *state)
 *
 * Loads all (but seg) registers from struct pointed to by arg.
 * "longjmp".
 *
 * Segment registers are not loaded, maybe they should? XXX
 * Maybe loading them causes an exception? Fix later. XXX
 *
 * Never returns. The stack is unwound.
 */
	.globl _emul_load_state
	.globl emul_load_state
_emul_load_state:
emul_load_state:
	/* First load a few registers */
	movl	4(%esp),%edx	/* point to register buffer */
	movl	16(%edx),%edi	/* edi state[4] */
	movl	20(%edx),%esi	/* esi state[5] */
	movl	24(%edx),%ebp	/* fp  state[6] */
	movl	32(%edx),%ebx	/* ebx state[8] */
	movl	56(%edx),%ecx	/* flags state[14] */
	pushl	%ecx		/* via stack */
	popf
	/* 
	 * Now edx points to the state, eax will point to the new stack
	 * and ecx is used as a temp variable
	 */
	movl	60(%edx),%eax	/* sp state[15] */
	movl	48(%edx),%ecx	/* pc state[12] */
	movl	%ecx,-4(%eax)	/* pc onto top of new stack */
	movl	44(%edx),%ecx	/* eax state[11] */
	movl	%ecx,-8(%eax)	/* eax onto top of new stack */

	movl	40(%edx),%ecx	/* ecx state[10] */
	movl	36(%edx),%edx	/* edx state[9] */

	subl	$8,%eax		/* Point to eax on new stack */
	movl	%eax,%esp	/* Unwind */
	popl	%eax		/* Pop eax */
	ret			/* Pop pc */
