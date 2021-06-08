/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994 Johannes Helander
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND JOHANNES HELANDER ALLOW FREE USE OF THIS
 * SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND JOHANNES
 * HELANDER DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: emulator_base.c,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
 * Revision 1.5  1995/08/21  20:10:23  mike
 * further qualify HPBSD test
 *
 * Revision 1.4  1995/08/18  17:57:12  mike
 * hack for cross-building under BSD/SOM
 *
 * Revision 1.3  1995/07/12  00:38:35  sclawson
 * Added fix for using GNU ld v2.x when compiling for mach UX on the i386.
 *
 * Revision 1.2  1995/07/11  21:24:30  sclawson
 * fixed for gnu ld v.2  on the x86
 *
 * Revision 1.1.1.2  1995/03/23  01:15:41  law
 * lites-950323 from jvh.
 *
 */
/* 
 * Print arguments to emulation link command. Some of these are hard
 * to figure out in a Makefile.
 */

#include <machine/param.h>

/* Use macro TARGET_* as we might be cross compiling and host defines on */

#if defined(TARGET_i386) || defined(TARGET_I386)
main() {
	/* XXX this is really linker dependent, not system dependent. */
#	ifdef linux
        printf("-e __start -qmagic -static -Ttext 0x%x\n",
               EMULATOR_BASE + 0x1000);
#	elif OSF1
	printf("-e _start -Ttext %x\n", EMULATOR_BASE + 0x1000);
#	else
#       ifdef GNU_LD2
#include <sys/exec.h>

        printf("-e __start -qmagic -Ttext 0x%x\n",
		 EMULATOR_BASE + sizeof(struct exec) + 0x1000);
#       else
	/* No 0x here as the old linker makes the address zero! */
        printf("-e __start -T %x\n", EMULATOR_BASE + 0x1000);
#	endif
#	endif
	return 0;
}
#elif defined(TARGET_mips)
main() {
	printf("-e __start -T %x -D %x\n",
               EMULATOR_BASE,
               EMULATOR_BASE + (1024*1024));
	return 0;
}
#elif defined(TARGET_ns532)
#include <sys/exec.h>

main() {
#	ifdef GNU_LD2
	printf("-e __start -Ttext %lx\n", EMULATOR_BASE + sizeof (struct exec) + 0x1000);
#	else
	printf("-e __start -Ttext %x\n", EMULATOR_BASE + 0x1000);
#	endif
	return 0;
}
#elif defined(TARGET_PARISC)
/* XXX hack, hack, hack: assume SOM for ode environment */
main() {
        printf("%x\n", EMULATOR_BASE + 0x1000);
	return 0;
}
#elif defined(hp800) && __GNUC_MINOR__ == 5
/* XXX hack, hack, hack: assume SOM for BSD cross build */
main() {
	printf("-x -N -u $START$ -R %x\n", EMULATOR_BASE + 0x1000);
	return 0;
}
#elif defined(TARGET_parisc)
main() {
        printf("-N -Ttext %x\n", EMULATOR_BASE + 0x1000);
	return 0;
}
#elif defined(TARGET_alpha)
main() {
        printf("-e __start -T %lx -D %lx -non_shared\n",
	       EMULATOR_BASE + 0x10000, EMULATOR_BASE + (1024 * 1024));
	return 0;
}
#elif defined(TARGET_sun)
#include <sys/exec.h>
main() {
	printf("%x\n", EMULATOR_BASE + sizeof (struct exec));
	return 0;
}
#endif
