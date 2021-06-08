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
 * $Log: bsd_types_gen.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/08/15  06:49:32  sclawson
 * modifications from lites-1.1-950808
 *
 * Revision 1.1.1.1  1995/03/02  21:49:47  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.2  93/02/26  12:55:55  rwd
 * 	include <sys/systm.h>.
 * 	[93/02/26  12:36:53  rwd]
 * 
 * Revision 2.1  92/04/21  17:10:51  rwd
 * BSDSS
 * 
 *
 */

/*
 * Generate definitions for Mig interfaces.  MiG can't handle random
 * C definitions or expressions any better than the assembler.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/resource.h>
#include <sys/sysctl.h>

main()
{
	printf("#define\tPATH_LENGTH %d\n",
			roundup(MAXPATHLEN,sizeof(int)));
	printf("#define\tSMALL_ARRAY_LIMIT %d\n",
			4096);
	printf("#define\tFD_SET_LIMIT %d\n",
			howmany(FD_SETSIZE, NFDBITS));
	printf("#define\tGROUPS_LIMIT %d\n",
			NGROUPS);
	printf("#define\tHOST_NAME_LIMIT %d\n",
			MAXHOSTNAMELEN);
	printf("#define\tVATTR_SIZE %d\n",
	       (sizeof(struct vattr) + sizeof(int) - 1) / sizeof(int));
	printf("#define\tRUSAGE_SIZE %d\n",
	       (sizeof(struct rusage) + sizeof(int) - 1) / sizeof(int));
	printf("#define\tSIGACTION_SIZE %d\n",
	       (sizeof(struct sigaction) + sizeof(int) - 1) / sizeof(int));
	printf("#define\tSIGSTACK_SIZE %d\n",
	       (sizeof(struct sigstack) + sizeof(int) - 1) / sizeof(int));
 	printf("#define\tCTL_MAXNAME %d\n", CTL_MAXNAME);

	return (0);
};
