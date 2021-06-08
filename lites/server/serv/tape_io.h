/* 
 * Mach Operating System
 * Copyright (c) 1994 Ian Dall
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * IAN DALL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * IAN DALL DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: tape_io.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	serv/tape_io.h
 *	Author:	Ian Dall
 *	Date:	Sep 1994
 *
 *	Prototypes for char IO to tape functions.
 */

int tape_open(dev_t, int, int, struct proc *);
int tape_close(dev_t, int, int, struct proc *);
int tape_read(dev_t, struct uio *, int);
int tape_write(dev_t, struct uio *, int);
int tape_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
mach_port_t tape_port(dev_t);
