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
 * $Log: MASTER.h,v $
 * Revision 1.2  2000/10/27 01:53:08  welchd
 *
 * Updated to latest source
 *
 * Revision 1.5  1996/02/17  01:05:43  sclawson
 * added --enable-slice option.
 *
 * Revision 1.4  1995/08/18  18:17:51  mike
 * comment out mutex_holder_assert in DEBUG config til we figure out why it
 * causes panics
 *
 * Revision 1.3  1995/08/13  03:38:47  gback
 * added MSDOS FS option to LARGE configuration
 *
 * Revision 1.2  1995/08/10  23:24:02  gback
 * added EXT2FS support into 'LARGE' configuration
 *
 * Revision 1.1.1.1  1995/03/02  21:49:26  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	conf/MASTER
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Standard Configuration Components
 */

/*   Options that must always be there */
#define	SERVER	lites+mtime+muarea+file_ports+vnpager+old_synch

/*  Minimal configuration */
#define	BOOT	SERVER+ether+inet+ffs+pty

/*  Standard configuration */
#define	STD	BOOT+second_server+syscalltrace+compat_43+compat_oldsock+kernfs+nfs+atsys+cd9660

/*  Everything that is known to work */
#define	LARGE	STD+union+msdosfs+ext2fs+slice

#define	DEBUG	debug+lineno+diagnostic+assertions+queue_assertions+machid_register/*+mutex_holder_assert*/

/*  Compiles but untested. */
#define	UNTESTED mfs+umapfs+fdesc+cd9660+portal+lfs+sl+ns+nsip+lfs+quota+gateway+mrouting

/*  Unimplemented or does not compile. */
#define	ALMOST	ccitt+hdlc+iso+tpip+eon
#define	NOTYET	llc+procfs+sysvshm

/* Move options from NOTYET to ALMOST to UNTESTED to LARGE as work progresses.
*/
