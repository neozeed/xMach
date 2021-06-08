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
 * $Log: select.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1996/03/14  21:08:35  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 * 12-Oct-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Use ffs() instead of find_first_set() since the former can be
 *	inlined.
 *
 *
 */
/* 
 *	File:	serv/select.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	June 1994
 *
 *	Select implementation for Lites.
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)sys_generic.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/assert.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>

int	selwait = 0;
int	nselcoll = 0;
struct mutex select_lock = MUTEX_NAMED_INITIALIZER("select_lock");

extern const struct timeval infinite_time; /* XXX */

/*
 * Select system call.
 */
void select()
{
	panic("select called through emul_generic");
}

mach_error_t s_select(
	struct proc	*p,
	int		nd,
	fd_set		*in,
	fd_set		*ou,
	fd_set		*ex,
	struct timeval	*tv,
	integer_t	*retval)
{
	fd_set obits[3];
	struct timeval atv;
	int s, ncoll, error = 0;
	u_int ni;
	struct timeval time, *timeo;
	proc_invocation_t pk = get_proc_invocation();

	assert(pk->k_p == p);

	bzero((caddr_t)obits, sizeof(obits));
	if (nd > FD_SETSIZE)
		return (EINVAL);
	if (nd > p->p_fd->fd_nfiles)
		nd = p->p_fd->fd_nfiles;	/* forgiving; slightly wrong */

	if (tv) {
		bcopy((void *) tv, (void *) &atv, sizeof(atv));

		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		if (timercmp(&atv, &infinite_time, >))
		    atv = infinite_time; /* XXX */
		get_time(&time);
		timevaladd(&atv, &time);
		timeo = &atv;
		/*
		 * Avoid inadvertently sleeping forever.
		 */
	} else {
		timeo = 0;
	}
retry:
	ncoll = nselcoll;	/* atomic. select lock not needed */
	/* PK_SELECT is protected by spls so p->p_lock is not really needed */
	mutex_lock(&p->p_lock);
	pk->k_flag |= PK_SELECT;
	mutex_unlock(&p->p_lock);
	error = selscan_main(p, nd, in, ou, ex, obits, retval);
	if (error || *retval)
		goto done;
	s = splhigh();		  /* for tsleep/wakeup consistency */
	mutex_lock(&select_lock); /* protects nselcoll */
	mutex_lock(&p->p_lock);	  /* protects pk */
	/*   woken up during selscan       collision wakeup during selscan */
	if ((pk->k_flag & PK_SELECT) == 0 || nselcoll != ncoll) {
		mutex_unlock(&p->p_lock);
		mutex_unlock(&select_lock);
		splx(s);
		goto retry;
	}
	pk->k_flag &= ~PK_SELECT;
	mutex_unlock(&p->p_lock);
	mutex_unlock(&select_lock);
	error = tsleep_abs((caddr_t)&selwait, PSOCK | PCATCH, "select", timeo);
	splx(s);
	if (error == 0)
		goto retry;
done:
	mutex_lock(&p->p_lock);
	pk->k_flag &= ~PK_SELECT;
	mutex_unlock(&p->p_lock);
	/* select is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
#define	putbits(name, x) \
	if (name) \
	    bcopy((void *) &obits[x], (void *) name, sizeof(fd_set));

	if (error == 0) {
		putbits(in, 0);
		putbits(ou, 1);
		putbits(ex, 2);
#undef putbits
	}
	return (error);
}

mach_error_t selscan_sub(
	struct proc	*p,
	int		nd,
	struct filedesc *fdp,
	fd_set		*iset,
	int		flag,
	fd_set		*oset,	/* OUT */
	int		*count)	/* OUT */
{
	int i, j, fd;
	fd_mask bits;
	struct file *fp;

	for (i = 0; i < nd; i += NFDBITS) {
		bits = iset->fds_bits[i/NFDBITS];
		while ((j = ffs(bits))
		       && (fd = i + --j) < nd)
		{
			bits &= ~(1 << j);
			fp = fdp->fd_ofiles[fd];
			if (fp == NULL)
			    return EBADF;
			assert(fp->f_type == DTYPE_VNODE || fp->f_type == DTYPE_SOCKET);
			if ((*fp->f_ops->fo_select)(fp, flag, p)) {
				FD_SET(fd, oset);
				++*count;
			}
		}
	}
	return KERN_SUCCESS;
}

mach_error_t selscan_main(
	struct proc	*p,
	int		nd,
	fd_set		*in,
	fd_set		*ou,
	fd_set		*ex,
	fd_set		*obits,
	integer_t	*retval)
{
	struct filedesc *fdp = p->p_fd;
	int n = 0;
	int error = 0;

	if (in)
	    error = selscan_sub(p, nd, fdp, in, FREAD, &obits[0], &n);
	if (error == KERN_SUCCESS && ou)
	    error = selscan_sub(p, nd, fdp, ou, FWRITE, &obits[1], &n);
	if (error == KERN_SUCCESS && ex)
	    error = selscan_sub(p, nd, fdp, ex, 0, &obits[2], &n);

	if (error == KERN_SUCCESS)
	    *retval = (integer_t)n;
	return error;
}

/*ARGSUSED*/
boolean_t seltrue(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{

	return (1);
}

/*
 * Record a select request.
 */
void
selrecord(selector, sip)
	struct proc *selector;
	struct selinfo *sip;
{
	struct proc *p;
	proc_invocation_t pk = get_proc_invocation();
	int s;

start:
	assert(pk->k_p == selector);

	s = splhigh();	/* for pk->k_wchan */
	mutex_lock(&select_lock);
	if (sip->si_pk == pk) {
		mutex_unlock(&select_lock);
		splx(s);
		return;
	}
	if (sip->si_pk && (p = sip->si_pk->k_p)) {
#if 0
		/* 
		 * The problem was in bsd_emulator_error that didn't
		 * release the process's lock before calling printf
		 * causing selwakeup to be called on the logger.
		 * There should be no real deadlock here.
		 */
		if (!mutex_try_lock(&p->p_lock)) {
			/* 
			 * XXX Taking p->p_lock may deadlock
			 * XXX the retry code is a kludge to avoid it.
			 */
			mutex_unlock(&select_lock);
			splx(s);
			cthread_yield();
			goto start;
		}
#else
		mutex_lock(&p->p_lock);
#endif
		if (pk->k_wchan == (caddr_t)&selwait)
		    sip->si_flags |= SI_COLL;
		else
		    sip->si_pk = pk;
		mutex_unlock(&p->p_lock);
	} else {
		sip->si_pk = pk;
	}
	mutex_unlock(&select_lock);
	splx(s);
}

/* 
 * XXX horrendous kludge to fix race in socket select when
 * XXX ETHER_AS_SYSCALL is true.
 */
void select_kludge_wakeup()
{
	int s = splhigh();
	mutex_lock(&select_lock);
	nselcoll++;
	mutex_unlock(&select_lock);
	wakeup((caddr_t)&selwait);
	splx(s);
}

/*
 * Do a wakeup when a selectable event occurs.
 */
void
selwakeup(struct selinfo *sip)
{
	struct proc *p;
	proc_invocation_t pk;
	int s;

	s = splhigh(); /* tsleep/wakeup consistency. must be same level */
restart:
	mutex_lock(&select_lock); /* protects sip and nselcoll */
	if (sip->si_pk == 0) {
		mutex_unlock(&select_lock);
		splx(s);
		return;
	}
	if (sip->si_flags & SI_COLL) {
		nselcoll++;
		sip->si_flags &= ~SI_COLL;
		mutex_unlock(&select_lock);
		wakeup((caddr_t)&selwait);
		goto restart;
	}
	pk = sip->si_pk;
	p = pk->k_p;
	sip->si_pk = 0;
	if (pk && p && p->p_ref > 0) {
		/* 
		 * p is not locked unless we are exiting in which case
		 * p_ref is zero.
		 */
		mutex_lock(&p->p_lock);
		/* pk->k_wchan is protected by splhigh AND p->lock */
		if (pk->k_wchan == (caddr_t)&selwait) {
			thread_unsleep(pk);
		} else {
			if (pk->k_flag & PK_SELECT)
			    pk->k_flag &= ~PK_SELECT;
		}
		mutex_unlock(&p->p_lock);
	}
	mutex_unlock(&select_lock);
	splx(s);
}
