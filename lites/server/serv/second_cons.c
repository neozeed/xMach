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
 * $Log: second_cons.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:57  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	serv/second_cons.c
 *	Author:	Randall W. Dean
 *	Date:	1992
 */

#include "second_server.h"

#include <serv/import_mach.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/synch.h>
#include <sys/ioctl_compat.h>

#include <serv/device_utils.h>

/*
 * For tty, we store the tty structure pointer in the hash table.
 */
#define	tty_hash_enter(dev, tp)	\
		dev_number_hash_enter(XDEV_CHAR(dev), (char *)(tp))
#define	tty_hash_remove(dev)	\
		dev_number_hash_remove(XDEV_CHAR(dev))
#define	tty_hash_lookup(dev)	\
		((struct tty *)dev_number_hash_lookup(XDEV_CHAR(dev)))

extern struct tty	*cons_tp;

extern struct tty *tty_find_tty(dev_t);

extern dev_t cons_dev_number;

int second_tty_open(dev_t, int, int, struct proc *);
void second_tty_start(struct tty *);
void second_cons_intr(void);

#define TTY_LOCK(x)
#define TTY_UNLOCK(x)

#if	SECOND_SERVER

extern	int	second_server;

second_cons_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	register int error;
	register int major_num;
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
		panic("no console configured");
	    }
	    dev_number_hash_enter(XDEV_CHAR(cons_dev_number), (char *)(cons_tp));
	}
	error = second_tty_open(cons_dev_number, flag, devtype, p);

	return (error);
}

mach_port_t
second_cons_port(dev_t dev)
{
	panic("second_cons_port");
}

second_tty_stop()
{
}

int second_tty_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	register struct tty *tp;
	static int first_console_open = 1;
	int error;

	/*
	 * Check whether tty is already open.
	 */
	
	tp = cdevsw[major(dev)].d_tty(dev);
	TTY_LOCK(tp);

	tp->t_oproc = second_tty_start;

	if ((tp->t_state & TS_ISOPEN) == 0) {
	    struct	sgttyb ttyb;

	    /*
	     * Set initial characters
	     */
	    ttychars(tp);

	    /*
	     * Get configuration parameters from device, put in tty structure
	     */

	    second_ioctl(0, TIOCGETP, &ttyb);

	    /* From OSF/1: */
	    tp->t_iflag = TTYDEF_IFLAG;
	    tp->t_oflag = TTYDEF_OFLAG;
	    tp->t_lflag = TTYDEF_LFLAG;
	    tp->t_cflag = CS8|CREAD;
	    tp->t_line = 0;
	    tp->t_state = TS_ISOPEN|TS_CARR_ON;

	    tp->t_ispeed = ttyb.sg_ispeed;
	    tp->t_ospeed = ttyb.sg_ospeed;
	    ttsetwater(tp);
	    if (ttyb.sg_flags & EVENP)
		tp->t_flags |= EVENP;
	    if (ttyb.sg_flags & ODDP)
		tp->t_flags |= ODDP;
	    if (ttyb.sg_flags & ECHO)
		tp->t_flags |= ECHO;
	    if (ttyb.sg_flags & CRMOD)
		tp->t_flags |= CRMOD;
	    if (ttyb.sg_flags & XTABS)
		tp->t_flags |= XTABS;

	    ttyb.sg_flags |= RAW;
	    ttyb.sg_flags &= ~ECHO;
	    second_ioctl(0, TIOCSETP, &ttyb);
	    /*
	     * Pretend that carrier is always on, until I figure out
	     * how to do it right.
	     */
	    tp->t_state |= TS_CARR_ON; /* should get from TTY_STATUS */
	}
	else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0) {
		error = EBUSY;
		goto out;
	}

	if ((flag & FREAD) && first_console_open) {
		/* create thread to read on standard input */	
		ux_create_thread(second_cons_intr);

		first_console_open = 0;
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
		if (error = ttysleep(tp, (void *)&tp->t_rawq, TTIPRI | PCATCH,
				     "secondttyopen", 0))
			goto out;
		/*
		 * some devices sleep on t_state...	XXX
		 */
	    }
	}

	error = (*linesw[tp->t_line].l_open)(dev, tp);
 out:
	TTY_UNLOCK(tp);
	return (error);
}

void second_cons_intr()
{
	char c;
	struct tty *tp = tty_find_tty(cons_dev_number);
	struct proc *p;
	proc_invocation_t pk = get_proc_invocation();

	system_proc(&p, "ConsoleTTY");

	cthread_wire();

	pk->k_ipl = -1;

  	while (1) if (second_read(0, &c, 1) == 1) {
		TTY_LOCK(tp);
		interrupt_enter(SPLTTY);
		(*linesw[tp->t_line].l_rint)(c, tp);
		interrupt_exit(SPLTTY);
		TTY_UNLOCK(tp);
        } else
	  	printf("second_read failed\n");
}

void second_tty_start(struct tty *tp)
{
	int	cc, s;

	s = spltty();
	if ((tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) == 0) {
	    if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
		    tp->t_state &= ~TS_ASLEEP;
		    wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
#if 0
		if (!queue_empty(&tp->t_wsel)) {
			selwakeup(&tp->t_wsel);
/*			tp->t_state &= ~TS_WCOLL; */
		}
#endif
	    }

	    /* get characters from tp->t_outq,
	       send to device */
	    while (tp->t_outq.c_cc != 0) {
		cc = ndqb(&tp->t_outq, 0);	
		second_write(1, tp->t_outq.c_cf, cc);
		ndflush(&tp->t_outq, cc);
	    }
	}
	splx(s);
}

second_cnputc(int c)
{
	char cc = c;
    	second_write(2, &cc, 1);
	if (c == '\n')
	    cnputc('\r');
}

#endif	/* SECOND_SERVER */
