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
 * $Log: e_linux_sysent.c,v $
 * Revision 1.4  2000/11/26 01:35:55  welchd
 *
 * Fixed to getdents interface
 *
 * Revision 1.3  2000/11/07 00:41:29  welchd
 *
 * Added support for executing dynamically linked Linux ELF binaries
 *
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/09/03  21:22:16  gback
 * added support for Linux mount and reboot
 *
 * Revision 1.1.1.1  1995/03/02  21:49:29  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_linux_sysent.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	System call switch table for Linux compat.
 */

#include <e_defs.h>
#include <syscall_table.h>

#define SYS_NULL(routine)	{ E_GENERIC, e_nosys, 0, # routine }
#define sysg(routine, nargs)    { nargs, (int (*)()) routine, 0, # routine }
#define SYS_REG(routine)	{ E_CHANGE_REGS, routine, 0, # routine }
#define SYSG(routine, nargs)	SYS_NULL(routine)

int emul_generic();

int e_nosys();
int e_execv(), getpagesize();
int e_sigaltstack(), e_vread(), e_vwrite(), e_vhangup(), e_vlimit();
int e_getdopt(), e_setdopt(), e_vtimes();
int e_sigblock(), e_task_by_pid(), e_pid_by_task(), e_init_process();
int e_sigsetmask(), e_setreuid(), e_setregid(), e_quota();
int e_htg_syscall(), e_creat(), e_access();
int e_machine_wait(), e_machine_fork(), e_machine_vfork(), e_bnr_sigvec();

int e_sysv_fstatfs(), e_sysv_statfs(), e_sysv_nice(), e_sysv_stime();
int e_sysv_signal(), e_sysv_alarm(), e_sysv_lstat(), e_sysv_stat();
int e_isc4_lstat(), e_isc4_stat(), e_isc4_sysi86(), e_sysv_time();
int e_linux_reboot(), e_linux_mount();
int e_linux_uselib(), e_linux_mmap(), e_linux_brk(), e_linux_stat();
int e_linux_lstat(), e_linux_fstat(), e_linux_readdir(), e_linux_open();
int e_linux_sigprocmask(), e_linux_sigaction(), e_linux_ioctl();
int e_linux_select(), e_linux_socketcall(), e_linux_uname();
int e_linux_sigreturn(), e_linux_sysvipc(), e_linux_kill(), e_linux_wait4();

int e_linux_getdents();

int e_linux_getcwd();

int e_bnr_lseek(), e_bnr_stat(), e_bnr_lstat(), e_bnr_fstat();
int e_bnr_truncate(), e_bnr_ftruncate(), e_bnr_getdirentries();
int e_bnr_getrlimit(), e_bnr_setrlimit(), e_bnr_wait4();

int e_mapped_gettimeofday();

int e_linux_pause();
int e_linux_nanosleep();
int e_linux_stat64();
int e_linux_fstat64();
int e_linux_lstat64();
int e_linux_lchown32();
int e_linux_fcntl64();

struct sysent e_linux_sysent[] = {
	SYS_NULL(e_setup),					/* 0 */
	sysg(exit, 1),						/* 1 */
	SYS_REG(e_machine_fork),					/* 2 */
	sysg(e_read, 3),					/* 3 */
	sysg(e_write, 3),					/* 4 */
	sysg(e_linux_open, 3),					/* 5 */
	sysg(e_close, 1),					/* 6 */
	SYSG(e_sysv_waitpid, 3),				/* 7 */
	sysg(e_creat, 2),					/* 8 */
	sysg(e_link, 2),					/* 9 */
	sysg(e_unlink, 1),					/* 10 */
	sysg(e_execve, 3),					/* 11 */
	sysg(e_chdir, 1),					/* 12 */
	sysg(e_sysv_time, 1),	/* TIME */			/* 13 */
	sysg(e_mknod, 3),					/* 14 */
	sysg(e_chmod, 2),					/* 15 */
	sysg(e_chown, 3),					/* 16 */
	sysg(e_obreak, 1),					/* 17 */
	SYSG(e_linux_stat, 3),	/* STAT */			/* 18 */
	sysg(e_bnr_lseek, 3),					/* 19 */
	sysg(e_getpid, 0),					/* 20 */
	sysg(e_linux_mount, 5),					/* 21 */
	sysg(e_unmount, 2),					/* 22 */
	sysg(e_setuid, 1),					/* 23 */
	sysg(e_getuid, 0),					/* 24 */
	sysg(e_sysv_stime, 1),	/* STIME */			/* 25 */
	sysg(e_ptrace, 4),					/* 26 */
	sysg(e_sysv_alarm, 1),	/* ALARM */			/* 27 */
	sysg(e_linux_fstat, 2),	/* FSTAT */			/* 28 */
	sysg(e_linux_pause, 0),	/* PAUSE */			/* 29 */
	sysg(e_utimes, 2),	/* UTIME */			/* 30 */
	SYSG(e_sysv_stty, ?),	/* STTY */			/* 31 */
	SYSG(e_sysv_gtty, ?),	/* GTTY */			/* 32 */
	sysg(e_access, 2),					/* 33 */
	sysg(e_sysv_nice, 1),	/* NICE */			/* 34 */
	SYSG(e_linux_ftime, ?),					/* 35 */
	sysg(e_sync, 0),					/* 36 */
	sysg(e_linux_kill, 2),					/* 37 */
	sysg(e_rename, 2),					/* 38 */
	sysg(e_mkdir, 1),					/* 39 */
	sysg(e_rmdir, 1),					/* 40 */
	sysg(e_dup, 1),						/* 41 */
	sysg(e_pipe, 1),	/* !=BSD: No return value */	/* 42 */
	SYSG(e_sysv_times, 0),	/* TIMES */			/* 43 */
	SYSG(e_sysv_prof, ?),	/* PROF */			/* 44 */
	sysg(e_linux_brk, 1),					/* 45 */
	sysg(e_setgid, 1),	/* SETGID */			/* 46 */
	sysg(e_getgid, 0),					/* 47 */
	sysg(e_sysv_signal, 2),	/* SIGNAL */			/* 48 */ 
	sysg(e_geteuid, 0),					/* 49 */
	sysg(e_getegid, 0),					/* 50 */
	sysg(e_sysacct, 1),					/* 51 */
	SYSG(e_linux_phys, ?),					/* 52 */
	SYSG(e_sysv_lock, 0),					/* 53 */
	sysg(e_linux_ioctl, 3),					/* 54 */
	sysg(e_fcntl, 3),					/* 55 */
	SYSG(e_linux_mpx, ?),					/* 56 */
	sysg(e_setpgid, 2),					/* 57 */
	SYSG(e_linux_ulimit, ?),				/* 58 */
	sysg(e_linux_uname, 1),					/* 59 */
	sysg(e_umask, 1),					/* 60 */
	sysg(e_chroot, 1),					/* 61 */
	SYSG(e_linux_ustat, ?),					/* 62 */
	sysg(e_dup2, 2),					/* 63 */
	sysg(e_getppid, 0),					/* 64 */
	sysg(e_getpgrp, 0),					/* 65 */
	sysg(e_setsid, 0),					/* 66 */
	sysg(e_linux_sigaction, 3),				/* 67 */
	SYSG(e_linux_sgetmask, ?),				/* 68 */
	SYSG(e_linux_ssetmask, ?),				/* 69 */
	sysg(e_setreuid, 2),					/* 70 */
	sysg(e_setregid, 2),					/* 71 */
	sysg(e_sigsuspend, 1),					/* 72 */
	sysg(e_sigpending, 1),					/* 73 */
	sysg(e_sethostname, 2),					/* 74 */
	sysg(e_bnr_setrlimit, 2),				/* 75 */
	sysg(e_bnr_getrlimit, 2),				/* 76 */
	sysg(e_getrusage, 2),					/* 77 */
	sysg(e_mapped_gettimeofday, 2),				/* 78 */
	sysg(e_settimeofday, 2),				/* 79 */
	sysg(e_getgroups, 2),					/* 80 */
	sysg(e_setgroups, 2),					/* 81 */
	sysg(e_linux_select, 1),				/* 82 */
	sysg(e_symlink, 2),					/* 83 */
	SYSG(e_linux_lstat, ?),					/* 84 */
	sysg(e_readlink, 3),					/* 85 */
	sysg(e_linux_uselib, 1),				/* 86 */
	sysg(e_swapon, 1),					/* 87 */
	sysg(e_linux_reboot, 3),				/* 88 */
	sysg(e_linux_readdir, 3),			        /* 89 */
	sysg(e_linux_mmap, 1),	/* args in buffer */		/* 90 */
	sysg(e_munmap, 2),			 		/* 91 */
	sysg(e_bnr_truncate, 2),				/* 92 */
	sysg(e_bnr_ftruncate, 2),				/* 93 */
	sysg(e_fchmod, 2),					/* 94 */
	sysg(e_fchown, 3),					/* 95 */
	sysg(e_getpriority, 2),					/* 96 */
	sysg(e_setpriority, 3),					/* 97 */
	SYSG(e_linux_profil, ?),	       			/* 98 */
	sysg(e_sysv_statfs, 2),					/* 99 */
	sysg(e_sysv_fstatfs, 2),				/* 100 */
	SYSG(e_linux_ioperm, ?),				/* 101 */
	sysg(e_linux_socketcall, 2),				/* 102 */
	SYSG(e_linux_syslog, ?),				/* 103 */
	sysg(e_getitimer, 2),					/* 104 */
	sysg(e_setitimer, 3),					/* 105 */
	sysg(e_linux_stat, 2),					/* 106 */
	sysg(e_linux_lstat, 2),					/* 107 */
	sysg(e_linux_fstat, 2),					/* 108 */
	sysg(e_linux_uname, 1),		/* new uname */		/* 109 */
	SYSG(e_linux_iopl, ?),					/* 110 */
	sysg(e_vhangup, 0),					/* 111 */
	SYSG(e_linux_idle, ?),					/* 112 */
	SYSG(e_linux_vm86, ?),					/* 113 */
	sysg(e_linux_wait4, 4),					/* 114 */
	SYSG(e_linux_swapoff, ?),				/* 115 */
	SYSG(e_linux_sysinfo, ?),				/* 116 */
	sysg(e_linux_sysvipc, 5),				/* 117 */
	sysg(e_fsync, 1),					/* 118 */
	sysg(e_linux_sigreturn, 1),				/* 119 */
	sysg(e_setdomainname, 2),				/* 120 */
	sysg(e_linux_uname, 1),		/* old iuname */	/* 121 */
	sysg(e_linux_uname, 1),					/* 122 */
	SYSG(e_linux_modify_ldt, ?),				/* 123 */
	SYSG(e_linux_adjtimex, ?),				/* 124 */
	sysg(e_mprotect, 3),					/* 125 */
	sysg(e_linux_sigprocmask, 3),				/* 126 */
	SYSG(e_linux_create_module, ?),				/* 127 */
	SYSG(e_linux_init_module, ?),				/* 128 */
	SYSG(e_linux_delete_module, ?),				/* 129 */
	SYSG(e_linux_get_kernel_syms, ?),			/* 130 */
	sysg(e_quotactl, 4),					/* 131 */
	SYSG(e_sysv_getpgid, ?),				/* 132 */
	sysg(e_fchdir, 1),					/* 133 */
	SYSG(e_linux_bdflush, ?),				/* 134 */
        SYSG(e_linux_sysfs, ?),                                 /* 135 */
        SYSG(e_linux_personality, ?),                           /* 136 */
        SYSG(e_linux_afs_syscall, ?),                           /* 137 */
        SYSG(e_linux_setfsuid, ?),                              /* 138 */
        SYSG(e_linux_setfsgid, ?),                              /* 139 */
        SYSG(e_linux__llseek, ?),                               /* 140 */
        sysg(e_linux_getdents, 3),                              /* 141 */
        SYSG(e_linux__newselect, ?),                            /* 142 */
        SYSG(e_linux_flock, ?),                                 /* 143 */
        SYSG(e_linux_msync, ?),                                 /* 144 */
        SYSG(e_linux_readv, ?),                                 /* 145 */
        SYSG(e_linux_writev, ?),                                /* 146 */
        SYSG(e_linux_getsid, ?),                                /* 147 */
        SYSG(e_linux_fdatasync, ?),                             /* 148 */
        SYSG(e_linux__sysctl, ?),                               /* 149 */
        SYSG(e_linux_mlock, ?),                                 /* 150 */
        SYSG(e_linux_munlock, ?),                               /* 151 */
        SYSG(e_linux_mlockall, ?),                              /* 152 */
        SYSG(e_linux_munlockall, ?),                            /* 153 */
        SYSG(e_linux_sched_setparam, ?),                        /* 154 */
        SYSG(e_linux_sched_getparam, ?),                        /* 155 */
        SYSG(e_linux_sched_setscheduler, ?),                    /* 156 */
        SYSG(e_linux_sched_getscheduler, ?),                    /* 157 */
        SYSG(e_linux_sched_yield, ?),                           /* 158 */
        SYSG(e_linux_sched_get_priority_max, ?),                /* 159 */
        SYSG(e_linux_sched_get_priority_min, ?),                /* 160 */
        SYSG(e_linux_sched_get_rr_interval, ?),                 /* 161 */
        sysg(e_linux_nanosleep, 2),                             /* 162 */
        SYSG(e_linux_mremap, ?),                                /* 163 */
        SYSG(e_linux_setresuid, ?),                             /* 164 */
        SYSG(e_linux_getresuid, ?),                             /* 165 */
        SYSG(e_linux_vm86, ?),                                  /* 166 */
        SYSG(e_linux_query_module, ?),                          /* 167 */
        SYSG(e_linux_poll, ?),                                  /* 168 */
        SYSG(e_linux_nfsservctl, ?),                            /* 169 */
        SYSG(e_linux_setresgid, ?),                             /* 170 */
        SYSG(e_linux_getresgid, ?),                             /* 171 */
        SYSG(e_linux_prctl, ?),                                 /* 172 */
        SYSG(e_linux_rt_sigreturn, ?),                          /* 173 */
        SYSG(e_linux_rt_sigaction, ?),                          /* 174 */
        SYSG(e_linux_rt_sigprocmask, ?),                        /* 175 */
        SYSG(e_linux_rt_sigpending, ?),                         /* 176 */
        SYSG(e_linux_rt_sigtimedwait, ?),                       /* 177 */
        SYSG(e_linux_rt_sigqueueinfo, ?),                       /* 178 */
        SYSG(e_linux_rt_sigsuspend, ?),                         /* 179 */
        SYSG(e_linux_pread, ?),                                 /* 180 */
        SYSG(e_linux_pwrite, ?),                                /* 181 */
        SYSG(e_linux_chown, ?),                                 /* 182 */
        sysg(e_linux_getcwd, 2),                                /* 183 */
        SYSG(e_linux_capget, ?),                                /* 184 */
        SYSG(e_linux_capset, ?),                                /* 185 */
        SYSG(e_linux_sigaltstack, ?),                           /* 186 */
        SYSG(e_linux_sendfile, ?),                              /* 187 */
        SYSG(e_linux_getpmsg, ?),                               /* 188 */
        SYSG(e_linux_setpmsg, ?),                               /* 189 */
        SYSG(e_linux_vfork, ?),                                 /* 190 */
        SYSG(e_linux_ugetrlimit, ?),                            /* 191 */
        SYSG(e_linux_mmap2, ?),                                 /* 192 */
        SYSG(e_linux_truncate64, ?),                            /* 193 */
        SYSG(e_linux_ftruncate64, ?),                           /* 194 */
        sysg(e_linux_stat64, 3),                                /* 195 */
        sysg(e_linux_lstat64, 3),                               /* 196 */
        sysg(e_linux_fstat64, 3),                               /* 197 */
        sysg(e_linux_lchown32, 3),                              /* 198 */
        SYSG(e_linux_getuid32, ?),                              /* 199 */
        SYSG(e_linux_getgid32, ?),                              /* 200 */
        SYSG(e_linux_geteuid32, ?),                             /* 201 */
        SYSG(e_linux_getegid32, ?),                             /* 202 */
        SYSG(e_linux_setreuid32, ?),                            /* 203 */
        SYSG(e_linux_setregid32, ?),                            /* 204 */
        SYSG(e_linux_getgroups32, ?),                           /* 205 */
        SYSG(e_linux_setgroups32, ?),                           /* 206 */
        SYSG(e_linux_fchown32, ?),                              /* 207 */
        SYSG(e_linux_setresuid32, ?),                           /* 208 */
        SYSG(e_linux_getresuid32, ?),                           /* 209 */
        SYSG(e_linux_setresgid32, ?),                           /* 210 */
        SYSG(e_linux_getresgid32, ?),                           /* 211 */
        SYSG(e_linux_chown32, ?),                               /* 212 */
        SYSG(e_linux_setuid32, ?),                              /* 213 */
        SYSG(e_linux_setgid32, ?),                              /* 214 */
        SYSG(e_linux_setfsuid32, ?),                            /* 215 */
        SYSG(e_linux_setfsgid32, ?),                            /* 216 */
        SYSG(e_linux_pivot_root, ?),                            /* 217 */
        SYSG(e_linux_mincore, ?),                               /* 218 */
        SYSG(e_linux_madvise, ?),                               /* 219 */
        SYSG(e_linux_getdents64, ?),                            /* 220 */
        sysg(e_linux_fcntl64, 3),                               /* 221 */
        SYSG(e_linux_invalid, ?),                               /* 222 */
};

int	e_linux_nsysent = sizeof(e_linux_sysent)/sizeof(struct sysent);
