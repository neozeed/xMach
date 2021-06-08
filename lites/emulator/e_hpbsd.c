/* 
 * Copyright (c) 1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: e_hpbsd.c 1.4 95/03/03$
 */

/*
 * Utah 4.3/4.4 BSD hybrid specific system call handling stubs.
 */

#include <e_defs.h>
#include <sys/dirent.h>

#ifndef PT_TRACE_ME
#define PT_TRACE_ME 0
#endif

/*
 * BSD version expects status to be returned in ret1.
 * Also, pid is only a short (ala BNR).
 */
errno_t e_hpbsd_wait4(
	short		pid,
	int		*status,
	int		options,
	struct rusage	*rusage,
	int		*rvals)
{
	errno_t err;

	err = e_lite_wait4((pid_t) pid, status, options, rusage, rvals);
	if (!err) {
		rvals[0] &= 0xffff;
		rvals[1] = status ? *status : 0;
	}

	return err;
}

/*
 * HP-UX (and BSD) version have an extra argument.
 */
errno_t e_hpbsd_ptrace(int request, pid_t pid, int *addr, int data, int *addr2)
{
	errno_t err;
	kern_return_t kr = 0;

	if (request != PT_TRACE_ME)
	    return EINVAL;

#ifdef notyet
	kr = bsd_set_trace(process_self(), TRUE);
#endif
	return kr ? e_mach_error_to_errno(kr) : 0;
}

/*
 * Nearly the same as setpgid:
 *	arguments are ints
 *	pgrp of -1 is the same as getpgrp
 */
errno_t e_hpbsd_setpgrp(int pid, int pgrp, int *rpgrp)
{
	errno_t err;
	kern_return_t kr;
	int rv[2];

	struct {
		int pid;	/* XXX pid_t but sign extend */
		int pgrp;	/* XXX pid_t but sign extend */
	} a;

	if (pgrp < 0) {
		kr = emul_generic(process_self(), SYS_getpgrp, &a, &rv);
		if (rpgrp && kr == 0)
			*rpgrp = rv[0];
	} else {
		a.pgrp = pgrp;
		a.pid = pid;
		kr = emul_generic(process_self(), SYS_setpgid, &a, &rv);
	}
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/*
 * Return self and child times in 60ths of a second.
 */
errno_t e_hpbsd_times(int *timesbuf)
{
	return ENOSYS;
}

/*
 * Enable/disable profiling
 */
errno_t e_hpbsd_profil(char *samples, int size, int offset, int scale)
{
	return ENOSYS;
}

/*
 * Enable/disable kernel tracing
 */
errno_t e_hpbsd_ktrace(char *tracefile, int ops, int trpoints, int pid)
{
	return ENOSYS;
}

/*
 * Old style signal handling
 */
errno_t e_hpbsd_ssig(int sig, void (*func)())
{
	return ENOSYS;
}

/* 
 * 4.3 style getpid/getuid/getgid syscalls.
 *
 * XXX not HPBSD specific, should be elsewhere. e_bnr_stubs.c?
 */
errno_t e_hpbsd_getpid(int *rv)
{
	errno_t err;
	kern_return_t kr;

	struct {
		int dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getpid, &a, rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_hpbsd_getuid(int *rv)
{
	errno_t err;
	kern_return_t kr;

	struct {
		int dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getuid, &a, rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_hpbsd_getgid(int *rv)
{
	errno_t err;
	kern_return_t kr;

	struct {
		int dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getgid, &a, rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/*
 * New style nfssvc call.  Should be the standard.
 */
errno_t e_hpbsd_nfssvc(int flags, void *argstructp)
{
	errno_t err;
	kern_return_t kr;
	int rv[2];

	struct {
		int flag;
		char *argp;
	} a;

	a.flag = flags;
	a.argp = argstructp;
	kr = emul_generic(process_self(), SYS_nfssvc, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/*
 * Several unimplemented syscalls do nothing but return no error
 */
errno_t e_hpbsd_noerror()
{
	return 0;
}

/*
 * Mount syscall.
 *
 * Utah BSD has 16-bit uid_t and gid_t and a slightly different ucred
 * structure, hence the export_args structure is a different size.
 * This causes grief for all the various mount types.
 */
errno_t e_hpbsd_mount(int type, const char *dir, int flags, void *data)
{
	short *fp, *tp;
	short mount_args_buf[128];	/* XXX */
	int i;

	switch (type) {
	case MOUNT_UFS:
	case MOUNT_MFS:
	case MOUNT_CD9660:
		break;
	case MOUNT_NFS:
	default:
		return e_mount(type, dir, flags, data);
	}

	fp = (short *) data;
	tp = mount_args_buf;

	/* fspec */
	*tp++ = *fp++; *tp++ = *fp++;
	/* ex_flags */
	*tp++ = *fp++; *tp++ = *fp++;
	/* ex_root */
	*tp++ = 0; *tp++ = *fp++;
	/* ex_cred.cr_ref */
	*tp++ = *fp++; *tp++ = 0;
	/* ex_cred.cr_uid */
	*tp++ = 0; *tp++ = *fp++;
	/* ex_cred.cr_ngroups */
	*tp++ = *fp++; *tp++ = 0;
	/* ex_cred.cr_groups */
	for (i = 0; i < NGROUPS; i++) {
		*tp++ = 0; *tp++ = *fp++;
	}
	/* skip HPBSD cr_ruid, cr_rgid */
	fp += 2;
	/* ex_addr */
	*tp++ = *fp++; *tp++ = *fp++;
	/* ex_addrlen */
	*tp++ = *fp++; *tp++ = *fp++;
	/* ex_mask */
	*tp++ = *fp++; *tp++ = *fp++;
	/* ex_masklen */
	*tp++ = *fp++; *tp++ = *fp++;
	switch (type) {
	case MOUNT_MFS:
		/* base */
		*tp++ = *fp++; *tp++ = *fp++;
		/* size */
		*tp++ = *fp++; *tp++ = *fp++;
		break;
	case MOUNT_CD9660:
		/* flags */
		*tp++ = *fp++; *tp++ = *fp++;
		break;
	default:
		break;
	}
	return e_mount(type, dir, flags, (void *) mount_args_buf);
}
