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
 * $Log: cons.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:51  law
 * lites-950323 from jvh.
 *
 * Revision 2.2  93/03/12  10:54:59  rwd
 * 	cdevsw and linesw interfaces  have changed slightly!
 * 	[93/03/10            rwd]
 * 
 * Revision 2.1  92/04/21  17:10:52  rwd
 * BSDSS
 * 
 *
 */

/*
 * Console output.  Handles aliasing other ttys to console.
 */

#include "second_server.h"

#include <serv/server_defs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/synch.h>

#include <serv/device_utils.h>

struct tty	cons_tty;
struct tty	*cons_tp = &cons_tty;
struct mutex	cons_lock = MUTEX_NAMED_INITIALIZER("cons_lock");

dev_t			cons_dev_number = NODEV;
mach_port_t		console_port;
extern mach_port_t	device_server_port;

extern struct tty *tty_find_tty();

#if	SECOND_SERVER
extern	int	second_server;
#endif	/* SECOND_SERVER */

void cons_initialization()
{
    bzero(cons_tp, sizeof (struct tty));
}

mach_error_t cons_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	register int error;
	register int major_num;

#if	SECOND_SERVER
	if (second_server) {
		return second_cons_open(dev, flag, devtype, p);
	}
#endif	/* SECOND_SERVER */

	if (cons_dev_number == NODEV) {
	    /*
	     * Look for console.
	     */
	    for (major_num = 0; major_num < nchrdev; major_num++) {
		if (!strcmp(cdevsw[major_num].d_name, "console")) {
		    cons_dev_number = makedev(major_num, 0);
		    break;
		}
	    }
	    if (cons_dev_number == NODEV) {
		    dprintf("LITES: No console configured");
		    panic("no console configured");
	    }
	}

	error = tty_open(cons_dev_number, flag, devtype, p);

	cons_tp = tty_find_tty(cons_dev_number);

#if	0
	/* TTYLOC */
	if (cons_tp->t_ttyloc.tlc_hostid == 0) {
	    cons_tp->t_ttyloc.tlc_hostid = TLC_MYHOST;
	    cons_tp->t_ttyloc.tlc_ttyid  = TLC_CONSOLE;
	}
#endif	0

	return (error);
}

/* to satisfy console redirection */
mach_error_t cons_write(
	dev_t	dev,
	struct uio *uio,
	int flag)
{
	return (tty_write(dev, uio, flag));
}

/* to satisfy console redirection */
mach_error_t cons_ioctl(
	dev_t		dev,
	ioctl_cmd_t	cmd,
	caddr_t		data,
	int		flag)
{
	return (tty_ioctl(dev, cmd, data, flag));
}

mach_port_t
cons_port(dev_t dev)
{
	return cons_tp->t_device_port;
}

#define	CONS_BUF_SIZE	1024
char	cons_buf[CONS_BUF_SIZE];
int	cons_buf_pos = 0;

void
cnputc(int c)
{
	unsigned int cw;
#if	SECOND_SERVER
	if (second_server) {
		second_cnputc(c);
		return;
	}
#endif	/* SECOND_SERVER */

	if (c == '\n')
	    cnputc('\r');

	mutex_lock(&cons_lock);
	cons_buf[cons_buf_pos++] = c;
	if (cons_buf_pos >= CONS_BUF_SIZE || c == '\n') {
		(void) device_write_inband(cons_tp->t_device_port,
					   0, 0,
					   &cons_buf[0], cons_buf_pos,
					   &cw);
		cons_buf_pos = 0;
	}
	mutex_unlock(&cons_lock);
}

void console_init()
{
	kern_return_t	rc;

#if	SECOND_SERVER
	if (second_server) {
		return;
	}
#endif	/* SECOND_SERVER */

	/*
	 * Open the console
	 */
	rc = device_open(device_server_port,
#if OSF_LEDGERS
			 MACH_PORT_NULL,
#endif
			 D_READ|D_WRITE,
#if OSFMACH3
			 security_id,
#endif
			 "console",
			 &console_port);
	if (rc != KERN_SUCCESS) {
		dprintf("console_init failed");
		panic("console_init");
	}
}

