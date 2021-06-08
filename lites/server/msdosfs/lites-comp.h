/*
 * Copyright (c) 1995 The University of Utah and
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
 *      Utah $Hdr$
 */

/*
 * this file defines a few definitions to use the msdosfs code
 * taken from the latest FreeBSD 2.0
 */

#ifndef _LITES_COMP_H
#define _LITES_COMP_H

#ifdef LITES
#define M_MSDOSFSMNT    M_TEMP          /* account for memory allocation
                                           XXX sys/malloc.h */
#define M_MSDOSFSFAT    M_TEMP          /* account for memory allocation
                                           XXX sys/malloc.h */
#define curproc         get_proc()      /* current process context */
#define VT_MSDOSFS      VT_PC           /* is that what PC means? - if not,
                                           XXX sys/vnode.h */

/*
 * BSD has a global timeval time
 */
#include <sys/kernel.h>                 /* defines get_time() macro */
#define time            (*_current_time())
static struct timeval *_current_time()
{
        static struct timeval tv;
        get_time(&tv);
        return &tv;
}

#define adjkerntz 	0		/* this says that the CMOS clock is
					   correct. !? */
#define prtactive	0		/* don't print a message in reclaim */

#define VFS_SET(x,y,z,w)		/* not needed */

#include <sys/mount.h>

#endif /* LITES */
#endif /* _LITES_COMP_H */
