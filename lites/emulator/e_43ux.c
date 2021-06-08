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
 * $Log: e_43ux.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:31  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/e_43ux.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	System call handler functions for backwards compatibility with
 *	4.3 BSD and CMU UX.
 */

#include <e_defs.h>

#ifdef COMPAT_43

errno_t e_cmu_43ux_mmap(
	caddr_t		addr,
	size_t		len,
	int		prot,
	int		flags,
	int		fd,
	bnr_off_t	offset,
	caddr_t		*addr_out)
{
	errno_t err;
	int newprot = 0;
	int newflags = 0;

/* UX values */
#define CMU_43UX_MAP_PRIVATE	2
#define CMU_43UX_MAP_SHARED	1

#define CMU_43UX_PROT_READ	1
#define CMU_43UX_PROT_WRITE	2
#define CMU_43UX_PROT_EXEC	4

	/* Fixup parameters */
	if (prot & CMU_43UX_PROT_READ)
	    newprot = PROT_READ;
	if (prot & CMU_43UX_PROT_WRITE)
	    newprot |= PROT_WRITE;
	if (prot & CMU_43UX_PROT_EXEC)
	    newprot |= PROT_EXEC;

	if (flags & CMU_43UX_MAP_SHARED)
	    newflags = MAP_SHARED;
	if (flags & CMU_43UX_MAP_PRIVATE)
	    newflags = MAP_PRIVATE;
	newflags |= MAP_FIXED; /* True for X server at least */

	return e_mmap(addr, len, newprot, newflags, fd, offset, addr_out);
}

int
e_creat(char *fname, int fmode, int *rval)
{
	return e_open(fname,
		      O_WRONLY | O_CREAT | O_TRUNC,
		      fmode,
		      rval);
}

errno_t e_quota(
	int 		a1,
	int		a2,
	int		a3,
	int		a4,
	int 		*rval)
{
	return ENOSYS;
}


errno_t e_vread()
{
	return ENOSYS;
}

errno_t e_vwrite()
{
	return ENOSYS;
}

errno_t e_vhangup()
{
	return ENOSYS;
}

errno_t e_vlimit()
{
	return ENOSYS;
}

errno_t e_getdopt()
{
	return ENOSYS;
}

errno_t e_setdopt()
{
	return ENOSYS;
}

errno_t e_vtimes()
{
	return ENOSYS;
}

errno_t e_sigsetmask(sigset_t mask, sigset_t *oset)
{
	return e_sigprocmask(SIG_SETMASK, &mask, oset);
}

/* 
 * The sigsuspend manual page is misleading. The binary interface is
 * not the same!
 */
errno_t e_sigpause(int mask)
{
	sigset_t sigmask = mask;
	return e_sigsuspend(sigmask);
}

errno_t e_setreuid(uid_t ruid, uid_t euid)
{
	errno_t err;
	kern_return_t kr;
	int rv[2];

	struct {
		int ruid;
		int euid;
	} a;
	a.ruid = ruid;
	a.euid = euid;

	kr = emul_generic(our_bsd_server_port, SYS_setreuid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_setregid(gid_t rgid, gid_t egid)
{
	errno_t err;
	kern_return_t kr;
	int rv[2];

	struct {
		int rgid;
		int egid;
	} a;
	a.rgid = rgid;
	a.egid = egid;

	kr = emul_generic(our_bsd_server_port, SYS_setregid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_killpg()
{
	return ENOSYS;
}
#endif /* COMPAT_43 */
