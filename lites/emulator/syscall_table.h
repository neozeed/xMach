/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994 Johannes Helander
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND JOHANNES HELANDER ALLOW FREE USE OF THIS
 * SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND JOHANNES
 * HELANDER DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: syscall_table.h,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/09/01  23:27:54  mike
 * HP-UX compatibility support from Jeff F.
 *
 * Revision 1.1.1.1  1995/03/02  21:49:28  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File: emulator/syscall_table.h
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Definition of system call table.
 */

struct sysent {
	int	nargs;		/* number of arguments, or special code */
	int	(*routine)();
	int	binary_patched;	/* Is the SVC replaced by a CALL? */
	char	*name;
};

/*
 * Special arguments:
 */
#define	E_GENERIC	(-1)
				/* no specialized routine */
#define	E_CHANGE_REGS	(-2)
				/* may change registers */

/*
 * Exported system call table
 */
extern struct sysent	e_bsd_sysent[];	/* normal system calls */
extern int		e_bsd_nsysent;
extern struct sysent	e_cmu_43ux_sysent[];
extern int		e_cmu_43ux_nsysent;
extern struct sysent	e_isc4_sysent[];
extern int		e_isc4_nsysent;
extern struct sysent	e_linux_sysent[];
extern int		e_linux_nsysent;
extern struct sysent	e_hpbsd_sysent[];
extern int		e_hpbsd_nsysent;
extern struct sysent	e_hpux_sysent[];
extern int		e_hpux_nsysent;
extern struct sysent	e_ultrix_sysent[];
extern int		e_ultrix_nsysent;
extern struct sysent	e_osf1_sysent[];
extern int		e_osf1_nsysent;


extern struct sysent	sysent_task_by_pid;
extern struct sysent	sysent_pid_by_task;
extern struct sysent	sysent_htg_ux_syscall;
extern struct sysent	sysent_init_process;
extern struct sysent	sysent_table;
extern struct sysent	null_sysent;

extern struct sysent *current_sysent;
extern int current_nsysent;
