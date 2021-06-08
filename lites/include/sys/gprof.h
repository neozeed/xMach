/*
 * Mach Operating System
 * Copyright (c) 1993 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log: gprof.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:34  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.2  93/08/11  14:32:37  mrt
 * 	Moved from UX.
 * 	Added external symbols for user applications
 * 	[93/08/06            bershad]
 * 
 * Revision 2.2  93/02/16  15:42:52  mrt
 * 		Definitions for gprof data structures.
 * 	[92/05/03            cmaeda]
 * 
 * 		Rewritten to correspond to new mcount implementation.
 * 	[92/04/14            cmaeda]
 * 
 * 	File:	sys/gprof.h
 * 	Author:	Chris Maeda, Carnegie Mellon University 
 * 	Date:	March 1992
 * 
 *
 * Defines data structures for in-server profiling a la gprof.
 *
 */

#ifndef	_GPROF_H_
#define _GPROF_H_
/*
 * The profiler keeps two datastructures, a pc histogram and a call graph.
 */

#define CHUNK unsigned short	/* type of entry in the pc histogram */

extern int gprof_no_startup_from_crt0;    /* set to TRUE for no default profiling */
extern int gprof_no_control_server;      /* set to TRUE for no control server*/
extern int gprof_no_auto_initialization_from_crt0;	/* set to TRUE to eliminate monstartup */

/*
 * An edge of the call graph.
 *
 * Note that this looks a lot like a struct gprof_call below.
 */

typedef struct _callarc {
	unsigned long	ca_from;
	unsigned long	ca_to;
	unsigned long	ca_count;
	struct _callarc *ca_next;
} callarc_t;

#define CG_RESOL 	4 /* resolution of call graph entries (in bytes) */
#define CG_SHIFT	2

#define GPROF_SAMPLE_INTERVAL 1000 /* milliseconds */


/*
 * The server provides monitoring data in a piece of virtual memory
 * that is coincidentally layed out exactly like a gprof file.
 *
 * The first thing in the buffer is this header immediately followed
 * by the pc histogram.  The first entry of the histogram applies to
 * pc values starting at LOW.  The last entry of the histogram
 * applies to pc values starting at HIGH.  NBYTES is the size of the
 * histogram plus the header.
 *
 * The number of entries in the histogram is
 * (NBYTES - sizeof(struct gprof_header)) / sizeof(CHUNK).
 *
 * The span of each histogram entry is
 * (HIGH - LOW) / number of entries.
 */

struct gprof_header {
	unsigned long 	low;
	unsigned long 	high;
	long 		nbytes;
};

/*
 * After the histogram comes the call count.  Each call graph entry
 * records the number of calls made to a function from a single pc
 * value.  (A function can be called from many places.)
 *
 * FROM is the caller's pc described as an offset into the program text.
 * TO is the address of the called function.
 * NCALLS is the call count.
 */

struct gprof_call {
	unsigned long	from;
	unsigned long	to;
	unsigned long	ncalls;
};


typedef char *char_array;


#endif	/* _GPROF_H_ */
