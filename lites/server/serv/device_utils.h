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
 * $Log: device_utils.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.1  92/04/21  17:10:57  rwd
 * BSDSS
 * 
 */

/*
 * Support routines for device interface in out-of-kernel kernel.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <serv/import_mach.h>

#ifdef	KERNEL
#define	KERNEL__
#undef	KERNEL
#endif	KERNEL
#include <device/device_types.h>
#ifdef	KERNEL__
#undef	KERNEL__
#define	KERNEL	1
#endif	KERNEL__

extern mach_port_t	device_server_port;

/*
 * The dev_number_hash table contains both block and character
 * devices.  Distinguish the two.
 */
typedef	unsigned int xdev_t;			/* extended device type */
#define	XDEV_BLOCK(dev)	((xdev_t)(dev) | 0x80000000)
#define	XDEV_CHAR(dev)	((xdev_t)(dev))

extern void	dev_utils_init();

extern void	dev_number_hash_enter();	/* dev_t, char * */
extern void	dev_number_hash_remove();	/* dev_t */
extern char *	dev_number_hash_lookup(xdev_t);	/* dev_t */

extern int	dev_error_to_errno();		/* int */

mach_error_t bdev_name_string(dev_t, char str[]);
