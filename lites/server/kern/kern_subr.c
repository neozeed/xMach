/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994 Johannes Helander
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND JOHANNES HELANDER ALLOW FREE USE OF THIS
 * SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND JOHANNES
 * HELANDER DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: kern_subr.c,v $
 * Revision 1.2  2000/10/27 01:58:45  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:45  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	 kern_subr.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *	Origin:	 Adapted to Lites from 4.4 BSD Lite.
 */
/*
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)kern_subr.c	8.3 (Berkeley) 1/21/94
 */

#include "diagnostic.h"

#include <serv/server_defs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/queue.h>

extern vm_offset_t extend_reply_msg(vm_offset_t, vm_size_t, vm_size_t);
extern void extend_current_output(vm_size_t);

mach_error_t uiomove(cp, n, uio)
	register caddr_t cp;
	register int n;
	register struct uio *uio;
{
	register struct iovec *iov;
	u_int cnt;
	int error = 0;
	proc_invocation_t pk = get_proc_invocation();

#if DIAGNOSTIC
	if (uio->uio_rw != UIO_READ && uio->uio_rw != UIO_WRITE)
		panic("uiomove: mode");
	if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != get_proc())
		panic("uiomove proc");
#endif
	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		if (uio->uio_segflg == UIO_SYSSPACE || pk->k_reply_msg == 0) {
		      /* UIO_SYSSPACE */
			if (uio->uio_rw == UIO_READ) {
				bcopy((caddr_t)cp, iov->iov_base, cnt);
			} else {
				boolean_t on_master = pk->k_master_lock;
				/*
				 * Unlock master lock to touch user data.
				 */
				if (on_master)
				    unix_release();
				bcopy(iov->iov_base, (caddr_t)cp, cnt);
				if (on_master)
				    unix_master();
			}
		} else {
			/* UIO_USERSPACE || UIO_USERISPACE */
			if (uio->uio_rw == UIO_READ) {
				vm_offset_t user_addr;

				user_addr = extend_reply_msg(
						(vm_offset_t) iov->iov_base,
							     cnt,
							     iov->iov_len);
				bcopy(cp, (void *) user_addr, cnt);
				extend_current_output(cnt);
			} else {
				error = copyin(iov->iov_base, cp, cnt);
				if (error)
				    return (error);
			}
		}

		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

/*
 * Give next character to user as result of read.
 */
mach_error_t ureadc(c, uio)
	register int c;
	register struct uio *uio;
{
	register struct iovec *iov;
	proc_invocation_t pk = get_proc_invocation();

again:
	if (uio->uio_iovcnt == 0 || uio->uio_resid == 0)
		panic("ureadc");
	iov = uio->uio_iov;
	if (iov->iov_len == 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}

	if (uio->uio_segflg == UIO_SYSSPACE || pk->k_reply_msg == 0) {
		/* UIO_SYSSPACE */
		*iov->iov_base = c;
	} else {
		/* UIO_USERSPACE || UIO_USERISPACE */
		register char *		user_addr;

		user_addr = (char *) extend_reply_msg(
						(vm_offset_t) iov->iov_base,
						iov->iov_len, iov->iov_len);
		*user_addr = c;
		extend_current_output(1);
	}
	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

/*
 * General routine to allocate a hash table.
 */
void *
hashinit(elements, type, hashmask)
	int elements, type;
	u_long *hashmask;
{
	long hashsize;
	LIST_HEAD(generic, generic) *hashtbl;
	int i;

	if (elements <= 0)
		panic("hashinit: bad cnt");
	for (hashsize = 1; hashsize <= elements; hashsize <<= 1)
		continue;
	hashsize >>= 1;
	hashtbl = bsd_malloc((u_long)hashsize * sizeof(*hashtbl), type, M_WAITOK);
	for (i = 0; i < hashsize; i++)
		LIST_INIT(&hashtbl[i]);
	*hashmask = hashsize - 1;
	return (hashtbl);
}
