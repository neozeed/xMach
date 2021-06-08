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
 * $Log: e_isc4_sysent.c,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:29  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_isc4_sysent.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	System call switch table for Interactive binaries.
 */

#include <e_defs.h>
#include <syscall_table.h>

#define SYS_NULL(routine)	{ E_GENERIC, e_nosys, 0, # routine }
#define syss(routine, nargs)    { nargs, (int (*)()) routine, 0, # routine }
#define sysg(routine, nargs)    { nargs, (int (*)()) routine, 0, # routine }
#define SYS_REG(routine)	{ E_CHANGE_REGS, routine, 0, # routine }
#define SYSE(routine)           { E_GENERIC, (int (*)()) routine,0, # routine }

int emul_generic();

int e_nosys();
int e_execv(), getpagesize();
int e_sigaltstack(), e_vread(), e_vwrite(), e_vhangup(), e_vlimit();
int e_getdopt(), e_setdopt(), e_send(), e_recv(), e_vtimes();
int e_sigblock(), e_task_by_pid(), e_pid_by_task(), e_init_process();
int e_sigsetmask(), e_setreuid(), e_setregid(), e_killpg(), e_quota();
int e_htg_syscall(), e_creat(), e_access();
int e_machine_wait(), e_machine_fork(), e_machine_vfork(), e_bnr_sigvec();

int e_sysv_fstatfs(), e_sysv_statfs(), e_sysv_nice(), e_sysv_stime();
int e_sysv_signal(), e_sysv_alarm(), e_sysv_time();
int e_sysv_lstat(), e_sysv_stat(), e_sysv_fstat(), e_isc4_sysi86();

int e_bnr_lseek(), e_bnr_stat(), e_bnr_lstat(), e_bnr_fstat();
int e_bnr_truncate(), e_bnr_ftruncate(), e_bnr_getdirentries();
int e_bnr_wait4();

struct sysent e_isc4_sysent[] = {
	SYS_NULL(e_indir),					/* 0 */
	sysg(exit, 1),						/* 1 */
	SYS_REG(e_machine_fork),				/* 2 */
	sysg(e_read, 3),					/* 3 */
	syss(e_write, 3),					/* 4 */
	syss(e_open, 3),					/* 5 */
	sysg(e_close, 1),					/* 6 */
	sysg(e_bnr_wait4, 4),	/* WAIT */			/* 7 */
	syss(e_creat, 2),					/* 8 */
	syss(e_link, 2),					/* 9 */
	syss(e_unlink, 1),					/* 10 */
	SYS_NULL(e_execv),	/* EXEC */			/* 11 */
	syss(e_chdir, 1),					/* 12 */
	sysg(e_sysv_time, 1),	/* TIME */			/* 13 */
	syss(e_mknod, 3),					/* 14 */
	syss(e_chmod, 2),					/* 15 */
	syss(e_chown, 3),					/* 16 */
	sysg(e_obreak, 1),					/* 17 */
	sysg(e_sysv_stat, 2),	/* STAT */			/* 18 */
	sysg(e_bnr_lseek, 3),					/* 19 */
	sysg(e_getpid, 0),					/* 20 */
	syss(e_mount, 4),					/* 21 */
	syss(e_unmount, 2),					/* 22 */
	sysg(e_setuid, 1),					/* 23 */
	sysg(e_getuid, 0),					/* 24 */
	sysg(e_sysv_stime, 1),	/* STIME */			/* 25 */
	sysg(e_ptrace, 4),					/* 26 */
	sysg(e_sysv_alarm, 1),	/* ALARM */			/* 27 */
	sysg(e_sysv_fstat, 2),	/* FSTAT */			/* 28 */
	sysg(e_nosys, 0),	/* PAUSE */			/* 29 */
	syss(e_utimes, 2),	/* UTIME  XXX */		/* 30 */
	sysg(e_nosys, 0),	/* STTY */			/* 31 */
	sysg(e_nosys, 0),	/* GTTY */			/* 32 */
	syss(e_access, 2),					/* 33 */
	sysg(e_sysv_nice, 1),	/* NICE */			/* 34 */
	syss(e_sysv_statfs, 2),	/* STATFS */			/* 35 */
	syss(e_sync, 0),					/* 36 */
	sysg(e_kill, 2),					/* 37 */
	syss(e_sysv_fstatfs, 2),/* FSTATFS */			/* 38 */
	sysg(e_nosys, 0),	/* GETPGRP */			/* 39 */
	sysg(e_nosys, 0),	/* CXENIX */			/* 40 */
	sysg(e_dup, 1),						/* 41 */
	sysg(e_pipe, 0),	/* two return values */		/* 42 */
	sysg(e_nosys, 0),	/* TIMES */			/* 43 */
	sysg(e_nosys, 0),	/* PROF */			/* 44 */
	sysg(e_nosys, 0),	/* LOCK/PLOCK */		/* 45 */
	sysg(e_setgid, 1),	/* SETGID */			/* 46 */
	sysg(e_getgid, 0),					/* 47 */
	sysg(e_sysv_signal, 2),	/* SIGNAL */			/* 48 */ 
	sysg(e_nosys, 0),	/* MSGSYS */			/* 49 */
	sysg(e_isc4_sysi86, 2),	/* SYSI86 */			/* 50 */
	syss(e_sysacct, 1),					/* 51 */
	sysg(e_nosys, 0),	/* SHMSYS */			/* 52 */
	sysg(e_nosys, 0),	/* SEMSYS */			/* 53 */
	syss(e_ioctl, 3),					/* 54 */
	sysg(e_nosys, 0),	/* UADMIN */			/* 55 */
	sysg(e_nosys, 0),	/* NO */			/* 56 */
	sysg(e_nosys, 0),	/* UTSSYS */			/* 57 */
	syss(e_fsync, 1),	/* FSYNC XXX */			/* 58 */
	sysg(e_execve, 3),	/* EXECE */			/* 59 */
	sysg(e_umask, 1),					/* 60 */
	syss(e_chroot, 1),					/* 61 */
	syss(e_fcntl, 3),	/* FCNTL XXX */			/* 62 */
	sysg(e_nosys, 0),	/* ULIMIT */			/* 63 */
	sysg(e_nosys, 0),	/* NO */			/* 64 */
	sysg(e_nosys, 0),	/* NO */			/* 65 */
	sysg(e_nosys, 0),	/* NO */			/* 66 */
	sysg(e_nosys, 0),	/* NO */			/* 67 */
	sysg(e_nosys, 0),	/* NO */			/* 68 */
	sysg(e_nosys, 0),	/* NO */			/* 69 */
	sysg(e_nosys, 1),	/* ADVFS */			/* 70 */
	sysg(e_nosys, 0),	/* UNADVFS */			/* 71 */
	sysg(e_nosys, 0),	/* RMOUNT */			/* 72 */
	sysg(e_nosys, 0),	/* RUNMOUNT */			/* 73 */
	sysg(e_nosys, 0),	/* RFSTART */			/* 74 */
	sysg(e_nosys, 0),	/* NO */			/* 75 */
	sysg(e_nosys, 0),	/* RDEBUG */			/* 76 */
	sysg(e_nosys, 0),	/* RFSTOP */			/* 77 */
	sysg(e_nosys, 0),	/* RFSYS */			/* 78 */
	syss(e_rmdir, 1),	/* RMDIR XXX */			/* 79 */
	syss(e_mkdir, 2),	/* MKDIR XXX */			/* 80 */
	sysg(e_nosys, 0),	/* GETDENTS */			/* 81 */
	sysg(e_nosys, 0),	/* NO */			/* 82 */
	sysg(e_nosys, 0),	/* NO */			/* 83 */
	sysg(e_nosys, 0),	/* SYSFS */			/* 84 */
	sysg(e_nosys, 0),	/* GETMSG */			/* 85 */
	sysg(e_nosys, 0),	/* PUTMSG */			/* 86 */
	sysg(e_nosys, 0),	/* POLL */			/* 87 */
	sysg(e_nosys, 0),	/* NO */			/* 88 */
	sysg(e_nosys, 0),	/* NO */		        /* 89 */
	syss(e_symlink, 2),	/* SYMLINK XXX */		/* 90 */
	sysg(e_sysv_lstat, 2),	/* LSTAT */			/* 91 */
	syss(e_readlink, 3),	/* READLINK XXX */		/* 92 */
	sysg(e_nosys, 0),	/* NO */			/* 93 */
	sysg(e_nosys, 0),	/* NO */			/* 94 */
	sysg(e_nosys, 0),	/* NO */			/* 95 */
	sysg(e_nosys, 0),	/* NO */			/* 96 */
	sysg(e_nosys, 0),	/* NO */			/* 97 */
	sysg(e_nosys, 0),	/* NO */			/* 98 */
	sysg(e_nosys, 0),	/* NO */			/* 99 */
	sysg(e_nosys, 0),	/* NO */			/* 100 */
	sysg(e_nosys, 0),	/* NO */			/* 101 */
	sysg(e_nosys, 0),	/* NO */			/* 102 */
	sysg(e_nosys, 0),	/* NO */			/* 103 */
	sysg(e_nosys, 0),	/* NO */			/* 104 */
	sysg(e_nosys, 0),	/* SYSISC */			/* 105 */
	sysg(e_nosys, 0),	/* NO */			/* 106 */
	sysg(e_nosys, 0),	/* NO */			/* 107 */
	sysg(e_nosys, 0),	/* NO */			/* 108 */
	sysg(e_nosys, 0),	/* NO */			/* 109 */
	sysg(e_nosys, 0),	/* NO */			/* 110 */
	sysg(e_nosys, 0),	/* NO */			/* 111 */
	sysg(e_nosys, 0),	/* NO */			/* 112 */
	sysg(e_nosys, 0),	/* NO */			/* 113 */
	sysg(e_nosys, 0),	/* NO */			/* 114 */
	sysg(e_nosys, 0),	/* NO */			/* 115 */
	sysg(e_nosys, 0),	/* NO */			/* 116 */
	sysg(e_nosys, 0),	/* NO */			/* 117 */
	sysg(e_nosys, 0),	/* NO */			/* 118 */
	sysg(e_nosys, 0),	/* NO */			/* 119 */
	sysg(e_nosys, 0),	/* NO */			/* 120 */
	sysg(e_nosys, 0),	/* NO */			/* 121 */
	sysg(e_nosys, 0),	/* NO */			/* 122 */
	sysg(e_nosys, 0),	/* NO */			/* 123 */
	sysg(e_nosys, 0),	/* NO */			/* 124 */
	sysg(e_nosys, 0),	/* NO */			/* 125 */
	sysg(e_nosys, 0),	/* NO */			/* 126 */
	sysg(e_nosys, 0),	/* CLOCAL */			/* 127 */
};

int	e_isc4_nsysent = sizeof(e_isc4_sysent)/sizeof(struct sysent);
