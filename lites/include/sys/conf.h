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
 * $Log: conf.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:33  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	sys/conf.h
 *	Origin:	Adapted to Lites from 4.4 BSD Lite.
 *
 */
/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)conf.h	8.3 (Berkeley) 1/21/94
 */

#ifndef _SYS_CONF_H_
#define _SYS_CONF_H_

#ifdef LITES
#include <serv/import_mach.h>
#endif

#define	C_MINOR		(0x100)		/* not all minors have same name */
#define	C_BLOCK(n)	(0x200 | ((n)<<16))
					/* fold 'n' minor device numbers
					   into partitions on same device */
#define	C_BLOCK_GET(f)	(((f)>>16) & 0xFF)

/*
 * Definitions of device driver entry switches
 */

struct buf;
struct proc;
struct tty;
struct uio;
struct vnode;

struct bdevsw {
        char    *d_name;
	int	d_flags;
	int	(*d_open)	__P((dev_t dev, int oflags, int devtype,
				     struct proc *p));
	int	(*d_close)	__P((dev_t dev, int fflag, int devtype,
				     struct proc *p));
	int	(*d_strategy)	__P((struct buf *bp));
	int	(*d_ioctl)	__P((dev_t dev, ioctl_cmd_t cmd, caddr_t data,
				     int fflag, struct proc *p));
	int	(*d_dump)	();	/* parameters vary by architecture */
	int	(*d_psize)	__P((dev_t dev));
};

#ifdef KERNEL
extern struct bdevsw bdevsw[];
#endif

struct cdevsw {
        char    *d_name;
	int	d_flags;
	int	(*d_open)	__P((dev_t dev, int oflags, int devtype,
				     struct proc *p));
	int	(*d_close)	__P((dev_t dev, int fflag, int devtype,
				     struct proc *));
	int	(*d_read)	__P((dev_t dev, struct uio *uio, int ioflag));
	int	(*d_write)	__P((dev_t dev, struct uio *uio, int ioflag));
	int	(*d_ioctl)	__P((dev_t dev, ioctl_cmd_t cmd, caddr_t data,
				     int fflag, struct proc *p));
	int	(*d_stop)	__P((struct tty *tp, int rw));
	int	(*d_reset)	__P((int uban));	/* XXX */
	struct	tty *(*d_tty)	__P((dev_t dev));
	int	(*d_select)	__P((dev_t dev, int which, struct proc *p));
	/* return pager port. eg. device_map */
	kern_return_t (*d_mmap)	__P((mach_port_t, vm_prot_t, vm_offset_t,
				     vm_size_t, mach_port_t *, int));
	int	(*d_strategy)	__P((struct buf *bp));
	mach_port_t (*d_port)	__P((dev_t));
};

#ifdef KERNEL
extern struct cdevsw cdevsw[];

/* symbolic sleep message strings */
extern char devopn[], devio[], devwait[], devin[], devout[];
extern char devioc[], devcls[];
#endif

struct linesw {
	int	(*l_open)	__P((dev_t dev, struct tty *tp));
	int	(*l_close)	__P((struct tty *tp, int flag));
	int	(*l_read)	__P((struct tty *tp, struct uio *uio,
				     int flag));
	int	(*l_write)	__P((struct tty *tp, struct uio *uio,
				     int flag));
	int	(*l_ioctl)	__P((struct tty *tp, ioctl_cmd_t cmd,
				     caddr_t data, int flag, struct proc *p));
	int	(*l_rint)	__P((int c, struct tty *tp));
	int	(*l_start)	__P((struct tty *tp));
	int	(*l_modem)	__P((struct tty *tp, int flag));
};

#ifdef KERNEL
extern struct linesw linesw[];
#endif

struct swdevt {
	dev_t	sw_dev;
	int	sw_flags;
	int	sw_nblks;
	struct	vnode *sw_vp;
};
#define	SW_FREED	0x01
#define	SW_SEQUENTIAL	0x02
#define sw_freed	sw_flags	/* XXX compat */

#ifdef KERNEL
extern struct swdevt swdevt[];

int null_open(dev_t, int, int, struct proc *);
int null_close(dev_t, int, int, struct proc *);
int null_strategy(struct buf *);
int null_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int null_dump(dev_t);
int null_psize(dev_t);
int null_read(dev_t, struct uio *, int);
int null_write(dev_t, struct uio *, int);
int null_select(dev_t, int, struct proc *);
int null_stop(struct tty *, int);
struct tty *null_tty(dev_t);
int null_reset(int);
int null_mmap(mach_port_t, vm_prot_t, vm_offset_t, vm_size_t, mach_port_t *,
	      int);
mach_port_t null_port(dev_t);
int null_lioctl(struct tty *, ioctl_cmd_t, caddr_t, int);

int nodev_open(dev_t, int, int, struct proc *);
int nodev_close(dev_t, int, int, struct proc *);
int nodev_strategy(struct buf *);
int nodev_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int nodev_read(dev_t, struct uio *, int);
int nodev_write(dev_t, struct uio *, int);
int nodev_select(dev_t, int, struct proc *);
int nodev_stop(struct tty *, int);
int nodev_reset(int);
int nodev_mmap(mach_port_t, vm_prot_t, vm_offset_t, vm_size_t, mach_port_t *,
	       int);
int nodev_lopen(dev_t, struct tty *, int);
int nodev_lclose(struct tty *, int);
int nodev_lread(struct tty *, struct uio *, int);
int nodev_lwrite(struct tty *, struct uio *, int);
int nodev_rint(int, struct tty *);
int nodev_start(struct tty *);
int nodev_modem(struct tty *, int);
#endif

#endif /* !_SYS_CONF_H_ */
