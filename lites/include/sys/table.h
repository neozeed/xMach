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
 * 13-Jan-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Added TBL_PROCINFO. Made it number 11 for compat with UX. Made
 *	TBL_ENVIRONMENT 22 instead of 11.
 *
 * $Log: table.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:35  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.1  92/04/21  17:17:53  rwd
 * BSDSS
 * 
 *
 */

#ifndef	_SYS_TABLE_H_
#define _SYS_TABLE_H_

#define TBL_TTYLOC		0	/* index by device number */
#define TBL_U_TTYD		1	/* index by process ID */
#define TBL_UAREA		2	/* index by process ID */
#define TBL_LOADAVG		3	/* (no index) */
#define TBL_INCLUDE_VERSION	4	/* (no index) */
#define TBL_FSPARAM		5	/* index by device number */
#define TBL_ARGUMENTS		6	/* index by process ID */
#define TBL_MAXUPRC		7	/* index by process ID */
#define TBL_AID			8	/* index by process ID */
#define TBL_MODES		9	/* index by process ID */
#define TBL_PROCINFO		10	/* index by proc table slot */
#define	TBL_BOOTTIME		11	/* (no index) */
#define TBL_SYSINFO		12	/* (no index) */
#define TBL_DKINFO		13	/* index by disk */
#define TBL_TTYINFO		14	/* (no index) */
#define TBL_MSGDS		15	/* index by array index */
#define TBL_SEMDS		16	/* index by array index */
#define TBL_SHMDS		17	/* index by array index */
#define TBL_MSGINFO		18	/* index by structure element */
#define TBL_SEMINFO		19	/* index by structure element */
#define TBL_SHMINFO		20	/* index by structure element */
#define TBL_INTR		21	/* (no index) */
#define TBL_ENVIRONMENT		22	/* index by process ID */

#define MSGINFO_MAX		0	/* max message size */
#define MSGINFO_MNB		1	/* max # bytes on queue */
#define MSGINFO_MNI		2	/* # of message queue identifiers */
#define MSGINFO_TQL		3	/* # of system message headers */

#define	SEMINFO_MNI		0	/* # of semaphore identifiers */
#define	SEMINFO_MSL		1	/* max # of semaphores per id */
#define	SEMINFO_OPM		2	/* max # of operations per semop call */
#define	SEMINFO_UME		3	/* max # of undo entries per process */
#define	SEMINFO_VMX		4	/* semaphore maximum value */
#define	SEMINFO_AEM		5	/* adjust on exit max value */

#define SHMINFO_MAX		0	/* max shared memory segment size */
#define SHMINFO_MIN		1	/* min shared memory segment size */
#define SHMINFO_MNI		2	/* num shared memory identifiers */
#define SHMINFO_SEG		3	/* max attached shared memory segments per process */

/*
 * Values for ihs_type -- don't really belong here, but...
 */
					/* interrupt types */
#define INTR_NOSPEC   0x0000
#define INTR_HARDCLK  0x0001
#define INTR_SOFTCLK  0x0002
#define INTR_DEVICE   0x0004
#define INTR_OTHER    0x0008
#define INTR_STRAY    0x8000
#define INTR_DISABLED 0x4000

#define INTR_CLOCK    (INTR_HARDCLK|INTR_SOFTCLK)
#define INTR_NOTCLOCK (~INTR_CLOCK)



/*
 *  TBL_FSPARAM data layout
 */

struct tbl_fsparam
{
    long tf_used;		/* free fragments */
    long tf_iused;		/* free inodes */
    long tf_size;		/* total fragments */
    long tf_isize;		/* total inodes */
};


/*
 *  TBL_LOADAVG data layout
 */

struct tbl_loadavg
{
    union {
	    long   l[3];
	    double d[3];
    } tl_avenrun;
    int    tl_lscale;		/* 0 scale when floating point */
    long   tl_mach_factor[3];
};


/*
 *  TBL_INTR data layout
 */

struct tbl_intr
{
	long   	in_devintr;	/* Device interrupts (non-clock) */
	long   	in_context;	/* Context switches */
	long   	in_syscalls;	/* Syscalls */
	long   	in_forks;	/* Forks */
	long   	in_vforks;	/* Vforks */
};


/*
 *  TBL_MODES bit definitions
 */

#define UMODE_P_GID	01	/* - 4.2 parent GID on inode create */
#define UMODE_NOFOLLOW	02	/* - don't follow symbolic links */
#define UMODE_NONICE	04	/* - don't auto-nice long job */



/*
 *	TBL_PROCINFO data layout
 */
#define PI_COMLEN	19	/* length of command string */
struct tbl_procinfo
{
    int		pi_uid;		/* (effective) user ID */
    int		pi_pid;		/* proc ID */
    int		pi_ppid;	/* parent proc ID */
    int		pi_pgrp;	/* proc group ID */
    dev_t	pi_ttyd;	/* controlling terminal number */
    int		pi_status;	/* process status: */
#define PI_EMPTY	0	    /* no process */
#define PI_ACTIVE	1	    /* active process */
#define PI_EXITING	2	    /* exiting */
#define PI_ZOMBIE	3	    /* zombie */
    int		pi_flag;	/* other random flags */
    char	pi_comm[PI_COMLEN+1];
				/* short command name */
    int		pi_ruid;        /* (real) user ID */
    int		pi_svuid;       /* saved (effective) user ID */
    int         pi_rgid;        /* (real) group ID */
    int         pi_svgid;       /* saved (effective) group ID */
    int		pi_session;	/* session ID */
    int         pi_tpgrp;       /* tty pgrp */
    int         pi_tsession;    /* tty session id */
    int         pi_jobc;        /* # procs qualifying pgrp for job control */
    int         pi_cursig;
    int         pi_sig;         /* signals pending */
    int         pi_sigmask;     /* current signal mask */
    int         pi_sigignore;   /* signals being ignored */
    int         pi_sigcatch;    /* signals being caught by user */
};

/*
 *	TBL_SYSINFO data layout
 */
struct tbl_sysinfo {
        long	si_user;		/* User time */
        long	si_nice;		/* Nice time */
        long	si_sys;			/* System time */
        long	si_idle;		/* Idle time */
        long    si_hz;
        long    si_phz;
	long 	si_boottime;		/* Boot time in seconds */
};

/*
 *	TBL_DKINFO data layout
 */
#define DI_NAMESZ	8
struct tbl_dkinfo {
        int	di_ndrive;
        int	di_busy;
        long	di_time;
        long	di_seek;
        long	di_xfer;
        long	di_wds;
        long	di_wpms;
        int	di_unit;
        char    di_name[DI_NAMESZ+1];
};
        
/*
 *	TBL_TTYINFO data layout
 */
struct tbl_ttyinfo {
        long	ti_nin;
        long	ti_nout;
        long	ti_cancc;
        long	ti_rawcc;
};

#endif	_SYS_TABLE_H_
