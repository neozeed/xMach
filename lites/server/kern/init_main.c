/* 
 * Mach Operating System
 * Copyright (c) 1994 Johannes Helander
 * Copyright (c) 1994 Timo Rinne
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * JOHANNES HELANDER AND TIMO RINNE ALLOW FREE USE OF THIS SOFTWARE IN
 * ITS "AS IS" CONDITION.  JOHANNES HELANDER AND TIMO RINNE DISCLAIM
 * ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM
 * THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: init_main.c,v $
 * Revision 1.2  2000/10/27 01:58:45  welchd
 *
 * Updated to latest source
 *
 * Revision 1.4  1995/08/23  18:33:44  mike
 * cleanup EXT2FS code: only one ifdef
 *
 * Revision 1.3  1995/08/15  06:48:57  sclawson
 * modifications from lites-1.1-950808
 *
 * Revision 1.2  1995/08/10  22:47:06  gback
 * added support for mounting a EXT2FS fs as root on startup.
 *
 * Revision 1.1.1.2  1995/03/23  01:16:38  law
 * lites-950323 from jvh.
 *
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)init_main.c	8.9 (Berkeley) 1/21/94
 */

#include "map_uarea.h"
#include "vnpager.h"
#include "sysvshm.h"
#include "machid_register.h"
#include "second_server.h"
#include "sl.h"
#include "ppp.h"
#include "ext2fs.h"
#include "devfs.h"

#include <serv/server_defs.h>
#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <ufs/ufs/quota.h>
#include <miscfs/devfs/devfs.h>

#include <machine/cpu.h>

extern char init_program_path[];
extern char init_program_name[];
extern	struct user *proc0paddr;

#if SECOND_SERVER
extern int second_server;
extern mach_port_t mid_server;
#endif SECOND_SERVER

char	copyright[] =
"Copyright (c) 1982, 1986, 1989, 1991, 1993\n        The Regents of the University of California.\nCopyright (c) 1992 Carnegie Mellon University.\nCopyright (c) 1994, 1995 Johannes Helander (Helsinki University of Technology).\nAll rights reserved.\n\n";

/* Components of the first process -- never freed. */
struct	session session0;
struct	pgrp pgrp0;
struct	proc proc0;
struct	pcred cred0;
struct	filedesc0 filedesc0;
struct	plimit limit0;
struct	vmspace vmspace0;
struct	proc *initproc, *pageproc;

struct proc *allproc; 		/* List of active procs. */
struct mutex allproc_lock;
struct proc *zombproc;		/* List of zombie procs. */

int	cmask = CMASK;

struct	vnode *rootvp, *swapdev_vp;
struct	timeval boottime;
struct	timeval runtime;
int	boothowto = RB_KDB;	/* XXX should be ifdeffed */

/*
 * System startup; initialize the world, create process 0, mount root
 * filesystem, and fork to create init and pagedaemon.  Most of the
 * hard work is done in the lower-level initialization routines including
 * startup(), which does memory initialization and autoconfiguration.
 */
noreturn system_setup()
{
	register struct proc *p;
	register struct filedesc0 *fdp;
	register struct pdevinit *pdev;
	register int i;
	int s;
	extern int (*mountroot) __P((void));
	extern struct pdevinit pdevinit[];
	kern_return_t	kr;
	proc_invocation_t pk = get_proc_invocation();
	mach_port_t exception_port;

#if	MAP_UAREA
	int		cproc = 0;
	vm_address_t	shared_address = 0;

	/*
	 * Get default_pager port and shared memory port
	 */

	kr = vm_set_default_memory_manager(privileged_host_port,
					   &default_pager_port);

	if (kr != KERN_SUCCESS)
	    panic("getting default pager port: %s", mach_error_string(kr));

	if (default_pager_port == MACH_PORT_NULL) {
	  dprintf("(lites) Waiting for default pager\n");
	  do {
	    kr = vm_set_default_memory_manager(privileged_host_port,
					       &default_pager_port);
	    if (kr != KERN_SUCCESS)
	      panic("getting default pager port: %s", mach_error_string(kr));
	  } while (default_pager_port == MACH_PORT_NULL);
	}

	kr = default_pager_object_create(default_pager_port,
					 &shared_memory_port,
					 4*vm_page_size*(maxproc+1));

	if (kr != KERN_SUCCESS)
	    panic("getting shared_memory port: %s", mach_error_string(kr));

#endif MAP_UAREA
	init_mapped_timezone();

	p = &proc0;
	p->p_task = mach_task_self();
	p->p_thread = mach_thread_self();

	/* 
	 * Allocate request port for p0. It is used for dead port
	 * notifications etc.
	 */
	kr = port_object_allocate_receive(&p->p_req_port, POT_PROCESS, p);
	assert(kr == KERN_SUCCESS);

#if MAP_UAREA
	alloc_mapped_uarea(p);
#endif MAP_UAREA
	pk->k_master_lock = 0;
	queue_init(&p->p_servers);
	p->p_servers_count = 0;
	p->p_ref = 1;		/* p0 should never die */

	condition_init(&p->p_condition);
	mutex_init(&p->p_lock);
	p->p_pid = 0;
	proc_set_condition_names(p);
	server_thread_register_locked(p);

	/*
	 * Create process 0 (the swapper).
	 */
	allproc = (volatile struct proc *)p;
	p->p_prev = (struct proc **)&allproc;
	p->p_pgrp = &pgrp0;
	pgrphash[0] = &pgrp0;
	pgrp0.pg_mem = p;
	pgrp0.pg_session = &session0;
	session0.s_count = 1;
	session0.s_leader = p;

	p->p_flag = P_INMEM | P_SYSTEM;
	p->p_stat = SRUN;
	p->p_nice = NZERO;
	bcopy("swapper", p->p_comm, sizeof ("swapper"));

	/* Create credentials. */
	cred0.p_refcnt = 1;
	p->p_cred = &cred0;
	p->p_ucred = crget();
	p->p_ucred->cr_ngroups = 1;	/* group 0 */

	/* Create the file descriptor table. */
	fdp = &filedesc0;
	p->p_fd = &fdp->fd_fd;
	fdp->fd_fd.fd_refcnt = 1;
	fdp->fd_fd.fd_cmask = cmask;
	fdp->fd_fd.fd_ofiles = fdp->fd_dfiles;
	fdp->fd_fd.fd_ofileflags = fdp->fd_dfileflags;
	fdp->fd_fd.fd_nfiles = NDFILE;

	/*
	 * Set initial limits
	 */
#if	MAP_UAREA
	p->p_vmspace = &p->p_shared_rw->us_vmspace;
	p->p_limit = &p->p_shared_ro->us_limit;
#else
	p->p_vmspace = &p->p_realvmspace;
	p->p_limit = &limit0;
#endif
	p->p_vmspace->vm_ssize = 80;				/* XXX */
	p->p_vmspace->vm_refcnt = 1;
	for (i = 0; i < sizeof(p->p_rlimit)/sizeof(p->p_rlimit[0]); i++)
		p->p_limit->pl_rlimit[i].rlim_cur =
		    p->p_limit->pl_rlimit[i].rlim_max = RLIM_INFINITY;
	p->p_limit->pl_rlimit[RLIMIT_NOFILE].rlim_cur = NOFILE;
	p->p_limit->pl_rlimit[RLIMIT_NPROC].rlim_cur = MAXUPRC;
	limit0.p_refcnt = 1;

	/* Allocate a prototype map so we have something to fork. */
	p->p_vmspace = &vmspace0;
	p->p_vmspace->vm_refcnt = 1;
	p->p_addr = proc0paddr;				/* XXX */

	/*
	 * We continue to place resource usage info and signal
	 * actions in the user struct so they're pageable.
	 */
	p->p_stats = &p->p_addr->u_stats;
	p->p_sigacts = &p->p_addr->u_sigacts;

	bufinit();

	/*
	 * Initialize per uid information structure and charge
	 * root for one process.
	 */
	usrinfoinit();
	(void)chgproccnt(0, 1);

	/* Configure virtual memory system, set vm rlimits. */
	vm_init_limits(p);

	unix_master();

	/* Initialize the file systems. */
	vfsinit();

	/* Initialize mbuf's. */
	mbinit();		/* XXX should be mbuf_init */
	exec_system_init();

	/* Initialize clists. */
	clist_init();

#if SYSVSHM
	/* Initialize System V style shared memory. */
	shminit();
#endif

#ifndef LITES	/* XXX pdevinit not defined anywhere */
	/* Attach pseudo-devices. */
	for (pdev = pdevinit; pdev->pdev_attach != NULL; pdev++)
		(*pdev->pdev_attach)(pdev->pdev_count);
#endif
	{
		extern loopattach(int);
		loopattach(0);			/* XXX */
	}
#if NSL > 0
	{
		extern slattach(void);
		slattach();			/* XXX */
	}
#endif
#if NPPP > 0
	{
		extern pppattach(void);
		pppattach();			/* XXX */
	}
#endif
	/*
	 * Start network server thread.
	 */
	netisr_init();

	/*
	 * Initialize protocols.  Block reception of incoming packets
	 * until everything is ready.
	 */
	s = splimp();
	ifinit();
	domaininit();
	splx(s);

	/*
	 * Open the console.
	 */
	(void) cons_open(makedev(0,0), FREAD|FWRITE, 0, p);

	printf("%s\n",version);
	printf(copyright);


#if VNPAGER
	vn_pager_init();
#endif

	/* Mount the root file system. */
	kr = (*mountroot)();
#if EXT2FS
	/* XXX if FFS fails, fall back to EXT2FS */
	if (kr == EINVAL) {
		extern int ext2_mountroot __P((void));

		kr = ext2_mountroot();
	}
#endif
	if (kr != KERN_SUCCESS)
		panic("cannot mount root 0x%x %s", kr, mach_error_string(kr));

	/* Get the vnode for '/'.  Set fdp->fd_fd.fd_cdir to reference it. */
	kr = VFS_ROOT(mountlist.tqh_first, &rootvnode);
	if (kr != KERN_SUCCESS) {
		panic("cannot find root vnode: x%x %s",
		      kr, mach_error_string(kr));
	}
	fdp->fd_fd.fd_cdir = rootvnode;
	VREF(fdp->fd_fd.fd_cdir);
	VOP_UNLOCK(rootvnode);
	fdp->fd_fd.fd_rdir = NULL;

	/*
	 * Now can look at time, having had a chance to verify the time
	 * from the file system.  Reset p->p_rtime as it may have been
	 * munched in mi_switch() after the time got set.
	 */
	get_time(&boottime);
	p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;
        
        /*
	 * Mount the device filesystem if required
	 */
#if DEVFS
        mount_devfs();
#endif
   
	/* Initialize signal state for process 0. */
	siginit(p);

	initproc = newproc(p, TRUE, FALSE);

	/* exception handler */
	exception_port = ux_handler_setup();
#if OSFMACH3
	/* set all exception types but EXC_MASK_MACH_SYSCALL */
	kr = task_set_exception_ports(initproc->p_task,
				      EXC_MASK_BAD_ACCESS|EXC_MASK_BAD_INSTRUCTION|EXC_MASK_ARITHMETIC|EXC_MASK_EMULATION|EXC_MASK_SOFTWARE|EXC_MASK_SOFTWARE|EXC_MASK_BREAKPOINT|EXC_MASK_SYSCALL,
				      exception_port,
				      EXCEPTION_DEFAULT,
				      0);
#else /* OSFMACH3 */
	kr = task_set_exception_port(initproc->p_task,
				     exception_port);
#endif /* OSFMACH3 */
	if (kr != KERN_SUCCESS)
	    panic("system_setup: can't set exception port: %s",
		  mach_error_string(kr));

	/*
	 * Let ps know what we are
	 */
	strncpy(p->p_comm, "Lites server", sizeof(p->p_comm));
#if MACHID_REGISTER && SECOND_SERVER
	if(second_server) {
	    kr = netname_look_up(name_server_port, "", "MachID", &mid_server);
	    if (kr) {
		dprintf("(lites) No MachID server is running: %s.\n",
			mach_error_string(kr));
		mid_server = MACH_PORT_NULL;
	    }
	}
#endif /* MACHID_REGISTER */
	{
		vm_address_t arg_addr = 0x2000000;
		vm_size_t arg_size = 0;
		int arg_count = 0;
		int env_count = 0;
		char *args[] = {init_program_name, "-s", 0};
		char *envs[] = {0};
		mach_error_t kr;
#if 1	/* was if i386 */
		/* XXX Remove boothowto */
		args[1] = (boothowto & RB_SINGLE) ? "-s" : 0; /* XXX */
#endif
		/* Clear address space */
		emul_init_process(initproc);
		kludge_out_args(initproc, &arg_addr, &arg_size,
				&arg_count, &env_count,
				args, envs);
		kr = s_execve(initproc, init_program_path,
				   arg_addr, arg_size, arg_count, env_count);
		if (kr != KERN_SUCCESS) {
			panic("first program (%s) exec failed: x%x %s",
			      init_program_path, kr, mach_error_string(kr));
		}
	}

	/*
	 * Start the first task!
	 */
	(void) thread_resume(initproc->p_thread);

	/*
	 * Release the master lock...
	 */
	unix_release();

	/*
	 * Become the first server thread
	 */
        server_thread_deregister(p);
	ux_server_loop();
	/* NOTREACHED */
}
