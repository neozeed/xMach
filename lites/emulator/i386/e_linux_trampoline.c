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
 * $Log: e_linux_trampoline.c,v $
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
 * Revision 1.1.1.2  1995/03/23  01:15:40  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/i386/e_linux_trampoline.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Linux binary emulation.
 */

#define KERNEL
#include <sys/errno.h>
#undef KERNEL
#include <e_defs.h>
#include <syscall_table.h>
#include <i386/eflags.h>
#include <sys/elf.h>

#define	DEBUG 1

extern int syscall_debug;

extern emul_linux_trampoline();
extern void e_exception_thread(void);
void e_linux_setup(task_t, enum binary_type);

extern void (*emul_child_init_func)();

void linux_child_init()
{
	e_linux_setup(mach_task_self(), e_my_binary_type);
}

extern int _linux_libc_trampoline_code();
extern vm_size_t _linux_libc_trampoline_size;
extern int emul_linux_trampoline_2();
extern int emul_linux_trampoline_3();

extern int emul_low_entry;
extern int emul_high_entry;

void emul_linux_setup3(task_t task, enum binary_type binary_type)
{
	int i;
        kern_return_t	rc;

	for (i = emul_low_entry;
	     i <= emul_high_entry;
	     i++) {
		rc = task_set_emulation(task,
					(vm_address_t) emul_linux_trampoline_3,
					i);
	}
	rc = task_set_emulation(task,
			(vm_address_t) emul_linux_trampoline_3,
			-33);
	rc = task_set_emulation(task,
			(vm_address_t) emul_linux_trampoline_3,
			-34);
	rc = task_set_emulation(task,
			(vm_address_t) emul_linux_trampoline_3,
			-41);
	rc = task_set_emulation(task,
			(vm_address_t) emul_linux_trampoline_3,
			-52);

	        current_nsysent = e_linux_nsysent;
		current_sysent = e_linux_sysent;
}

void e_linux_setup(task_t task, enum binary_type binary_type)
{
	vm_offset_t addr;
	vm_offset_t *vector;
	kern_return_t kr;
        
	/* Setup trampoline for patched libc */
	addr = 0xa0100000; /* See linux-libc/sysdeps/linux/i386/sysdep.h */
	vm_allocate(mach_task_self(), &addr, 0x1000, FALSE);
	/* vm_allocate fails after fork. Doesn't matter at all */
	vector = (vm_offset_t *) addr;
	*vector = (vm_offset_t) emul_linux_trampoline_2;

	emul_child_init_func = linux_child_init;

	current_nsysent = e_linux_nsysent;
	current_sysent = e_linux_sysent;

	e_exception_setup();
	e_fork_a_thread(e_exception_thread);
}

int (*func)() = (int (*)()) -1;	/* XXX make local below but now for debugging*/

#define DLINFO_ITEMS 13

/* Symbolic values for the entries in the auxiliary table
   put on the initial stack */
#define AT_NULL   0	/* end of vector */
#define AT_IGNORE 1	/* entry should be ignored */
#define AT_EXECFD 2	/* file descriptor of program */
#define AT_PHDR   3	/* program headers for program */
#define AT_PHENT  4	/* size of program header entry */
#define AT_PHNUM  5	/* number of program headers */
#define AT_PAGESZ 6	/* system page size */
#define AT_BASE   7	/* base address of interpreter */
#define AT_FLAGS  8	/* flags */
#define AT_ENTRY  9	/* entry point of program */
#define AT_NOTELF 10	/* program is not ELF */
#define AT_UID    11	/* real uid */
#define AT_EUID   12	/* effective uid */
#define AT_GID    13	/* real gid */
#define AT_EGID   14	/* effective gid */
#define AT_PLATFORM 15  /* string identifying CPU for optimizations */
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */

noreturn emul_exec_linux_start_new(
	int			*kframe,
	enum binary_type	binary_type,
	int			argc,
	char			*argv[],
	char			*envp[],
	struct exec_load_info	*interpreter_li,
	struct exec_load_info   *li,
	union exec_data         *hdr,
        unsigned int            load_addr,
	unsigned int            interp_load_addr)
{
   struct i386_thread_state state;
   int x;
   char* p;
   char* u_platform;
   int* sp;
   int* csp;
   unsigned int envc;
   unsigned int i;
   void* final_argv;
   void* final_envp;
   
   if (x = emul_save_state(&state))
     exit(x);
   
   /*
    * Count the number of environment variables
    */
   envc = 0;
   while (envp[envc] != NULL)
     {
	envc++;
     }
   
   /*
    * Set up the registers
    */
//   e_emulator_error("interpreter_li->pc 0x%x", interpreter_li->pc);
   state.eip = interpreter_li->pc;
   state.ebx = 0;
   state.ebp = 0;
   state.esi = 0;
   state.edi = 0;
   
   /*
    * Create the call frame
    */
   p = (char *)kframe;
   
   u_platform = p - (strlen("i386") + 1);
   strcpy(u_platform, "i386");
   
   sp = (int *)((~15UL & (unsigned long)(u_platform)) - 16UL);
   csp = sp;
   csp -= (DLINFO_ITEMS*2) + 2;
   csp -= envc+1;
   csp -= argc+1;
   csp -= 1; /* argc itself */
   if ((unsigned long)csp & 15UL)
     sp -= ((unsigned long)csp & 15UL) / sizeof(*sp);
   
   sp -= 2;
   sp[0] = AT_NULL;
   sp[1] = 0;
   
   sp -= 2;
   sp[0] = AT_PLATFORM;
   sp[1] = (unsigned int)u_platform;
   
   sp -= 2;
   sp[0] = AT_HWCAP;
   sp[1] = 0;
   
   /*
    * Put the dynamic table on the stack
    */
   
   sp -= 11*2;
   /*
    * Put the offset of the program's headers
    */
   sp[0] = AT_PHDR;
   sp[1] = load_addr + hdr->elf.ehdr.e_phoff;
//   e_emulator_error("load_addr + hdr->elf.ehdr.e_phoff 0x%x",
//		    load_addr + hdr->elf.ehdr.e_phoff);
   /*
    * Put the size of the headers
    */
   sp[2] = AT_PHENT;
   sp[3] = sizeof(Elf32_Phdr);
   /*
    * Put the number of headers
    */
   sp[4] = AT_PHNUM;
   sp[5] = hdr->elf.ehdr.e_phnum;
//   e_emulator_error("hdr->elf.ehdr.e_phnum %d", hdr->elf.ehdr.e_phnum);
   sp[6] = AT_PAGESZ;
   sp[7] = 4096;
   /*
    * Interpreter load address
    */
   sp[8] = AT_BASE;
   sp[9] = interp_load_addr;
//   e_emulator_error("interp_load_addr 0x%x", interp_load_addr);
   sp[10] = AT_FLAGS;
   sp[11] = 0;
   /*
    * Program entrypoint
    */
   sp[12] = AT_ENTRY;
   sp[13] = li->pc;
//   e_emulator_error("li->pc 0x%x", li->pc);
   sp[14] = AT_UID;
   sp[15] = 0;
   sp[16] = AT_EUID;
   sp[17] = 0;
   sp[18] = AT_GID;
   sp[19] = 0;
   sp[20] = AT_EGID;
   sp[21] = 0;

//   e_emulator_error("sp 0x%x envc %d", sp, envc);
   
   sp -= (envc + 1);
   final_envp = (void *)sp;
   for (i = 0; i < envc; i++)
     {
	sp[i] = envp[i];
     }
   sp[envc] = 0;
   
//   e_emulator_error("sp 0x%x", sp);
   
   sp -= (argc + 1);
   final_argv = (void *)sp;
   for (i = 0; i < argc; i++)
     {
	sp[i] = argv[i];
     }
   sp[argc] = 0;
   
//   e_emulator_error("sp 0x%x", sp);
   
   sp--;
   sp[0] = argc;
   
//   e_emulator_error("argc %d final_argv 0x%x final_envp 0x%x",
//		    argc, final_argv, final_envp);
   
   /* Unwind stack */
   state.uesp = (int) sp;

#if 0   
   e_emulator_error("sp 0x%x", sp);
   for (i = 0; i < 8; i++)
     {
	e_emulator_error("sp[%d] 0x%x sp[%d] 0x%x sp[%d] 0x%x sp[%d] 0x%x",
			 i*4, sp[i*4], (i*4)+1, sp[(i*4)+1],
			 (i*4)+2, sp[(i*4)+2], (i*4)+3, sp[(i*4)+3]);
     }
#endif   
   
   if (syscall_debug > 1)
     e_emulator_error("emul_exec_linux_start: starting at x%x k=x%x (x%x x%x x%x x%x)",
		      li->pc, kframe, kframe[0], kframe[1],
		      kframe[2], kframe[3]);
   (noreturn) emul_load_state(&state);
   /*NOTREACHED*/
   
}

noreturn emul_exec_linux_start(
	int			*kframe,
	enum binary_type	binary_type,
	int			argc,
	char			*argv[],
	char			*envp[],
	struct exec_load_info	*li)
{
	struct i386_thread_state state;
	int x;

	if (x = emul_save_state(&state))
	    exit(x);

	state.eip = li->pc;
	state.ebx = 0;
	state.ebp = 0;
	state.esi = 0;
	state.edi = 0;

	/* 
	 * Linux has a slightly different kframe.
	 * The start function should be called with sp
	 * pointing to argc. above that argp and envp.
	 *
	 * The BSD/SYSV frame has sp pointing to argc with argv0 arv1
	 * ... argvN 0 envv0 ...
	 *
	 * What I can't see is why neither could have been a normal
	 * function call frame!
	 *
	 * Pushing the extra arguments is safe even if we are running
	 * on the same stack as a few extra words were allocated by
	 * the server when it left room for argv etc.
	 */

	/* Replace argc with envp */
	*kframe = (int) envp;

	/* Push pointer to argv start. */
	kframe--;		/* 4 bytes. it's an int pointer */
	*kframe = (int) argv;

	/* And argc */
	kframe--;
	*kframe = argc;

	/* Unwind stack */
	state.uesp = (int) kframe;

	if (syscall_debug > 1)
	    e_emulator_error("emul_exec_linux_start: starting at x%x k=x%x (x%x x%x x%x x%x)",
			     li->pc, kframe, kframe[0], kframe[1],
			     kframe[2], kframe[3]);
	(noreturn) emul_load_state(&state);
	/*NOTREACHED*/
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

struct emul_linux_slow_regs {
	int	fp;
	int	edi;
	int	esi;
	int	ebx;

	int	edx;
	int	ecx;
	int	eax;
	int	pc;
};

struct emul_linux_regs {
	int	fp;
	int	edi;
	int	esi;
	int	ebx;

	int	edx;
	int	ecx;
	int	pc;
	int	eax;
};

extern void emul_linux_syscall(struct emul_linux_regs *regs);

/* From int 0x80 through fixup thread */
void emul_linux_slow_syscall(struct emul_linux_slow_regs *sr)
{
	struct emul_linux_regs r;
        
	r.fp = sr->fp;
	r.edi = sr->edi;
	r.esi = sr->esi;
	r.ebx = sr->ebx;
	r.edx = sr->edx;
	r.ecx = sr->ecx;
	r.pc = sr->pc;
	r.eax = sr->eax;

	emul_linux_syscall(&r);

	sr->fp = r.fp;
	sr->edi = r.edi;
	sr->esi = r.esi;
	sr->ebx = r.ebx;
	sr->edx = r.edx;
	sr->ecx = r.ecx;
	sr->pc = r.pc;
	sr->eax = r.eax;
}

/*
 * System calls enter here.
 */
void emul_linux_syscall(struct emul_linux_regs *regs)
{
	int syscode;
	int error;
	struct sysent *callp;
	int rval[2];
	boolean_t interrupt;
	int *args;

restart_system_call:
	interrupt = FALSE;
	error = 0;

	args = (int *)(regs + 1);	/* args on stack top */
	args++;	/* point to first argument - skip return address */

	syscode = regs->eax;
        
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
		callp = &e_bsd_sysent[SYS_table];
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
	    e_emulator_error("emul_linux_syscall[%d] %s(x%x, x%x, x%x, x%x, x%x)",
			     syscode, callp->name, regs->ebx, regs->ecx,
			     regs->edx, regs->esi, regs->edi);
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
		error = (*callp->routine)(regs->ebx, rval);
		break;
	    case 2:
		error = (*callp->routine)(
				regs->ebx, regs->ecx,
				rval);
		break;
	    case 3:
		error = (*callp->routine)(
				regs->ebx, regs->ecx, regs->edx,
				rval);
		break;
	    case 4:
		error = (*callp->routine)(
				regs->ebx, regs->ecx, regs->edx,
				regs->esi,
				rval);
		break;
	    case 5:
		error = (*callp->routine)(
				regs->ebx, regs->ecx, regs->edx,
				regs->esi, regs->edi,
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
		    e_emulator_error("JUSTRETURN[%d] %s", syscode, callp->name);
		/* Do not alter registers */
		break;

	    case 0:
		/* Success */
		regs->eax = rval[0];
/*		regs->edx = rval[1]; */
		if (syscall_debug > 2)
		    e_emulator_error("return[%d] %s = x%x",
				     syscode, callp->name, rval[0]);
		break;

	    case ERESTART:
		if (syscall_debug > 1)
		    e_emulator_error("RESTART[%d] %s", syscode, callp->name);
		/* restart call */
		/* 
		 * Since there are two routes into the server it is
		 * not possible to just fix the program counter.
		 * Instead a simple goto will do. (But do signals first).
		 */
		/* regs->pc -= 2; */
		take_signals_and_do_sigreturn();
		goto restart_system_call;
		break;

	    default:
		/* error */
		regs->eax = -error; /* negated errno */
		if (syscall_debug > 1)
		    e_emulator_error("err_return[%d] %s(x%x, x%x, x%x, x%x) -> %d",
				     syscode, callp->name,
				     regs->ebx, regs->ecx, regs->edx, regs->esi,
				     error);
		break;
	}

	/*
	 * Handle interrupt request
	 */
	e_checksignals(&interrupt);

	if (error == ERESTART || error == EINTR || interrupt)
	    take_signals_and_do_sigreturn();
}

/*
 * System calls enter here.
 */
void emul_linux_syscall3(struct emul_regs *regs)
{
	int syscode;
	int error;
	struct sysent *callp;
	int rval[2];
	boolean_t interrupt;
	int *args;

restart_system_call:
	interrupt = FALSE;
	error = 0;

//	args = (int *)(regs+1);	/* args on stack top */         
        args = (int *)(regs);
	args++;	/* point to first argument - skip return address */

	syscode = regs->eax;
        
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
		callp = &e_bsd_sysent[SYS_table];
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
	    e_emulator_error("emul_linux_syscall3[%d] %s(x%x, x%x, x%x, x%x, x%x)",
			     syscode, callp->name, regs->ebx, regs->ecx,
			     regs->edx, regs->esi, regs->edi);
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
		error = (*callp->routine)(regs->ebx, rval);
		break;
	    case 2:
		error = (*callp->routine)(
				regs->ebx, regs->ecx,
				rval);
		break;
	    case 3:
		error = (*callp->routine)(
				regs->ebx, regs->ecx, regs->edx,
				rval);
		break;
	    case 4:
		error = (*callp->routine)(
				regs->ebx, regs->ecx, regs->edx,
				regs->esi,
				rval);
		break;
	    case 5:
		error = (*callp->routine)(
				regs->ebx, regs->ecx, regs->edx,
				regs->esi, regs->edi,
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
		    e_emulator_error("JUSTRETURN[%d] %s", syscode, callp->name);
		/* Do not alter registers */
		break;

	    case 0:
		/* Success */
		regs->eax = rval[0];
/*		regs->edx = rval[1]; */
		if (syscall_debug > 2) {
		    e_emulator_error("return[%d] %s = x%x",
				     syscode, callp->name, rval[0]);
		}
		break;

	    case ERESTART:
		if (syscall_debug > 1)
		    e_emulator_error("RESTART[%d] %s", syscode, callp->name);
		/* restart call */
		/* 
		 * Since there are two routes into the server it is
		 * not possible to just fix the program counter.
		 * Instead a simple goto will do. (But do signals first).
		 */
		/* regs->pc -= 2; */
		take_signals_and_do_sigreturn();
		goto restart_system_call;
		break;

	    default:
		/* error */
		regs->eax = -error; /* negated errno */
		if (syscall_debug > 1)
		    e_emulator_error("err_return[%d] %s(x%x, x%x, x%x, x%x, x%x) -> %d",
				     syscode, callp->name,
				     regs->ebx, regs->ecx, regs->edx, regs->esi, 
				     regs->edi,
				     error);
		break;
	}

	/*
	 * Handle interrupt request
	 */
	e_checksignals(&interrupt);

	if (error == ERESTART || error == EINTR || interrupt)
	    take_signals_and_do_sigreturn();
}
