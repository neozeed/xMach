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
 * $Log: malloc.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:34  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	include/sys/malloc.h
 *	Origin:	Adapted to Lites from 4.4 BSD Lite.
 */
/*
 * Copyright (c) 1987, 1993
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
 *	@(#)malloc.h	8.3 (Berkeley) 1/12/94
 */

#ifndef _SYS_MALLOC_H_
#define	_SYS_MALLOC_H_

#include <sys/zalloc.h>

/*
 * flags to malloc
 */
#define	M_WAITOK	0x0000
#define	M_NOWAIT	0x0001

/*
 * Types of memory to be allocated
 */
#if 0
extern zone_t		zone_M_FREE;
#define	M_FREE		zone_M_FREE	/* should be on free list */
extern zone_t		zone_M_MBUF;
#define	M_MBUF		zone_M_MBUF	/* mbuf */
extern zone_t		zone_M_DEVBUF;
#define	M_DEVBUF	zone_M_DEVBUF	/* device driver memory */
extern zone_t		zone_M_SOCKET;
#define	M_SOCKET	zone_M_SOCKET	/* socket structure */
extern zone_t		zone_M_PCB;
#define	M_PCB		zone_M_PCB	/* protocol control block */
extern zone_t		zone_M_RTABLE;
#define	M_RTABLE	zone_M_RTABLE	/* routing tables */
extern zone_t		zone_M_HTABLE;
#define	M_HTABLE	zone_M_HTABLE	/* IMP host tables */
extern zone_t		zone_M_FTABLE;
#define	M_FTABLE	zone_M_FTABLE	/* fragment reassembly header */
extern zone_t		zone_M_ZOMBIE;
#define	M_ZOMBIE	zone_M_ZOMBIE	/* zombie proc status */
extern zone_t		zone_M_IFADDR;
#define	M_IFADDR	zone_M_IFADDR	/* interface address */
extern zone_t		zone_M_SOOPTS;
#define	M_SOOPTS	zone_M_SOOPTS	/* socket options */
extern zone_t		zone_M_SONAME;
#define	M_SONAME	zone_M_SONAME	/* socket name */
extern zone_t		zone_M_NAMEI;
#define	M_NAMEI		zone_M_NAMEI	/* namei path name buffer */
extern zone_t		zone_M_GPROF;
#define	M_GPROF		zone_M_GPROF	/* kernel profiling buffer */
extern zone_t		zone_M_IOCTLOPS;
#define	M_IOCTLOPS	zone_M_IOCTLOPS	/* ioctl data buffer */
extern zone_t		zone_M_MAPMEM;
#define	M_MAPMEM	zone_M_MAPMEM	/* mapped memory descriptors */
extern zone_t		zone_M_CRED;
#define	M_CRED		zone_M_CRED	/* credentials */
extern zone_t		zone_M_PGRP;
#define	M_PGRP		zone_M_PGRP	/* process group header */
extern zone_t		zone_M_SESSION;
#define	M_SESSION	zone_M_SESSION	/* session header */
extern zone_t		zone_M_IOV;
#define	M_IOV		zone_M_IOV	/* large iov's */
extern zone_t		zone_M_MOUNT;
#define	M_MOUNT		zone_M_MOUNT	/* vfs mount struct */
extern zone_t		zone_M_FHANDLE;
#define	M_FHANDLE	zone_M_FHANDLE	/* network file handle */
extern zone_t		zone_M_NFSREQ;
#define	M_NFSREQ	zone_M_NFSREQ	/* NFS request header */
extern zone_t		zone_M_NFSMNT;
#define	M_NFSMNT	zone_M_NFSMNT	/* NFS mount structure */
extern zone_t		zone_M_NFSNODE;
#define	M_NFSNODE	zone_M_NFSNODE	/* NFS vnode private part */
extern zone_t		zone_M_VNODE;
#define	M_VNODE		zone_M_VNODE	/* Dynamically allocated vnodes */
extern zone_t		zone_M_CACHE;
#define	M_CACHE		zone_M_CACHE /* Dynamically allocated cache entries */
extern zone_t		zone_M_DQUOT;
#define	M_DQUOT		zone_M_DQUOT	/* UFS quota entries */
extern zone_t		zone_M_UFSMNT;
#define	M_UFSMNT	zone_M_UFSMNT	/* UFS mount structure */
extern zone_t		zone_M_SHM;
#define	M_SHM		zone_M_SHM /* SVID compatible shared memory segments */
extern zone_t		zone_M_VMMAP;
#define	M_VMMAP		zone_M_VMMAP	/* VM map structures */
extern zone_t		zone_M_VMMAPENT;
#define	M_VMMAPENT	zone_M_VMMAPENT	/* VM map entry structures */
extern zone_t		zone_M_VMOBJ;
#define	M_VMOBJ		zone_M_VMOBJ	/* VM object structure */
extern zone_t		zone_M_VMOBJHASH;
#define	M_VMOBJHASH	zone_M_VMOBJHASH	/* VM object hash structure */
extern zone_t		zone_M_VMPMAP;
#define	M_VMPMAP	zone_M_VMPMAP	/* VM pmap */
extern zone_t		zone_M_VMPVENT;
#define	M_VMPVENT	zone_M_VMPVENT	/* VM phys-virt mapping entry */
extern zone_t		zone_M_VMPAGER;
#define	M_VMPAGER	zone_M_VMPAGER	/* XXX: VM pager struct */
extern zone_t		zone_M_VMPGDATA;
#define	M_VMPGDATA	zone_M_VMPGDATA	/* XXX: VM pager private data */
extern zone_t		zone_M_FILE;
#define	M_FILE		zone_M_FILE	/* Open file structure */
extern zone_t		zone_M_FILEDESC;
#define	M_FILEDESC	zone_M_FILEDESC	/* Open file descriptor table */
extern zone_t		zone_M_LOCKF;
#define	M_LOCKF		zone_M_LOCKF	/* Byte-range locking structures */
extern zone_t		zone_M_PROC;
#define	M_PROC		zone_M_PROC	/* Proc structures */
extern zone_t		zone_M_SUBPROC;
#define	M_SUBPROC	zone_M_SUBPROC	/* Proc sub-structures */
extern zone_t		zone_M_SEGMENT;
#define	M_SEGMENT	zone_M_SEGMENT	/* Segment for LFS */
extern zone_t		zone_M_LFSNODE;
#define	M_LFSNODE	zone_M_LFSNODE	/* LFS vnode private part */
extern zone_t		zone_M_FFSNODE;
#define	M_FFSNODE	zone_M_FFSNODE	/* FFS vnode private part */
extern zone_t		zone_M_MFSNODE;
#define	M_MFSNODE	zone_M_MFSNODE	/* MFS vnode private part */
extern zone_t		zone_M_NQLEASE;
#define	M_NQLEASE	zone_M_NQLEASE	/* Nqnfs lease */
extern zone_t		zone_M_NQMHOST;
#define	M_NQMHOST	zone_M_NQMHOST	/* Nqnfs host address table */
extern zone_t		zone_M_NETADDR;
#define	M_NETADDR	zone_M_NETADDR	/* Export host address structure */
extern zone_t		zone_M_NFSSVC;
#define	M_NFSSVC	zone_M_NFSSVC	/* Nfs server structure */
extern zone_t		zone_M_NFSUID;
#define	M_NFSUID	zone_M_NFSUID	/* Nfs uid mapping structure */
extern zone_t		zone_M_NFSD;
#define	M_NFSD		zone_M_NFSD	/* Nfs server daemon structure */
extern zone_t		zone_M_IPMOPTS;
#define	M_IPMOPTS	zone_M_IPMOPTS	/* internet multicast options */
extern zone_t		zone_M_IPMADDR;
#define	M_IPMADDR	zone_M_IPMADDR	/* internet multicast address */
extern zone_t		zone_M_IFMADDR;
#define	M_IFMADDR	zone_M_IFMADDR	/* link-level multicast address */
extern zone_t		zone_M_MRTABLE;
#define	M_MRTABLE	zone_M_MRTABLE	/* multicast routing tables */
extern zone_t		zone_M_ISOFSMNT;
#define	M_ISOFSMNT	zone_M_ISOFSMNT	/* ISOFS mount structure */
extern zone_t		zone_M_ISOFSNODE;
#define	M_ISOFSNODE	zone_M_ISOFSNODE	/* ISOFS vnode private part */
extern zone_t		zone_M_TEMP;
#define	M_TEMP		zone_M_TEMP	/* misc temporary data buffers */
extern zone_t		zone_M_LAST;
#define	M_LAST		zone_M_LAST	/* Must be last type + 1 */
#endif

#define	M_FREE		0	/* should be on free list */
#define	M_MBUF		1	/* mbuf */
#define	M_DEVBUF	2	/* device driver memory */
#define	M_SOCKET	3	/* socket structure */
#define	M_PCB		4	/* protocol control block */
#define	M_RTABLE	5	/* routing tables */
#define	M_HTABLE	6	/* IMP host tables */
#define	M_FTABLE	7	/* fragment reassembly header */
#define	M_ZOMBIE	8	/* zombie proc status */
#define	M_IFADDR	9	/* interface address */
#define	M_SOOPTS	10	/* socket options */
#define	M_SONAME	11	/* socket name */
#define	M_NAMEI		12	/* namei path name buffer */
#define	M_GPROF		13	/* kernel profiling buffer */
#define	M_IOCTLOPS	14	/* ioctl data buffer */
#define	M_MAPMEM	15	/* mapped memory descriptors */
#define	M_CRED		16	/* credentials */
#define	M_PGRP		17	/* process group header */
#define	M_SESSION	18	/* session header */
#define	M_IOV		19	/* large iov's */
#define	M_MOUNT		20	/* vfs mount struct */
#define	M_FHANDLE	21	/* network file handle */
#define	M_NFSREQ	22	/* NFS request header */
#define	M_NFSMNT	23	/* NFS mount structure */
#define	M_NFSNODE	24	/* NFS vnode private part */
#define	M_VNODE		25	/* Dynamically allocated vnodes */
#define	M_CACHE		26	/* Dynamically allocated cache entries */
#define	M_DQUOT		27	/* UFS quota entries */
#define	M_UFSMNT	28	/* UFS mount structure */
#define	M_SHM		29	/* SVID compatible shared memory segments */
#define	M_VMMAP		30	/* VM map structures */
#define	M_VMMAPENT	31	/* VM map entry structures */
#define	M_VMOBJ		32	/* VM object structure */
#define	M_VMOBJHASH	33	/* VM object hash structure */
#define	M_VMPMAP	34	/* VM pmap */
#define	M_VMPVENT	35	/* VM phys-virt mapping entry */
#define	M_VMPAGER	36	/* XXX: VM pager struct */
#define	M_VMPGDATA	37	/* XXX: VM pager private data */
#define	M_FILE		38	/* Open file structure */
#define	M_FILEDESC	39	/* Open file descriptor table */
#define	M_LOCKF		40	/* Byte-range locking structures */
#define	M_PROC		41	/* Proc structures */
#define	M_SUBPROC	42	/* Proc sub-structures */
#define	M_SEGMENT	43	/* Segment for LFS */
#define	M_LFSNODE	44	/* LFS vnode private part */
#define	M_FFSNODE	45	/* FFS vnode private part */
#define	M_MFSNODE	46	/* MFS vnode private part */
#define	M_NQLEASE	47	/* Nqnfs lease */
#define	M_NQMHOST	48	/* Nqnfs host address table */
#define	M_NETADDR	49	/* Export host address structure */
#define	M_NFSSVC	50	/* Nfs server structure */
#define	M_NFSUID	51	/* Nfs uid mapping structure */
#define	M_NFSD		52	/* Nfs server daemon structure */
#define	M_IPMOPTS	53	/* internet multicast options */
#define	M_IPMADDR	54	/* internet multicast address */
#define	M_IFMADDR	55	/* link-level multicast address */
#define	M_MRTABLE	56	/* multicast routing tables */
#define M_ISOFSMNT	57	/* ISOFS mount structure */
#define M_ISOFSNODE	58	/* ISOFS vnode private part */
#define	M_TEMP		74	/* misc temporary data buffers */
#define	M_LAST		75	/* Must be last type + 1 */

#if 0
#define	MALLOC(space, cast, size, type, flags) \
	(space) = (cast)zalloc(type)
#define FREE(addr, type) zfree(type, (caddr_t)(addr))
#else
#define	MALLOC(space, cast, size, type, flags) \
	(space) = (cast)malloc(size)
#define FREE(addr, type) free((caddr_t)(addr))

#define	bsd_malloc(size, type, flags) malloc(size)
#define bsd_free(addr, type) free((void *)(addr))
#endif
#endif /* !_SYS_MALLOC_H_ */
