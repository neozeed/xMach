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
 * $Log: e_readwrite.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:29  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/e_readwrite.c
 *	Author:	Randall W. Dean
 *	Date:	1992
 *
 * 	read/write.
 */

#include <e_defs.h>

#define	DEBUG 1

#if	DEBUG
#define	EPRINT(a) e_emulator_error a
#else	DEBUG
#define	EPRINT(a)
#endif	DEBUG

errno_t OLDe_writev(int fdes, struct iovec *iov, int iovcnt, int *nwritten)
{
	errno_t        err;
	kern_return_t  kr;
	int            intr;
	int            i;
	int            len;
	char          *cp;
	struct iovec  *iovp;
	unsigned int   count;
	int            result;
	char          *bufptr;
	struct iovec   aiov[16];
	char           buf[SMALL_ARRAY_LIMIT];
#ifdef	MAP_UAREA
	int            shared = 0;
#ifdef	MAP_FILE
	int            nocopy = 0;
	register       struct file_info *fd = &shared_base_rw->us_file_info[fdes];

	if (fdes < 0 ||
#ifndef	OSF1_SERVER
	   (shared_enabled && fdes > shared_base_ro->us_lastfile)) {
#else	OSF1_SERVER
	   (shared_enabled && fdes > shared_base_ro->us_file_state.uf_lastfile)) {
#endif	OSF1_SERVER
	    EPRINT(("e_writev badfile"));
	    return EBADF;
	}
#endif	MAP_FILE
#endif	MAP_UAREA

	if (iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
	    EPRINT(("e_writev too many vectors"));
	    return (EINVAL);
	}

	bcopy((char *)iov, (char *)aiov, iovcnt * sizeof(struct iovec));

	count = 0;
	for (i = 0, iovp = aiov; i < iovcnt; i++, iovp++) {
	    len = iovp->iov_len;
	    if (len < 0) {
		EPRINT(("e_writev invalid vector length %x", len));
		return (EINVAL);
	    }
	    count += len;
	}
#ifdef	MAP_UAREA
#ifdef	MAP_FILE
	share_lock(&fd->lock);
	if (fd->mapped && fd->open) {
	    share_unlock(&fd->lock);
	    nocopy = 1;
	} else {
	    share_unlock(&fd->lock);
#endif	MAP_FILE
	    spin_lock(&readwrite_lock);
	    if (count <= 2*vm_page_size && readwrite_inuse == 0) {
		bufptr = shared_readwrite;
		readwrite_inuse = shared = 1;
		spin_unlock(&readwrite_lock);
	    } else {
		spin_unlock(&readwrite_lock);
#endif	MAP_UAREA
		if (count <= SMALL_ARRAY_LIMIT) {
		    /*
		     * Short write.  Copy into buffer.
		     */
		    bufptr = buf;
		}
		else {
		    /*
		     * Long write.  Allocate memory to fill.
		     * (Hope that no one uses this to write large
		     *  amounts of data; we`ll lose on the copying.)
		     */
		    (void) vm_allocate(mach_task_self(),
				(vm_offset_t *)&bufptr,
				count,
				TRUE);
		}
#ifdef	MAP_UAREA
	    }
#ifdef	MAP_FILE
	}
#endif	MAP_FILE
#endif	MAP_UAREA
	cp = bufptr;
	for (i = 0, iovp = aiov; i < iovcnt; i++, iovp++) {
	    len = iovp->iov_len;
#if	defined(MAP_UAREA) && defined(MAP_FILE)
	    if (nocopy) {
		if (!e_maprw(process_self(), &intr, fdes, iovp->iov_base, len,
		             nwritten, 1, &result) || result != ESUCCESS) {
		    EPRINT(("e_writev e_maprw %d",result));
		    return (result);
		}
	    } else
#endif	defined(MAP_UAREA) && defined(MAP_FILE)
	    bcopy(iovp->iov_base, cp, len);
	    cp += len;
	}

#ifdef	MAP_UAREA
#ifdef	MAP_FILE
	if (nocopy) {
		*nwritten  = count;
		return ESUCCESS;
	}
#endif	MAP_FILE
	if (shared) {
	    result = e_readwrite(process_self(), &intr, fdes, 0, count, 
				 nwritten, 1, 0);
	    spin_lock(&readwrite_lock);
	    readwrite_inuse = 0;
	    spin_unlock(&readwrite_lock);
	} else {
#endif	MAP_UAREA
	    result = ((count <= SMALL_ARRAY_LIMIT) ? bsd_write_short
						   : bsd_write_long)
			(process_self(),
			&intr,
			fdes,
			bufptr,
			count,
			nwritten);

	    if (result != 0) {
		EPRINT(("e_writev bsd_write_* %d",result));
	    }
	    if (count > SMALL_ARRAY_LIMIT)
		(void) vm_deallocate(mach_task_self(), (vm_offset_t)bufptr, count);

#ifdef	MAP_UAREA
	}
#endif	MAP_UAREA

	return result ? e_mach_error_to_errno(result) : 0;
}

errno_t OLDe_readv(int fdes, struct iovec *iov, int iovcnt, int *nread)
{
	errno_t        err;
	kern_return_t  kr;
	int            intr;
	integer_t      rv[2];
	int            i;
	int            len;
	struct iovec  *iovp;
	unsigned int   count, count_requested;
	int            result;
	integer_t      arg[3];
	struct iovec   aiov[16];
	char           buf[SMALL_ARRAY_LIMIT];
#ifdef	MAP_UAREA
	int            shared = 0;
#ifdef	MAP_FILE
	int            nocopy = 0;
	register       struct file_info *fd = &shared_base_rw->us_file_info[fdes];

	if (fdes < 0 ||
#ifndef	OSF1_SERVER
	   (shared_enabled && fdes > shared_base_ro->us_lastfile)) {
#else	OSF1_SERVER
	   (shared_enabled && fdes > shared_base_ro->us_file_state.uf_lastfile)) {
#endif	OSF1_SERVER
	    EPRINT(("e_writev badfile"));
	    return EBADF;
	}
#endif	MAP_FILE
#endif	MAP_UAREA

	if (iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
	    EPRINT(("e_readv too many vectors"));
	    return (EINVAL);
	}

	bcopy((char *)iov, (char *)aiov, iovcnt * sizeof(struct iovec));

	count = 0;
	for (i = 0, iovp = aiov; i < iovcnt; i++, iovp++) {
	    len = iovp->iov_len;
	    if (len < 0) {
		EPRINT(("e_readv invalid vector length %x", len));
		EPRINT(("        count = %d  vector =%d", count, i));
		EPRINT(("        iovcnt = %d", iovcnt));
		return (EINVAL);
	    }
	    count += len;
	}

	count_requested = count;
	arg[0] = fdes;
	arg[2] = (integer_t)count;

#ifdef	MAP_UAREA
#ifdef	MAP_FILE
	share_lock(&fd->lock);
	if (fd->mapped && fd->open) {
	    share_unlock(&fd->lock);
	    nocopy = 1;
	} else {
	    share_unlock(&fd->lock);
#endif	MAP_FILE
	    spin_lock(&readwrite_lock);
	    if (count <= 2*vm_page_size && readwrite_inuse == 0) {
		arg[1] = (integer_t) shared_readwrite;
		readwrite_inuse = shared = 1;
		spin_unlock(&readwrite_lock);
	    } else {
		spin_unlock(&readwrite_lock);
#endif	MAP_UAREA
		if (count <= SMALL_ARRAY_LIMIT) {
		    /*
		     * Short read.  Copy into buffer.
		     */
		    arg[1] = (integer_t)buf;
		}
		else {
		    /*
		     * Long read.  Allocate memory to fill.
		     * (Hope that no one uses this to read large
		     *  amounts of data; we`ll lose on the copying.)
		     */
		    (void) vm_allocate(mach_task_self(),
				       (vm_offset_t *) &arg[1],
				       count,
				       TRUE);
		}
#ifdef	MAP_UAREA
	    }
#ifdef	MAP_FILE
	}

	if (nocopy) {
	    result = 0;
	} else
#endif	MAP_FILE
	    if (shared) {
		result = e_readwrite(process_self(), &intr, fdes, 0,
				     count, rv, 0, 0);
		if (rv[0]>count) {
			EPRINT(("e_readv e_readwrite ask %d got %d",count,rv[0]));
		}
		count = rv[0];
	    } else
#endif	MAP_UAREA
	    {
		result = emul_generic(process_self(), &intr,
				      SYS_read, arg, rv);
		if (rv[0]>count) {
			EPRINT(("e_readv SYS_read ask %d got %d",count,rv[0]));
		}
		count = rv[0];
		if (result != 0) {
			EPRINT(("e_readv SYS_read %d",result));
		}
	    }

	if (result == 0) {
	    char *cp;

	    cp = (char *)arg[1];

	    for (i = 0, iovp = aiov; i < iovcnt; i++, iovp++) {
		len = iovp->iov_len;
#if	defined(MAP_UAREA) && defined(MAP_FILE)
		if (nocopy) {
			if (!e_maprw(process_self(), &intr, fdes, iovp->iov_base,
				     len, rv, 0, &result) || result != ESUCCESS) {
				EPRINT(("e_readv e_maprw %d", result));
				if (result == 0)
				    *nread = rv[0];
				return result;
			}
			if (rv[0] == 0) {
				*nread = rv[0] = (integer_t)(cp - arg[1]);
				if (rv[0] > arg[2]) {
					EPRINT(("r_readv ask %d got %d",
						arg[2], rv[0]));
				}
				return ESUCCESS;
			}
		} else
#endif	defined(MAP_UAREA) && defined(MAP_FILE)
		    bcopy(cp, iovp->iov_base, len);
		cp += len;
	    }
	}

#ifdef	MAP_UAREA
	if (shared) {
	    spin_lock(&readwrite_lock);
	    readwrite_inuse = 0;
	    spin_unlock(&readwrite_lock);
	}
#endif	MAP_UAREA

	if (count_requested > SMALL_ARRAY_LIMIT)
#ifdef	MAP_UAREA
	    if (!shared)
#ifdef	MAP_FILE
		if(!nocopy)
#endif	MAP_FILE
#endif	MAP_UAREA
		    (void) vm_deallocate(mach_task_self(), (vm_offset_t)arg[1],
					 count_requested);

	*nread = count;
	if (rv[0] > arg[2]) {
	    EPRINT(("r_readv(2) ask %d got %d",arg[2],rv[0]));
	}
	return result ? e_mach_error_to_errno(result) : 0;
}

errno_t OLDe_read(int fd, void *buf, size_t nbytes, size_t *nread)
{
	errno_t err;
	kern_return_t kr;
	int intr;
	integer_t rv[2];

	struct {
		int fd;
		void *buf;
		size_t nbytes;
	} a;
	a.nbytes = nbytes;
	a.buf = buf;
	a.fd = fd;

	kr = emul_generic(process_self(), &intr, SYS_read, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*nread = rv[0];
	return err;
}

errno_t OLDe_write(int fd, const void *buf, size_t nbytes, size_t *nwritten)
{
	errno_t err;
	kern_return_t kr;
	int intr;

#if defined(MAP_UAREA) && !defined(alpha)
	int result = 0;
#ifdef	MAP_FILE
	if (shared_enabled &&
	    e_maprw(process_self(), &intr, fd, buf, nbytes, nwritten, 1,
		    &result)) {

		return e_mach_error_to_errno(result);
	} else
#endif	MAP_FILE
	if (shared_enabled && nbytes <= 2*vm_page_size) {
	    spin_lock(&readwrite_lock);
	    if (readwrite_inuse) {
		spin_unlock(&readwrite_lock);
		goto server;
	    }
	    readwrite_inuse = 1;
	    spin_unlock(&readwrite_lock);
	    kr = e_readwrite(process_self(), &intr, fd, buf,
			       nbytes, nwritten, 1, 1);
	    return kr ? e_mach_error_to_errno(kr) : 0;
	}
server:
#endif /* MAP_UAREA */
	kr = ((nbytes <= SMALL_ARRAY_LIMIT) ? bsd_write_short
					    : bsd_write_long
		)(process_self(),
		  &intr,
		  fd,
		  buf,
		  nbytes,
		  nwritten);

	err = kr ? e_mach_error_to_errno(kr) : 0;

	return err;
}

