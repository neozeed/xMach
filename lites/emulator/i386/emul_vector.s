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
 * $Log: emul_vector.s,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
# Revision 1.1.1.2  1995/03/23  01:15:38  law
# lites-950323 from jvh.
#
 */
/* 
 *	File:	emulator/i386/emul_vector.s
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Entry points from system call trampolines.
 */

#ifdef __ELF__
#define EXT(x) x
#else
#define EXT(x) _ ## x
#endif

/*
 * Emulator entry from BSD or SysV binaries.
 *
 *					ffffffff
 *		...
 *		args
 *		pc to return to
 * ESP->	flags
 *					00000000
 * system call number is in %eax
 */

/*
 * Generic call.
 */
	.globl _emul_trampoline
	.globl emul_trampoline
_emul_trampoline:
emul_trampoline:
	pushl	%eax			/* save registers that C does not */
	pushl	%ecx
	pushl	%edx

	pushl	%ebx			/* save the remaining registers */
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	pushl	%esp			/* push argument (pointer to regs) */
	call	EXT(emul_syscall)		/* call C code */
	addl	$4,%esp			/* pop parameter */
/*
 * Return
 */
/*	.globl	emul_exit */
emul_exit:
	popl	%ebp			/* restore registers */
	popl	%edi
	popl	%esi
	popl	%ebx

	popl	%edx			/* restore regs that C does not save */
	popl	%ecx
	popl	%eax
	popf				/* restore flags */
	ret				/* return to user */


	.globl _emul_linux_trampoline_3
	.globl emul_linux_trampoline_3
_emul_linux_trampoline_3:
emul_linux_trampoline_3:
	pushl	%eax			/* save registers that C does not */
	pushl	%ecx
	pushl	%edx

	pushl	%ebx			/* save the remaining registers */
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	pushl	%esp			/* push argument (pointer to regs) */
	call	EXT(emul_linux_syscall3)		/* call C code */
	addl	$4,%esp			/* pop parameter */

	popl	%ebp			/* restore registers */
	popl	%edi
	popl	%esi
	popl	%ebx

	popl	%edx			/* restore regs that C does not save */
	popl	%ecx
	popl	%eax
	popf				/* restore flags */
	ret				/* return to user */



	.globl _emul_linux_trampoline
	.globl _emul_linux_trampoline_2
	.globl emul_linux_trampoline
	.globl emul_linux_trampoline_2
/* From int 0x80 */
_emul_linux_trampoline:
emul_linux_trampoline:
	pushl	%eax			/* save registers that C does not */
	pushl	%ecx
	pushl	%edx

	pushl	%ebx			/* save the remaining registers */
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	pushl	%esp			/* push argument (pointer to regs) */
	call	EXT(emul_linux_slow_syscall)	/* call C code */
	addl	$4,%esp			/* pop parameter */
	popl	%ebp			/* restore registers */
	popl	%edi
	popl	%esi
	popl	%ebx

	popl	%edx			/* restore regs that C does not save */
	popl	%ecx
	popl	%eax
	ret				/* return to user */

/* From libc SVC replacement */
_emul_linux_trampoline_2:
emul_linux_trampoline_2:
	pushl	%ecx
	pushl	%edx

	pushl	%ebx			/* save the remaining registers */
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	pushl	%esp			/* push argument (pointer to regs) */
	call	EXT(emul_linux_syscall)	/* call C code */
	addl	$4,%esp			/* pop parameter */
	popl	%ebp			/* restore registers */
	popl	%edi
	popl	%esi
	popl	%ebx

	popl	%edx			/* restore regs that C does not save */
	popl	%ecx
	ret				/* return to user */

/* EOF */
