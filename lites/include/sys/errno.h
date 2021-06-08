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
 * $Log: errno.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:33  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	Error code definitions for Lites.
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	November 1994
 *	Origin: Based on 4.4BSD Lite.
 *
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
 *	@(#)errno.h	8.5 (Berkeley) 1/21/94
 */

#ifndef _SYS_ERRNO_H_
#define _SYS_ERRNO_H_

#ifndef _POSIX_SOURCE
typedef int errno_t;
#define	ESUCCESS	0
#endif

#define	___EPERM	1		/* Operation not permitted */
#define	___ENOENT	2		/* No such file or directory */
#define	___ESRCH	3		/* No such process */
#define	___EINTR	4		/* Interrupted system call */
#define	___EIO		5		/* Input/output error */
#define	___ENXIO	6		/* Device not configured */
#define	___E2BIG	7		/* Argument list too long */
#define	___ENOEXEC	8		/* Exec format error */
#define	___EBADF	9		/* Bad file descriptor */
#define	___ECHILD	10		/* No child processes */
#define	___EDEADLK	11		/* Resource deadlock avoided */
					/* 11 was EAGAIN */
#define	___ENOMEM	12		/* Cannot allocate memory */
#define	___EACCES	13		/* Permission denied */
#define	___EFAULT	14		/* Bad address */
#ifndef _POSIX_SOURCE
#define	___ENOTBLK	15		/* Block device required */
#endif
#define	___EBUSY	16		/* Device busy */
#define	___EEXIST	17		/* File exists */
#define	___EXDEV	18		/* Cross-device link */
#define	___ENODEV	19		/* Operation not supported by device */
#define	___ENOTDIR	20		/* Not a directory */
#define	___EISDIR	21		/* Is a directory */
#define	___EINVAL	22		/* Invalid argument */
#define	___ENFILE	23		/* Too many open files in system */
#define	___EMFILE	24		/* Too many open files */
#define	___ENOTTY	25		/* Inappropriate ioctl for device */
#ifndef _POSIX_SOURCE
#define	___ETXTBSY	26		/* Text file busy */
#endif
#define	___EFBIG	27		/* File too large */
#define	___ENOSPC	28		/* No space left on device */
#define	___ESPIPE	29		/* Illegal seek */
#define	___EROFS	30		/* Read-only file system */
#define	___EMLINK	31		/* Too many links */
#define	___EPIPE	32		/* Broken pipe */

/* math software */
#define	___EDOM		33		/* Numerical argument out of domain */
#define	___ERANGE	34		/* Result too large */

/* non-blocking and interrupt i/o */
#define	___EAGAIN	35		/* Resource temporarily unavailable */
#ifndef _POSIX_SOURCE
#define	___EWOULDBLOCK	___EAGAIN	/* Operation would block */
#define	___EINPROGRESS	36		/* Operation now in progress */
#define	___EALREADY	37		/* Operation already in progress */

/* ipc/network software -- argument errors */
#define	___ENOTSOCK	38		/* Socket operation on non-socket */
#define	___EDESTADDRREQ	39		/* Destination address required */
#define	___EMSGSIZE	40		/* Message too long */
#define	___EPROTOTYPE	41		/* Protocol wrong type for socket */
#define	___ENOPROTOOPT	42		/* Protocol not available */
#define	___EPROTONOSUPPORT	43	/* Protocol not supported */
#define	___ESOCKTNOSUPPORT	44	/* Socket type not supported */
#define	___EOPNOTSUPP	45		/* Operation not supported */
#define	___EPFNOSUPPORT	46		/* Protocol family not supported */
#define	___EAFNOSUPPORT	47		/* Address family not supported by protocol family */
#define	___EADDRINUSE	48		/* Address already in use */
#define	___EADDRNOTAVAIL	49	/* Can't assign requested address */

/* ipc/network software -- operational errors */
#define	___ENETDOWN	50		/* Network is down */
#define	___ENETUNREACH	51		/* Network is unreachable */
#define	___ENETRESET	52		/* Network dropped connection on reset */
#define	___ECONNABORTED	53		/* Software caused connection abort */
#define	___ECONNRESET	54		/* Connection reset by peer */
#define	___ENOBUFS	55		/* No buffer space available */
#define	___EISCONN	56		/* Socket is already connected */
#define	___ENOTCONN	57		/* Socket is not connected */
#define	___ESHUTDOWN	58		/* Can't send after socket shutdown */
#define	___ETOOMANYREFS	59		/* Too many references: can't splice */
#define	___ETIMEDOUT	60		/* Operation timed out */
#define	___ECONNREFUSED	61		/* Connection refused */

#define	___ELOOP	62		/* Too many levels of symbolic links */
#endif /* _POSIX_SOURCE */
#define	___ENAMETOOLONG	63		/* File name too long */

/* should be rearranged */
#ifndef _POSIX_SOURCE
#define	___EHOSTDOWN	64		/* Host is down */
#define	___EHOSTUNREACH	65		/* No route to host */
#endif /* _POSIX_SOURCE */
#define	___ENOTEMPTY	66		/* Directory not empty */

/* quotas & mush */
#ifndef _POSIX_SOURCE
#define	___EPROCLIM	67		/* Too many processes */
#define	___EUSERS	68		/* Too many users */
#define	___EDQUOT	69		/* Disc quota exceeded */

/* Network File System */
#define	___ESTALE	70		/* Stale NFS file handle */
#define	___EREMOTE	71		/* Too many levels of remote in path */
#define	___EBADRPC	72		/* RPC struct is bad */
#define	___ERPCMISMATCH	73		/* RPC version wrong */
#define	___EPROGUNAVAIL	74		/* RPC prog. not avail */
#define	___EPROGMISMATCH	75	/* Program version wrong */
#define	___EPROCUNAVAIL	76		/* Bad procedure for program */
#endif /* _POSIX_SOURCE */

#define	___ENOLCK	77		/* No locks available */
#define	___ENOSYS	78		/* Function not implemented */

#ifndef _POSIX_SOURCE
#define	___EFTYPE	79		/* Inappropriate file type or format */
#define	___EAUTH	80		/* Authentication error */
#define	___ENEEDAUTH	81		/* Need authenticator */
#define	___ELAST	81		/* Must be equal largest errno */
#endif /* _POSIX_SOURCE */

#if defined(LITES)
/* LITES error codes visible to native applications and emulators */

/* This must correspond with mach/error.h */
#define LITES_ERRNO_BASE (3<<14)

#define	LITES_EPERM		(EPERM + LITES_ERRNO_BASE)
#define	LITES_ENOENT		(ENOENT + LITES_ERRNO_BASE)
#define	LITES_ESRCH		(ESRCH + LITES_ERRNO_BASE)
#define	LITES_EINTR		(EINTR + LITES_ERRNO_BASE)
#define	LITES_EIO		(EIO + LITES_ERRNO_BASE)
#define	LITES_ENXIO		(ENXIO + LITES_ERRNO_BASE)
#define	LITES_E2BIG		(E2BIG + LITES_ERRNO_BASE)
#define	LITES_ENOEXEC		(ENOEXEC + LITES_ERRNO_BASE)
#define	LITES_EBADF		(EBADF + LITES_ERRNO_BASE)
#define	LITES_ECHILD		(ECHILD + LITES_ERRNO_BASE)
#define	LITES_EDEADLK		(EDEADLK + LITES_ERRNO_BASE)
#define	LITES_ENOMEM		(ENOMEM + LITES_ERRNO_BASE)
#define	LITES_EACCES		(EACCES + LITES_ERRNO_BASE)
#define	LITES_EFAULT		(EFAULT + LITES_ERRNO_BASE)
#define	LITES_ENOTBLK		(ENOTBLK + LITES_ERRNO_BASE)
#define	LITES_EBUSY		(EBUSY + LITES_ERRNO_BASE)
#define	LITES_EEXIST		(EEXIST + LITES_ERRNO_BASE)
#define	LITES_EXDEV		(EXDEV + LITES_ERRNO_BASE)
#define	LITES_ENODEV		(ENODEV + LITES_ERRNO_BASE)
#define	LITES_ENOTDIR		(ENOTDIR + LITES_ERRNO_BASE)
#define	LITES_EISDIR		(EISDIR + LITES_ERRNO_BASE)
#define	LITES_EINVAL		(EINVAL + LITES_ERRNO_BASE)
#define	LITES_ENFILE		(ENFILE + LITES_ERRNO_BASE)
#define	LITES_EMFILE		(EMFILE + LITES_ERRNO_BASE)
#define	LITES_ENOTTY		(ENOTTY + LITES_ERRNO_BASE)
#define	LITES_ETXTBSY		(ETXTBSY + LITES_ERRNO_BASE)
#define	LITES_EFBIG		(EFBIG + LITES_ERRNO_BASE)
#define	LITES_ENOSPC		(ENOSPC + LITES_ERRNO_BASE)
#define	LITES_ESPIPE		(ESPIPE + LITES_ERRNO_BASE)
#define	LITES_EROFS		(EROFS + LITES_ERRNO_BASE)
#define	LITES_EMLINK		(EMLINK + LITES_ERRNO_BASE)
#define	LITES_EPIPE		(EPIPE + LITES_ERRNO_BASE)
#define	LITES_EDOM		(EDOM + LITES_ERRNO_BASE)
#define	LITES_ERANGE		(ERANGE + LITES_ERRNO_BASE)
#define	LITES_EAGAIN		(EAGAIN + LITES_ERRNO_BASE)
#define	LITES_EWOULDBLOCK	(EWOULDBLOCK + LITES_ERRNO_BASE)
#define	LITES_EINPROGRESS	(EINPROGRESS + LITES_ERRNO_BASE)
#define	LITES_EALREADY		(EALREADY + LITES_ERRNO_BASE)
#define	LITES_ENOTSOCK		(ENOTSOCK + LITES_ERRNO_BASE)
#define	LITES_EDESTADDRREQ	(EDESTADDRREQ + LITES_ERRNO_BASE)
#define	LITES_EMSGSIZE		(EMSGSIZE + LITES_ERRNO_BASE)
#define	LITES_EPROTOTYPE	(EPROTOTYPE + LITES_ERRNO_BASE)
#define	LITES_ENOPROTOOPT	(ENOPROTOOPT + LITES_ERRNO_BASE)
#define	LITES_EPROTONOSUPPORT	(EPROTONOSUPPORT + LITES_ERRNO_BASE)
#define	LITES_ESOCKTNOSUPPORT	(ESOCKTNOSUPPORT + LITES_ERRNO_BASE)
#define	LITES_EOPNOTSUPP	(EOPNOTSUPP + LITES_ERRNO_BASE)
#define	LITES_EPFNOSUPPORT	(EPFNOSUPPORT + LITES_ERRNO_BASE)
#define	LITES_EAFNOSUPPORT	(EAFNOSUPPORT + LITES_ERRNO_BASE)
#define	LITES_EADDRINUSE	(EADDRINUSE + LITES_ERRNO_BASE)
#define	LITES_EADDRNOTAVAIL	(EADDRNOTAVAIL + LITES_ERRNO_BASE)
#define	LITES_ENETDOWN		(ENETDOWN + LITES_ERRNO_BASE)
#define	LITES_ENETUNREACH	(ENETUNREACH + LITES_ERRNO_BASE)
#define	LITES_ENETRESET		(ENETRESET + LITES_ERRNO_BASE)
#define	LITES_ECONNABORTED	(ECONNABORTED + LITES_ERRNO_BASE)
#define	LITES_ECONNRESET	(ECONNRESET + LITES_ERRNO_BASE)
#define	LITES_ENOBUFS		(ENOBUFS + LITES_ERRNO_BASE)
#define	LITES_EISCONN		(EISCONN + LITES_ERRNO_BASE)
#define	LITES_ENOTCONN		(ENOTCONN + LITES_ERRNO_BASE)
#define	LITES_ESHUTDOWN		(ESHUTDOWN + LITES_ERRNO_BASE)
#define	LITES_ETOOMANYREFS	(ETOOMANYREFS + LITES_ERRNO_BASE)
#define	LITES_ETIMEDOUT		(ETIMEDOUT + LITES_ERRNO_BASE)
#define	LITES_ECONNREFUSED	(ECONNREFUSED + LITES_ERRNO_BASE)
#define	LITES_ELOOP		(ELOOP + LITES_ERRNO_BASE)
#define	LITES_ENAMETOOLONG	(ENAMETOOLONG + LITES_ERRNO_BASE)
#define	LITES_EHOSTDOWN		(EHOSTDOWN + LITES_ERRNO_BASE)
#define	LITES_EHOSTUNREACH	(EHOSTUNREACH + LITES_ERRNO_BASE)
#define	LITES_ENOTEMPTY		(ENOTEMPTY + LITES_ERRNO_BASE)
#define	LITES_EPROCLIM		(EPROCLIM + LITES_ERRNO_BASE)
#define	LITES_EUSERS		(EUSERS + LITES_ERRNO_BASE)
#define	LITES_EDQUOT		(EDQUOT + LITES_ERRNO_BASE)
#define	LITES_ESTALE		(ESTALE + LITES_ERRNO_BASE)
#define	LITES_EREMOTE		(EREMOTE + LITES_ERRNO_BASE)
#define	LITES_EBADRPC		(EBADRPC + LITES_ERRNO_BASE)
#define	LITES_ERPCMISMATCH	(ERPCMISMATCH + LITES_ERRNO_BASE)
#define	LITES_EPROGUNAVAIL	(EPROGUNAVAIL + LITES_ERRNO_BASE)
#define	LITES_EPROGMISMATCH	(EPROGMISMATCH + LITES_ERRNO_BASE)
#define	LITES_EPROCUNAVAIL	(EPROCUNAVAIL + LITES_ERRNO_BASE)
#define	LITES_ENOLCK		(ENOLCK + LITES_ERRNO_BASE)
#define	LITES_ENOSYS		(ENOSYS + LITES_ERRNO_BASE)
#define	LITES_EFTYPE		(EFTYPE + LITES_ERRNO_BASE)
#define	LITES_EAUTH		(EAUTH + LITES_ERRNO_BASE)
#define	LITES_ENEEDAUTH		(ENEEDAUTH + LITES_ERRNO_BASE)
#define	LITES_ELAST		(ELAST + LITES_ERRNO_BASE)

/* pseudo-errors returned inside kernel to modify return to process */
#define	ERESTART	-1		/* restart syscall */
#define	EJUSTRETURN	-2		/* don't modify regs, just return */

#endif /* LITES */


#if defined(LITES) && defined(KERNEL)
/* Internal to LITES server (for source compatibilty reasons) */

#define	EPERM		(___EPERM + LITES_ERRNO_BASE)
#define	ENOENT		(___ENOENT + LITES_ERRNO_BASE)
#define	ESRCH		(___ESRCH + LITES_ERRNO_BASE)
#define	EINTR		(___EINTR + LITES_ERRNO_BASE)
#define	EIO		(___EIO + LITES_ERRNO_BASE)
#define	ENXIO		(___ENXIO + LITES_ERRNO_BASE)
#define	E2BIG		(___E2BIG + LITES_ERRNO_BASE)
#define	ENOEXEC		(___ENOEXEC + LITES_ERRNO_BASE)
#define	EBADF		(___EBADF + LITES_ERRNO_BASE)
#define	ECHILD		(___ECHILD + LITES_ERRNO_BASE)
#define	EDEADLK		(___EDEADLK + LITES_ERRNO_BASE)
#define	ENOMEM		(___ENOMEM + LITES_ERRNO_BASE)
#define	EACCES		(___EACCES + LITES_ERRNO_BASE)
#define	EFAULT		(___EFAULT + LITES_ERRNO_BASE)
#define	ENOTBLK		(___ENOTBLK + LITES_ERRNO_BASE)
#define	EBUSY		(___EBUSY + LITES_ERRNO_BASE)
#define	EEXIST		(___EEXIST + LITES_ERRNO_BASE)
#define	EXDEV		(___EXDEV + LITES_ERRNO_BASE)
#define	ENODEV		(___ENODEV + LITES_ERRNO_BASE)
#define	ENOTDIR		(___ENOTDIR + LITES_ERRNO_BASE)
#define	EISDIR		(___EISDIR + LITES_ERRNO_BASE)
#define	EINVAL		(___EINVAL + LITES_ERRNO_BASE)
#define	ENFILE		(___ENFILE + LITES_ERRNO_BASE)
#define	EMFILE		(___EMFILE + LITES_ERRNO_BASE)
#define	ENOTTY		(___ENOTTY + LITES_ERRNO_BASE)
#define	ETXTBSY		(___ETXTBSY + LITES_ERRNO_BASE)
#define	EFBIG		(___EFBIG + LITES_ERRNO_BASE)
#define	ENOSPC		(___ENOSPC + LITES_ERRNO_BASE)
#define	ESPIPE		(___ESPIPE + LITES_ERRNO_BASE)
#define	EROFS		(___EROFS + LITES_ERRNO_BASE)
#define	EMLINK		(___EMLINK + LITES_ERRNO_BASE)
#define	EPIPE		(___EPIPE + LITES_ERRNO_BASE)
#define	EDOM		(___EDOM + LITES_ERRNO_BASE)
#define	ERANGE		(___ERANGE + LITES_ERRNO_BASE)
#define	EAGAIN		(___EAGAIN + LITES_ERRNO_BASE)
#define	EWOULDBLOCK	(___EWOULDBLOCK + LITES_ERRNO_BASE)
#define	EINPROGRESS	(___EINPROGRESS + LITES_ERRNO_BASE)
#define	EALREADY	(___EALREADY + LITES_ERRNO_BASE)
#define	ENOTSOCK	(___ENOTSOCK + LITES_ERRNO_BASE)
#define	EDESTADDRREQ	(___EDESTADDRREQ + LITES_ERRNO_BASE)
#define	EMSGSIZE	(___EMSGSIZE + LITES_ERRNO_BASE)
#define	EPROTOTYPE	(___EPROTOTYPE + LITES_ERRNO_BASE)
#define	ENOPROTOOPT	(___ENOPROTOOPT + LITES_ERRNO_BASE)
#define	EPROTONOSUPPORT	(___EPROTONOSUPPORT + LITES_ERRNO_BASE)
#define	ESOCKTNOSUPPORT	(___ESOCKTNOSUPPORT + LITES_ERRNO_BASE)
#define	EOPNOTSUPP	(___EOPNOTSUPP + LITES_ERRNO_BASE)
#define	EPFNOSUPPORT	(___EPFNOSUPPORT + LITES_ERRNO_BASE)
#define	EAFNOSUPPORT	(___EAFNOSUPPORT + LITES_ERRNO_BASE)
#define	EADDRINUSE	(___EADDRINUSE + LITES_ERRNO_BASE)
#define	EADDRNOTAVAIL	(___EADDRNOTAVAIL + LITES_ERRNO_BASE)
#define	ENETDOWN	(___ENETDOWN + LITES_ERRNO_BASE)
#define	ENETUNREACH	(___ENETUNREACH + LITES_ERRNO_BASE)
#define	ENETRESET	(___ENETRESET + LITES_ERRNO_BASE)
#define	ECONNABORTED	(___ECONNABORTED + LITES_ERRNO_BASE)
#define	ECONNRESET	(___ECONNRESET + LITES_ERRNO_BASE)
#define	ENOBUFS		(___ENOBUFS + LITES_ERRNO_BASE)
#define	EISCONN		(___EISCONN + LITES_ERRNO_BASE)
#define	ENOTCONN	(___ENOTCONN + LITES_ERRNO_BASE)
#define	ESHUTDOWN	(___ESHUTDOWN + LITES_ERRNO_BASE)
#define	ETOOMANYREFS	(___ETOOMANYREFS + LITES_ERRNO_BASE)
#define	ETIMEDOUT	(___ETIMEDOUT + LITES_ERRNO_BASE)
#define	ECONNREFUSED	(___ECONNREFUSED + LITES_ERRNO_BASE)
#define	ELOOP		(___ELOOP + LITES_ERRNO_BASE)
#define	ENAMETOOLONG	(___ENAMETOOLONG + LITES_ERRNO_BASE)
#define	EHOSTDOWN	(___EHOSTDOWN + LITES_ERRNO_BASE)
#define	EHOSTUNREACH	(___EHOSTUNREACH + LITES_ERRNO_BASE)
#define	ENOTEMPTY	(___ENOTEMPTY + LITES_ERRNO_BASE)
#define	EPROCLIM	(___EPROCLIM + LITES_ERRNO_BASE)
#define	EUSERS		(___EUSERS + LITES_ERRNO_BASE)
#define	EDQUOT		(___EDQUOT + LITES_ERRNO_BASE)
#define	ESTALE		(___ESTALE + LITES_ERRNO_BASE)
#define	EREMOTE		(___EREMOTE + LITES_ERRNO_BASE)
#define	EBADRPC		(___EBADRPC + LITES_ERRNO_BASE)
#define	ERPCMISMATCH	(___ERPCMISMATCH + LITES_ERRNO_BASE)
#define	EPROGUNAVAIL	(___EPROGUNAVAIL + LITES_ERRNO_BASE)
#define	EPROGMISMATCH	(___EPROGMISMATCH + LITES_ERRNO_BASE)
#define	EPROCUNAVAIL	(___EPROCUNAVAIL + LITES_ERRNO_BASE)
#define	ENOLCK		(___ENOLCK + LITES_ERRNO_BASE)
#define	ENOSYS		(___ENOSYS + LITES_ERRNO_BASE)
#define	EFTYPE		(___EFTYPE + LITES_ERRNO_BASE)
#define	EAUTH		(___EAUTH + LITES_ERRNO_BASE)
#define	ENEEDAUTH	(___ENEEDAUTH + LITES_ERRNO_BASE)
#define	ELAST		(___ELAST + LITES_ERRNO_BASE)
#endif /* defined(LITES) && defined(KERNEL) */


#if !defined(KERNEL)
/* BSD Application error codes */
extern int errno;			/* global error number */

#define	EPERM		___EPERM
#define	ENOENT		___ENOENT
#define	ESRCH		___ESRCH
#define	EINTR		___EINTR
#define	EIO		___EIO
#define	ENXIO		___ENXIO
#define	E2BIG		___E2BIG
#define	ENOEXEC		___ENOEXEC
#define	EBADF		___EBADF
#define	ECHILD		___ECHILD
#define	EDEADLK		___EDEADLK
#define	ENOMEM		___ENOMEM
#define	EACCES		___EACCES
#define	EFAULT		___EFAULT
#ifndef _POSIX_SOURCE
#define	ENOTBLK		___ENOTBLK
#endif
#define	EBUSY		___EBUSY
#define	EEXIST		___EEXIST
#define	EXDEV		___EXDEV
#define	ENODEV		___ENODEV
#define	ENOTDIR		___ENOTDIR
#define	EISDIR		___EISDIR
#define	EINVAL		___EINVAL
#define	ENFILE		___ENFILE
#define	EMFILE		___EMFILE
#define	ENOTTY		___ENOTTY
#ifndef _POSIX_SOURCE
#define	ETXTBSY		___ETXTBSY
#endif
#define	EFBIG		___EFBIG
#define	ENOSPC		___ENOSPC
#define	ESPIPE		___ESPIPE
#define	EROFS		___EROFS
#define	EMLINK		___EMLINK
#define	EPIPE		___EPIPE
#define	EDOM		___EDOM
#define	ERANGE		___ERANGE
#define	EAGAIN		___EAGAIN
#ifndef _POSIX_SOURCE
#define	EWOULDBLOCK	___EWOULDBLOCK
#define	EINPROGRESS	___EINPROGRESS
#define	EALREADY	___EALREADY
#define	ENOTSOCK	___ENOTSOCK
#define	EDESTADDRREQ	___EDESTADDRREQ
#define	EMSGSIZE	___EMSGSIZE
#define	EPROTOTYPE	___EPROTOTYPE
#define	ENOPROTOOPT	___ENOPROTOOPT
#define	EPROTONOSUPPORT	___EPROTONOSUPPORT
#define	ESOCKTNOSUPPORT	___ESOCKTNOSUPPORT
#define	EOPNOTSUPP	___EOPNOTSUPP
#define	EPFNOSUPPORT	___EPFNOSUPPORT
#define	EAFNOSUPPORT	___EAFNOSUPPORT
#define	EADDRINUSE	___EADDRINUSE
#define	EADDRNOTAVAIL	___EADDRNOTAVAIL
#define	ENETDOWN	___ENETDOWN
#define	ENETUNREACH	___ENETUNREACH
#define	ENETRESET	___ENETRESET
#define	ECONNABORTED	___ECONNABORTED
#define	ECONNRESET	___ECONNRESET
#define	ENOBUFS		___ENOBUFS
#define	EISCONN		___EISCONN
#define	ENOTCONN	___ENOTCONN
#define	ESHUTDOWN	___ESHUTDOWN
#define	ETOOMANYREFS	___ETOOMANYREFS
#define	ETIMEDOUT	___ETIMEDOUT
#define	ECONNREFUSED	___ECONNREFUSED
#define	ELOOP		___ELOOP
#endif /* _POSIX_SOURCE */
#define	ENAMETOOLONG	___ENAMETOOLONG
#ifndef _POSIX_SOURCE
#define	EHOSTDOWN	___EHOSTDOWN
#define	EHOSTUNREACH	___EHOSTUNREACH
#endif
#define	ENOTEMPTY	___ENOTEMPTY
#ifndef _POSIX_SOURCE
#define	EPROCLIM	___EPROCLIM
#define	EUSERS		___EUSERS
#define	EDQUOT		___EDQUOT
#define	ESTALE		___ESTALE
#define	EREMOTE		___EREMOTE
#define	EBADRPC		___EBADRPC
#define	ERPCMISMATCH	___ERPCMISMATCH
#define	EPROGUNAVAIL	___EPROGUNAVAIL
#define	EPROGMISMATCH	___EPROGMISMATCH
#define	EPROCUNAVAIL	___EPROCUNAVAIL
#endif /* _POSIX_SOURCE */
#define	ENOLCK		___ENOLCK
#define	ENOSYS		___ENOSYS
#ifndef _POSIX_SOURCE
#define	EFTYPE		___EFTYPE
#define	EAUTH		___EAUTH
#define	ENEEDAUTH	___ENEEDAUTH
#define	ELAST		___ELAST
#endif /* _POSIX_SOURCE */
#endif /* !defined(KERNEL) */

#endif /* !_SYS_ERRNO_H_ */
