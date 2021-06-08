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
 * 12-Dec-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Remove code for old ioctl cmds from tty_ioctl(). There is hair in
 *	ttioctl to convert from old cmds to new cmds.
 *
 * $Log: tty_io.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/08/15  06:49:39  sclawson
 * modifications from lites-1.1-950808
 *
 * Revision 1.1.1.2  1995/03/23  01:17:00  law
 * lites-950323 from jvh.
 *
 * Revision 2.5  93/03/12  10:55:18  rwd
 * 	cdevsw and linesw interfaces  have changed slightly!
 * 	[93/03/10            rwd]
 * 	Ansification.
 * 	[93/03/02  13:41:37  rwd]
 * 
 * Revision 2.4  93/02/26  12:56:32  rwd
 * 	Include sys/systm.h for printf prototypes.
 * 	[92/12/09            rwd]
 * 
 * Revision 2.3  92/07/09  16:27:28  mrt
 * 	Deal with mach_kernel/bnr2ss baud rate conversion.  From
 * 	A.Richter.
 * 	[92/07/06            rwd]
 * 
 * Revision 2.2  92/05/25  14:46:41  rwd
 * 	Set termios defaults here.
 * 	[92/05/04            rwd]
 * 
 * Revision 2.1  92/04/21  17:11:13  rwd
 * BSDSS
 * 
 *
 */

/*
 * Interface between MACH tty devices and BSD ttys.
 */
#include "second_server.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/synch.h>
#include <sys/ioctl_compat.h>
#include <sys/systm.h>
#include <sys/assert.h>

#include <serv/server_defs.h>
#include <serv/device.h>
#include <serv/device_utils.h>

#if	SECOND_SERVER
extern	int	second_server;
#endif	/* SECOND_SERVER */

/*
 * For tty, we store the tty structure pointer in the hash table.
 */
#define	tty_hash_enter(dev, tp)	\
		dev_number_hash_enter(XDEV_CHAR(dev), (char *)(tp))
#define	tty_hash_remove(dev)	\
		dev_number_hash_remove(XDEV_CHAR(dev))
#define	tty_hash_lookup(dev)	\
		((struct tty *)dev_number_hash_lookup(XDEV_CHAR(dev)))

/*
 * We cannot deallocate the tty when it is closed, since other
 * structures have handles that are not reference-counted (p->p_ttyp).
 * Instead, we just close the device port.
 */

/*
 * Exported version of tty_hash_lookup.
 */
struct tty *
tty_find_tty(dev_t dev)
{
	return (tty_hash_lookup(dev));
}

void tty_read_reply(char *, int, char *, unsigned int);
void tty_write_reply(char *, int, int);
void tty_start(struct tty *);

static mach_error_t tty_param(struct tty *tp, struct termios *t);
extern struct tty *	cons_tp;	/* console TTY */

static long old_baudrates[] = {
   0, 50, 75, 110, 134, 150, 200, 300,
   600, 1200, 1800, 2400, 4800, 9600, 19200, 38400
};
#define NUM_BAUDRATES	(sizeof( old_baudrates ) / sizeof( long ))

static long
baudrate_to_speed( long baudrate )
{
   int i;
   for( i = 0; i < NUM_BAUDRATES; i++ ) {
      if (old_baudrates[i] == baudrate) return i;
   }
   return baudrate;
}

static long
speed_to_baudrate( long speed )
{
   return (speed < NUM_BAUDRATES) ? old_baudrates[speed] : speed;
}

/*
 * Open tty.
 */
mach_error_t tty_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	struct tty *tp;
	boolean_t	new_tty;
	kern_return_t	kr;

	/*
	 * Check whether tty is already open.
	 */
	tp = tty_hash_lookup(dev);
	if (tp == 0) {
	    /*
	     * Create new TTY structure.
	     */
	    tp = (struct tty *)malloc(sizeof(struct tty));
	    bzero(tp, sizeof(struct tty));
	    tty_hash_enter(dev, tp);
	    tp->t_flags = 0;
	    tp->t_device_port = MACH_PORT_NULL;
	    tp->t_reply_port = MACH_PORT_NULL;

	    new_tty = TRUE;	/* may deallocate if open fails */
	}
	else
	    new_tty = FALSE;	/* old structure - may be pointers to it */
	    			/* cannot deallocate if open fails */

	if (tp->t_device_port == MACH_PORT_NULL) {
	    /*
	     * Device is closed - try to open it.
	     */
	    kern_return_t	rc;
	    mach_port_t	device_port;
	    dev_mode_t	mode;
#if OSFMACH3
	    dev_name_t  name;
#else
	    char	name[32];
#endif

	    /* get string name from device number */
	    rc = cdev_name_string(dev, name);
	    if (rc != 0)
		return (rc);	/* bad name */

	    mode = D_READ|D_WRITE;
	    /* open device */
	    rc = device_open(device_server_port,
#if OSF_LEDGERS
			     MACH_PORT_NULL,
#endif
			     mode,
#if OSFMACH3
			     security_id,
#endif
			     name,
			     &device_port);
	    if (rc != D_SUCCESS) {
		/*
		 * Deallocate tty structure, if newly created.
		 */
		if (new_tty) {
		    tty_hash_remove(dev);
		    free((char *)tp);
		}
		return (dev_error_to_errno(rc));
	    }

	    /*
	     * Check for alias of console.
	     */
	    if (cons_tp != 0 /* we use this to create cons_tp */
		    && device_port == cons_tp->t_device_port
		    && new_tty) {
		/*
		 * Opened console - use its tty
		 */
		tty_hash_remove(dev);
		free((char *)tp);

		tp = cons_tp;
		tty_hash_enter(dev, tp);
	    }
	    else {
		/*
		 * Save device-port and tty-structure for device number.
		 */
		tp->t_device_port = device_port;
	    }

	}

	tp->t_oproc = tty_start;
	tp->t_param = tty_param;

	if ((tp->t_state & TS_ISOPEN) == 0) {

	    struct tty_status		ttstat;
	    mach_msg_type_number_t	ttstat_count;
	    kern_return_t	rc;

	    /*
	     * Set initial characters
	     */
	    ttychars(tp);

	    tp->t_ospeed = tp->t_ispeed = TTYDEF_SPEED;
	    tp->t_iflag = TTYDEF_IFLAG;
	    tp->t_oflag = TTYDEF_OFLAG;
	    tp->t_lflag = TTYDEF_LFLAG;
	    tp->t_cflag = TTYDEF_CFLAG;

	      rc = tty_param(tp, &tp->t_termios);
	      if (rc != D_SUCCESS)
		return(rc);

	    ttsetwater(tp);

	    /*
	     * Pretend that carrier is always on, until I figure out
	     * how to do it right.
	     */
	    tp->t_state |= TS_CARR_ON; /* should get from TTY_STATUS */

	}
	else if ( tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0 ) {
	   return (EBUSY);
	}

	if (tp->t_reply_port == MACH_PORT_NULL) {
	    /*
	     * Create reply port for device read/write messages.
	     * Hook it up to device and tty.
	     */
#if 0
	    tp->t_reply_port = mach_reply_port();
	    reply_hash_enter(tp->t_reply_port,
			     (char *)tp,
			     tty_read_reply,
			     tty_write_reply);
#else
	    kr = port_object_allocate_receive(&tp->t_reply_port,
					      POT_TTY,
					      tp);
	    add_to_reply_port_set(tp->t_reply_port);
	    assert(kr == KERN_SUCCESS);
#endif
	}

	if ((flag & FREAD) && !(tp->t_state & TS_RQUEUED)) {
	    /*
	     * Post initial read.
	     */
	    (void) device_read_request_inband(tp->t_device_port,
					      tp->t_reply_port,
					      0,
					      0,	/* recnum */
					      tp->t_hiwat);

	    tp->t_state |= TS_RQUEUED;
	}
	/*
	 * Wait for CARR_ON
	 */
	if (flag & O_NDELAY) {
	    tp->t_state |= TS_ONDELAY;
	}
	else {
	    while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
		/*
		 * some devices sleep on t_state...	XXX
		 */
	    }
	}

	return (*linesw[tp->t_line].l_open)(dev, tp);
}

mach_error_t tty_close(dev_t dev, int flag, int mode, struct proc *p)
{
	struct tty *tp;

	/* get tty structure and port from dev */
	tp = tty_hash_lookup(dev);
	if (!tp)
		return ENODEV;

	(*linesw[tp->t_line].l_close)(tp, flag);

	/*
	 * Do not close the console (special case HACK HACK)
	 */
	if (tp != cons_tp) {
		/*
		 * Remove the reply port
		 */
#if 0
	    reply_hash_remove(tp->t_reply_port);
	    (void) mach_port_mod_refs(mach_task_self(), tp->t_reply_port,
				      MACH_PORT_RIGHT_RECEIVE, -1);
#else
	    /* This could be more elegant and lazy */
	    port_object_shutdown(tp->t_reply_port, TRUE);
#endif
	    tp->t_reply_port = MACH_PORT_NULL;

	    /*
	     * And close the device
	     */
	    (void) device_close(tp->t_device_port);
	    (void) mach_port_deallocate(mach_task_self(), tp->t_device_port);
	    tp->t_device_port = MACH_PORT_NULL;

	    /*
	     * Disable output
	     */
	    tp->t_oproc = 0;
	}
	/*
	 * Leave tty structure, but mark it closed.
	 */
	ttyclose(tp);
	return KERN_SUCCESS;
}

mach_error_t tty_read(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;

	/* get tty from device */
	tp = tty_hash_lookup(dev);
	if (!tp)
		return ENODEV;
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

mach_error_t tty_write(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;

	/* get tty from device */
	tp = tty_hash_lookup(dev);
	if (!tp)
		return ENODEV;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

tty_select(dev_t dev, int rw)
{
	struct tty *tp;

	/* get tty from device */
	tp = tty_hash_lookup(dev);

#if 0
	return ((*linesw[tp->t_line].l_select)(dev, rw));
#else
	return ENODEV;
#endif
}

/* Set the device parameters */
static mach_error_t tty_param(struct tty *tp, struct termios *t)
{
	struct tty_status  ttstat;
	mach_msg_type_number_t	ttstat_count;
	mach_error_t error;

	/*
	 * Get configuration parameters from device, put in tty structure
	 */
	ttstat_count = TTY_STATUS_COUNT;
	(void) device_get_status(tp->t_device_port,
				 TTY_STATUS,
				 (int *)&ttstat,
				 &ttstat_count);
	ttstat.tt_ispeed = baudrate_to_speed(t->c_ispeed);
	ttstat.tt_ospeed = baudrate_to_speed(t->c_ospeed);
	ttstat.tt_flags &= ~(TF_EVENP | TF_ODDP | TF_ECHO | TF_CRMOD | TF_LITOUT | TF_XTABS);
	if ((t->c_cflag & CSIZE) == CS8 || (tp->t_flags & PASS8))
	    ttstat.tt_flags |= TF_LITOUT;
	if (t->c_cflag & PARENB) {
		if (t->c_iflag & INPCK) {
			if (t->c_cflag & PARODD)
			    ttstat.tt_flags |= TF_EVENP;
			else
			    ttstat.tt_flags |= TF_ODDP;
		} else
		    ttstat.tt_flags |= (TF_ODDP | TF_EVENP);
	}
	if (t->c_lflag & ECHO)
	    ttstat.tt_flags |= TF_ECHO;
	if (t->c_iflag & ICRNL)
	    ttstat.tt_flags |= TF_CRMOD;
	if (t->c_oflag & OXTABS)
	    ttstat.tt_flags |= TF_XTABS;
	error = device_set_status(tp->t_device_port,
				  TTY_STATUS,
				  (int *)&ttstat,
				  ttstat_count);
	return (error);
}

mach_error_t tty_ioctl(
	dev_t		dev,
	ioctl_cmd_t	cmd,
	caddr_t		data,
	int		flag)
{
	struct tty *tp;
	int	error;
	struct proc *p = get_proc();

	struct tty_status	ttstat;
	mach_msg_type_number_t	ttstat_count;
	int			word;

	/* get tty from device */
	tp = tty_hash_lookup(dev);
	if (!tp)
		return ENODEV;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
	    return (error);

	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
	    return (error);
	}
	/* if command is one meant for device,
	   translate it into a device_set_status() and issue it.
	 */
	switch (cmd) {
	    case TIOCMODG:
	    case TIOCMGET:
		ttstat_count = TTY_MODEM_COUNT;
		(void) device_get_status(tp->t_device_port,
					 TTY_MODEM,
					 &word,
					 &ttstat_count);
		*(int *)data = word;
		break;
	    case TIOCMODS:
	    case TIOCMSET:
		word = *(int *)data;
		(void) device_set_status(tp->t_device_port,
					 TTY_MODEM,
					 &word,
					 TTY_MODEM_COUNT);
		break;
	    case TIOCMBIS:
	    case TIOCMBIC:
	    case TIOCSDTR:
	    case TIOCCDTR:
		ttstat_count = TTY_MODEM_COUNT;
		(void) device_get_status(tp->t_device_port,
					 TTY_MODEM,
					 &word,
					 &ttstat_count);
		switch (cmd) {
		    case TIOCMBIS:
			word |= *(int *)data;
			break;
		    case TIOCMBIC:
			word &= ~*(int *)data;
			break;
		    case TIOCSDTR:
			word |= TM_DTR;
			break;
		    case TIOCCDTR:
			word &= ~TM_DTR;
			break;
		}
		(void) device_set_status(tp->t_device_port,
					 TTY_MODEM,
					 &word,
					 TTY_MODEM_COUNT);
		break;
	    case TIOCSBRK:
		(void) device_set_status(tp->t_device_port,
					 TTY_SET_BREAK,
					 &word,
					 0);
		break;
	    case TIOCCBRK:
		(void) device_set_status(tp->t_device_port,
					 TTY_CLEAR_BREAK,
					 &word,
					 0);
		break;
	    default:
	    {
		/*
		 * Not one of the TTY ioctls - try sending
		 * the code to the device, and see what happens.
		 */
		mach_msg_type_number_t count;

		count = (cmd & ~(IOC_INOUT|IOC_VOID)) >> 16; /* bytes */
		count = (count + 3) >> 2;		     /* ints */
		if (count == 0)
		    count = 1;

		if (cmd & (IOC_VOID|IOC_IN)) {
		    error = device_set_status(tp->t_device_port,
					      cmd,
					      (int *)data,
					      count);
		    if (error)
			return (ENOTTY);
		}
		if (cmd & IOC_OUT) {
		    error = device_get_status(tp->t_device_port,
					      cmd,
					      (int *)data,
					      &count);
		    if (error)
			return (ENOTTY);
		break;
		}
	    }
	}

	return (0);
}

void tty_stop(struct tty *tp, int rw)
{
	int	s;

#if	SECOND_SERVER
	if (second_server) {
		second_tty_stop(tp, rw);
		return;
	}
#endif	/* SECOND_SERVER */
	s = spltty();

	if (tp->t_state & TS_BUSY) {
	    (void) device_set_status(tp->t_device_port,
				     (rw) ? TTY_FLUSH : TTY_STOP,
				     (int *)&rw,
				     1);
	    if ((tp->t_state & TS_TTSTOP) == 0)
		tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

void tty_read_reply(char *tp_ptr, int error, char data[], 
		    unsigned int data_count)
{
	struct tty *tp = (struct tty *)tp_ptr;
	int	i;

	if (!error && data_count > 0) {
	    interrupt_enter(SPLTTY);
	    for (i = 0; i < data_count; i++)
		(*linesw[tp->t_line].l_rint)(data[i] & 0xff, tp);
	    interrupt_exit(SPLTTY);
	} else if (error) {
	    printf("tty_read_reply: error = %x\n",error);
	    panic("tty_read_reply");
	}

	(void) device_read_request_inband(tp->t_device_port,
				   tp->t_reply_port,
				   0,		/* mode */
				   0,		/* recnum */
				   tp->t_hiwat);
}

void tty_write_reply(char *tp_ptr, int error, int bytes_written)
{
	struct tty *tp = (struct tty *)tp_ptr;

	if (error) {
	    printf("tty_write_reply: error = %x\n",error);
	    panic("tty_write_reply");
	    bytes_written = 0;
	}

	interrupt_enter(SPLTTY);

	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_FLUSH)
	    tp->t_state &= ~TS_FLUSH;
	else
	    ndflush(&tp->t_outq, bytes_written);

	if (tp->t_line)
	    (*linesw[tp->t_line].l_start)(tp);
	else
	    tty_start(tp);

	interrupt_exit(SPLTTY);

}

void tty_start(struct tty *tp)
{
	int	cc, s;
	kern_return_t result;

	s = spltty();
	if ((tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) == 0) {

	    if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
		    tp->t_state &= ~TS_ASLEEP;
		    wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	    }

	    /* get characters from tp->t_outq,
	       send to device */
	    if (tp->t_outq.c_cc != 0) {

		cc = ndqb(&tp->t_outq, 0);	/* device handles timeouts! */
		/* This assumes inband data size is large enough. */
		result = device_write_request_inband(tp->t_device_port,
						     tp->t_reply_port,
						     0,	/* mode */
						     0,	/* recnum */
						     tp->t_outq.c_cf,
						     cc);
		if (result != KERN_SUCCESS) {
		    printf("tty_start: device_write_request result = %x\n",
			   result);
		    Debugger("tty_start");
		} else 
		    tp->t_state |= TS_BUSY;
	    }
	}
	splx(s);
}

/*ARGSUSED*/
struct tty *
nulltty(dev_t dev)
{
	return (struct tty *) 0;
}
