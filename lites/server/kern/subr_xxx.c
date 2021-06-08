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
 * $Log: subr_xxx.c,v $
 * Revision 1.2  2000/10/27 01:58:45  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1996/03/14  21:08:27  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.1.1.1  1995/03/02  21:49:45  mike
 * Initial Lites release from hut.fi
 *
 * 12-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Remove find_first_set_bit() function since we can use ffs(),
 *	which can be inlined, instead.
 *
 */
/* 
 *	File:	subr_xxx.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *	Origin:	Adapted to LITES from 4.4 BSD Lite.
 *
 *	Default device switch methods.
 */
/*
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)subr_xxx.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Miscellaneous trivial functions, including many
 * that are often inline-expanded or done in assembler.
 */
#include <serv/import_mach.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>

/*
 * Unsupported device function (e.g. writing to read-only device).
 */
enodev()
{

	return (ENODEV);
}

/*
 * Unconfigured device function; driver not configured.
 */
enxio()
{

	return (ENXIO);
}

/*
 * Unsupported ioctl function.
 */
enoioctl()
{

	return (ENOTTY);
}

/*
 * Unsupported system function.
 * This is used for an otherwise-reasonable operation
 * that is not supported by the current system binary.
 */
enosys()
{

	return (ENOSYS);
}

/*
 * Return error for operation not supported
 * on a specific object or file type.
 */
eopnotsupp()
{

	return (EOPNOTSUPP);
}

/*
 * Generic null operation, always returns success.
 */
nullop()
{

	return (0);
}

int nodev_open(dev_t a, int b, int c, struct proc *d) { return ENODEV; }
int nodev_close(dev_t a, int b, int c, struct proc *d) { return ENODEV; }
int nodev_strategy(struct buf *a) { return ENODEV; }
int nodev_ioctl(dev_t a, int b, caddr_t c, int d, struct proc *e) { return ENODEV; }
int nodev_dump(dev_t a) { return ENODEV; }
int nodev_psize(dev_t a) { return ENODEV; }
int nodev_read(dev_t a, struct uio *b, int c) { return ENODEV; }
int nodev_write(dev_t a, struct uio *b, int c) { return ENODEV; }
int nodev_select(dev_t a, int b, struct proc *c) { return ENODEV; }
int nodev_stop(struct tty *a, int b) { return ENODEV; }
int nodev_reset(int a) { return ENODEV; }
kern_return_t nodev_mmap(mach_port_t device, vm_prot_t prot,vm_offset_t offset,
			 vm_size_t size, mach_port_t *pager, int unmap)
{ return ENODEV; }
int nodev_rint(int a, struct tty *b) { return ENODEV; }
int nodev_start(struct tty *a) { return ENODEV; }
int nodev_modem(struct tty *a, int b) { return ENODEV; }
int nodev_lopen(dev_t a, struct tty *b, int c) { return ENODEV; }
int nodev_lclose(struct tty * a, int b) {return ENODEV; }
int nodev_lread(struct tty * a, struct uio * b, int c) {return ENODEV; }
int nodev_lwrite(struct tty * a, struct uio * b, int c) {return ENODEV; }

int null_open(dev_t a, int b, int c, struct proc *d) { return 0; }
int null_close(dev_t a, int b, int c, struct proc *d) { return 0; }
int null_strategy(struct buf *a) { return 0; }
int null_ioctl(dev_t a, int b, caddr_t c, int d, struct proc *e) { return -1; }
int null_dump(dev_t a) { return 0; }
int null_psize(dev_t a) { return 0; }
int null_read(dev_t a, struct uio *b, int c) { return 0; }
int null_write(dev_t a, struct uio *b, int c) { return 0; }
int null_select(dev_t a, int b, struct proc *c) { return 0; }
int null_stop(struct tty *a, int b) { return 0; }
struct tty *null_tty(dev_t a) { return 0; }
int null_reset(int a) { return 0; }
kern_return_t null_mmap(mach_port_t device, vm_prot_t prot, vm_offset_t offset,
			vm_size_t size, mach_port_t *pager, int unmap)
{ return EINVAL; }
mach_port_t null_port() { return 0; }
int null_lioctl(struct tty *a, int b, caddr_t c, int d) { return -1; }
