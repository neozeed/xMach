/* 
 * Mach Operating System
 * Copyright (c) 1995 Johannes Helander
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
 * $Log: e_osf1.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:33  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	e_osf1.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1995.
 *
 *	Emulation for OSF/1 system calls on the Alpha.
 */

#include <e_defs.h>

/* OSF1 values */
#define	OSF1_MAP_FILE		0x000
#define	OSF1_MAP_SHARED		0x001
#define OSF1_MAP_PRIVATE	0x002
#define	OSF1_MAP_ANONYMOUS	0x010
#define OSF1_MAP_TYPE		0x0f0
#define	OSF1_MAP_FIXED		0x100
#define	OSF1_MAP_HASSEMAPHORE	0x200
#define	OSF1_MAP_INHERIT	0x400
#define	OSF1_MAP_UNALIGNED	0x800

#define OSF1_PROT_READ	1
#define OSF1_PROT_WRITE	2
#define OSF1_PROT_EXEC	4

errno_t e_osf1_mmap(
	caddr_t		addr,
	size_t		len,
	int		prot,
	int		flags,
	int		fd,
	off_t		offset,
	caddr_t		*addr_out)
{
	int newprot = 0;
	int newflags = 0;

	/* Fixup parameters */
	if (prot & OSF1_PROT_READ) {
		newprot = PROT_READ;
	}
	if (prot & OSF1_PROT_WRITE) {
		newprot |= PROT_WRITE;
	}
	if (prot & OSF1_PROT_EXEC) {
		newprot |= PROT_EXEC;
	}

	if (flags & OSF1_MAP_SHARED)
		newflags |= MAP_SHARED;

	if (flags & OSF1_MAP_PRIVATE)
		newflags |= MAP_PRIVATE;

	if (flags & OSF1_MAP_ANONYMOUS)
		newflags |= MAP_ANON;

	switch (flags & OSF1_MAP_TYPE) {
	case OSF1_MAP_ANONYMOUS:
	  	newflags |= MAP_ANON;
		break;
	case OSF1_MAP_FILE:
		/* not MAP_ANON */
		break;
	default:
		return e_kernel_error_to_lites_error(LITES_EINVAL);
	}

	if (flags & OSF1_MAP_FIXED)
		newflags |= MAP_FIXED;

	if (flags & OSF1_MAP_HASSEMAPHORE)
		newflags |= MAP_HASSEMAPHORE;

	if (flags & OSF1_MAP_INHERIT)
		newflags |= MAP_INHERIT;

	if (flags & OSF1_MAP_UNALIGNED)
		return e_kernel_error_to_lites_error(LITES_EINVAL);

	return e_mmap(addr, len, newprot, newflags, fd, offset, addr_out);
}

#include <e_templates.h>
DEF_STAT(osf1)
DEF_LSTAT(osf1)
DEF_FSTAT(osf1)

errno_t e_64_pipe(integer_t *fds)
{
	int fd[2];
	errno_t err;

	err = e_pipe(fd);
	if (err)
	  return err;
	fds[0] = fd[0];
	fds[1] = fd[1];
	return ESUCCESS;
}

errno_t e_osf1_getuid(natural_t uids[2])
{
	errno_t err;

	uids[0] = 0;
	err = e_getuid((uid_t *) &uids[0]);
	if (err)
	  return err;

	uids[1] = 0;
	err = e_geteuid((uid_t *) &uids[1]);
	return err;
}

errno_t e_osf1_getgid(natural_t gids[2])
{
	errno_t err;

	gids[0] = 0;
	err = e_getgid((gid_t *) &gids[0]);
	if (err)
	  return err;

	gids[1] = 0;
	err = e_getegid((gid_t *) &gids[1]);
	return err;
}

#define OSF1_RLIMIT_NOFILE 6
#define OSF1_RLIMIT_AS 7

struct osf1_rlimit {
	unsigned long	rlim_cur;
	unsigned long	rlim_max;
};

errno_t e_osf1_getrlimit(int resource, struct osf1_rlimit *rl)
{
	struct rlimit r;
	errno_t err;
	int method;

	switch (resource) {
	      case OSF1_RLIMIT_NOFILE:
		method = RLIMIT_NOFILE;
		break;
	      case OSF1_RLIMIT_AS:
		return e_kernel_error_to_lites_error(LITES_EOPNOTSUPP);
	      default:
		method = resource;
	}
		
	err = e_lite_getrlimit(method, rl);
	if (err)
	    return err;
	rl->rlim_cur = r.rlim_cur;
	rl->rlim_max = r.rlim_max;
	return ESUCCESS;
}

errno_t e_osf1_setrlimit(int resource, struct osf1_rlimit *rl)
{
	struct rlimit r;
	errno_t err;
	int method;

	switch (resource) {
	      case OSF1_RLIMIT_NOFILE:
		method = RLIMIT_NOFILE;
		break;
	      case OSF1_RLIMIT_AS:
		return e_kernel_error_to_lites_error(LITES_EOPNOTSUPP);
	      default:
		method = resource;
	}
		
	r.rlim_cur = rl->rlim_cur;
	r.rlim_max = rl->rlim_max;

	return e_lite_setrlimit(method, rl);
}
