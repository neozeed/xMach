/* 
 * Mach Operating System
 * Copyright (c) 1994 Johannes Helander
 * Copyright (c) 1994 Timo Rinne
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * JOHANNES HELANDER AND TIMO RINNE ALLOW FREE USE OF THIS SOFTWARE IN
 * ITS "AS IS" CONDITION.  JOHANNES HELANDER AND TIMO RINNE DISCLAIM ANY
 * LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
 * USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: e_sysv.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:31  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	 emulator/e_sysv_stubs.c
 *	Authors:
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *	Timo Rinne, Helsinki University of Technology, 1994.
 *	Date:	 February 1994
 *
 *	System V system call handler functions.
 */

#include <e_defs.h>
#include <e_templates.h>

DEF_STAT(sysv)
DEF_LSTAT(sysv)
DEF_FSTAT(sysv)

/*
 * ISC statfs struct is similar to sysv traditional one.
 */
struct sysv_statfs {
    short   f_fstyp;    /* File system type */
    short   f_bsize;    /* Block size */
    short   f_frsize;   /* Fragment size */
    long    f_blocks;   /* Total number of blocks */
    long    f_bfree;    /* Count of free blocks */
    long    f_files;    /* Total number of file nodes */
    long    f_ffree;    /* Count of free file nodes */
    char    f_fname[6]; /* Volume name */
    char    f_fpack[6]; /* Pack name */
};

static void bsd_statfs_2_sysv_statfs(struct statfs *bsdbuf,
				     struct sysv_statfs *sysvbuf)
{
    sysvbuf->f_fstyp = bsdbuf->f_type;
    sysvbuf->f_bsize = bsdbuf->f_bsize;
    sysvbuf->f_frsize = -1; /* Undefined */
    sysvbuf-> f_blocks = bsdbuf->f_blocks;
    sysvbuf->f_bfree = bsdbuf->f_bavail;
    sysvbuf->f_files = bsdbuf->f_files;
    sysvbuf->f_ffree = bsdbuf->f_ffree;
    /* Beware.  BSD carries 32 bytes XXX */
    strncpy(sysvbuf->f_fname, bsdbuf->f_mntonname, 6);
    strncpy(sysvbuf->f_fpack, bsdbuf->f_mntfromname, 6);
    return;
}

errno_t e_sysv_statfs(const char *path, struct sysv_statfs *buf)
{
	errno_t err;
	kern_return_t kr;
	int rv[2];
	struct statfs bsdbuf;

	struct {
		const char *path;
		struct statfs *buf;
	} a;
	a.path = path;
	a.buf = &bsdbuf;

	kr = emul_generic(our_bsd_server_port, SYS_statfs, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if(!err)
	    bsd_statfs_2_sysv_statfs(&bsdbuf, buf);
	return err;
}

errno_t e_sysv_fstatfs(int fd, struct sysv_statfs *buf)
{
	errno_t err;
	kern_return_t kr;
	int rv[2];
	struct statfs bsdbuf;

	struct {
		int fd;
		struct statfs *buf;
	} a;
	a.fd = fd;
	a.buf = &bsdbuf;

	kr = emul_generic(our_bsd_server_port, SYS_fstatfs, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if(!err)
	    bsd_statfs_2_sysv_statfs(&bsdbuf, buf);
	return err;
}

/*
 * SYSV has nice interface.  In BSD nice is a library function.
 */
errno_t e_sysv_nice(int inc)
{
        int pri;
	errno_t err;
	
        err = e_getpriority(PRIO_PROCESS, 0, &pri);
        if(err)
	    return(err);
        return(e_setpriority(PRIO_PROCESS, 0, pri + inc));
}

errno_t e_isc4_sysi86(int cmd, long longarg)
{
/*
    int intarg = (int)longarg;
    char *strarg = (char *)longarg;

    switch(CMD) {
    };
*/
    return ENOSYS;
}

errno_t e_sysv_stime(time_t *t)
{
    return EPERM; /* We don't allow ISC to modify system colck. XXX */
}

/*
 * Modified from NetBSD library function alarm(3).
 */
errno_t e_sysv_alarm(unsigned int secs, 
		     unsigned int *ret)
{
    struct itimerval it, oitv;
    struct itimerval *itp = &it;
    errno_t err;

    itp->it_interval.tv_sec = itp->it_interval.tv_usec = 0; 
    itp->it_value.tv_sec = secs;
    itp->it_value.tv_usec = 0;
    if ((err = e_setitimer(ITIMER_REAL, itp, &oitv)) != ESUCCESS) {
	*ret = ((unsigned int)0xffffffff);
	return err;
    }
    if(oitv.it_value.tv_usec)
	oitv.it_value.tv_sec++;
    *ret = oitv.it_value.tv_sec;
    return ESUCCESS;
}

/*
 * Modified from NetBSD library function signal(3).
 */

sigset_t e_sysv_sigintr;

static inline errno_t e_sysv_sigemptyset(sigset_t *set)
{
        *set = 0;
        return ESUCCESS;
}

static inline errno_t e_sysv_sigismember(const sigset_t *set, int signo, int *ret)
{
        if (signo <= 0 || signo >= NSIG)
                return EINVAL;
	*ret = ((*set & sigmask(signo)) != 0);
	return ESUCCESS;
}

errno_t e_sysv_signal(int s, sig_t a, sig_t *ret)
{
    struct sigaction sa, osa;
    errno_t err;
    int r;

    sa.sa_handler = a;
    e_sysv_sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if ((err = e_sysv_sigismember(&e_sysv_sigintr, s, &r)) != ESUCCESS) {
	*ret = ((sig_t)0xffffffff);
	return EINVAL;
    }
    if(r == 0)
	sa.sa_flags |= SA_RESTART;
    if ((err = e_sigaction(s, &sa, &osa)) != ESUCCESS) {
	*ret = ((sig_t)0xffffffff);
	return err;
    }
    *ret = osa.sa_handler;
    return ESUCCESS;
}

errno_t e_sysv_time(time_t *t, time_t *retval)
{

	struct timeval tv;
	errno_t err;

	err = e_mapped_gettimeofday(&tv, 0);
	if (err)
	    return err;
	if (t)
	    *t = tv.tv_sec;
	if (retval)
	    *retval = tv.tv_sec;
	return ESUCCESS;
}
