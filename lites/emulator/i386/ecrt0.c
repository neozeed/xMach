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
 * $Log: ecrt0.c,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:38  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/i386/ecrt0.o
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
/* 
 * 	Emulator entry point.
 */

/* just in case someone defined it to empty */
#undef register

#define cthread_sp() \
	({  int _sp__; \
	    __asm__("movl %%esp, %0" \
	      : "=g" (_sp__) ); \
	    _sp__; })

int	(*mach_init_routine)();
int exit();

extern	unsigned char	_eprol;
#ifdef __ELF__
__start()
#else
_start()
#endif
{
	/* 
	 * The server set_emulator_state() sets pc, sp, efl, ebx, edi
	 * sp points to the arguments.
	 * ebx points to the BSS dirty page (shared with data)
	 *	that needs to be cleared.
	 * edi is the clearing count for the BSS fragment.
	 */
	register char *zero_start asm("ebx");
	register int zero_count asm("edi");
	register int sp;

	/* Store the stack pointer in a variable */
	sp = cthread_sp();

	/* Clear beginning of BSS (on the page shared with DATA) */
	for ( ; zero_count > 0; zero_count--)
	    *zero_start++ = 0;

	mach_init();

asm(".globl __eprol");
asm("__eprol:");

	exit(emulator_main(sp, sp));
}
    

