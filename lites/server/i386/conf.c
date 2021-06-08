/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994 Johannes Helander
 * Copyright (c) 1994 Timo Rinne
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND JOHANNES HELANDER AND TIMO RINNE ALLOW FREE USE
 * OF THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * JOHANNES HELANDER AND TIMO RINNE DISCLAIM ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: conf.c,v $
 * Revision 1.2  2000/10/27 01:58:45  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:28  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	 i386/server/conf.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *	Timo Rinne, Helsinki University of Technology, 1994.
 *
 *	Simplified configuration.
 */
/*-
 * Copyright (c) 1991, 1993
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
 *	@(#)conf.c	8.3 (Berkeley) 1/21/94
 */

#include <serv/import_mach.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>	/* for B_TAPE */
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/vnode.h>

#include <serv/device.h>
#include <serv/device_utils.h>

#include <sys/ioctl.h>	/* for tty */
#include <sys/tty.h>
#include <sys/proc.h>

#include <serv/tape_io.h>	/* tape_io prototypes */
/*
 * Block devices all use the same open/close/strategy routines.
 */
int bdev_open(dev_t, int, int, struct proc *);
int bdev_close(dev_t, int, int, struct proc *);
int bio_strategy(struct buf *);
int bdev_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int bdev_dump(dev_t);
int bdev_size(dev_t);
#define	bdev_ops	bdev_open, bdev_close, bio_strategy, bdev_ioctl,\
			bdev_dump, bdev_size

struct bdevsw	bdevsw[] =
{
/*0*/	{ "hd",		C_BLOCK(8),	bdev_ops },	/* isa */
/*1*/   { "",           0,              bdev_ops },
/*2*/	{ "fd",		C_BLOCK(8),	bdev_ops },	/* isa */
/*3*/	{ "wt",		B_TAPE,		bdev_ops },	/* isa */
/*4*/	{ "sd",		C_BLOCK(8),	bdev_ops },	/* ipsc, isa */
/*5*/	{ "st",		C_BLOCK(16),	bdev_ops },	/* ipsc, isa */
/*6*/	{ "cd",		C_BLOCK(8),	bdev_ops },	/* ipsc, isa */

};
int	nblkdev = sizeof(bdevsw)/sizeof(bdevsw[0]);

int char_open(dev_t, int, int, struct proc *);
int char_close(dev_t, int, int, struct proc *);
int char_read(dev_t, struct uio *, int);
int char_write(dev_t, struct uio *, int);
int char_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int char_select(dev_t, int, struct proc *);
mach_port_t char_port(dev_t);
#define	char_ops \
	char_open, char_close, char_read, char_write, char_ioctl, \
	null_stop, null_reset, null_tty, char_select, device_map, \
	null_strategy, char_port

int disk_open(dev_t, int, int, struct proc *);
int disk_close(dev_t, int, int, struct proc *);
int disk_read(dev_t, struct uio *, int);
int disk_write(dev_t, struct uio *, int);
int disk_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int isa_disk_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
extern mach_port_t disk_port(dev_t);
#define	disk_ops \
	disk_open, disk_close, disk_read, disk_write, disk_ioctl, \
	null_stop, null_reset, null_tty, seltrue, null_mmap, \
	null_strategy, disk_port

#define	isa_disk_ops \
	disk_open, disk_close, disk_read, disk_write, isa_disk_ioctl, \
	null_stop, null_reset, null_tty, seltrue, null_mmap, \
	null_strategy, disk_port

#define	tape_ops \
	tape_open, tape_close, tape_read, tape_write, tape_ioctl, \
	null_stop, null_reset, null_tty, seltrue, null_mmap, \
	null_strategy, tape_port

int tty_open(dev_t, int, int, struct proc *);
int tty_close(dev_t, int, int, struct proc *);
int tty_read(dev_t, struct uio *, int);
int tty_write(dev_t, struct uio *, int);
int tty_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int ttselect(dev_t, int, struct proc *);
int tty_stop(struct tty *, int);
struct tty *tty_find_tty(dev_t);
#define	tty_ops	\
	tty_open, tty_close, tty_read, tty_write, tty_ioctl,\
	tty_stop, null_reset, tty_find_tty, ttselect, null_mmap, \
	null_strategy, null_port

int cons_open(dev_t, int, int, struct proc *);
int cons_write(dev_t, struct uio *, int);
int cons_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
extern mach_port_t cons_port(dev_t);
#define	console_ops	\
	cons_open, tty_close, tty_read, cons_write, cons_ioctl, \
	tty_stop, null_reset, tty_find_tty, ttselect, device_map, \
	null_strategy, cons_port

int cttyopen(dev_t, int, int, struct proc *);
int cttyread(dev_t, struct uio *, int);
int cttywrite(dev_t, struct uio *, int);
int cttyioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int cttyselect(dev_t, int, struct proc *);
#define	ctty_ops \
 	cttyopen, null_close, cttyread, cttywrite, cttyioctl, \
 	null_stop, null_reset, null_tty, cttyselect, null_mmap, \
	null_strategy, null_port

int logopen(dev_t, int, int, struct proc *);
int logclose(dev_t, int, int, struct proc *);
int logread(dev_t, struct uio *, int);
int logioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int logselect(dev_t, int, struct proc *);
#define	log_ops \
	logopen, logclose, logread, null_write, logioctl, \
	null_stop, null_reset, null_tty, logselect, null_mmap, \
	null_strategy, null_port

int mmopen(dev_t, int, int, struct proc *);
int mmread(dev_t, struct uio *, int);
int mmwrite(dev_t, struct uio *, int);
#define	mm_ops \
	mmopen, null_close, mmread, mmwrite, nodev_ioctl, \
	null_stop, null_reset, null_tty, seltrue, null_mmap, \
	null_strategy, null_port

int ptsopen(dev_t, int, int, struct proc *);
int ptsclose(dev_t, int, int, struct proc *);
int ptsread(dev_t, struct uio *, int);
int ptswrite(dev_t, struct uio *, int);
int ptyioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
int ptsselect(dev_t, int, struct proc *);
int ptsstop(struct tty *, int);
struct tty *pty_find_tty(dev_t);
#define	pts_ops \
	ptsopen, ptsclose, ptsread, ptswrite, ptyioctl, \
	ptsstop, null_reset, pty_find_tty, ttselect, null_mmap, \
	null_strategy, null_port

int ptcopen(dev_t, int, int, struct proc *);
int ptcclose(dev_t, int, int, struct proc *);
int ptcread(dev_t, struct uio *, int);
int ptcwrite(dev_t, struct uio *, int);
int ptcselect(dev_t, int, struct proc *);
#define	ptc_ops \
	ptcopen, ptcclose, ptcread, ptcwrite, ptyioctl, \
	null_stop, null_reset, null_tty, ptcselect, null_mmap, \
	null_strategy, null_port

int iopl_open(dev_t, int, int, struct proc *);
mach_port_t iopl_port(dev_t);
#define	iopl_ops \
	iopl_open, char_close, null_read, null_write, null_ioctl, \
	null_stop, null_reset, null_tty, seltrue, device_map, \
	null_strategy, char_port

/* Keyboard has it's own IOCTL */
int kbd_ioctl(dev_t, ioctl_cmd_t, caddr_t, int, struct proc *);
#define	kbd_ops \
	char_open, char_close, char_read, char_write, kbd_ioctl, \
	null_stop, null_reset, null_tty, char_select, null_mmap, \
	null_strategy, null_port

/* Like char_ops but less methods */
#define	maptime_ops \
	char_open, char_close, null_read, null_write, null_ioctl, \
	null_stop, null_reset, null_tty, null_select, device_map, \
	null_strategy, char_port

/* Mapped timezone and time offset */
int maptz_open(dev_t, int, int, struct proc *);
int maptz_close(dev_t, int, int, struct proc *);
mach_port_t maptz_port(dev_t);
kern_return_t maptz_map(mach_port_t, vm_prot_t, vm_offset_t, vm_size_t,
			mach_port_t *, int);
#define	maptz_ops \
	maptz_open, maptz_close, null_read, null_write, null_ioctl, \
	null_stop, null_reset, null_tty, null_select, maptz_map, \
	null_strategy, maptz_port

#define	no_ops \
	nodev_open, nodev_close, nodev_read, nodev_write, nodev_ioctl, \
	null_stop, nodev_reset, null_tty, nodev_select, nodev_mmap, \
	nodev_strategy, null_port

struct cdevsw	cdevsw[] =
{
/*0*/	{ "console",	0,		console_ops,	},
/*1*/	{ "",		0,		ctty_ops,	},	/* tty */
/*2*/	{ "",		0,		mm_ops,		},	/* kmem,null */
/*3*/	{ "hd",		C_BLOCK(8),	isa_disk_ops,	},	/* isa */
/*4*/	{ "",		0,		no_ops,		},      /* drum */
/*5*/	{ "",		0,		pts_ops,	},	/* pts */
/*6*/	{ "",		0,		ptc_ops,	},	/* ptc */
/*7*/	{ "",		0,		log_ops,	},      /* log */
/*8*/	{ "com",	0,		tty_ops,	},	/* isa */
/*9*/	{ "fd",		C_BLOCK(8),	isa_disk_ops,	},	/* isa */
/*10*/	{ "wt",		0,		char_ops,	},	/* isa */
/*11*/	{ "",		0,		no_ops,		},      /* xd */
/*12*/	{ "",		0,		no_ops,		},      /* pc */
/*13*/	{ "sd",		C_BLOCK(8),	isa_disk_ops,	},	/* isa */
/*14*/	{ "st",		C_BLOCK(16),	tape_ops,	},	/* isa */
/*15*/	{ "cd",		C_BLOCK(8),	isa_disk_ops,	},	/* isa */
/*16*/	{ "",		0,		no_ops,		}, 	/* - */
/*17*/	{ "",		0,		no_ops,		},      /* - */
/*18*/	{ "",		0,		no_ops,		},      /* - */
/*19*/	{ "",		0,		no_ops,		},      /* - */
/*20*/	{ "",		0,		no_ops,		},      /* - */
/*21*/	{ "",		0,		no_ops,		},      /* - */
/*22*/	{ "iopl",	0,		iopl_ops,	},      /* iopl */
/*23*/	{ "kbd",	0,		kbd_ops,	},      /* keyboard */
/*24*/	{ "mouse",	0,		char_ops,	},      /* mouse */
/*25*/	{ "time",	0,		maptime_ops,	},/* mapped time */
/*26*/	{ "",		0,		maptz_ops,	},/* mapped timezone */
/*27*/	{ "",		0,		no_ops,		},      /* - */
/*28*/	{ "",		0,		no_ops,		},      /* - */
/*29*/	{ "",		0,		no_ops,		},      /* - */
};
#define NCHRDEV sizeof(cdevsw)/sizeof(cdevsw[0])
int	nchrdev = NCHRDEV;

dev_t	cttydev = makedev(1, 0);
int	mem_no = 2;

/*
 * Conjure up a name string for funny devices (not all minors have
 * the same name).
 */
int
check_dev(dev_t	dev, char *str)
{
    return 0;
}

/*
 * ISA disk IOCTL
 */
#undef	p_flag			/* conflict from sys/{proc,user}.h */

#include "mach4_includes.h"
#include "osfmach3.h"

#if MACH4_INCLUDES || OSFMACH3
#include <i386/disk.h>
#else
#include <i386at/disk.h>
#endif

int isa_disk_ioctl(
	dev_t dev,
	ioctl_cmd_t cmd,
	caddr_t data,
	int flag,
	struct proc *p)
{
	mach_port_t		device_port = disk_port(dev);
	mach_msg_type_number_t	count;
	register int		error;

	switch (cmd) {
#if !OSFMACH3
	    case V_RDABS:
	    {
		char buf[512];

		error = device_set_status(device_port,
					  V_ABS,
			   (dev_status_t) &((struct absio *)data)->abs_sec,
					  1);
		if (error)
		    return (dev_error_to_errno(error));
		count = 512/sizeof(int);
		error = device_get_status(device_port,
					  cmd,
					  (dev_status_t) buf,
					  &count);

		if (error)
		    return (dev_error_to_errno(error));
		if (copyout(buf, ((struct absio *)data)->abs_buf, 512))
		    return (EFAULT);
		break;
	    }

	    case V_VERIFY:
	    {
		union vfy_io *vfy_io = (union vfy_io *) data;
		int vfy[2] = {	vfy_io->vfy_in.abs_sec,
				vfy_io->vfy_in.num_sec};

		error = device_set_status(device_port,
					  V_ABS,
					  vfy,
					  2);
		if (error)
		    return (dev_error_to_errno(error));
		count = sizeof (int)/sizeof(int);
		error = device_get_status(device_port,
					  cmd,
					  vfy,
					  &count);

		vfy_io->vfy_out.err_code = vfy[0];
		if (error)
		    return (dev_error_to_errno(error));
		break;
	    }

	    case V_WRABS:
	    {
		char buf[512];

		error = device_set_status(device_port,
			V_ABS,
			(dev_status_t) &((struct absio *)data)->abs_sec,
			1);
		if (error)
		    return (dev_error_to_errno(error));
		if (copyin(((struct absio *)data)->abs_buf, buf, 512))
		    return (EFAULT);
		count = 512/sizeof(int);
		error = device_set_status(device_port,
					  cmd,
					  (dev_status_t) buf,
					  count);
		if (error)
		    return (dev_error_to_errno(error));
		break;
	    }
#endif /* !OSFMACH3 */
	    default:
	    {
		return (disk_ioctl(dev, cmd, data, flag, p));
	    }
	}
	return (0);
}

/*
 * IOPL device has to give send rights to port to task.
 */

int iopl_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	kern_return_t	kr;
	mach_port_t	name, iopl_port;

	kr = char_open(dev, flag, devtype, p);
	if (kr)
	    return kr;

	iopl_port = char_port(dev);
	/*
	 * Give send rights to task to let it access IO
	 */
	name = 0x10000;	/* XXX */
	do {
	    kr = mach_port_insert_right(p->p_task,
					name++,
					iopl_port,
					MACH_MSG_TYPE_COPY_SEND);
	} while ((kr == KERN_NAME_EXISTS || kr == KERN_RIGHT_EXISTS)
		 && name < 0x10100); /* give up after 256 tries */
	if (kr != KERN_SUCCESS) {
		char_close(dev, flag, 0 /*mode*/, p);
		return EACCES;
	}
	return 0;
}

/*
 * Keyboard device uses char ops but has special ioctl of its own.
 */
int kbd_ioctl(
	dev_t dev,
	ioctl_cmd_t cmd,
	caddr_t data,
	int flag,
	struct proc *p)
{
	/* 
	 * XXX a newer version of this stuff is now in char_ioctl
	 * XXX which is not the proper place
	 */
	return (char_ioctl(dev, cmd, data, flag, p));
}

/* XXX from Lite with fixes. Cleanup */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(3, 0);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
boolean_t iskmemdev(dev_t dev)
{

	return FALSE;
}

boolean_t iszerodev(dev_t dev)
{
	return FALSE;
	return (major(dev) == 2 && minor(dev) == 12);
}

/*
 * Routine to determine if a device is a disk.
 *
 * A minimal stub routine can always return 0.
 */
boolean_t isdisk(dev_t dev, int type)
{

	switch (major(dev)) {
	case 0:
	case 2:
	case 4:
	case 6:
		if (type == VBLK)
			return TRUE;
		return FALSE;
	case 3:
	case 9:
	case 13:
	case 15:
		if (type == VCHR)
			return TRUE;
		/* fall through */
	default:
		return FALSE;
	}
	/* NOTREACHED */
}

static int chrtoblktbl[NCHRDEV] =  {
      /* VCHR */      /* VBLK */
	/* 0 */		NODEV,
	/* 1 */		NODEV,
	/* 2 */		NODEV,
	/* 3 */		0,	/* hd (esdi) */
	/* 4 */		NODEV,
	/* 5 */		NODEV,
	/* 6 */		NODEV,
	/* 7 */		NODEV,
	/* 8 */		NODEV,
	/* 9 */		2,	/* fd */
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	4,	/* sd */
	/* 14 */	NODEV,
	/* 15 */	6,	/* cd */
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	NODEV,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
			NODEV,
			NODEV,
	/* 23 */	NODEV,
			NODEV,
	/* 25 */	NODEV,
			NODEV,
	/* 27 */	NODEV,
			NODEV,
	/* 29 */	NODEV

};
/*
 * Routine to convert from character to block device number.
 *
 * A minimal stub routine can always return NODEV.
 */
dev_t chrtoblk(dev_t dev)
{
	int blkmaj;

	if (major(dev) >= NCHRDEV || (blkmaj = chrtoblktbl[major(dev)]) == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}

dev_t blktochr(dev_t bdev)
{
  int i;

  for (i = 0; i < NCHRDEV; i++) {
    if (major(bdev) == chrtoblktbl[i])
      return makedev(i, minor(bdev));
  }
  return NODEV;
}
