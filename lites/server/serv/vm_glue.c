/* 
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	@(#)vm_glue.c	8.6 (Berkeley) 1/5/94
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "map_uarea.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/assert.h>

#include <vm/vm.h>

#include <machine/cpu.h>

int	avefree = 0;		/* XXX */
unsigned maxdmap = MAXDSIZ;	/* XXX */
int	readbuffers = 0;	/* XXX allow kgdb to read kernel buffer pool */

/* Calling convention differs from BSD Lite */
static struct vmspace *vmspace_fork(struct vmspace *ovs, struct proc *p2)
{
	struct vmspace *vs;
#if MAP_UAREA
	vs = &p2->p_shared_rw->us_vmspace;
#else
	vs = &p2->p_realvmspace;
#endif
	bcopy(ovs, vs, sizeof(*vs));
	vs->vm_refcnt = 1;
	vs->vm_shm = 0;
	return vs;
}

/*
 * Implement fork's actions on an address space.
 * Here we arrange for the address space to be copied or referenced,
 * allocate a user struct (pcb and kernel stack), then call the
 * machine-dependent layer to fill those in and make the new process
 * ready to run.
 * NOTE: the kernel stack may be at a different location in the child
 * process, and thus addresses of automatic variables may be invalid
 * after cpu_fork returns in the child process.  We do nothing here
 * after cpu_fork returns.
 */
int
vm_fork(p1, p2, isvfork)
	register struct proc *p1, *p2;
	int isvfork;
{
	register struct user *up;
	vm_offset_t addr;
	kern_return_t kr;

	p2->p_vmspace = vmspace_fork(p1->p_vmspace, p2);

#ifdef SYSVSHM
	if (p1->p_vmspace->vm_shm)
		shmfork(p1, p2, isvfork);
#endif

	kr = vm_allocate(mach_task_self(), &addr, vm_page_size, TRUE);
	assert(kr == KERN_SUCCESS);
	up = (struct user *)addr;
	p2->p_addr = up;

	/*
	 * p_stats and p_sigacts currently point at fields
	 * in the user struct but not at &u, instead at p_addr.
	 * Copy p_sigacts and parts of p_stats; zero the rest
	 * of p_stats (statistics).
	 */
	p2->p_stats = &up->u_stats;
	p2->p_sigacts = &up->u_sigacts;
	up->u_sigacts = *p1->p_sigacts;
	bzero(&up->u_stats.pstat_startzero,
	    (unsigned) ((caddr_t)&up->u_stats.pstat_endzero -
	    (caddr_t)&up->u_stats.pstat_startzero));
	bcopy(&p1->p_stats->pstat_startcopy, &up->u_stats.pstat_startcopy,
	    ((caddr_t)&up->u_stats.pstat_endcopy -
	     (caddr_t)&up->u_stats.pstat_startcopy));

#ifdef LITES
	return ESUCCESS;
#else
	/*
	 * cpu_fork will copy and update the kernel stack and pcb,
	 * and make the child ready to run.  It marks the child
	 * so that it can return differently than the parent.
	 * It returns twice, once in the parent process and
	 * once in the child.
	 */
	return (cpu_fork(p1, p2));
#endif /* LITES */
}

void vm_exit(struct proc *p)
{
    vm_deallocate(mach_task_self(), (vm_address_t)p->p_addr, vm_page_size);
}

/*
 * Set default limits for VM system.
 * Called for proc 0, and then inherited by all others.
 */
void
vm_init_limits(p)
	register struct proc *p;
{

	/*
	 * Set up the initial limits on process VM.
	 * Set the maximum resident set size to be all
	 * of (reasonably) available memory.  This causes
	 * any single, large process to start random page
	 * replacement once it fills memory.
	 */
        p->p_rlimit[RLIMIT_STACK].rlim_cur = DFLSSIZ;
        p->p_rlimit[RLIMIT_STACK].rlim_max = MAXSSIZ;
        p->p_rlimit[RLIMIT_DATA].rlim_cur = DFLDSIZ;
        p->p_rlimit[RLIMIT_DATA].rlim_max = MAXDSIZ;
#ifndef LITES
	p->p_rlimit[RLIMIT_RSS].rlim_cur = ptoa(cnt.v_free_count);
#endif
}

/*
 * DEBUG stuff
 */

int indent = 0;

#include <machine/stdarg.h>		/* see subr_prf.c */

/*ARGSUSED2*/
void
#if __STDC__
iprintf(const char *fmt, ...)
#else
iprintf(fmt /* , va_alist */)
	char *fmt;
	/* va_dcl */
#endif
{
	register int i;
	va_list ap;

	for (i = indent; i >= 8; i -= 8)
		printf("\t");
	while (--i >= 0)
		printf(" ");
	va_start(ap, fmt);
	printf("%r", fmt, ap);
	va_end(ap);
}
