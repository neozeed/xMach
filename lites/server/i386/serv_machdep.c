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
 * $Log: serv_machdep.c,v $
 * Revision 1.2  2000/10/27 01:58:45  welchd
 *
 * Updated to latest source
 *
 * Revision 1.4  1995/08/29  19:48:18  mike
 * don't need include of i386/eflags.h anymore
 *
 * Revision 1.3  1995/08/29  19:36:49  mike
 * Init ts.efl to 0 instead of EFL_USER_SET (those bits are always set by
 * the kernel anyway)
 *
 * Revision 1.2  1995/08/10  23:31:28  gback
 * fixed pagemove so it a) handles not page-aligned pages (!?) using bcopy
 * and so that it b) panics on an error.
 *
 * Revision 1.1.1.1  1995/03/02  21:49:43  mike
 * Initial Lites release from hut.fi
 *
 */
/*
 *	Machine-dependent routines evicted from other BSD
 *	files.
 */

/*-
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include <serv/server_defs.h>
#include <sys/exec_file.h>	/* for struct exec_load_info */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/exec.h>
#include <sys/systm.h>		/* boothowto */
#include <sys/parallel.h>
#include <sys/syscall.h>
#include <sys/namei.h>
#include <sys/vnode.h>

void set_arg_addr(arg_size,
	     arg_start_p,
	     arg_size_p)
	u_int		arg_size;
	vm_offset_t	*arg_start_p;
	vm_size_t	*arg_size_p;
{
	/*
	 * Round arg size to fullwords
	 */
	arg_size = (arg_size + NBPW-1) & ~(NBPW - 1);

	/*
	 * Put argument list at top of stack.
	 */
	*arg_start_p = USRSTACK - arg_size;
	*arg_size_p = arg_size;
}

/*
 * Set up the process' registers to start the emulator - duplicate
 * what execve() in the emulator would do.
 */
void set_emulator_state(
	struct proc		*p,
	struct exec_load_info	*li)
{
    struct i386_thread_state	ts;
    unsigned int		count;
    kern_return_t ret;

    /*
     * Read the registers first, to get the correct
     * segments.
     */
    count = i386_THREAD_STATE_COUNT;
    if ((ret=thread_get_state(p->p_thread,
			      i386_THREAD_STATE,
			      (thread_state_t)&ts,
			      &count)) != KERN_SUCCESS)
	panic("set_emulator_state",ret);
    ts.eip = li->pc;
    ts.uesp = li->sp;
    ts.efl = 0;
    ts.ebx = li->zero_start;
    ts.edi = li->zero_count;

    if ((ret=thread_set_state(p->p_thread,
			      i386_THREAD_STATE,
			      (thread_state_t)&ts,
			      i386_THREAD_STATE_COUNT)) != KERN_SUCCESS)
	panic("set_emulator_state",ret);
}

/*
 * Do weird things to the first process' address space before loading
 * the emulator.
 */
void
emul_init_process(p)
	struct proc	*p;
{
	/*
	 * Clear out the entire address space.
	 */
	(void) vm_deallocate(p->p_task,
			VM_MIN_ADDRESS,
			(vm_size_t)(VM_MAX_ADDRESS - VM_MIN_ADDRESS));

}

void sigreturn()
{
    panic("sigreturn - MiG interface");
}
void osigcleanup()
{
    panic("osigcleanup - MiG interface");
}
	
/*
 * Clone the parent's registers into the child thread for fork.
 */
boolean_t
thread_dup(child_thread, new_state, new_state_count, parent_pid, rc)
	thread_t	child_thread;
	thread_state_t	new_state;
	unsigned int	new_state_count;
	int		parent_pid, rc;
{
	struct i386_thread_state *regs = (struct i386_thread_state *)new_state;

	if (new_state_count != i386_THREAD_STATE_COUNT)
	    return (FALSE);

	regs->eax = parent_pid;
	regs->edx = rc;

	(void) thread_set_state(child_thread, i386_THREAD_STATE,
				new_state, new_state_count);
	return (TRUE);
}

/* 
 * For sysctl and namei macros
 */
char machine[] = "i386";
char atsys[] = "i386_lites";

/*
 * From lite i386/machdep.c
 */
boolean_t msgbufmapped = FALSE;		/* set when safe to use msgbuf */

mach_error_t cpu_sysctl(
	int *name,
	u_int namelen,
	void *oldp,
	size_t *oldlenp,
	void *newp,
	size_t newlen,
	struct proc *p)
{

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
#ifdef notyet
	case CPU_CONSDEV:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &cn_tty->t_dev,
		    sizeof cn_tty->t_dev));
#endif
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

mach_error_t
copystr(void *fromaddr, void *toaddr, u_int maxlength, u_int *lencopied)
{
	u_int tally;

	tally = 0;
	while (maxlength--) {
		*(u_char *)toaddr = *(u_char *)fromaddr++;
		tally++;
		if (*(u_char *)toaddr++ == 0) {
			if(lencopied) *lencopied = tally;
			return(0);
		}
	}
	if(lencopied) *lencopied = tally;
	return(ENAMETOOLONG);
}

/* 
 * From lite i386/vm_machdep.c
 */
/*
 * Dump the machine specific header information at the start of a core dump.
 */
mach_error_t cpu_coredump(p, vp, cred)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
{

	return vn_rdwr(UIO_WRITE, vp, (caddr_t) p->p_addr, ctob(UPAGES),
		       (off_t)0, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT,
		       cred, (int *)NULL, p);
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
void pagemove(
	caddr_t from,
	caddr_t to,
	int size)
{
	kern_return_t kr = KERN_SUCCESS;

	if(size % PAGE_SIZE || (int)from % PAGE_SIZE) 
		bcopy(from, to, size);
	else
		kr = vm_write(mach_task_self(), (vm_offset_t) to,
		      (vm_offset_t) from, size);

	if(kr != KERN_SUCCESS) 
		panic("pagemove(from: %x, to: %x, size %d failed !\n",
			from, to, size);

	assert(kr == KERN_SUCCESS);
#if 0
	/* The mapping should stay. So the old junk stays around ... */
	/* There must be better ways to handle this whole thing on Mach... */
	kr = vm_deallocate(mach_task_self(), from, size);
	assert(kr == KERN_SUCCESS);
#endif
}

