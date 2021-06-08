/* 
 * Copyright (c) 1995 The University of Utah and
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
 * 	Utah $Hdr: e_hpux.c 1.1 95/09/01$
 */

/*
 * HP-UX specific system call handling stubs.
 *
 * For you file-surfers, the command:
 *	egrep '^static|^void|^int|^bool|^errno|^hpux|^\*\*' e_hpux.c
 * should tell you everything you need to know about this file.
 */

#include <e_defs.h>
#include <e_hpux.h>
#include <sys/wait.h>
#include <sys/sysctl.h>
#include <sys/termios.h>
#include <sys/filio.h>
#include <sys/ioctl_compat.h>

/*
** Support structures and routines.
*/

hpux_exsave_t	__hpux_exsave;		/* gets copied across exec() */

hpux_dev_t dev_bsdtohpux(dev_t dev)
{
	return (major(dev) << 24) | minor(dev);
}

static errno_t errno_bsdtohpux_map[BSD_NERR] = {
	/* ESUCCESS 0 */	  0,		/* EPERM 1 */		  1,
	/* ENOENT 2 */		  2,		/* ESRCH 3 */		  3,
	/* EINTR 4 */		  4,		/* EIO 5 */		  5,
	/* ENXIO 6 */		  6,		/* E2BIG 7 */		  7,
	/* ENOEXEC 8 */		  8,		/* EBADF 9 */		  9,
	/* ECHILD 10 */		 10,		/* EDEADLK 11 */	 45,
	/* ENOMEM 12 */		 12,		/* EACCES 13 */		 13,
	/* EFAULT 14 */		 14,		/* ENOTBLK 15 */	 15,
	/* EBUSY 16 */		 16,		/* EEXIST 17 */		 17,
	/* EXDEV 18 */		 18,		/* ENODEV 19 */		 19,
	/* ENOTDIR 20 */	 20,		/* EISDIR 21 */		 21,
	/* EINVAL 22 */		 22,		/* ENFILE 23 */		 23,
	/* EMFILE 24 */		 24,		/* ENOTTY 25 */		 25,
	/* ETXTBSY 26 */	 26,		/* EFBIG 27 */		 27,
	/* ENOSPC 28 */		 28,		/* ESPIPE 29 */		 29,
	/* EROFS 30 */		 30,		/* EMLINK 31 */		 31,
	/* EPIPE 32 */		 32,		/* EDOM 33 */		 33,
	/* ERANGE 34 */		 34,		/* EAGAIN 35 */		 11,
	/* EINPROGRESS 36 */	245,		/* EALREADY 37 */	244,
	/* ENOTSOCK 38 */	216,		/* EDESTADDRREQ 39 */	217,
	/* EMSGSIZE 40 */	218,		/* EPROTOTYPE 41 */	219,
	/* ENOPROTOOPT 42 */	220,		/* EPROTONOSUPPORT 43 */221,
	/* ESOCKTNOSUPPORT 44 */222,		/* EOPNOTSUPP 45 */	223,
	/* EPFNOSUPPORT 46 */	224,		/* EAFNOSUPPORT 47 */	225,
	/* EADDRINUSE 48 */	226,		/* EADDRNOTAVAIL 49 */	227,
	/* ENETDOWN 50 */	228,		/* ENETUNREACH 51 */	229,
	/* ENETRESET 52 */	230,		/* ECONNABORTED 53 */	231,
	/* ECONNRESET 54 */	232,		/* ENOBUFS 55 */	233,
	/* EISCONN 56 */	234,		/* ENOTCONN 57 */	235,
	/* ESHUTDOWN 58 */	236,		/* ETOOMANYREFS 59 */	237,
	/* ETIMEDOUT 60 */	238,		/* ECONNREFUSED 61 */	239,
	/* ELOOP 62 */		249,		/* ENAMETOOLONG 63 */	248,
	/* EHOSTDOWN 64 */	241,		/* EHOSTUNREACH 65 */	242,
	/* ENOTEMPTY 66 */	247,		/* EPROCLIM 67 */     HPUX_BERR,
	/* EUSERS 68 */		 68,		/* EDQUOT 69 */		69,
	/* ESTALE 70 */		 70,		/* EREMOTE 71 */	71,
	/* EBADRPC 72 */	HPUX_BERR,	/* ERPCMISMATCH 73 */ HPUX_BERR,
	/* EPROGUNAVAIL 74 */	HPUX_BERR,	/* EPROGMISMATCH 75 */HPUX_BERR,
	/* EPROCUNAVAIL 76 */	HPUX_BERR,	/* ENOLCK 77 */		46,
	/* ENOSYS 78 */		251,		/* EFTYPE 79 */	      HPUX_BERR,
	/* EAUTH 80 */		HPUX_BERR,	/* ENEEDAUTH 81 */    HPUX_BERR,	/* HPUX_OEAGAIN 82 */	11
};

errno_t errno_bsdtohpux(mach_error_t kr)
{
	if ((kr&(system_emask|sub_emask)) == unix_err(0) && kr <= LITES_ELAST) {
		unsigned int errcode = err_get_code(kr);
		if (errcode >= BSD_NERR)
			return HPUX_BERR;
		else
			return errno_bsdtohpux_map[errcode];
	}
	e_bad_mach_error(kr);
	/*NOTREACHED*/
	return 0;	/* avoid GCC warning */
}

static int rusage_bsdtohpux(struct rusage *ru, hpux_rusage_t *hpru)
{
	bcopy(ru, hpru, (int)((caddr_t)&ru->ru_oublock - (caddr_t)ru) + 4);
	hpru->ru_ioch = 0;
	bcopy(&ru->ru_msgsnd, &hpru->ru_msgsnd,
	      (int)((caddr_t)&ru->ru_nivcsw - (caddr_t)&ru->ru_msgsnd) + 4);
}

static int signo_bsdtohpux_map[NSIG] = {
	/*       0 */	 0,		/* HUP   1 */	 1,
	/* INT   2 */	 2,		/* QUIT  3 */	 3,
	/* ILL   4 */	 4,		/* TRAP  5 */	 5,
	/* ABRT  6 */	 6,		/* EMT   7 */	 7,
	/* FPE   8 */	 8,		/* KILL  9 */	 9,
	/* BUS  10 */	10,		/* SEGV 11 */	11,
	/* SYS  12 */	12,		/* PIPE 13 */	13,
	/* ALRM 14 */	14,		/* TERM 15 */	15,
	/* URG  16 */	29,		/* STOP 17 */	24,
	/* TSTP 18 */	25,		/* CONT 19 */	26,
	/* CHLD 20 */	18,		/* TTIN 21 */	27,
	/* TTOU 22 */	28,		/* IO   23 */	22,
	/* XCPU 24 */	 0,		/* XFSZ 25 */	 0,
	/* VTALRM 26 */	20,		/* PROF 27 */	21,
	/* WINCH 28 */	23,		/* INFO 29 */	 0,
	/* USR1 30 */	16,		/* USR2 31 */	17
};

static int signo_hpuxtobsd_map[HPUX_NSIG] = {
	/*       0 */	0,		/* HUP   1 */	SIGHUP,
	/* INT   2 */	SIGINT,		/* QUIT  3 */	SIGQUIT,
	/* ILL   4 */	SIGILL,		/* TRAP  5 */	SIGTRAP,
	/* ABRT  6 */	SIGABRT,	/* EMT   7 */	SIGEMT,
	/* FPE   8 */	SIGFPE,		/* KILL  9 */	SIGKILL,
	/* BUS  10 */	SIGBUS,		/* SEGV 11 */	SIGSEGV,
	/* SYS  12 */	SIGSYS,		/* PIPE 13 */	SIGPIPE,
	/* ALRM 14 */	SIGALRM,	/* TERM 15 */	SIGTERM,
	/* USR1 16 */	SIGUSR1,	/* USR2 17 */	SIGUSR2,
	/* CHLD 18 */	SIGCHLD,	/* PWR  19 */	0,
	/* VTALRM 20 */	SIGVTALRM,	/* PROF 21 */	SIGPROF,
	/* IO   22 */	SIGIO,		/* WINCH 23 */	SIGWINCH,
	/* STOP 24 */	SIGSTOP,	/* TSTP 25 */	SIGTSTP,
	/* CONT 26 */	SIGCONT,	/* TTIN 27 */	SIGTTIN,
	/* TTOU 28 */	SIGTTOU,	/* URG  29 */	SIGURG,
	/* LOST 30 */	0,		/* RESV 31 */	0
};

int signo_bsdtohpux(int sig)
{
	return ((unsigned)sig >= NSIG)? 0: signo_bsdtohpux_map[sig];
}

static int signo_hpuxtobsd(int hpsig)
{
	return ((unsigned)hpsig >= HPUX_NSIG)? 0: signo_hpuxtobsd_map[hpsig];
}

static int sigmask_bsdtohpux(int mask)
{
	int nm = 0;
	int bit;

	for (bit = 0; bit < 32; bit++)
		if (mask & (1 << bit))
			nm |= hpux_sigmask(signo_bsdtohpux(bit + 1));
	return nm;
}

static int sigmask_hpuxtobsd(int hpmask)
{
	int nm = 0;
	int bit;

	for (bit = 0; bit < 32; bit++)
		if (hpmask & (1 << bit))
			nm |= sigmask(signo_hpuxtobsd(bit + 1));
	return nm;
}

static void sigset_bsdtohpux(const sigset_t *mask, hpux_sigset_t *hpmask)
{
	long om = *mask, nm = 0;
	int bit;

	for (bit = 0; bit < 32; bit++)
		if (om & (1 << bit))
			nm |= hpux_sigmask(signo_bsdtohpux(bit + 1));
	hpmask->sigset[0] = nm;
}

static void sigset_hpuxtobsd(const hpux_sigset_t *hpmask, sigset_t *mask)
{
	long om = hpmask->sigset[0], nm = 0;
	int bit;

	for (bit = 0; bit < 32; bit++)
		if (om & (1 << bit))
			nm |= sigmask(signo_hpuxtobsd(bit + 1));
	*mask = nm;
}

static int waitstatus_bsdtohpux(int stat)
{
	/*
	 * We must convert signal numbers in returned wait*() status.
	 * Fortunately, we can use BSD macros since structs are identical.
	 */
	return	WIFSTOPPED(stat)? W_STOPCODE(signo_bsdtohpux(WSTOPSIG(stat))):
		WIFSIGNALED(stat)? W_EXITCODE(WEXITSTATUS(stat),
			signo_bsdtohpux(WTERMSIG(stat))):
		stat;
}

void statfs_bsdtohpux(struct statfs *buf, hpux_statfs_t *hpbuf)
{
	bzero(hpbuf, sizeof(*hpbuf));
	hpbuf->f_bsize = buf->f_bsize;
	hpbuf->f_blocks = buf->f_blocks;
	hpbuf->f_bfree = buf->f_bfree;
	hpbuf->f_bavail = buf->f_bavail;
	hpbuf->f_files = buf->f_files;
	hpbuf->f_ffree = buf->f_ffree;
	hpbuf->f_fsid[0] = dev_bsdtohpux(buf->f_fsid.val[0]);
	hpbuf->f_fsid[1] =
		(buf->f_fsid.val[1] == MOUNT_UFS)? HPUX_MOUNT_UFS:
		(buf->f_fsid.val[1] == MOUNT_NFS)? HPUX_MOUNT_NFS:
		(buf->f_fsid.val[1] == MOUNT_CD9660)? HPUX_MOUNT_CDFS:
		(buf->f_fsid.val[1] == MOUNT_MSDOS)? HPUX_MOUNT_PC: 0;
}

/*
** Time and timing-related system calls.
*/

errno_t e_hpux_ftime(hpux_timeb_t *timeb)
{
	struct timeval tv;
	struct timezone tz;
	errno_t err;

	if ((err = e_mapped_gettimeofday(&tv, &tz)) != ESUCCESS)
		return err;

	timeb->time = tv.tv_sec;
	timeb->millitm = tv.tv_usec / 1000;
	timeb->timezone = tz.tz_minuteswest;
	timeb->dstflag = tz.tz_dsttime;
	return ESUCCESS;
}

errno_t e_hpux_stime(hpux_time_t time)
{
	struct timezone zero_tz = { 0, 0 };
	struct timeval tv = { time, 0 };

	return e_settimeofday(&tv, &zero_tz);
}

errno_t e_hpux_time(hpux_time_t *time, hpux_time_t *time_r)
{
	struct timeval tv;
	errno_t err;

	if ((err = e_mapped_gettimeofday(&tv, 0)) == ESUCCESS) {
		if (time)
			*time = tv.tv_sec;
		*time_r = tv.tv_sec;
	}
	return err;
}

errno_t e_hpux_times(hpux_tms_t *tmsb, int *rv_p)
{
	errno_t err = ESUCCESS;
	struct rusage ru, cldru;
	struct timeval time, boottime;
	int mib[2], len;

#define	HPUXSCALE(t)	(t.tv_sec * HPUX_HZ + t.tv_usec * HPUX_HZ / 1000000)
	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	len = sizeof(boottime);
	if ((err = e_getrusage(RUSAGE_SELF, &ru)) == ESUCCESS &&
	    (err = e_getrusage(RUSAGE_CHILDREN, &cldru)) == ESUCCESS &&
	    (err = e_sysctl(mib,2,&boottime,&len,NULL,0,NULL)) == ESUCCESS &&
	    (err = e_mapped_gettimeofday(&time, 0)) == ESUCCESS) {
		tmsb->tms_utime = HPUXSCALE(ru.ru_utime);
		tmsb->tms_stime = HPUXSCALE(ru.ru_stime);
		tmsb->tms_cutime = HPUXSCALE(cldru.ru_utime);
		tmsb->tms_cstime = HPUXSCALE(cldru.ru_stime);
		rv_p[1] = HPUXSCALE(time) - HPUXSCALE(boottime);
	}
#undef	HPUXSCALE

	return err;
}

errno_t e_hpux_utime(const char *file, const hpux_utimbuf_t *utim)
{
	struct timeval tv[2];

	tv[0].tv_sec = utim->actime;
	tv[1].tv_sec = utim->modtime;
	tv[0].tv_usec = tv[1].tv_usec = 0;

	return e_utimes(file, tv);
}

/*
** Signal-related system calls.
*/

boolean_t __hpux_resethand[NSIG];
void (*__hpux_sigret)();
int __hpux_sigctx = HPUX_PA83_CONTEXT;

errno_t e_hpux_kill(hpux_pid_t pid, int hpsig)
{
	if (pid == HPUX_KILL_ALL_OTHERS) {
		e_emulator_error("HP-UX kill(KILL_ALL_OTHERS): not supported");
		return EINVAL;
	}

	return e_kill((pid_t)pid, signo_hpuxtobsd(hpsig));
}

errno_t e_hpux_sigaction(
	int sig,
	const hpux_sigaction_t *hpact,
	hpux_sigaction_t *ohpact)
{
	struct sigaction act, oact;
	boolean_t oresethand, blocked = FALSE;
	sigset_t mask, omask;
	errno_t err;

	sig = signo_hpuxtobsd(sig);
	oresethand = __hpux_resethand[sig];

	if (hpact) {
		act.sa_handler = hpact->sa_handler;
		sigset_hpuxtobsd(&hpact->sa_mask, &act.sa_mask);
		act.sa_flags = 0;
		if (hpact->sa_flags & HPUX_SA_ONSTACK) {
			e_emulator_error("HP-UX sigaction: onstack ignored");
			act.sa_flags |= SA_ONSTACK;
		}
		if (hpact->sa_flags & HPUX_SA_NOCLDSTOP)
			act.sa_flags |= SA_NOCLDSTOP;
		/*
		 * Emulate (or clear) HPUX_SA_RESETHAND signal semantics.
		 * We need to block the current signal to avoid a race
		 * in case e_sigaction() fails.
		 */
		mask = sigmask(sig);
		if (e_sigprocmask(SIG_BLOCK, &mask, &omask) == ESUCCESS)
			blocked = TRUE;
		if (hpact->sa_flags & HPUX_SA_RESETHAND)
			__hpux_resethand[sig] = TRUE;
		else
			__hpux_resethand[sig] = FALSE;
	}

	err = e_sigaction(sig, (hpact? &act: (struct sigaction *) 0),
	                  (ohpact? &oact: (struct sigaction *) 0));

	if (err)
		__hpux_resethand[sig] = oresethand;

	if (blocked)
		(void) e_sigprocmask(SIG_SETMASK, &omask, NULL);

	if (!err && ohpact) {
		ohpact->sa_handler = oact.sa_handler;
		sigset_bsdtohpux(&oact.sa_mask, &ohpact->sa_mask);
		if (oact.sa_flags & SA_ONSTACK)
			ohpact->sa_flags |= HPUX_SA_ONSTACK;
		if (oact.sa_flags & SA_NOCLDSTOP)
			ohpact->sa_flags |= HPUX_SA_NOCLDSTOP;
		if (__hpux_resethand[sig])
			ohpact->sa_flags |= HPUX_SA_RESETHAND;
	}

	return err;
}

errno_t e_hpux_sigblock(long hpmask, long *omask_r)
{
	errno_t err;
	long mask;

	if ((err = e_sigblock(sigmask_hpuxtobsd(hpmask), (int *)&mask)) ==
	    ESUCCESS)
		*omask_r = sigmask_bsdtohpux(mask);
	return err;
}

errno_t e_hpux_sigpause(long hpmask)
{
	sigset_t mask = sigmask_hpuxtobsd(hpmask);

	return e_sigsuspend(mask);
}

errno_t e_hpux_sigpending(hpux_sigset_t *hpset)
{
	errno_t err;
	sigset_t set;

	if ((err = e_sigpending(&set)) == ESUCCESS)
		sigset_bsdtohpux(&set, hpset);
	return err;
}

errno_t e_hpux_sigprocmask(
	int how,
	const hpux_sigset_t *hpset,
	hpux_sigset_t *ohpset)
{
	errno_t err;
	sigset_t set, oset;

	sigset_hpuxtobsd(hpset, &set);
	if ((err = e_sigprocmask(how, &set, &oset)) == ESUCCESS)
		sigset_bsdtohpux(&oset, ohpset);
	return err;
}

errno_t e_hpux_sigsetmask(long hpmask, long *omask_r)
{
	errno_t err;
	long mask;

	if ((err = e_sigsetmask(sigmask_hpuxtobsd(hpmask), (int *)&mask)) ==
	    ESUCCESS)
		*omask_r = sigmask_bsdtohpux(mask);
	return err;
}

errno_t e_hpux_sigsetreturn(void (*handler)(), int magic, int type)
{
	__hpux_sigret = handler;
	if (magic == 0x06211988)
		__hpux_sigctx = HPUX_PA89_CONTEXT;
	return ESUCCESS;
}

errno_t e_hpux_sigsetstatemask(int onstack, int hpmask)
{
	sigset_t mask, omask;

	mask = (sigset_t)sigmask_hpuxtobsd(hpmask);
	e_sigprocmask(SIG_SETMASK, &mask, &omask);

	if (onstack)
		e_emulator_error("HP-UX sigsetstatemask: onstack ignored");
	return ESUCCESS;
}

errno_t e_hpux_sigstack(const struct sigstack *ss, struct sigstack *oss)
{
	e_emulator_error("HP-UX sigstack: onstack ignored");
	return e_sigstack(ss, oss);
}

errno_t e_hpux_sigvec(int sig, const struct sigvec *nhpsv, struct sigvec *ohpsv)
{
	struct sigaction nsa, osa;
	boolean_t oresethand, blocked = FALSE;
	sigset_t mask, omask;
	errno_t err;

	sig = signo_hpuxtobsd(sig);
	oresethand = __hpux_resethand[sig];

	if (nhpsv) {
		nsa.sa_handler = nhpsv->sv_handler;
		nsa.sa_mask = sigmask_hpuxtobsd(nhpsv->sv_mask);
		nsa.sa_flags = 0;

		if (nhpsv->sv_flags & HPUX_SV_ONSTACK) {
			e_emulator_error("HP-UX sigvec: onstack ignored");
			nsa.sa_flags |= SA_ONSTACK;
		}
		if ((nhpsv->sv_flags & HPUX_SV_BSDSIG) == 0)
			nsa.sa_flags |= SA_NOCLDSTOP;
		/*
		 * Emulate (or clear) HPUX_SV_RESETHAND signal semantics.
		 * We need to block the current signal to avoid a race
		 * in case e_sigaction() fails.
		 */
		mask = sigmask(sig);
		if (e_sigprocmask(SIG_BLOCK, &mask, &omask) == ESUCCESS)
			blocked = TRUE;
		if (nhpsv->sv_flags & HPUX_SV_RESETHAND)
			__hpux_resethand[sig] = TRUE;
		else
			__hpux_resethand[sig] = FALSE;
	}
	err = e_sigaction(sig, (nhpsv? &nsa: (struct sigaction *) 0),
	                  (ohpsv? &osa: (struct sigaction *) 0));

	if (err)
		__hpux_resethand[sig] = oresethand;

	if (blocked)
		(void) e_sigprocmask(SIG_SETMASK, &omask, NULL);

	if (!err && ohpsv) {
		ohpsv->sv_handler = osa.sa_handler;
		ohpsv->sv_mask = sigmask_bsdtohpux(osa.sa_mask);
		ohpsv->sv_flags = 0;

		if (osa.sa_flags & SA_ONSTACK)
			ohpsv->sv_flags |= HPUX_SV_ONSTACK;
		if ((osa.sa_flags & SA_NOCLDSTOP) == 0)
			ohpsv->sv_flags |= HPUX_SV_BSDSIG;
		if (__hpux_resethand[sig])
			ohpsv->sv_flags |= HPUX_SV_RESETHAND;
	}

	return err;
}

errno_t e_hpux_suspend(const hpux_sigset_t *hpset)
{
	errno_t err;
	sigset_t set;

	sigset_hpuxtobsd(hpset, &set);
	return e_sigsuspend(set);
}

errno_t e_hpux_wait(int *hpstatus, int *rv_p)
{
	errno_t err;
	int status;
	pid_t pid;

	if ((err = e_lite_wait4(WAIT_ANY, &status, 0, 0, &pid)) == ESUCCESS) {
		rv_p[0] = (int)pid;
		rv_p[1] = (int)waitstatus_bsdtohpux(status);
		if (hpstatus)
			*hpstatus = rv_p[1];
	}
	return err;
}

errno_t e_hpux_wait3(int *hpstatus, int options, int *resv, hpux_pid_t *pid_r)
{
	errno_t err;
	int status;
	pid_t pid;

	if ((err = e_lite_wait4(WAIT_ANY, &status, options, resv, &pid)) ==
	    ESUCCESS) {
		*pid_r = pid;
		if (hpstatus)
			*hpstatus = waitstatus_bsdtohpux(status);
	}
	return err;
}

errno_t e_hpux_waitpid(
	hpux_pid_t hppid,
	int *hpstatus,
	int options,
	hpux_pid_t *pid_r)
{
	errno_t err;
	int status;
	pid_t pid;

	if ((err = e_lite_wait4(hppid, &status, options, 0, &pid)) == ESUCCESS){
		*pid_r = pid;
		if (hpstatus)
			*hpstatus = waitstatus_bsdtohpux(status);
	}
	return err;
}

/*
** System calls operating on files and file descriptors.
*/

#include <e_templates.h>
DEF_STAT(hpux)
DEF_LSTAT(hpux)
DEF_FSTAT(hpux)

errno_t e_hpux_dup2(int fd, int nfd)
{
	errno_t err = e_dup2(fd, nfd);

	if (err == ESUCCESS) {
		if ((u_int)nfd >= HPUX_NOFILE) {
			e_close(nfd);
			err = EMFILE;
		} else
			__hpux_fdflags[nfd] = __hpux_fdflags[fd];
	}

	return err;
}

errno_t e_hpux_dup(int fd, int *nfd_r)
{
	errno_t err = e_dup(fd, nfd_r);

	if (err == ESUCCESS) {
		if ((u_int)*nfd_r >= HPUX_NOFILE) {
			e_close(*nfd_r);
			err = EMFILE;
		} else
			__hpux_fdflags[*nfd_r] = __hpux_fdflags[fd];
	}

	return err;
}

errno_t e_hpux_fcntl(int fd, int cmd, int arg, int *res_r)
{
	hpux_flock_t *hpfl = (hpux_flock_t *)arg;
	unsigned int *pop;
	struct flock fl;
	errno_t err;

	if ((u_int)fd >= HPUX_NOFILE)
		return EBADF;
	else
		pop = &__hpux_fdflags[fd];

	switch (cmd) {
	    case F_DUPFD:
	    case F_GETFD:
	    case F_SETFD:
	    case F_GETFL:
		break;

	    case F_SETFL:
		if (arg & HPUX_FF_NONBLOCK)
			*pop |= HPUX_FF_NONBLOCK_ON;
		else
			*pop &= ~HPUX_FF_NONBLOCK_ON;
		if (arg & HPUX_FF_NDELAY)
			*pop |= HPUX_FF_NDELAY;
		else
			*pop &= ~HPUX_FF_NDELAY;
		if (*pop & (HPUX_FF_NONBLOCK_ON | HPUX_FF_FNDELAY_ON |
		            HPUX_FF_FIONBIO_ON) )
			arg |= FNDELAY;
		else
			arg &= ~FNDELAY;
		arg &= ~(HPUX_FF_NONBLOCK|HPUX_FF_FSYNCIO|HPUX_FF_FREMOTE);
		break;

	    case HPUX_F_GETLK:
	    case HPUX_F_SETLK:
	    case HPUX_F_SETLKW:
		fl.l_start = hpfl->hl_start;
		fl.l_len = hpfl->hl_len;
		fl.l_pid = hpfl->hl_pid;
		fl.l_type = hpfl->hl_type;
		fl.l_whence = hpfl->hl_whence;
		arg = (int)&fl;
		cmd = (cmd == HPUX_F_SETLKW)? F_SETLKW:
		      (cmd == HPUX_F_GETLK)? F_GETLK: F_SETLK;
		break;

	    default:
		return EINVAL;
	}
	err = e_fcntl(fd, cmd, arg, res_r);

	if (err == ESUCCESS) {
		switch (cmd) {
		    case F_DUPFD:
			if ((u_int)*res_r >= HPUX_NOFILE) {
				e_close(*res_r);
				err = EMFILE;
			} else
				__hpux_fdflags[*res_r] = __hpux_fdflags[fd];
			break;

		    case F_GETLK:
			hpfl->hl_start = fl.l_start;
			hpfl->hl_len = fl.l_len;
			hpfl->hl_pid = fl.l_pid;
			hpfl->hl_type = fl.l_type;
			hpfl->hl_whence = fl.l_whence;
			break;

		    case F_GETFL:
		    {
			unsigned int mode = *res_r;
			*res_r &= ~(O_CREAT|O_TRUNC|O_EXCL);
			if (mode & FNDELAY) {
				if (*pop & HPUX_FF_NONBLOCK_ON)
					*res_r |= HPUX_FF_NONBLOCK;
				if ((*pop & HPUX_FF_FNDELAY_ON) == 0)
					*res_r &= ~HPUX_FF_NDELAY;
			}
			if (mode & O_CREAT)
				*res_r |= HPUX_FF_FCREAT;
			if (mode & O_TRUNC)
				*res_r |= HPUX_FF_FTRUNC;
			if (mode & O_EXCL)
				*res_r |= HPUX_FF_FEXCL;
		    }
		}
	}
	return err;
}

errno_t e_hpux_fstatfs(int fd, hpux_statfs_t *hpbuf)
{
	errno_t err;
	struct statfs buf;

	err = e_fstatfs(fd, &buf);
	statfs_bsdtohpux(&buf, hpbuf);
	return err;
}

errno_t e_hpux_lockf(int fd, int func, hpux_off_t size)
{
	/*
	 * (XXX) lockf(), while not supported, always returns success!
	 */
	return ESUCCESS;
}

errno_t	e_hpux_mmap(
	const hpux_caddr_t *hpaddr,
	hpux_size_t hpsize,
	int hpprot,
	int hpflags,
	int fd,
	hpux_off_t hpoffset,
	hpux_caddr_t *addr_r)
{
	kern_return_t kr;
	mach_port_t handle;
	vm_offset_t addr = (vm_offset_t)hpaddr;
	vm_size_t size = (vm_size_t)round_page(hpsize);
	vm_prot_t prot, maxprot;
	vm_inherit_t inheritance;
	boolean_t copy, anywhere;

	prot = VM_PROT_NONE;
	if (hpprot & PROT_READ)
		prot |= VM_PROT_READ;
	if (hpprot & PROT_WRITE)
		prot |= VM_PROT_WRITE;
	if (hpprot & PROT_EXEC)
		prot |= VM_PROT_EXECUTE;

	anywhere = !(hpflags & HPUX_MAP_FIXED);
	copy = !(hpflags & HPUX_MAP_SHARED);
	maxprot = (hpflags & HPUX_MAP_SHARED) ? prot : VM_PROT_ALL;
	inheritance = (hpflags & HPUX_MAP_SHARED)? VM_INHERIT_SHARE:
		VM_INHERIT_COPY;

	/*
	 * If caller specified an address, we must check that:
	 *   1) address is page aligned, and
	 *   2) the requested address range is not in use.
	 */
	if (!anywhere) {
		vm_offset_t _addr = addr;
		vm_size_t _size;
		vm_prot_t _prot;
		vm_prot_t _max_prot;
		vm_inherit_t _inherit;
		boolean_t _shared;
		mach_port_t _object_name;
		vm_offset_t _offset;

		if (addr != trunc_page(addr))
			return EINVAL;
		kr = vm_region(mach_task_self(), &_addr, &_size,
			&_prot, &_max_prot, &_inherit, &_shared,
			&_object_name, &_offset);
		if (kr == KERN_SUCCESS && (addr + size) > _addr)
			return EINVAL;
	}

	if (hpflags & HPUX_MAP_ANONYMOUS)
		kr = vm_map(mach_task_self(), &addr, size, 0, anywhere,
		            MEMORY_OBJECT_NULL, 0, FALSE, VM_PROT_DEFAULT,
		            VM_PROT_ALL, VM_INHERIT_DEFAULT);
	else if ((kr=bsd_fd_to_file_port(process_self(), fd, &handle)) ==
	         ESUCCESS)
		kr = bsd_vm_map(process_self(), &addr, size, 0, anywhere,
		                handle, hpoffset, copy, prot, maxprot,
		                inheritance);

	if (kr != ESUCCESS)
		return e_mach_error_to_errno(kr);

	*addr_r = (hpux_caddr_t)addr;
	return ESUCCESS;
}

errno_t e_hpux_open(const char *path, int hpmode, int crtmode, int *fd_r)
{
	int mode;
	errno_t err;

	mode = hpmode;
	hpmode &= ~(HPUX_FF_NONBLOCK | HPUX_FF_FSYNCIO | HPUX_FF_FEXCL |
	            HPUX_FF_FTRUNC | HPUX_FF_FCREAT);
	if (mode & HPUX_FF_FCREAT) {
		/*
		 * simulate the pre-NFS behavior that opening a
		 * file for READ+CREATE ignores the CREATE (unless
		 * EXCL is set in which case we will return the
		 * proper error).
		 */
		if ((mode & HPUX_FF_FEXCL) || (mode+1) & 0x2)
			hpmode |= O_CREAT;
	}
	if (mode & HPUX_FF_FTRUNC)
		hpmode |= O_TRUNC;
	if (mode & HPUX_FF_FEXCL)
		hpmode |= O_EXCL;
	if (mode & HPUX_FF_NONBLOCK)
		hpmode |= O_NDELAY;
	err = e_open(path, hpmode, crtmode, fd_r);
	if (err == ESUCCESS) {
		/*
		 * Check that we dont allow more open files than HP-UX
		 * will allow.  This also prevents __hpux_fdflags[] overflow.
		 */
		if ((u_int)*fd_r >= HPUX_NOFILE) {
			e_close(*fd_r);
			err = EMFILE;
		} else if (hpmode & O_NDELAY) {
			int nblkioon = 1;
			(void) e_ioctl(*fd_r, FIONBIO, (char *)&nblkioon);

			__hpux_fdflags[*fd_r] = (mode & HPUX_FF_NONBLOCK)?
			    HPUX_FF_NONBLOCK_ON: HPUX_FF_FNDELAY_ON;
		}
	}
	return err;
}

errno_t e_hpux_poll(hpux_pollfd_t *pollfds, int nfd, int tout, int *rv_p)
{
	errno_t err;
	fd_set infds, outfds, exfds;
	struct timeval tv, *tvp;
	int i, fd, maxfd, scnt, nsel;
	short events, *revents;

	if (nfd < 0 || nfd >= HPUX_NOFILE || tout < -1)
		return EINVAL;

	scnt = 0;
	maxfd = -1;
	FD_ZERO(&infds);
	FD_ZERO(&outfds);
	FD_ZERO(&exfds);
	for (i = 0; i < nfd; i++) {
		if ((fd = pollfds[i].fd) < 0)	/* ignore negative poll fd's */
			continue;
		events = pollfds[i].events;
		revents = &pollfds[i].revents;
		*revents = 0;

		if (fd < 0)	/* ignore negative poll fd's */
			continue;
		else if (fd >= HPUX_NOFILE) {
			*revents |= HPUX_POLLNVAL;
			scnt++;
			continue;
		}
		if (events & ~(HPUX_POLLIN|HPUX_POLLOUT|HPUX_POLLERR)) {
			e_emulator_error("HP-UX poll: %x bits not implemented",
			   (events & ~(HPUX_POLLIN|HPUX_POLLOUT|HPUX_POLLERR)));
			*revents |= HPUX_POLLERR;
			scnt++;
			continue;
		}
		if (events & (HPUX_POLLIN|HPUX_POLLOUT|HPUX_POLLERR)) {
			if (fd > maxfd)
				maxfd = fd;
			if (events & HPUX_POLLIN)
				FD_SET(fd, &infds);
			if (events & HPUX_POLLOUT)
				FD_SET(fd, &outfds);
			if (events & HPUX_POLLERR)
				FD_SET(fd, &exfds);
		}
	}

	if (tout == -1)
		tvp = NULL;
	else {
		if (scnt)			/* if revents set, dont block */
			tout = 0;
		tv.tv_sec = tout / 1000;
		tv.tv_usec = (tout % 1000) * 1000;
		tvp = &tv;
	}

	err = e_select(maxfd+1, &infds, &outfds, &exfds, tvp, &nsel);

	scnt = 0;
	for (i = 0; i < nfd; i++) {
		fd = pollfds[i].fd;
		revents = &pollfds[i].revents;
		if (FD_ISSET(fd, &infds))
			*revents |= HPUX_POLLIN;
		if (FD_ISSET(fd, &outfds))
			*revents |= HPUX_POLLOUT;
		if (FD_ISSET(fd, &exfds))
			*revents |= HPUX_POLLERR;
		if (*revents)
			scnt++;
	}
	*rv_p = scnt;
	return err;
}

errno_t e_hpux_statfs(const char *path, hpux_statfs_t *hpbuf)
{
	errno_t err;
	struct statfs buf;

	err = e_statfs(path, &buf);
	statfs_bsdtohpux(&buf, hpbuf);
	return err;
}

/*
** Read and write system calls.
*/

/*
 * Same as BSD except for non-blocking behavior.
 * There are three types of non-blocking reads/writes in HP-UX checked
 * in the following order:
 *
 *	O_NONBLOCK: return -1 and errno == EAGAIN
 *	O_NDELAY:   return 0
 *	FIOSNBIO:   return -1 and errno == EWOULDBLOCK
 *
 * HPUX_RWCHKFD(fd) checks that fd is a reasonable file descriptor
 * HPUX_RWFIXERR(err, fd, rval) adjusts EWOULDBLOCK for HP-UX.
 */
#define	HPUX_RWCHKFD(fd)				\
	if ((u_int)fd >= HPUX_NOFILE)			\
		return EBADF
#define	HPUX_RWFIXERR(err, fd, rval)			\
	if (err == EWOULDBLOCK) {			\
		unsigned int *fp = &__hpux_fdflags[fd];	\
		if (*fp & HPUX_FF_NONBLOCK_ON) {		\
			*rval = -1;			\
			err = HPUX_OEAGAIN;		\
		} else if (*fp & HPUX_FF_FNDELAY_ON) {	\
			*rval = 0;			\
			err = ESUCCESS;			\
		}					\
	}

errno_t e_hpux_read(
	int fd,
	void *buf,
	hpux_size_t nbytes,
	hpux_size_t *nread_r)
{
	errno_t err;

	HPUX_RWCHKFD(fd);
	err = e_read(fd, buf, nbytes, nread_r);
	HPUX_RWFIXERR(err, fd, nread_r);
	return err;
}

errno_t e_hpux_readv(
	int fd,
	struct iovec *iov,
	int iovlen,
	hpux_size_t *nread_r)
{
	errno_t err;

	HPUX_RWCHKFD(fd);
	err = e_readv(fd, iov, iovlen, nread_r);
	HPUX_RWFIXERR(err, fd, nread_r);
	return err;
}

errno_t e_hpux_write(
	int fd,
	const void *buf,
	hpux_size_t nbytes,
	hpux_size_t *nwrite_r)
{
	errno_t err;

	HPUX_RWCHKFD(fd);
	err = e_write(fd, buf, nbytes, nwrite_r);
	HPUX_RWFIXERR(err, fd, nwrite_r);
	return err;
}

errno_t e_hpux_writev(
	int fd,
	const struct iovec *iov,
	int iovlen,
	hpux_size_t *nwrite_r)
{
	errno_t err;

	HPUX_RWCHKFD(fd);
	err = e_writev(fd, iov, iovlen, nwrite_r);
	HPUX_RWFIXERR(err, fd, nwrite_r);
	return err;
}
#undef	HPUX_RWCHKFD
#undef	HPUX_RWFIXERR

/*
** Ioctl / Termio system calls and support.
*/

static int baud_bsdtohpux(long speed)
{
	long hpspeed = HPUX_TIO_B0;

	switch (speed) {
	    case B50:
		hpspeed = HPUX_TIO_B50; break;
	    case B75:
		hpspeed = HPUX_TIO_B75; break;
	    case B110:
		hpspeed = HPUX_TIO_B110; break;
	    case B134:
		hpspeed = HPUX_TIO_B134; break;
	    case B150:
		hpspeed = HPUX_TIO_B150; break;
	    case B200:
		hpspeed = HPUX_TIO_B200; break;
	    case B300:
		hpspeed = HPUX_TIO_B300; break;
	    case B600:
		hpspeed = HPUX_TIO_B600; break;
	    case B1200:
		hpspeed = HPUX_TIO_B1200; break;
	    case B1800:
		hpspeed = HPUX_TIO_B1800; break;
	    case B2400:
		hpspeed = HPUX_TIO_B2400; break;
	    case B4800:
		hpspeed = HPUX_TIO_B4800; break;
	    case B9600:
		hpspeed = HPUX_TIO_B9600; break;
	    case B19200:
		hpspeed = HPUX_TIO_B19200; break;
	    case B38400:
		hpspeed = HPUX_TIO_B38400; break;
	    case B57600:
		hpspeed = HPUX_TIO_B57600; break;
	    case B115200:
		hpspeed = HPUX_TIO_B115200; break;
	    case B230400:
		hpspeed = HPUX_TIO_B230400; break;
	}
	return hpspeed;
}

static int baud_hpuxtobsd(int hpspeed)
{
	static char baud_hpuxtobsd_map[32] = {
		B0,	B50,	B75,	B110,	B134,	B150,	B200,	B300,
		B600,	B0,	B1200,	B1800,	B2400,	B0,	B4800,	B0,
		B9600,	B19200,	B38400,	B57600,	B115200,B230400,B0,	B0,
		B0,	B0,	B0,	B0,	B0,	B0,	EXTA,	EXTB
	};

	return baud_hpuxtobsd_map[hpspeed & HPUX_TIO_CBAUD];
}

static int ioctl_hpuxtobsd(unsigned int com)
{
	switch (com) {
	    case HPUX_TIOCSLTC:
		com = TIOCSLTC; break;
	    case HPUX_TIOCGLTC:
		com = TIOCGLTC; break;
	    case HPUX_TIOCSPGRP:
		com = TIOCSPGRP; break;
	    case HPUX_TIOCGPGRP:
		com = TIOCGPGRP; break;
	    case HPUX_TIOCLBIS:
		com = TIOCLBIS; break;
	    case HPUX_TIOCLBIC:
		com = TIOCLBIC; break;
	    case HPUX_TIOCLSET:
		com = TIOCLSET; break;
	    case HPUX_TIOCLGET:
		com = TIOCLGET; break;
	    case HPUX_TIOCGWINSZ:
		com = TIOCGWINSZ; break;
	    case HPUX_TIOCSWINSZ:
		com = TIOCSWINSZ; break;
	}
	return com;
}

static void hpux_termiostotermio(hpux_termios_t *tios, hpux_termio_t *tio)
{
	int i;

	tio->c_iflag = tios->c_iflag;
	tio->c_oflag = tios->c_oflag;
	tio->c_cflag = tios->c_cflag;
	tio->c_lflag = tios->c_lflag;
	tio->c_line = tios->c_reserved;
	for (i = 0; i <= HPUX_VSWTCH; i++)
		tio->c_cc[i] = tios->c_cc[i];
	if (tios->c_lflag & HPUX_TIO_ICANON) {
		tio->c_cc[HPUX_VEOF] = tios->c_cc[HPUX_VEOF];
		tio->c_cc[HPUX_VEOL] = tios->c_cc[HPUX_VEOL];
	} else {
		tio->c_cc[HPUX_VMIN] = tios->c_cc[HPUX_VMINS];
		tio->c_cc[HPUX_VTIME] = tios->c_cc[HPUX_VTIMES];
	}
}

static void hpux_termiototermios(
	hpux_termio_t *tio,
	hpux_termios_t *tios,
	struct termios *bsdtios)
{
	int i;

	bzero((char *)tios, sizeof *tios);
	tios->c_iflag = tio->c_iflag;
	tios->c_oflag = tio->c_oflag;
	tios->c_cflag = tio->c_cflag;
	tios->c_lflag = tio->c_lflag;
	tios->c_reserved = tio->c_line;
	for (i = 0; i <= HPUX_VSWTCH; i++)
		tios->c_cc[i] = tio->c_cc[i];
	if (tios->c_lflag & HPUX_TIO_ICANON) {
		tios->c_cc[HPUX_VEOF] = tio->c_cc[HPUX_VEOF];
		tios->c_cc[HPUX_VEOL] = tio->c_cc[HPUX_VEOL];
		tios->c_cc[HPUX_VMINS] = 0;
		tios->c_cc[HPUX_VTIMES] = 0;
	} else {
		tios->c_cc[HPUX_VEOF] = 0;
		tios->c_cc[HPUX_VEOL] = 0;
		tios->c_cc[HPUX_VMINS] = tio->c_cc[HPUX_VMIN];
		tios->c_cc[HPUX_VTIMES] = tio->c_cc[HPUX_VTIME];
	}
	tios->c_cc[HPUX_VSUSP] = bsdtios->c_cc[VSUSP];
	tios->c_cc[HPUX_VSTART] = bsdtios->c_cc[VSTART];
	tios->c_cc[HPUX_VSTOP] = bsdtios->c_cc[VSTOP];
}

static errno_t hpux_termio(int fd, int com, caddr_t data)
{
	struct termios tios;
	hpux_termios_t htios;
	errno_t error;
	int line, fdflags;
	int newi = 0;

	switch (com) {
	case HPUX_TCGETATTR:
		newi = 1;
		/* fall into ... */
	case HPUX_TCGETA:
		/*
		 * Get BSD terminal state
		 */
		if ((error = e_ioctl(fd,TIOCGETA,(char *) &tios)) != ESUCCESS ||
		    (error = e_fcntl(fd,F_GETFL,NULL,&fdflags)) != ESUCCESS)
			break;
		bzero((char *)&htios, sizeof htios);
		/*
		 * Set iflag.
		 * Same through ICRNL, no BSD equivs for IUCLC, IENQAK
		 */
		htios.c_iflag = tios.c_iflag & 0x1ff;
		if (tios.c_iflag & IXON)
			htios.c_iflag |= HPUX_TIO_IXON;
		if (tios.c_iflag & IXOFF)
			htios.c_iflag |= HPUX_TIO_IXOFF;
		if (tios.c_iflag & IXANY)
			htios.c_iflag |= HPUX_TIO_IXANY;
		/*
		 * Set oflag.
		 * No BSD equivs for OLCUC/OCRNL/ONOCR/ONLRET/OFILL/OFDEL
		 * or any of the delays.
		 */
		if (tios.c_oflag & OPOST)
			htios.c_oflag |= HPUX_TIO_OPOST;
		if (tios.c_oflag & ONLCR)
			htios.c_oflag |= HPUX_TIO_ONLCR;
		if (tios.c_oflag & OXTABS)
			htios.c_oflag |= HPUX_TIO_TAB3;
		/*
		 * Set cflag.
		 * Baud from ospeed, rest from cflag.
		 */
		htios.c_cflag = baud_bsdtohpux(tios.c_ospeed);
		switch (tios.c_cflag & CSIZE) {
		case CS5:
			htios.c_cflag |= HPUX_TIO_CS5; break;
		case CS6:
			htios.c_cflag |= HPUX_TIO_CS6; break;
		case CS7:
			htios.c_cflag |= HPUX_TIO_CS7; break;
		case CS8:
			htios.c_cflag |= HPUX_TIO_CS8; break;
		}
		if (tios.c_cflag & CSTOPB)
			htios.c_cflag |= HPUX_TIO_CSTOPB;
		if (tios.c_cflag & CREAD)
			htios.c_cflag |= HPUX_TIO_CREAD;
		if (tios.c_cflag & PARENB)
			htios.c_cflag |= HPUX_TIO_PARENB;
		if (tios.c_cflag & PARODD)
			htios.c_cflag |= HPUX_TIO_PARODD;
		if (tios.c_cflag & HUPCL)
			htios.c_cflag |= HPUX_TIO_HUPCL;
		if (tios.c_cflag & CLOCAL)
			htios.c_cflag |= HPUX_TIO_CLOCAL;
		/*
		 * Set lflag.
		 * No BSD equiv for XCASE.
		 */
		if (tios.c_lflag & ECHOE)
			htios.c_lflag |= HPUX_TIO_ECHOE;
		if (tios.c_lflag & ECHOK)
			htios.c_lflag |= HPUX_TIO_ECHOK;
		if (tios.c_lflag & ECHO)
			htios.c_lflag |= HPUX_TIO_ECHO;
		if (tios.c_lflag & ECHONL)
			htios.c_lflag |= HPUX_TIO_ECHONL;
		if (tios.c_lflag & ISIG)
			htios.c_lflag |= HPUX_TIO_ISIG;
		if (tios.c_lflag & ICANON)
			htios.c_lflag |= HPUX_TIO_ICANON;
		if (tios.c_lflag & NOFLSH)
			htios.c_lflag |= HPUX_TIO_NOFLSH;
		/*
		 * Line discipline
		 */
		if (!newi) {
			line = 0;
			(void) e_ioctl(fd, TIOCGETD, (caddr_t)&line);
			htios.c_reserved = line;
		}
		/*
		 * Set editing chars.
		 * No BSD equiv for VSWTCH.
		 */
		htios.c_cc[HPUX_VINTR] = tios.c_cc[VINTR];
		htios.c_cc[HPUX_VQUIT] = tios.c_cc[VQUIT];
		htios.c_cc[HPUX_VERASE] = tios.c_cc[VERASE];
		htios.c_cc[HPUX_VKILL] = tios.c_cc[VKILL];
		htios.c_cc[HPUX_VEOF] = tios.c_cc[VEOF];
		htios.c_cc[HPUX_VEOL] = tios.c_cc[VEOL];
		htios.c_cc[HPUX_VEOL2] = tios.c_cc[VEOL2];
		htios.c_cc[HPUX_VSWTCH] = 0;
#if 1
		/*
		 * XXX since VMIN and VTIME are not implemented,
		 * we need to return something reasonable.
		 * Otherwise a GETA/SETA combo would always put
		 * the tty in non-blocking mode (since VMIN == VTIME == 0).
		 */
		if (fdflags & FNONBLOCK) {
			htios.c_cc[HPUX_VMINS] = 0;
			htios.c_cc[HPUX_VTIMES] = 0;
		} else {
			htios.c_cc[HPUX_VMINS] = 6;
			htios.c_cc[HPUX_VTIMES] = 1;
		}
#else
		htios.c_cc[HPUX_VMINS] = tios.c_cc[VMIN];
		htios.c_cc[HPUX_VTIMES] = tios.c_cc[VTIME];
#endif
		htios.c_cc[HPUX_VSUSP] = tios.c_cc[VSUSP];
		htios.c_cc[HPUX_VSTART] = tios.c_cc[VSTART];
		htios.c_cc[HPUX_VSTOP] = tios.c_cc[VSTOP];
		if (newi)
			bcopy((char *)&htios, data, sizeof htios);
		else
			hpux_termiostotermio(&htios, (hpux_termio_t *)data);
		break;

	case HPUX_TCSETATTR:
	case HPUX_TCSETATTRD:
	case HPUX_TCSETATTRF:
		newi = 1;
		/* fall into ... */
	case HPUX_TCSETA:
	case HPUX_TCSETAW:
	case HPUX_TCSETAF:
		/*
		 * Get old characteristics and determine if we are a tty.
		 */
		if ((error = e_ioctl(fd, TIOCGETA, (caddr_t)&tios)) != ESUCCESS)
			break;
		if (newi)
			bcopy(data, (char *)&htios, sizeof htios);
		else
			hpux_termiototermios((hpux_termio_t *)data,
			                     &htios, &tios);
		/*
		 * Set iflag.
		 * Same through ICRNL, no HP-UX equiv for IMAXBEL
		 */
		tios.c_iflag &= ~(IXON|IXOFF|IXANY|0x1ff);
		tios.c_iflag |= htios.c_iflag & 0x1ff;
		if (htios.c_iflag & HPUX_TIO_IXON)
			tios.c_iflag |= IXON;
		if (htios.c_iflag & HPUX_TIO_IXOFF)
			tios.c_iflag |= IXOFF;
		if (htios.c_iflag & HPUX_TIO_IXANY)
			tios.c_iflag |= IXANY;
		/*
		 * Set oflag.
		 * No HP-UX equiv for ONOEOT
		 */
		tios.c_oflag &= ~(OPOST|ONLCR|OXTABS);
		if (htios.c_oflag & HPUX_TIO_OPOST)
			tios.c_oflag |= OPOST;
		if (htios.c_oflag & HPUX_TIO_ONLCR)
			tios.c_oflag |= ONLCR;
		if (htios.c_oflag & HPUX_TIO_TAB3)
			tios.c_oflag |= OXTABS;
		/*
		 * Set cflag.
		 * No HP-UX equiv for CCTS_OFLOW/CCTS_IFLOW/MDMBUF
		 */
		tios.c_cflag &=
			~(CSIZE|CSTOPB|CREAD|PARENB|PARODD|HUPCL|CLOCAL);
		switch (htios.c_cflag & HPUX_TIO_CSIZE) {
		case HPUX_TIO_CS5:
			tios.c_cflag |= CS5; break;
		case HPUX_TIO_CS6:
			tios.c_cflag |= CS6; break;
		case HPUX_TIO_CS7:
			tios.c_cflag |= CS7; break;
		case HPUX_TIO_CS8:
			tios.c_cflag |= CS8; break;
		}
		if (htios.c_cflag & HPUX_TIO_CSTOPB)
			tios.c_cflag |= CSTOPB;
		if (htios.c_cflag & HPUX_TIO_CREAD)
			tios.c_cflag |= CREAD;
		if (htios.c_cflag & HPUX_TIO_PARENB)
			tios.c_cflag |= PARENB;
		if (htios.c_cflag & HPUX_TIO_PARODD)
			tios.c_cflag |= PARODD;
		if (htios.c_cflag & HPUX_TIO_HUPCL)
			tios.c_cflag |= HUPCL;
		if (htios.c_cflag & HPUX_TIO_CLOCAL)
			tios.c_cflag |= CLOCAL;
		/*
		 * Set lflag.
		 * No HP-UX equiv for ECHOKE/ECHOPRT/ECHOCTL
		 * IEXTEN treated as part of ICANON
		 */
		tios.c_lflag &= ~(ECHOE|ECHOK|ECHO|ISIG|ICANON|IEXTEN|NOFLSH);
		if (htios.c_lflag & HPUX_TIO_ECHOE)
			tios.c_lflag |= ECHOE;
		if (htios.c_lflag & HPUX_TIO_ECHOK)
			tios.c_lflag |= ECHOK;
		if (htios.c_lflag & HPUX_TIO_ECHO)
			tios.c_lflag |= ECHO;
		if (htios.c_lflag & HPUX_TIO_ECHONL)
			tios.c_lflag |= ECHONL;
		if (htios.c_lflag & HPUX_TIO_ISIG)
			tios.c_lflag |= ISIG;
		if (htios.c_lflag & HPUX_TIO_ICANON)
			tios.c_lflag |= (ICANON|IEXTEN);
		if (htios.c_lflag & HPUX_TIO_NOFLSH)
			tios.c_lflag |= NOFLSH;
		/*
		 * Set editing chars.
		 * No HP-UX equivs of VWERASE/VREPRINT/VDSUSP/VLNEXT
		 * /VDISCARD/VSTATUS/VERASE2
		 */
		tios.c_cc[VINTR] = htios.c_cc[HPUX_VINTR];
		tios.c_cc[VQUIT] = htios.c_cc[HPUX_VQUIT];
		tios.c_cc[VERASE] = htios.c_cc[HPUX_VERASE];
		tios.c_cc[VKILL] = htios.c_cc[HPUX_VKILL];
		tios.c_cc[VEOF] = htios.c_cc[HPUX_VEOF];
		tios.c_cc[VEOL] = htios.c_cc[HPUX_VEOL];
		tios.c_cc[VEOL2] = htios.c_cc[HPUX_VEOL2];
		tios.c_cc[VMIN] = htios.c_cc[HPUX_VMINS];
		tios.c_cc[VTIME] = htios.c_cc[HPUX_VTIMES];
		tios.c_cc[VSUSP] = htios.c_cc[HPUX_VSUSP];
		tios.c_cc[VSTART] = htios.c_cc[HPUX_VSTART];
		tios.c_cc[VSTOP] = htios.c_cc[HPUX_VSTOP];

		/*
		 * Set the new stuff
		 */
		if (com == HPUX_TCSETA || com == HPUX_TCSETATTR)
			com = TIOCSETA;
		else if (com == HPUX_TCSETAW || com == HPUX_TCSETATTRD)
			com = TIOCSETAW;
		else
			com = TIOCSETAF;
		if ((error = e_ioctl(fd, com, (caddr_t)&tios)) == ESUCCESS) {
			/*
			 * Set line discipline
			 */
			if (!newi) {
				line = htios.c_reserved;
				(void) e_ioctl(fd, TIOCSETD, (caddr_t)&line);
			}
			/*
			 * Set non-blocking IO if VMIN == VTIME == 0, clear
			 * if not.  Should handle the other cases as well.
			 * Note it isn't correct to just turn NBIO off like
			 * we do as it could be on as the result of a fcntl
			 * operation.
			 *
			 * XXX - wouldn't need to do this at all if VMIN/VTIME
			 * were implemented.
			 */
			if ((error = e_fcntl(fd, F_GETFL, NULL, &fdflags)) ==
			    ESUCCESS) {
				int flags, nbio, ignore;

				nbio = (htios.c_cc[HPUX_VMINS] == 0 &&
					htios.c_cc[HPUX_VTIMES] == 0);
				if (nbio && (fdflags & FNONBLOCK) == 0 ||
				    !nbio && (fdflags & FNONBLOCK)) {
					(void) e_hpux_fcntl(fd, F_GETFL, NULL,
						&flags);
					if (nbio)
						flags |= HPUX_FF_NDELAY;
					else
						flags &= ~HPUX_FF_NDELAY;
					(void) e_hpux_fcntl(fd, F_SETFL,
						flags, &ignore);
				}
			}
		}
		break;

	default:
		error = EINVAL;
		break;
	}
	return error;
}

errno_t e_hpux_ioctl(int fd, unsigned int com, char *data)
{
	errno_t error = ESUCCESS;

	if ((u_int)fd >= HPUX_NOFILE)
		return EBADF;

	/*
	 * The differences here are:
	 *	IOC_IN also means IOC_VOID if the size portion is zero.
	 *	no FIOCLEX/FIONCLEX/FIOASYNC/FIOGETOWN/FIOSETOWN
	 *	the sgttyb struct is 2 bytes longer
	 */
	switch(com) {
	    case HPUX_FIOSNBIO:

	    {
		unsigned int *ofp = &__hpux_fdflags[fd];
		int tmp;

		if (*(int *)data)
			*ofp |= HPUX_FF_FIONBIO_ON;
		else
			*ofp &= ~HPUX_FF_FIONBIO_ON;
		/*
		 * Only set/clear if O_NONBLOCK/FNDELAY not in effect
		 */
		if ((*ofp & (HPUX_FF_NONBLOCK_ON|HPUX_FF_FNDELAY_ON)) == 0) {
			tmp = *ofp & HPUX_FF_FIONBIO_ON;
			error = e_ioctl(fd, FIONBIO, (caddr_t)&tmp);
		} else {
			/*
			 * Here is an oddity... it's possible that we are
			 * dealing with an invalid file descriptor; to be
			 * consistent, we will check that it is valid.
			 */
			tmp = e_ioctl(fd, FIOGETOWN, (caddr_t)&tmp);
			if (tmp == EBADF)	/* all we care about */
				error = tmp;
		}
		break;
	    }
	    case HPUX_TIOCCONS:
		*(int *)data = 1;
		error = e_ioctl(fd, TIOCCONS, data);
		break;

	    /* BSD-style job control ioctls */
	    case HPUX_TIOCLBIS:
	    case HPUX_TIOCLBIC:
	    case HPUX_TIOCLSET:
		*(int *)data &= HPUX_LTOSTOP;
		if (*(int *)data & HPUX_LTOSTOP)
			*(int *)data = LTOSTOP;
		/* fall into */

	    /* simple mapping cases */
	    case HPUX_TIOCLGET:
	    case HPUX_TIOCSLTC:
	    case HPUX_TIOCGLTC:
	    case HPUX_TIOCSPGRP:
	    case HPUX_TIOCGPGRP:
	    case HPUX_TIOCGWINSZ:
	    case HPUX_TIOCSWINSZ:
		error = e_ioctl(fd, ioctl_hpuxtobsd(com), data);
		if (error == ESUCCESS && com == HPUX_TIOCLGET) {
			*(int *)data &= LTOSTOP;
			if (*(int *)data & LTOSTOP)
				*(int *)data = HPUX_LTOSTOP;
		}
		break;

	    /* SYS 5 termio and POSIX termios */
	    case HPUX_TCGETA:
	    case HPUX_TCSETA:
	    case HPUX_TCSETAW:
	    case HPUX_TCSETAF:
	    case HPUX_TCGETATTR:
	    case HPUX_TCSETATTR:
	    case HPUX_TCSETATTRD:
	    case HPUX_TCSETATTRF:
		error = hpux_termio(fd, com, data);
		break;

	    default:
		error = e_ioctl(fd, com, data);
		break;
	}

	return error;
}

/*
** System / Resource control system calls.
*/

errno_t e_hpux_getrlimit(int resource, hpux_rlimit_t *hprl)
{
	struct rlimit rl;
	errno_t err;

	switch (resource) {
	    case HPUX_RLIMIT_NOFILE:
		if ((err = e_lite_getrlimit(RLIMIT_NOFILE, &rl)) == ESUCCESS) {
			hprl->rlim_cur = rl.rlim_cur;
			hprl->rlim_max = rl.rlim_max;
		}
		break;

	    default:	/* HP-UX does not support anything else */
		err = EINVAL;
		break;
	}
	return err;
}

errno_t e_hpux_getrusage(int who, hpux_rusage_t *hpru)
{
	errno_t err;
	struct rusage ru;

	if ((err = e_getrusage(who, &ru)) == ESUCCESS)
		rusage_bsdtohpux(&ru, hpru);
	return err;
}

errno_t e_hpux_setrlimit(int resource, const hpux_rlimit_t *hprl)
{
	struct rlimit rl;
	errno_t err;

	switch (resource) {
	    case HPUX_RLIMIT_NOFILE:
		rl.rlim_cur = hprl->rlim_cur;
		rl.rlim_max = hprl->rlim_max;
		err = e_lite_setrlimit(RLIMIT_NOFILE, &rl);
		break;

	    default:	/* HP-UX does not support anything else */
		err = EINVAL;
		break;
	}
	return err;
}

errno_t e_hpux_sysconf(int name, long *res_r)
{
	errno_t err = ESUCCESS;

	switch (name) {
	    case HPUX_SYSCONF_ARGMAX:	/* maximum arguments for exec */
		*res_r = ARG_MAX;
		break;


	    case HPUX_SYSCONF_CLDMAX:	/* maximum child processes */
		*res_r = CHILD_MAX;
		break;

	    case HPUX_SYSCONF_CLKTICK:	/* clock ticks per second */
		*res_r = 100;		/* (XXX) see sysctl/KERN_CLOCKRATE */
		break;

	    case HPUX_SYSCONF_OPENMAX:	/* open files */
		*res_r = NOFILE;
		break;

	    case HPUX_SYSCONF_CPUTYPE:	/* CPU type (XXX for now) */
		*res_r = HPUX_SYSCONF_CPUPA11;
		break;

	    case HPUX_SYSCONF_PAGESIZE:	/* pagesize */
		*res_r = NBPG;
		break;

	    default:			/* something not yet implemented */
		e_emulator_error("HP-UX sysconf(%d): not implemented", name);
		err = EINVAL;
		break;
	}

	return err;
}


errno_t e_hpux_utssys(hpux_utsname_t *uts, hpux_dev_t dev, int request)
{
	int mib[2];
	size_t len;
	errno_t err, rval;

	switch (request) {
	    case 0:	/* uname */
	    {
		int i;
		char bigbuf[256];	/* use a large buffer to avoid ENOMEM */

#define	GETMIB(top, sub, field, loc)	\
		mib[0] = top;			\
		mib[1] = sub;			\
		len = sizeof(bigbuf);		\
		if ((err=e_sysctl(mib,2,&bigbuf,&len,NULL,0,NULL))!=ESUCCESS) \
			rval = err;		\
		bcopy(bigbuf, uts->field, MIN(len, sizeof(uts->field))); \
		uts->field[sizeof(uts->field)-1] = '\0';

		GETMIB(CTL_KERN, KERN_OSTYPE, sysname, bigbuf);
		GETMIB(CTL_KERN, KERN_HOSTNAME, nodename, bigbuf);
		for (i = 0; i < sizeof(uts->nodename); i++)
			if (uts->nodename[i] == '.') {	/* unqualify */
				uts->nodename[i] = '\0';
				break;
			}
		GETMIB(CTL_KERN, KERN_OSRELEASE, release, bigbuf);
		GETMIB(CTL_KERN, KERN_VERSION, version, bigbuf);
		GETMIB(CTL_HW, HW_MACHINE, version, bigbuf);
		GETMIB(CTL_KERN, KERN_HOSTID, version, &i);
		(void) sprintf(uts->idnumber, "%d", i);
		break;
#undef	GETMIB
	    }

	    case 5:	/* gethostname */
		mib[0] = CTL_KERN;
		mib[1] = KERN_HOSTNAME;
		len = dev;
		if ((err = e_sysctl(mib,2,uts,&len,NULL,0,NULL)) != ESUCCESS)
			rval = err;
		break;

	    case 1:	/* ?? */
	    case 2:	/* ustat */
	    case 3:	/* ?? */
	    case 4:	/* sethostname */
	    default:
		return EINVAL;
		break;
	}
	return ESUCCESS;
}

errno_t e_hpux_ulimit(int cmd, long newlimit, int *lim_r)
{
	struct rlimit rlim;
	errno_t err;

	if ((err = e_lite_getrlimit(RLIMIT_FSIZE, &rlim)) == ESUCCESS) {
		switch (cmd) {
		    case 2:
			newlimit *= 512;
			rlim.rlim_cur = rlim.rlim_max = newlimit;
			err = e_lite_setrlimit(RLIMIT_FSIZE, &rlim);
			/* else fall into... */

		    case 1:
			*lim_r = rlim.rlim_max / 512;
			break;

		    case 3:
			err = e_lite_getrlimit(RLIMIT_DATA, &rlim);
			*lim_r = rlim.rlim_max;
			break;

		    default:
			err = EINVAL;
		}
	}

	return err;
}

/*
** Miscellaneous system calls.
*/

errno_t e_hpux_alarm(unsigned int secs, unsigned int *secs_r)
{
	struct itimerval it, oitv;
	struct itimerval *itp = &it;
	errno_t err;

	itp->it_interval.tv_sec = itp->it_interval.tv_usec = 0;
	itp->it_value.tv_sec = secs;
	itp->it_value.tv_usec = 0;
	if ((err = e_setitimer(ITIMER_REAL, itp, &oitv)) != ESUCCESS) {
		*secs_r = ((unsigned int)0xffffffff);
		return err;
	}
	if (oitv.it_value.tv_usec)
		oitv.it_value.tv_sec++;
	*secs_r = oitv.it_value.tv_sec;
	return ESUCCESS;
}

errno_t e_hpux_execve(const char *fname, const char *argp[], const char *envp[])
{
	extern int shared_enabled;
	extern struct ushared_rw *shared_base_rw;

	/*
	 * First, prepare to copy state across to exec'd process.
	 *
	 * N.B. In the case of __hpux_exsave.fdflags, we will blindly copy
	 * everything rather than look at CLOSE_ON_EXEC; this is safe, as
	 * other (closed) descriptors will need to be open/dup'd before
	 * being used (causing fdflags to be initialized).
	 */
	__e_hpux_exsave(&__hpux_exsave, sizeof(__hpux_exsave));
	return e_execve(fname, argp, envp);
}

errno_t e_hpux_execv(const char *fname, const char *argp[])
{
	return e_hpux_execve(fname, argp, NULL);
}

errno_t e_hpux_fork(int *argp, int *rval, void *regs)
{
	/*
	 * For fork, parent returns 0 in rval[1] (ret1), child returns 1.
	 * Under HP-UX, vfork() is same as fork().
	 */
	errno_t err = e_fork((pid_t *)rval);
	if (err) {
		if (err == EAGAIN)
			err = HPUX_OEAGAIN;
		return err;
	}
	rval[1] = (rval[0])? 0: 1;

	return ESUCCESS;
}

errno_t e_hpux_getdomainname(char *name, int namelen)
{
	(void) strncpy(name, __hpux_domainname, namelen);
	return ESUCCESS;
}

errno_t e_hpux_getgid(hpux_gid_t *gid_r)
{
	errno_t err;
	gid_t gid;

	if ((err = e_getgid(&gid)) == ESUCCESS)
		*gid_r = gid;
	return err;
}

errno_t e_hpux_getpgrp2(hpux_pid_t hppid, hpux_pid_t *pgrp_r)
{
	errno_t err;
	pid_t pgrp;

	/*
	 * (XXX) e_hpux_getpgrp2() needs server hook when pid is not self.
	 * Error now...
	 */
	if (hppid != 0) {
		pid_t mypid;

		if (e_getpid(&mypid) == ESUCCESS && mypid != hppid) {
			e_emulator_error("HP-UX getpgrp2(%d): invalid pid",
				hppid);
			return EPERM;
		}
	}

	if ((err = e_getpgrp(&pgrp)) == ESUCCESS)
		*pgrp_r = pgrp;
	return err;
}

errno_t e_hpux_getpid(hpux_pid_t *pid_r)
{
	errno_t err;
	pid_t pid;

	if ((err = e_getpid(&pid)) == ESUCCESS)
		*pid_r = pid;
	return err;
}

errno_t e_hpux_getuid(hpux_uid_t *uid_r)
{
	errno_t err;
	uid_t uid;

	if ((err = e_getuid(&uid)) == ESUCCESS)
		*uid_r = uid;
	return err;
}

errno_t e_hpux_nice(int inc, int *pri_r)
{
	int pri;
	errno_t err;

	if ((err = e_getpriority(PRIO_PROCESS, 0, &pri)) == ESUCCESS) {
		pri += inc;
		if (pri > 19)
			pri = 19;
		else if (pri < -20)
			pri = -20;
		if ((err = e_setpriority(PRIO_PROCESS, 0, pri)) == ESUCCESS)
			*pri_r = pri;
	}
	return err;
}

errno_t e_hpux_rtprio(hpux_pid_t pid, int hpprio, int *prio_r)
{
	errno_t err;
	int prio;

	if (hpprio < HPUX_RTPRIO_MIN && hpprio > HPUX_RTPRIO_MAX &&
	    hpprio != HPUX_RTPRIO_NOCHG && hpprio != HPUX_RTPRIO_RTOFF)
		err = EINVAL;
	else if ((err = e_getpriority(PRIO_PROCESS, pid, &prio)) == ESUCCESS) {
		if (prio < 0)
			*prio_r = (prio + 16) << 3;
		else
			*prio_r = HPUX_RTPRIO_RTOFF;

		switch (hpprio) {
		    case HPUX_RTPRIO_NOCHG:
			break;

		    case HPUX_RTPRIO_RTOFF:
			if (prio < 0) {
				prio = 0;
				err = e_setpriority(PRIO_PROCESS, pid, prio);
			}
			break;

		    default:
			prio = (hpprio >> 3) - 16;
			err = e_setpriority(PRIO_PROCESS, pid, prio);
			break;
		}
	}
	return err;
}

errno_t e_hpux_mpctl(int cmd, int arg1, int arg2, int *res_r)
{
	errno_t err = ESUCCESS;

	switch (cmd) {
	    case 1:		/* number of cpus */
		*res_r = 1;	/* (XXX) for now */
		break;

	    case 2:		/* first cpu */
		*res_r = 0;	/* (XXX) for now */
		break;

	    default:		/* something not yet implemented */
		e_emulator_error("HP-UX mpctl(%d): not implemented", cmd);
		err = EINVAL;
		break;
	}
	return err;
}

errno_t e_hpux_setdomainname(char *name, int namelen)
{
	if (namelen > HPUX_DOMAINNAME_SIZE)
		namelen = HPUX_DOMAINNAME_SIZE;
	(void) strncpy(__hpux_domainname, name, namelen);
	__hpux_domainname[HPUX_DOMAINNAME_SIZE] = '\0';
	return ESUCCESS;
}

errno_t e_hpux_setpgrp(int flag, hpux_pid_t *pgrp_r)
{
	errno_t err = ESUCCESS;
	pid_t pid, pgrp;

	if ((err = e_getpgrp(&pgrp)) == ESUCCESS && flag &&
	    (err = e_getpid(&pid)) == ESUCCESS && pid != pgrp &&
	    (err = e_setpgid(pid, pgrp)) == ESUCCESS)
		pgrp = pid;

	if (err == ESUCCESS)
		*pgrp_r = (hpux_pid_t)pgrp;

	return err;
}

errno_t e_hpux_setpgrp2(hpux_pid_t pid, hpux_pid_t pgrp, hpux_pid_t *pgrp_r)
{
	/* empirically determined */
	if (pgrp < 0 || pgrp >= 30000)
		return EINVAL;
	return e_hpbsd_setpgrp((int)pid, (int)pgrp, (int *)pgrp_r);
}

/*
** Network-related system calls.
*/

static int __hpux_sockopts[2];		/* dont bother saving across exec */

errno_t e_hpux_getsockopt(
	int fd,
	int level,
	int optname,
	void *optval,
	int *optlen)
{
	if (optname==HPUX_SOSND_COPYAVOID || optname==HPUX_SORCV_COPYAVOID) {
		*(int *)optval = __hpux_sockopts[optname-HPUX_SOSND_COPYAVOID];
		*optlen = sizeof(int);
		return ESUCCESS;
	}

	return e_getsockopt(fd, level, optname, optval, optlen);
}

errno_t e_hpux_setsockopt(
	int fd,
	int level,
	int optname,
	const void *optval,
	int optlen)
{
	if (optname==HPUX_SOSND_COPYAVOID || optname==HPUX_SORCV_COPYAVOID) {
		__hpux_sockopts[optname-HPUX_SOSND_COPYAVOID] = *(int *)optval;
		return ESUCCESS;
	}

	return e_setsockopt(fd, level, optname, optval, optlen);
}

/*
** (XXX) SysV shm/sem (i.e. return ESUCCESS and hope for the best).
*/

errno_t e_hpux_semctl(int semid, int semnum, int cmd, int arg)
{
	return ESUCCESS;
}

errno_t e_hpux_semget(hpux_key_t key, int nsems, int semflg)
{
	return ESUCCESS;
}

errno_t e_hpux_semop(int semid, void *sops, unsigned int nsops)
{
	return ESUCCESS;
}

errno_t e_hpux_shmat(int id, const void *addr, int shmflg, caddr_t *addr_r)
{
	static int shmfake[1024];

	/*
	 * In order for X11 to work, we need only return a valid address.
	 * If we return fail, X11 exits.  If we return garbage, X11 will
	 * dump core doing a memset(0, 0, 4)... dont ask how I know this.
	 */
	*addr_r = (caddr_t)&shmfake;
	return ESUCCESS;
}

errno_t e_hpux_shmctl(int id, int cmd, hpux_shmid_ds_t *buf)
{
	return ESUCCESS;
}

errno_t e_hpux_shmctl_old(int id, int cmd, hpux_shmid_ds_old_t *buf)
{
	return ESUCCESS;
}

errno_t e_hpux_shmdt(const void *addr)
{
	return ESUCCESS;
}

errno_t e_hpux_shmget(hpux_key_t key, hpux_size_t size, int shmflg, int *id_r)
{
	return ESUCCESS;
}
