/* 
 * Mach Operating System
 * Copyright (c) 1994 Johannes Helander
 * Copyright (c) 1994 Ian Dall
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * JOHANNES HELANDER AND IAN DALL ALLOW FREE USE OF THIS SOFTWARE IN
 * ITS "AS IS" CONDITION.  JOHANNES HELANDER AND IAN DALL DISCLAIM ANY
 * LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
 * USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: e_lite_sysent.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/09/08  06:10:48  mike
 * the "fast" version of sigprocmask (e_bsd_sigprocmask has only 2 args
 *
 * Revision 1.1.1.1  1995/03/02  21:49:28  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_lite_sysent.c
 *	Authors:
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *	Ian Dall, Adelaide, 1994.
 *
 *
 *	System call switch table for 4.4 BSD LITE binaries
 *	on 32 bit machines.
 *
 *	Still missing:
 *	  pathconf, fpathconf, mlock and munlock
 *	  vadvise.
 *
 *      Missing but optional:
 *	  ktrace, LFS  codes (lfs_bmapv, lfs_markv, lfs_segclean, lfs_segwait)
 *
 *      Missing and unlikely to need:
 *	  profil (mach has other ways of doing it)
 *	  4.2 BSD sigreturn  (compatability only). 
 */

#include <e_defs.h>
#include <syscall_table.h>

#define SYS_NULL(routine)	{ E_GENERIC, e_nosys, 0, # routine }
#define sysg(routine, nargs)    { nargs, (int (*)()) routine, 0, # routine }
#define SYS_REG(routine)	{ E_CHANGE_REGS, routine, 0, # routine }

int emul_generic();

int e_nosys();
int e_execv(), getpagesize();
int e_sigaltstack(), e_vread(), e_vwrite(), e_vhangup(), e_vlimit();
int e_getdopt(), e_setdopt(), e_send(), e_recv(), e_vtimes();
int e_sigblock(), e_task_by_pid(), e_pid_by_task(), e_init_process();
int e_sigsetmask(), e_setreuid(), e_setregid(), e_killpg(), e_quota();
int e_htg_syscall(), e_creat(), e_access();
int e_machine_wait(), e_machine_fork(), e_machine_vfork(), e_bnr_sigvec();
int e_bsd_sigreturn(), e_bsd_sigprocmask(), e_sysctl();

int e_bnr_lseek(), e_bnr_stat(), e_bnr_lstat(), e_bnr_fstat();
int e_bnr_truncate(), e_bnr_ftruncate(), e_bnr_getdirentries();
int e_bnr_mmap(), e_bnr_uname(), e_bnr_getrlimit(), e_bnr_setrlimit();
int e_bnr_wait4(), e_bnr_shmsys(), e_bnr_recvmsg(), e_bnr_accept();
int e_bnr_recvfrom(), e_bnr_sendmsg(), e_bnr_getpeername();
int e_bnr_getsockname();

int e_lite_stat(), e_lite_fstat(), e_lite_lstat(), e_lite_lseek();
int e_lite_truncate(), e_lite_ftruncate(), e_lite_getrlimit();
int e_lite_setrlimit(), e_lite_getdirentries();
int e_lite_wait4(), e_lite_mmap();

int e_machine_sigreturn();

int e_mapped_gettimeofday();

struct sysent e_bsd_sysent[] = {
	SYS_NULL(e_indir),		/* 0 */
	sysg(exit, 1),			/* 1 */
	SYS_REG(e_machine_fork),	/* 2 */
	sysg(e_read, 3),		/* 3 */
	sysg(e_write, 3),		/* 4 */
	sysg(e_open, 3),		/* 5 */
	sysg(e_close, 1),		/* 6 */
	sysg(e_lite_wait4, 4),		/* 7 */
	sysg(e_creat, 2),		/* 8 */
	sysg(e_link, 2),		/* 9 */
	sysg(e_unlink, 1),		/* 10 */
	SYS_NULL(e_execv),		/* 11 */
	sysg(e_chdir, 1),		/* 12 */
	sysg(e_fchdir, 1),		/* 13 */
	sysg(e_mknod, 3),		/* 14 */
	sysg(e_chmod, 2),		/* 15 */
	sysg(e_chown, 3),		/* 16 */
	sysg(e_obreak, 1),		/* 17 */
	sysg(e_getfsstat, 3),		/* 18 */
	sysg(e_bnr_lseek, 3),		/* 19 */
	sysg(e_getpid, 0),		/* 20 */
	sysg(e_mount, 4),		/* 21 */
	sysg(e_unmount, 2),		/* 22 */
	sysg(e_setuid, 1),		/* 23 */
	sysg(e_getuid, 0),		/* 24 */
	sysg(e_geteuid, 0),		/* 25 */
	sysg(e_ptrace, 4),		/* 26 */
	sysg(e_recvmsg, 3),		/* 27 */
	sysg(e_sendmsg, 3),		/* 28 */
	sysg(e_recvfrom, 6),		/* 29 */
	sysg(e_accept, 3),		/* 30 */
	sysg(e_getpeername, 3),		/* 31 */
	sysg(e_getsockname, 3),		/* 32 */
	sysg(e_access, 2),		/* 33 */
	sysg(e_chflags, 2),		/* 34 */
	sysg(e_fchflags, 2),		/* 35 */
	sysg(e_sync, 0),		/* 36 */
	sysg(e_kill, 2),		/* 37 */
	sysg(e_bnr_stat, 2),		/* 38 */
	sysg(e_getppid, 0),		/* 39 */
	sysg(e_bnr_lstat, 2),		/* 40 */
	sysg(e_dup, 1),			/* 41 */
	sysg(e_pipe, 0),		/* two return values	   42 */
	sysg(e_getegid, 0),		/* 43 */
	sysg(e_nosys, 4),		/* 44 profil unimplemented */
	sysg(e_nosys, 0),		/* 45 ktrace unimplemented*/
	sysg(e_sigaction, 3),		/* 46 */
	sysg(e_getgid, 0),		/* 47 */
	sysg(e_bsd_sigprocmask, 2),	/* 48 */
	sysg(e_getlogin, 2),		/* 49 */
	sysg(e_setlogin, 1),		/* 50 */
	sysg(e_sysacct, 1),		/* 51 */
	sysg(e_sigpending, 1),		/* 52 */
	sysg(e_nosys, 3),		/* 53 */
	sysg(e_ioctl, 3),		/* 54 */
	sysg(e_reboot, 1),		/* 55 */
	sysg(e_revoke, 1),		/* 56 */
	sysg(e_symlink, 2),		/* 57 */
	sysg(e_readlink, 3),		/* 58 */
	sysg(e_execve, 3),		/* 59 */
	sysg(e_umask, 1),		/* 60 */
	sysg(e_chroot, 1),		/* 61 */
	sysg(e_bnr_fstat, 2),		/* 62 */
	sysg(e_getkerninfo, 4),		/* 63 */
	sysg(e_getpagesize, 0),		/* 64 */
	sysg(e_msync, 2),		/* 65 */
	SYS_REG(e_machine_vfork),	/* 66 */
	sysg(e_vread, 0),		/* 67 */
	sysg(e_vwrite, 0),		/* 68 */
	sysg(e_sbrk, 1),		/* 69 */
	sysg(e_nosys, 1),		/* 70 */
	sysg(e_bnr_mmap, 6),		/* 71 */
	sysg(e_nosys, 0),		/* 72 vadvise unimplemented */
	sysg(e_munmap, 2),		/* 73 */
	sysg(e_mprotect, 3),		/* 74 */
	sysg(e_madvise, 3),		/* 75 */
	sysg(e_vhangup, 0),		/* 76 */
	sysg(e_vlimit, 0),		/* 77 */
	sysg(e_mincore, 3),		/* 78 */
	sysg(e_getgroups, 2),		/* 79 */
	sysg(e_setgroups, 2),		/* 80 */
	sysg(e_getpgrp, 0),		/* 81 */
	sysg(e_setpgid, 2),		/* 82 */
	sysg(e_setitimer, 3),		/* 83 */
	SYS_REG(e_machine_wait),	/* 84 */
	sysg(e_swapon, 1),		/* 85 */
	sysg(e_getitimer, 2),		/* 86 */
	sysg(e_gethostname, 2),		/* 87 */
	sysg(e_sethostname, 2),		/* 88 */
	sysg(e_getdtablesize, 0),	/* 89 */
	sysg(e_dup2, 2),		/* 90 */
	sysg(e_getdopt, 2),		/* 91 */
	sysg(e_fcntl, 3),		/* 92 */
	sysg(e_select, 5),		/* 93 */
	sysg(e_setdopt, 2),		/* 94 */
	sysg(e_fsync, 1),		/* 95 */
	sysg(e_setpriority, 3),		/* 96 */
	sysg(e_socket, 3),		/* 97 */
	sysg(e_connect, 3),		/* 98 */
	sysg(e_bnr_accept, 3),		/* 99 */
	sysg(e_getpriority, 2),		/* 100 */
	sysg(e_send, 4),		/* 101 */
	sysg(e_recv, 4),		/* 102 */
#ifdef parisc
	SYS_REG(e_machine_sigreturn),	/* 103 */
#else
	sysg(e_bsd_sigreturn, 1),	/* 103 */
#endif
	sysg(e_bind, 3),		/* 104 */
	sysg(e_setsockopt, 5),		/* 105 */
	sysg(e_listen, 2),		/* 106 */
	sysg(e_vtimes, 0),		/* 107 */
	sysg(e_bnr_sigvec, 3),		/* 108 (the tramp arg isn't used) */
	sysg(e_sigblock, 1),		/* 109 */
	sysg(e_sigsetmask, 1),		/* 110 */
	sysg(e_sigsuspend, 1),		/* 111 */
	sysg(e_sigstack, 2),		/* 112 */
	sysg(e_bnr_recvmsg, 3),		/* 113 */
	sysg(e_bnr_sendmsg, 3),		/* 114 */
	sysg(e_nosys, 2),		/* 115 vtrace unimplemented*/
	sysg(e_mapped_gettimeofday, 2),	/* 116 */
	sysg(e_getrusage, 2),		/* 117 */
	sysg(e_getsockopt, 5),		/* 118 */
	sysg(e_nosys, 0),		/* 119 */
	sysg(e_readv, 3),		/* 120 */
	sysg(e_writev, 3),		/* 121 */
	sysg(e_settimeofday, 2),	/* 122 */
	sysg(e_fchown, 3),		/* 123 */
	sysg(e_fchmod, 2),		/* 124 */
	sysg(e_bnr_recvfrom, 6),	/* 125 */
	sysg(e_setreuid, 2),		/* 126 */
	sysg(e_setregid, 2),		/* 127 */
	sysg(e_rename, 2),		/* 128 */
	sysg(e_bnr_truncate, 2),	/* 129 */
	sysg(e_bnr_ftruncate, 2),	/* 130 */
	sysg(e_flock, 2),		/* 131 */
	sysg(e_mkfifo, 2),		/* 132 */
	sysg(e_sendto, 6),		/* 133 */
	sysg(e_shutdown, 2),		/* 134 */
	sysg(e_socketpair, 4),		/* 135 */
	sysg(e_mkdir, 2),		/* 136 */
	sysg(e_rmdir, 1),		/* 137 */
	sysg(e_utimes, 2),		/* 138 */
	sysg(e_nosys, 0),		/* 139 4.2 sigreturn unimplemented*/
	sysg(e_adjtime, 2),		/* 140 */
	sysg(e_bnr_getpeername, 3),	/* 141 */
	sysg(e_gethostid, 0),		/* 142 */
	sysg(e_sethostid, 1),		/* 143 */
	sysg(e_bnr_getrlimit, 2),	/* 144 */
	sysg(e_bnr_setrlimit, 2),	/* 145 */
	sysg(e_killpg, 2),		/* 146 */
	sysg(e_setsid, 0),		/* 147 */
	sysg(e_quotactl, 4),		/* 148 */
	sysg(e_quota, 4),		/* 149 */
	sysg(e_bnr_getsockname, 3),	/* 150 */
	sysg(e_nosys, 0),		/* 151 */
	sysg(e_nosys, 0),		/* 152 */
	sysg(e_nosys, 0),		/* 153 */
	sysg(e_nosys, 0),		/* 154 */
	sysg(e_nfssvc, 5),		/* 155 */
	sysg(e_bnr_getdirentries, 4),	/* 156 */
	sysg(e_statfs, 2),		/* 157 */
	sysg(e_fstatfs, 2),		/* 158 */
	sysg(e_nosys, 0),		/* 159 */
	sysg(e_async_daemon, 0),	/* 160 */
	sysg(e_getfh, 2),		/* 161 */
	sysg(e_getdomainname, 2),	/* 162 */
	sysg(e_setdomainname, 2),	/* 163 */
	sysg(e_bnr_uname, 1),		/* 164 */
	sysg(e_nosys, 0),		/* 165 */
	sysg(e_nosys, 0),		/* 166 */
	sysg(e_nosys, 0),		/* 167 */
	sysg(e_nosys, 0),		/* 168 */
	sysg(e_nosys, 0),		/* 169 */
	sysg(e_nosys, 0),		/* 170 */
	sysg(e_bnr_shmsys, 4),		/* 171 */
	sysg(e_nosys, 0),		/* 172 */
	sysg(e_nosys, 0),		/* 173 */
	sysg(e_nosys, 0),		/* 174 */
	sysg(e_nosys, 0),		/* 175 */
	sysg(e_nosys, 0),		/* 176 */
	sysg(e_nosys, 0),		/* 177 */
	sysg(e_nosys, 0),		/* 178 */
	sysg(e_nosys, 0),		/* 179 */
	sysg(e_nosys, 0),		/* 180 */
	sysg(e_setgid, 1),		/* 181 */
	sysg(e_setegid, 1),		/* 182 */
	sysg(e_seteuid, 1),		/* 183 */

	/* 184 -- 187 are LFS specific (and unimplemented) */
	sysg(e_nosys, 0),		/* 184 */
	sysg(e_nosys, 0),		/* 185 */
	sysg(e_nosys, 0),		/* 186 */
	sysg(e_nosys, 0),		/* 187 */

	sysg(e_lite_stat, 2),		/* 188 */
	sysg(e_lite_fstat, 2),		/* 189 */
	sysg(e_lite_lstat, 2),		/* 190 */
	sysg(e_nosys, 0),		/* 191 pathconf unimplemented */
	sysg(e_nosys, 0),		/* 192 fpathconf unimplemented */
	sysg(e_nosys, 0),		/* 193 */
	sysg(e_lite_getrlimit, 2),	/* 194 */
	sysg(e_lite_setrlimit, 2),	/* 195 */
	sysg(e_lite_getdirentries, 4),	/* 196 */
	sysg(e_lite_mmap, 8),		/* 197 */
	sysg(e_nosys, 0),		/* 198 */
	sysg(e_lite_lseek, 5),		/* 199 */
	sysg(e_lite_truncate, 4),	/* 200 */
	sysg(e_lite_ftruncate, 4),	/* 201 */
	sysg(e_sysctl, 6),		/* 202 */
	sysg(e_nosys, 0),		/* 203 mlock unimplemented */
	sysg(e_nosys, 0),		/* 204 munlock unimplemented */
};

int	e_bsd_nsysent = sizeof(e_bsd_sysent)/sizeof(struct sysent);

struct sysent   sysent_task_by_pid	= sysg(e_task_by_pid, 1);
struct sysent   sysent_pid_by_task	= sysg(e_pid_by_task, 4);
struct sysent   sysent_htg_ux_syscall	= SYS_REG(e_htg_syscall);
struct sysent   sysent_init_process	= sysg(e_init_process, 1);
struct sysent	null_sysent 		= sysg(e_nosys, 0);
