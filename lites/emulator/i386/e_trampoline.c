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
 * 24-Oct-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Added support for 8 argument system calls.
 *	Detect 4.4 indirect syscall.
 *
 * $Log: e_trampoline.c,v $
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
 * Revision 1.3  1996/03/14  21:07:47  sclawson
 * Ian Dall's signal fixes.
 *
 * Revision 1.2  1995/08/15  06:48:38  sclawson
 * modifications from lites-1.1-950808
 *
 * Revision 1.1.1.2  1995/03/23  01:15:39  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	emulator/i386/e_trampoline.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *	Origin:	Partially based on CMU's old emulator.
 *
 *	Handler for emulated BSD and SysV system calls.
 */

#define KERNEL
#include <sys/errno.h>
#undef KERNEL

#include <e_defs.h>
#include <syscall_table.h>
#include <i386/eflags.h>

int	emul_low_entry = -9;
//int	emul_high_entry = 210;
int     emul_high_entry = 222;

extern emul_trampoline();
extern void emul_linux_setup3(task_t task, enum binary_type binary_type);

int current_nsysent = 0;
struct sysent *current_sysent = 0;

int	emul_linux_trap = 0;

void emul_setup(task_t task, enum binary_type binary_type)
{
	int i;
        kern_return_t	rc;


	switch (binary_type) {
	      case BT_LINUX:
	      case BT_LINUX_O:
	      case BT_LINUX_SHLIB:
	      case BT_LINUX_ELF:
	      case BT_LINUX_DYN_ELF:
		/* Linux uses a nonstandard system call calling convention */
	        if (emul_linux_trap) {
		    e_linux_setup(mach_task_self(), binary_type);
		    return;
		}
	      default:
		/* code below */
	}
   
   	switch (binary_type) {
	      case BT_LINUX:
	      case BT_LINUX_O:
	      case BT_LINUX_SHLIB:
	      case BT_LINUX_ELF:
	      case BT_LINUX_DYN_ELF:
		/* Linux uses a nonstandard system call calling convention */
	            emul_linux_setup3(mach_task_self(), binary_type);
		    return;
	      default:
		/* code below */
	}
  
   
	for (i = emul_low_entry;
	     i <= emul_high_entry;
	     i++) {
		rc = task_set_emulation(task,
					(vm_address_t) emul_trampoline,
					i);
	}
	rc = task_set_emulation(task,
			(vm_address_t) emul_trampoline,
			-33);
	rc = task_set_emulation(task,
			(vm_address_t) emul_trampoline,
			-34);
	rc = task_set_emulation(task,
			(vm_address_t) emul_trampoline,
			-41);
	rc = task_set_emulation(task,
			(vm_address_t) emul_trampoline,
			-52);

	switch (binary_type) {
	      case BT_LINUX:
	      case BT_LINUX_O:
	      case BT_LINUX_SHLIB:
	      case BT_LINUX_ELF:
	        current_nsysent = e_linux_nsysent;
		current_sysent = e_linux_sysent;
		break;
	      case BT_CMU_43UX:
		current_nsysent = e_cmu_43ux_nsysent;
		current_sysent = e_cmu_43ux_sysent;
		break;
	      case BT_ISC4:
		current_nsysent = e_isc4_nsysent;
		current_sysent = e_isc4_sysent;
		break;
	      case BT_386BSD:
	      case BT_NETBSD:
	      case BT_FREEBSD:
	      default:
		current_nsysent = e_bsd_nsysent;
		current_sysent = e_bsd_sysent;
		break;
	}
}

struct emul_regs {
	int	fp;
	int	edi;
	int	esi;
	int	ebx;

	int	edx;
	int	ecx;
	int	eax;
	int	ps;
	int	pc;
};

errno_t e_htg_syscall()
{
	return e_mach_error_to_errno(LITES_ENOSYS);
}

/*
 * Wait has a weird parameter passing mechanism.
 */
errno_t
e_machine_wait(
	int			*argp,
	int			*rval,
	struct emul_regs	*regs)
{
	errno_t err;
	short wpid;
	int istat;

#define	EFL_ALLCC	(EFL_CF|EFL_PF|EFL_ZF|EFL_SF)
#define WAIT_ANY (-1)

	if ((regs->ps & EFL_ALLCC) == EFL_ALLCC) {
#if 0
	    new_args[0] = WAIT_ANY;	/* pid */
	    new_args[1] = argp[0];	/* istat */
	    new_args[2] = regs2->ecx;	/* options (also argp[1]) */
	    new_args[3] = regs2->edx;	/* rusage_p (also argp[2]) */
#endif
	    err = e_bnr_wait4(WAIT_ANY, &istat, regs->ecx,
			      (struct rusage *) regs->edx, &wpid);
	} else {
	    err = e_bnr_wait4(WAIT_ANY, &istat, 0, 0, &wpid);
	}
	rval[0] = wpid;
	rval[1] = istat;	/* sic */
	return err;
}

#if defined(COMPAT_43) && 0		/* OBSOLETE */
/* 
 * Compatibility with historical brain damage.
 * Fortunately new code doesn't use sigvec so it is not necessary to
 * be compatible with that simultaneously.
 */
int OLD_i386_e_sigvec(
	int			*argp,
	int			*rval,
	struct emul_regs	*regs)
{
	sigvec_t nsv, spare;
	sigvec_t *osv = (sigvec_t *) argp[2];
	/* if osv is null assume caller was not interested in it */
	if (!osv)
	    osv = &spare;
	if (argp[1])
	    nsv = *(sigvec_t *) argp[1];

	return bsd_osigvec(our_bsd_server_port,
			   argp[0],			/* sig */
			   !! (boolean_t) argp[1],	/* nsv is valid? */
			   nsv,
			   osv,
			   regs->edx & ~0x80000000); /* trampoline */
}

#if 0
int i386_e_sigvec(
	int			*argp,
	int			*rval,
	struct emul_regs	*regs)
{
	return e_bnr_sigvec((int) argp[0],
			    (struct sigvec *) argp[1],	/* nsv */
			    (struct sigvec *) argp[2],	/* osv */
			    (vm_address_t) regs->edx & ~0x80000000);/* tramp */
}
#endif /* 0 */
#endif /* COMPAT_43 */

errno_t e_machine_fork(
	int			*argp,
	int			*rval,
	struct emul_regs	*regs)
{
	errno_t err;

	err = e_fork((pid_t *) rval);
	if (err)
	    return err;
	if (*rval)
	    rval[1] = 0;
	else
	    rval[1] = 1;
	return ESUCCESS;
}

errno_t e_machine_vfork(
	int			*argp,
	int			*rval,
	struct emul_regs	*regs)
{
	errno_t err;

	err = e_vfork((pid_t *) rval);
	if (err)
	    return err;
	if (*rval)
	    rval[1] = 0;	/* parent edx = 0 */
	else
	    rval[1] = 1;	/* child edx = 1 */
	return ESUCCESS;
}

errno_t i386_e_htg_syscall()
{
	return e_mach_error_to_errno(LITES_ENOSYS);
}

/*
 * System calls enter here.
 */
void
emul_syscall(struct emul_regs *regs)
{
	int syscode;
	int error;
	struct sysent *callp;
	int rval[2];
	boolean_t interrupt;
	int *args;

restart_system_call:
	interrupt = FALSE;
	error = ESUCCESS;
   
        args = (int *)(regs);
        if (syscall_debug > 2)
	    e_emulator_error("emul_syscall (x%x, x%x, x%x, x%x, x%x, x%x, x%x, x%x)",
			     args[0], args[1], args[2],
			     args[3], args[4], args[5], args[6], args[7]);
   
	args = (int *)(regs+1);	/* args on stack top */
	args++;	/* point to first argument - skip return address */

	syscode = regs->eax;

#if defined(MAP_UAREA) && 0
	if (shared_enabled) {
		if (shared_base_ro->us_cursig) {
			error = ERESTART;
			goto signal;
		}
	}
#endif	MAP_UAREA

 	/*
 	 * Check for indirect system call.
 	 */
 	switch (syscode) {
	      case 0 /* SYS_syscall */:
		  syscode = *args++;
		  break;
		case 198 /* SYS___syscall */:
		    /* code is a quad argument with the high order bits 0.
		     * Endian dependent XXX
		     */
		    syscode = *args;
		  args += 2;
		  break;
	  }

	/*
	 * Find system call table entry for the system call.
	 */
	if (syscode >= current_nsysent)
	    callp = &null_sysent;
	else if (syscode >= 0)
	    callp = &current_sysent[syscode];
	else {
	    /*
	     * Negative system call numbers are CMU extensions.
	     */
	    if (syscode == -33)
		callp = &sysent_task_by_pid;
	    else if (syscode == -6)
		callp = &e_cmu_43ux_sysent[SYS_table];
	    else if (syscode == -34)
		callp = &sysent_pid_by_task;
	    else if (syscode == -41)
		callp = &sysent_init_process;
	    else if (syscode == -59)
		callp = &sysent_htg_ux_syscall;
	    else
		callp = &null_sysent;
	}

	if (syscall_debug > 2)
	    e_emulator_error("emul_syscall[%d] %s(x%x, x%x, x%x, x%x, x%x, x%x, x%x, x%x)",
			     syscode, callp->name, args[0], args[1], args[2],
			     args[3], args[4], args[5], args[6], args[7]);
	/*
	 * Set up the initial return values.
	 */
	rval[0] = 0;
	rval[1] = regs->edx;

	/*
	 * Call the routine, passing arguments according to the table
	 * entry.
	 */
	switch (callp->nargs) {
	    case 0:
		error = (*callp->routine)(rval);
		break;
	    case 1:
		error = (*callp->routine)(args[0], rval);
		break;
	    case 2:
		error = (*callp->routine)(
				args[0], args[1],
				rval);
		break;
	    case 3:
		error = (*callp->routine)(
				args[0], args[1], args[2],
				rval);
		break;
	    case 4:
		error = (*callp->routine)(
				args[0], args[1], args[2], args[3],
				rval);
		break;
	    case 5:
		error = (*callp->routine)(
				args[0], args[1], args[2], args[3], args[4],
				rval);
		break;
	    case 6:
		error = (*callp->routine)(
				args[0], args[1], args[2],
				args[3], args[4], args[5],
				rval);
		break;
	    case 7:
		error = (*callp->routine)(
				args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],
				rval);
		break;
	    case 8:
		error = (*callp->routine)(
				args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],
				args[7],
				rval);
		break;

	    case -1:	/* generic */
		error = (*callp->routine)(
				syscode,
				args,
				rval);
		break;

	    case -2:	/* pass registers to modify */
		error = (*callp->routine)(
				args,
				rval,
				regs);
		break;
	}

	/*
	 * Set up return values.
	 */

#ifdef	MAP_UAREA
signal:
#endif	MAP_UAREA

	switch (error) {
	    case EJUSTRETURN:
		if (syscall_debug > 1)
		    e_emulator_error("JUSTRETURN[%d] %s", syscode,callp->name);
		/* Do not alter registers */
		break;

	    case 0:
		/* Success */
		regs->ps &= ~EFL_CF;
		regs->eax = rval[0];
		regs->edx = rval[1];
		if (syscall_debug > 2)
		    e_emulator_error("return[%d] %s = (x%x x%x)",
				     syscode, callp->name, rval[0], rval[1]);
		break;

	    case ERESTART:
		if (syscall_debug > 1)
		    e_emulator_error("RESTART[%d] %s", syscode, callp->name);
		/* restart call */
		/* regs->pc -= 7; */
		take_signals_and_do_sigreturn();
		goto restart_system_call;
		break;

	    default:
		/* error */
		regs->ps |= EFL_CF;
		regs->eax = error;
		if (syscall_debug > 1)
		    e_emulator_error("err_return[%d] %s -> %d",
				     syscode, callp->name, error);
		break;
	}

	/*
	 * Handle interrupt request
	 */
	e_checksignals(&interrupt);

	if (error == EINTR || interrupt)
	    take_signals_and_do_sigreturn();
}

/* Additional junk below argc is on the stack */
boolean_t with_kframe = TRUE;

noreturn emul_exec_start(
	int			*kframe,
	enum binary_type	binary_type,
	int			argc,
	char			*argv[],
	char			*envp[],
	struct exec_load_info	*li)
{
	struct i386_thread_state state;
	int x;

	switch (binary_type) {
	      case BT_LINUX:
	      case BT_LINUX_O:
	      case BT_LINUX_SHLIB:
	      case BT_LINUX_ELF:
		emul_exec_linux_start(kframe, binary_type,
				      argc, argv, envp, li);
	      default:
		/* code below */
	}

	if (x = emul_save_state(&state))
	    exit(x);

	state.eip = li->pc;
	state.uesp = (unsigned int) kframe;

	state.ebx = 0;
	state.ebp = 0;
	state.esi = 0;
	state.edi = 0;

	if (syscall_debug > 1)
	    e_emulator_error("emul_exec_start: starting at x%x k=x%x (x%x x%x x%x x%x)",
			     li->pc, kframe, kframe[0], kframe[1],
			     kframe[2], kframe[3]);
	emul_load_state(&state);
	/*NOTREACHED*/
}
