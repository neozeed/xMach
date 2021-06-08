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
 *	Utah $Hdr: e_hpux.h 1.2 95/09/02$
 */

/*
** Miscellaneous #define's.
*/

#define	HPUX_HZ			100	/* HP-UX clock ticks / second */
#define	HPUX_NOFILE		60	/* max number of open files/process */
#define	HPUX_DOMAINNAME_SIZE	64	/* for get/setdomainname() */

#define	HPUX_OEAGAIN		82	/* map EWOULDBLOCK to HP-UX EAGAIN */
#define	HPUX_BERR		1000	/* BSD errno that cant map to HP-UX */
#define	BSD_NERR	(___ELAST+2)	/* 1 for 0, plus 1 for HPUX_OEAGAIN */

/*
** Miscellaneous typedef's.
*/

typedef	char *		hpux_caddr_t;
typedef unsigned long	hpux_clock_t;
typedef	unsigned short	hpux_cnode_t;
typedef	long		hpux_dev_t;
typedef	long		hpux_fsid_t[2];
typedef	long		hpux_gid_t;
typedef	unsigned long	hpux_ino_t;
typedef long		hpux_key_t;
typedef	unsigned short	hpux_mode_t;
typedef	short		hpux_nlink_t;
typedef	long		hpux_off_t;
typedef	long		hpux_pid_t;
typedef	unsigned short	hpux_site_t;
typedef	unsigned int	hpux_size_t;
typedef	long		hpux_time_t;
typedef	long		hpux_uid_t;

/*
** Signal support.
*/

#define	HPUX_NSIG		32	/* HP-UX signal count (counting 0) */

#define	hpux_sigmask(m)	(1L << ((m)-1))

#define	HPUX_SV_ONSTACK		0x1
#define	HPUX_SV_BSDSIG		0x2
#define	HPUX_SV_RESETHAND	0x4

#define	HPUX_SA_ONSTACK		0x1
#define	HPUX_SA_RESETHAND	0x4
#define	HPUX_SA_NOCLDSTOP	0x8

#define	HPUX_KILL_ALL_OTHERS	((hpux_pid_t) 0x7fff)
#define	HPUX_PA83_CONTEXT	0x1
#define	HPUX_PA89_CONTEXT	0x2
#define	HPUX_SHAREDLIBS		0x2

typedef	struct {
	long		sigset[8];
} hpux_sigset_t;

typedef struct {
	void		(*sa_handler)();
	hpux_sigset_t	sa_mask;
	int		sa_flags;
} hpux_sigaction_t;

typedef struct {
	int		fm_edp;		/* external dp */
	int		fm_esr4;	/* external sr4 */
	int		fm_erp;		/* external rp */
	int		fm_crp;		/* current rp */
	int		fm_sl;		/* static link */
	int		fm_clup;	/* clean up */
	int		fm_ep;		/* extension ptr */
	int		fm_psp;		/* previous sp */
} hpux_framemarker_t;

#ifdef parisc		/* ...else will not compile on other architectures */
typedef struct {
	struct	parisc_thread_state ss_state;	/* original exception frame */
	int	ss_cr16, ss_cr23;	/* part of HP-UX save_state struct */
	int	ss_syscall;		/* intrp'd system call number */
	int	ss_sysargs[4];		/* args to intrp'd syscall */
} hpux_sigstate_t;

typedef struct {
	int		sf_signum;		/* signo for handler */
	int		sf_code;		/* more info for handler */
	struct		sigcontext *sf_scp;	/* context ptr for handler */
	void		(*sf_handler)();	/* handler addr for u_sigc */
	hpux_sigstate_t	sf_state;		/* state of the hardware */
	struct		sigcontext sf_sc;	/* actual context */
	int		fixed_args[4];		/* fixed arg save area */
	hpux_framemarker_t sf_fm;		/* frame marker */
} hpux_sigframe_t;
#endif	/* parisc */

/*
** Definitions related to various system calls.
*/

#define	HPUX_FF_NDELAY		00000004
#define	HPUX_FF_FCREAT		00000400
#define	HPUX_FF_FTRUNC		00001000
#define	HPUX_FF_FEXCL		00002000
#define	HPUX_FF_FSYNCIO		00100000
#define	HPUX_FF_NONBLOCK	00200000
#define	HPUX_FF_FREMOTE		01000000

#define	HPUX_FF_NONBLOCK_ON	0x10
#define	HPUX_FF_FNDELAY_ON	0x20
#define	HPUX_FF_FIONBIO_ON	0x40

#define	HPUX_MAP_SHARED		0x0001	/* for e_hpux_mmap */
#define	HPUX_MAP_PRIVATE	0x0002
#define	HPUX_MAP_FIXED		0x0004
#define	HPUX_MAP_VARIABLE	0x0008
#define	HPUX_MAP_ANONYMOUS	0x0010
#define	HPUX_MAP_FILE		0x0020

#define	HPUX_SOSND_COPYAVOID	0x1009	/* for e_hpux_{get,set}sockopt */
#define	HPUX_SORCV_COPYAVOID	0x100A

#define HPUX_SYSCONF_ARGMAX	0	/* for e_hpux_sysconf */
#define HPUX_SYSCONF_CLDMAX	1
#define	HPUX_SYSCONF_CLKTICK	2
#define	HPUX_SYSCONF_OPENMAX	4
#define	HPUX_SYSCONF_CPUTYPE	10001
#define	HPUX_SYSCONF_CPUPA10	0x20B
#define	HPUX_SYSCONF_CPUPA11	0x210
#define HPUX_SYSCONF_PAGESIZE	0xbb9

#define	HPUX_RTPRIO_MIN		0	/* for e_hpux_rtprio */
#define	HPUX_RTPRIO_MAX		127
#define	HPUX_RTPRIO_NOCHG	1000
#define	HPUX_RTPRIO_RTOFF	1001

/*
** Structures (and definitions) related to various system calls.
*/

typedef struct {		/* for e_hpux_fcntl */
	short		hl_type;
	short		hl_whence;
	hpux_off_t	hl_start;
	hpux_off_t	hl_len;
	hpux_pid_t	hl_pid;
} hpux_flock_t;
#define	HPUX_F_GETLK		5
#define	HPUX_F_SETLK		6
#define	HPUX_F_SETLKW		7

typedef struct {		/* for e_hpux_poll */
	int fd;
	short events;
	short revents;
} hpux_pollfd_t;
#define	HPUX_POLLIN		0x0001
#define	HPUX_POLLPRI		0x0002
#define	HPUX_POLLOUT		0x0004
#define	HPUX_POLLERR		0x0008
#define	HPUX_POLLHUP		0x0010
#define	HPUX_POLLNVAL		0x0020
#define	HPUX_POLLRDNORM		0x0040
#define	HPUX_POLLRDBAND		0x0080
#define	HPUX_POLLWRNORM		0x0100
#define	HPUX_POLLWRBAND		0x0200
#define	HPUX_POLLMSG		0x0400

typedef struct {		/* for e_hpux_{get,set}rlimit */
	int		rlim_cur;	/* current (soft) limit */
	int		rlim_max;	/* hard limit */
} hpux_rlimit_t;
#define	HPUX_RLIMIT_NOFILE	6

typedef struct {		/* for e_hpux_getrusage */
	struct timeval	ru_utime;	/* user time used */
	struct timeval	ru_stime;	/* system time used */
	long		ru_maxrss;
#define	hpux_ru_first	ru_ixrss
	long		ru_ixrss;	/* integral shared memory size */
	long		ru_idrss;	/* integral unshared data " */
	long		ru_isrss;	/* integral unshared stack " */
	long		ru_minflt;	/* page reclaims */
	long		ru_majflt;	/* page faults */
	long		ru_nswap;	/* swaps */
	long		ru_inblock;	/* block input operations */
	long		ru_oublock;	/* block output operations */
	long		ru_ioch;	/* i/o characters (NOT IN BSD) */
	long		ru_msgsnd;	/* messages sent */
	long		ru_msgrcv;	/* messages received */
	long		ru_nsignals;	/* signals received */
	long		ru_nvcsw;	/* voluntary context switches */
	long		ru_nivcsw;	/* involuntary " */
#define	hpux_ru_last	ru_nivcsw
} hpux_rusage_t;

typedef struct {		/* for e_hpux_{stat,lstat,fstat} */
	hpux_dev_t	hst_dev;
	hpux_ino_t	hst_ino;
	hpux_mode_t	hst_mode;
	hpux_nlink_t	hst_nlink;
	u_short		hst_ouid;
	u_short		hst_ogid;
	hpux_dev_t	hst_rdev;
	hpux_off_t	hst_size;
	hpux_time_t	hst_atime;
	int		hst_spare1;
	hpux_time_t	hst_mtime;
	int		hst_spare2;
	hpux_time_t	hst_ctime;
	int		hst_spare3;
	long		hst_blksize;
	long		hst_blocks;
	u_int		hst_remote;
	hpux_dev_t	hst_netdev;
	hpux_ino_t	hst_netino;
	hpux_cnode_t	hst_cnode;
	hpux_cnode_t	hst_rcnode;
	hpux_site_t	hst_netsite;
	short		hst_fstype;
	long		hst_realdev;
	u_short		hst_basemode;
	u_short		hst_spareshort;
	hpux_uid_t	hst_uid;
	hpux_gid_t	hst_gid;
	long		hst_spare4[3];
} hpux_stat_t;

typedef struct {		/* for e_hpux_fstatfs */
	long		f_type;
	long		f_bsize;
	long		f_blocks;
	long		f_bfree;
	long		f_bavail;
	long		f_files;
	long		f_ffree;
	hpux_fsid_t	f_fsid;
	long		f_magic;
	long		f_featurebits;
	long		f_spare[4];
	hpux_site_t	f_cnode;
	short		f_pad;
} hpux_statfs_t;
#define	HPUX_MOUNT_UFS		0
#define	HPUX_MOUNT_NFS		1
#define	HPUX_MOUNT_CDFS		2
#define	HPUX_MOUNT_PC		3
#define	HPUX_MOUNT_DCFS		4

typedef struct {		/* for e_hpux_ftime */
	hpux_time_t	time;
	unsigned short	millitm;
	short		timezone;
	short		dstflag;
} hpux_timeb_t;

typedef struct {		/* for e_hpux_times */
	hpux_clock_t	tms_utime;
	hpux_clock_t	tms_stime;
	hpux_clock_t	tms_cutime;
	hpux_clock_t	tms_cstime;
} hpux_tms_t;

typedef struct {		/* for e_hpux_utime */
	hpux_time_t	actime;
	hpux_time_t	modtime;
} hpux_utimbuf_t;

typedef struct {		/* for e_hpux_utssys */
	char		sysname[9];
	char		nodename[9];
	char		release[9];
	char		version[9];
	char		machine[9];
	char		idnumber[15];
} hpux_utsname_t;

/*
** Ioctl's.
*/

#define	HPUX_TIOCSLTC	_IOW('T', 23, struct ltchars)
#define	HPUX_TIOCGLTC	_IOR('T', 24, struct ltchars)
#define	HPUX_TIOCLBIS	_IOW('T', 25, int)
#define	HPUX_TIOCLBIC	_IOW('T', 26, int)
#define	HPUX_TIOCLSET	_IOW('T', 27, int)
#define	HPUX_TIOCLGET	_IOR('T', 28, int)
#	define	HPUX_LTOSTOP	0000001
#define	HPUX_TIOCSPGRP	_IOW('T', 29, int)
#define	HPUX_TIOCGPGRP	_IOR('T', 30, int)
#define	HPUX_TIOCCONS	_IO('t', 104)
#define	HPUX_TIOCSWINSZ	_IOW('t', 106, struct winsize)
#define	HPUX_TIOCGWINSZ	_IOR('t', 107, struct winsize)

/* non-blocking IO; doesn't interfere with O_NDELAY */
#define	HPUX_FIOSNBIO	_IOW('f', 118, int)

/*
** HP-UX termio.
*/

#define	HPUX_NCC	8

typedef struct {
	u_short	c_iflag;	/* input modes */
	u_short	c_oflag;	/* output modes */
	u_short	c_cflag;	/* control modes */
	u_short	c_lflag;	/* line discipline modes */
	char    c_line;		/* line discipline */
	u_char	c_cc[HPUX_NCC];	/* control chars */
} hpux_termio_t;

/* control characters */
#define	HPUX_VINTR	0
#define	HPUX_VQUIT	1
#define	HPUX_VERASE	2
#define	HPUX_VKILL	3
#define	HPUX_VEOF	4
#define	HPUX_VEOL	5
#define	HPUX_VMIN	4
#define	HPUX_VTIME	5
#define	HPUX_VEOL2	6
#define	HPUX_VSWTCH	7

/* input modes */
#define	HPUX_TIO_IGNBRK	0x00000001	/* 0000001 */
#define	HPUX_TIO_BRKINT	0x00000002	/* 0000002 */
#define	HPUX_TIO_IGNPAR	0x00000004	/* 0000004 */
#define	HPUX_TIO_PARMRK	0x00000008	/* 0000010 */
#define	HPUX_TIO_INPCK	0x00000010	/* 0000020 */
#define	HPUX_TIO_ISTRIP	0x00000020	/* 0000040 */
#define	HPUX_TIO_INLCR	0x00000040	/* 0000100 */
#define	HPUX_TIO_IGNCR	0x00000080	/* 0000200 */
#define	HPUX_TIO_ICRNL	0x00000100	/* 0000400 */
#define	HPUX_TIO_IUCLC	0x00000200	/* 0001000 */
#define	HPUX_TIO_IXON	0x00000400	/* 0002000 */
#define	HPUX_TIO_IXANY	0x00000800	/* 0004000 */
#define	HPUX_TIO_IXOFF	0x00001000	/* 0010000 */
#define	HPUX_TIO_IENQAK	0x00002000	/* 0020000 */

/* output modes */
#define	HPUX_TIO_OPOST	0x00000001	/* 0000001 */
#define	HPUX_TIO_OLCUC	0x00000002	/* 0000002 */
#define	HPUX_TIO_ONLCR	0x00000004	/* 0000004 */
#define	HPUX_TIO_OCRNL	0x00000008	/* 0000010 */
#define	HPUX_TIO_ONOCR	0x00000010	/* 0000020 */
#define	HPUX_TIO_ONLRET	0x00000020	/* 0000040 */
#define	HPUX_TIO_OFILL	0x00000040	/* 0000100 */
#define	HPUX_TIO_OFDEL	0x00000080	/* 0000200 */
#define	HPUX_TIO_NLDLY	0x00000100	/* 0000400 */
#define	HPUX_TIO_NL0	0
#define	HPUX_TIO_NL1	0x00000100	/* 0000400 */
#define	HPUX_TIO_CRDLY	0x00000600	/* 0003000 */
#define	HPUX_TIO_CR0	0
#define	HPUX_TIO_CR1	0x00000200	/* 0001000 */
#define	HPUX_TIO_CR2	0x00000400	/* 0002000 */
#define	HPUX_TIO_CR3	0x00000600	/* 0003000 */
#define	HPUX_TIO_TABDLY	0x00001800	/* 0014000 */
#define	HPUX_TIO_TAB0	0
#define	HPUX_TIO_TAB1	0x00000800	/* 0004000 */
#define	HPUX_TIO_TAB2	0x00001000	/* 0010000 */
#define	HPUX_TIO_TAB3	0x00001800	/* 0014000 */
#define	HPUX_TIO_BSDLY	0x00002000	/* 0020000 */
#define	HPUX_TIO_BS0	0
#define	HPUX_TIO_BS1	0x00002000	/* 0020000 */
#define	HPUX_TIO_VTDLY	0x00004000	/* 0040000 */
#define	HPUX_TIO_VT0	0
#define	HPUX_TIO_VT1	0x00004000	/* 0040000 */
#define	HPUX_TIO_FFDLY	0x00008000	/* 0100000 */
#define	HPUX_TIO_FF0	0
#define	HPUX_TIO_FF1	0x00008000	/* 0100000 */

/* control modes */
#define	HPUX_TIO_CBAUD	0x0000001f	/* 0000037 */
#define	HPUX_TIO_B0	0
#define	HPUX_TIO_B50	0x00000001	/* 0000001 */
#define	HPUX_TIO_B75	0x00000002	/* 0000002 */
#define	HPUX_TIO_B110	0x00000003	/* 0000003 */
#define	HPUX_TIO_B134	0x00000004	/* 0000004 */
#define	HPUX_TIO_B150	0x00000005	/* 0000005 */
#define	HPUX_TIO_B200	0x00000006	/* 0000006 */
#define	HPUX_TIO_B300	0x00000007	/* 0000007 */
#define	HPUX_TIO_B600	0x00000008	/* 0000010 */
#define	HPUX_TIO_B900	0x00000009	/* 0000011 */
#define	HPUX_TIO_B1200	0x0000000a	/* 0000012 */
#define	HPUX_TIO_B1800	0x0000000b	/* 0000013 */
#define	HPUX_TIO_B2400	0x0000000c	/* 0000014 */
#define	HPUX_TIO_B3600	0x0000000d	/* 0000015 */
#define	HPUX_TIO_B4800	0x0000000e	/* 0000016 */
#define	HPUX_TIO_B7200	0x0000000f	/* 0000017 */
#define	HPUX_TIO_B9600	0x00000010	/* 0000020 */
#define	HPUX_TIO_B19200	0x00000011	/* 0000021 */
#define	HPUX_TIO_B38400	0x00000012	/* 0000022 */
#define	HPUX_TIO_B57600	0x00000013	/* 0000023 */
#define	HPUX_TIO_B115200 0x00000014	/* 0000024 */
#define	HPUX_TIO_B230400 0x00000015	/* 0000025 */
#define	HPUX_TIO_B460800 0x00000016	/* 0000026 */
#define	HPUX_TIO_EXTA	0x0000001e	/* 0000036 */
#define	HPUX_TIO_EXTB	0x0000001f	/* 0000037 */
#define	HPUX_TIO_CSIZE	0x00000060	/* 0000140 */
#define	HPUX_TIO_CS5	0
#define	HPUX_TIO_CS6	0x00000020	/* 0000040 */
#define	HPUX_TIO_CS7	0x00000040	/* 0000100 */
#define	HPUX_TIO_CS8	0x00000060	/* 0000140 */
#define	HPUX_TIO_CSTOPB	0x00000080	/* 0000200 */
#define	HPUX_TIO_CREAD	0x00000100	/* 0000400 */
#define	HPUX_TIO_PARENB	0x00000200	/* 0001000 */
#define	HPUX_TIO_PARODD	0x00000400	/* 0002000 */
#define	HPUX_TIO_HUPCL	0x00000800	/* 0004000 */
#define	HPUX_TIO_CLOCAL	0x00001000	/* 0010000 */
#define	HPUX_TIO_CRTS  	0x00002000	/* 0020000 */ /* Obsolete */

/* line discipline 0 modes */
#define	HPUX_TIO_ISIG	0x00000001	/* 0000001 */
#define	HPUX_TIO_ICANON	0x00000002	/* 0000002 */
#define	HPUX_TIO_XCASE	0x00000004	/* 0000004 */
#define	HPUX_TIO_ECHO	0x00000008	/* 0000010 */
#define	HPUX_TIO_ECHOE	0x00000010	/* 0000020 */
#define	HPUX_TIO_ECHOK	0x00000020	/* 0000040 */
#define	HPUX_TIO_ECHONL	0x00000040	/* 0000100 */
#define	HPUX_TIO_NOFLSH	0x00000080	/* 0000200 */

#define	HPUX_TCGETA	_IOR('T', 1, hpux_termio_t)
#define	HPUX_TCSETA	_IOW('T', 2, hpux_termio_t)
#define	HPUX_TCSETAW	_IOW('T', 3, hpux_termio_t)
#define	HPUX_TCSETAF	_IOW('T', 4, hpux_termio_t)

/*
** HP-UX termios.
*/

#define	HPUX_NCCS	16

typedef struct {
	u_int	c_iflag;	/* input modes */
	u_int	c_oflag;	/* output modes */
	u_int	c_cflag;	/* control modes */
	u_int	c_lflag;	/* line discipline modes */
	u_int	c_reserved;	/* future use */
	u_char	c_cc[HPUX_NCCS];	/* control chars */
} hpux_termios_t;

/* control characters */
#define	HPUX_VMINS	11	/* different than termio */
#define	HPUX_VTIMES	12	/* different than termio */
#define	HPUX_VSUSP	13
#define	HPUX_VSTART	14
#define	HPUX_VSTOP	15

#define	HPUX_TCGETATTR	_IOR('T', 16, hpux_termios_t)
#define	HPUX_TCSETATTR	_IOW('T', 17, hpux_termios_t)
#define	HPUX_TCSETATTRD	_IOW('T', 18, hpux_termios_t)
#define	HPUX_TCSETATTRF	_IOW('T', 19, hpux_termios_t)

/*
** SysV-style shared memory (not yet implemented).
*/

#define	HPUX_IPC_RMID	0		/* remove id */
#define	HPUX_IPC_SET	1		/* set options */
#define	HPUX_IPC_STAT	2		/* get options */
#define	HPUX_SHM_LOCK	3		/* lock down segment */
#define	HPUX_SHM_UNLOCK	4		/* unlock segment */

#define	HPUX_IPC_CREAT	0x00200		/* like O_CREAT */
#define	HPUX_IPC_EXCL	0x00400		/* like O_EXCL */
#define	HPUX_IPC_NOWAIT	0x00800		/* do not block */
#define	HPUX_SHM_RDONLY	0x01000		/* O_RDONLY (else O_RDWR) */

typedef struct {
	hpux_uid_t	uid;		/* owner's user id */
	hpux_gid_t	gid;		/* owner's group id */
	hpux_uid_t	cuid;		/* creator's user id */
	hpux_gid_t	cgid;		/* creator's group id */
	hpux_mode_t	mode;		/* access modes */
	u_short		seq;		/* slot usage sequence number */
	hpux_key_t	key;		/* key */
	u_short		unused1;
	u_short		unused2;
} hpux_ipc_perm_t;

typedef struct {
	hpux_ipc_perm_t	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* segment size (bytes) */
	void *		shm_ptbl;	/* ptr to associated page table */
	hpux_pid_t	shm_lpid;	/* pid of last shmop */
	hpux_pid_t	shm_cpid;	/* pid of creator */
	u_short		shm_nattch;	/* current # attached */
	u_short		shm_cnattch;	/* in memory # attached */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */
	char		shm_pad[24];
} hpux_shmid_ds_t;

typedef struct {
	u_short		uid;		/* user id */
	u_short		gid;		/* group id */
	u_short		cuid;		/* creator user id */
	u_short		cgid;		/* creator group id */
	u_short		mode;		/* r/w permission */
	u_short		seq;		/* sequence # (to gen unique id) */
	hpux_key_t	key;		/* user specified msg/sem/shm key */
	u_short		unused1;
	u_short		unused2;
} hpux_ipc_perm_old_t;

typedef struct {
	hpux_ipc_perm_old_t shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* segment size (bytes) */
	void *		shm_ptbl;	/* ptr to associated page table */
	u_short		shm_lpid;	/* pid of last shmop */
	u_short		shm_cpid;	/* pid of creator */
	u_short		shm_nattch;	/* current # attached */
	u_short		shm_cnattch;	/* in memory # attached */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */
	char		shm_pad[148];
} hpux_shmid_ds_old_t;

/*
** Things we save across exec (passed parent to child).
*/
typedef struct {
	int fdflags[HPUX_NOFILE];
	char domainname[HPUX_DOMAINNAME_SIZE+1];
} hpux_exsave_t;

extern hpux_exsave_t		__hpux_exsave;
#define	__hpux_fdflags		__hpux_exsave.fdflags
#define	__hpux_domainname	__hpux_exsave.domainname
