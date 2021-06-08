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
 * $Log: vm_syscalls.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:52  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	serv/vm_syscalls.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *	Origin:	empty m* syscall stubs taken from 4.4 BSD Lite.
 */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: vm_mmap.c 1.6 91/10/21$
 *
 *	@(#)vm_mmap.c	8.4 (Berkeley) 1/12/94
 */

#include <serv/server_defs.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <miscfs/specfs/specdev.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>

kern_return_t mmap_vm_map(
	struct proc 	*p,
	vm_address_t 	addr,
	vm_size_t 	size,
	boolean_t 	anywhere,
	mach_port_t 	port,
	vm_offset_t 	pager_offset,
	boolean_t 	copy,
	vm_prot_t 	curprot,
	vm_prot_t 	maxprot,
	vm_inherit_t 	inheritance,
	vm_address_t	*retval)
{
	proc_invocation_t pk = get_proc_invocation();
	kern_return_t kr;

	/*
	 *	Deallocate the existing memory, then map the appropriate
	 *	memory object into the space left.
	 */
	if (!anywhere)
	    kr = vm_deallocate(p->p_task, addr, size);
	else
	    kr = KERN_SUCCESS;
	if (kr == KERN_SUCCESS) {
		/*
		 *	We can't hold the master lock across vm_map,
		 *	because the vnode pager will need it.
		 */
		if (pk->k_master_lock)
		    master_unlock();

		ux_server_thread_busy(); /* An extra thread is needed */
		do {
			kr = vm_map(p->p_task,
				    &addr,
				    size,
				    0,
				    anywhere,
				    port,
				    pager_offset,
				    copy,
				    curprot,
				    maxprot,
				    inheritance);
			if (kr)
			    printf("mmap_vm_map: vm_map(%x %x %x %x %x %x %x %x %x %x %x) failed: %s\n",
				   p->p_task, addr, size, 0, anywhere, port,
				   pager_offset, copy, curprot, maxprot, inheritance,
				   mach_error_string(kr));
			/* GDB interrupts the send so we retry */
		} while (kr == MACH_SEND_INTERRUPTED);

		ux_server_thread_active();

		if (pk->k_master_lock)
		    master_lock();
	}
	if (kr == KERN_SUCCESS && retval != NULL)
	    *retval = addr;
	return kr;
}

/* System call. vm_map on file handle (representative memory object) */
/* unix_master is held */
mach_error_t file_vm_map(
	struct proc 	*p,
	vm_address_t 	*addr,
	vm_size_t 	size,
	vm_address_t	mask,
	boolean_t 	anywhere,
	struct file 	*file,
	vm_offset_t 	offset,
	boolean_t 	copy,
	vm_prot_t 	curprot,
	vm_prot_t 	maxprot,
	vm_inherit_t 	inheritance)
{
	vm_prot_t fprot = VM_PROT_NONE;
	struct vnode *vn;
	mach_port_t memory_object;
	mach_error_t kr;
	dev_t dev;

	/* Convert protection values to canonical form */
	if (file->f_flag & FREAD)
	    fprot |= VM_PROT_READ | VM_PROT_EXECUTE;
	if (file->f_flag & FWRITE)
	    fprot |= VM_PROT_WRITE;
	
	/* Check permissions */

	/* Must be readable */
	if ((fprot & VM_PROT_READ) == 0)
	    return EACCES;

	/* curprot must be lower or equal to maxprot */
	if (curprot & ~maxprot)
	    return EACCES;

	/* if read only must be copy on write or mapped ro */
	if (((fprot & VM_PROT_WRITE) == 0) && !copy
	    && (maxprot & VM_PROT_WRITE))
	{
		return EACCES;
	}

	/* Check file handle type */
	if (file->f_type != DTYPE_VNODE)
	    return EINVAL;

	/* Lookup vnode */
	vn = (struct vnode *)file->f_data;
	assert(vn->v_usecount >= 1);

	switch (vn->v_type) {
	      case VREG:
		/* Lookup (or create) actual memory object */
		memory_object = vnode_to_port(vn);
		if (!MACH_PORT_VALID(memory_object))
		    return EINVAL;

		/* The master lock is dropped so add a ref */
		vref(vn);
		kr = mmap_vm_map(p, *addr, size, anywhere, memory_object,
				 offset, copy, curprot, maxprot, inheritance,
				 (vm_address_t *) addr);
		vrele(vn);
		break;
	      case VCHR:
		dev = vn->v_rdev;
		memory_object = device_pager_create(dev, offset, size,
						    maxprot);
		if (!MACH_PORT_VALID(memory_object))
		    return EINVAL;
		kr = mmap_vm_map(p, *addr, size, anywhere, memory_object,
				 offset, copy, curprot, maxprot, inheritance,
				 (vm_address_t *) addr);
		/* device_pager_release(memory_object); XXX doesn't work yet */
		break;
	      default:
		return EINVAL;
	}

	return kr;
}

struct munmap_args {
	caddr_t	addr;
	int	len;
};
int munmap(
	struct proc *p,
	struct munmap_args *uap,
	int *retval)
{
	vm_offset_t addr;
	vm_size_t size;

#if DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("munmap(%d): addr %x len %x\n",
		       p->p_pid, uap->addr, uap->len);
#endif

	addr = trunc_page((vm_offset_t) uap->addr);
	if (uap->len < 0)
		return(EINVAL);
	size = (vm_size_t) round_page(uap->len);
	if (size == 0)
		return(0);
	(void) vm_deallocate(p->p_task, addr, size);
	return(0);
}

struct mprotect_args {
	caddr_t	addr;
	int	len;
	int	prot;
};
int
mprotect(
	struct proc *p,
	struct mprotect_args *uap,
	int *retval)
{
	vm_offset_t addr;
	vm_size_t size;
	vm_prot_t prot;

#if DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("mprotect(%d): addr %x len %x prot %d\n",
		       p->p_pid, uap->addr, uap->len, uap->prot);
#endif

	addr = (vm_offset_t) uap->addr;
	if ((addr != round_page(addr)) || uap->len < 0)
		return(EINVAL);
	size = round_page((vm_size_t) uap->len);
	/*
	 * Map protections
	 */
	prot = VM_PROT_NONE;
	if (uap->prot & PROT_READ)
		prot |= VM_PROT_READ;
	if (uap->prot & PROT_WRITE)
		prot |= VM_PROT_WRITE;
	if (uap->prot & PROT_EXEC)
		prot |= VM_PROT_EXECUTE;

	switch (vm_protect(p->p_task, addr, size, FALSE, prot)) {
	case KERN_SUCCESS:
		return (0);
	case KERN_PROTECTION_FAILURE:
		return (EACCES);
	}
	return (EINVAL);
}

struct msync_args {
	caddr_t	addr;
	int	len;
};
int
msync(p, uap, retval)
	struct proc *p;
	struct msync_args *uap;
	int *retval;
{
	vm_offset_t addr;
	vm_size_t size;
	int rv;
	boolean_t syncio, invalidate;

#ifdef DEBUG
	if (mmapdebug & (MDB_FOLLOW|MDB_SYNC))
		printf("msync(%d): addr %x len %x\n",
		       p->p_pid, uap->addr, uap->len);
#endif
	return EOPNOTSUPP;		/* XXX */
}

void
munmapfd(fd)
	int fd;
{
	struct proc *curproc = get_proc();
#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("munmapfd(%d): fd %d\n", curproc->p_pid, fd);
#endif

	/*
	 * XXX should vm_deallocate any regions mapped to this file
	 */
	curproc->p_fd->fd_ofileflags[fd] &= ~UF_MAPPED;
}

struct madvise_args {
	caddr_t	addr;
	int	len;
	int	behav;
};
/* ARGSUSED */
int
madvise(p, uap, retval)
	struct proc *p;
	struct madvise_args *uap;
	int *retval;
{

	/* Not yet implemented */
	return (EOPNOTSUPP);
}

struct mincore_args {
	caddr_t	addr;
	int	len;
	char	*vec;
};
/* ARGSUSED */
int
mincore(p, uap, retval)
	struct proc *p;
	struct mincore_args *uap;
	int *retval;
{

	/* Not yet implemented */
	return (EOPNOTSUPP);
}

struct mlock_args {
	caddr_t	addr;
	size_t	len;
};
int
mlock(p, uap, retval)
	struct proc *p;
	struct mlock_args *uap;
	int *retval;
{
	vm_offset_t addr;
	vm_size_t size;
	int error;
	extern int vm_page_max_wired;

#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("mlock(%d): addr %x len %x\n",
		       p->p_pid, uap->addr, uap->len);
#endif
	return EOPNOTSUPP;		/* XXX */
}

struct munlock_args {
	caddr_t	addr;
	size_t	len;
};
int
munlock(p, uap, retval)
	struct proc *p;
	struct munlock_args *uap;
	int *retval;
{
	vm_offset_t addr;
	vm_size_t size;
	int error;

#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("munlock(%d): addr %x len %x\n",
		       p->p_pid, uap->addr, uap->len);
#endif
	addr = (vm_offset_t)uap->addr;
	if ((addr != trunc_page(addr)) || uap->addr + uap->len < uap->addr)
		return (EINVAL);
#ifndef pmap_wired_count
	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
#endif
	size = round_page((vm_size_t)uap->len);

	return EOPNOTSUPP;		/* XXX */
}

/* FILES */
mach_error_t falloc_port(
	struct proc	*p,
	struct file	**resultfp,	/* OUT */
	mach_port_t	*image_port)		/* OUT */
{
	struct file *fp, *fq, **fpp;
	mach_error_t kr;

	if (nfiles >= maxfiles) {
		tablefull("file");
		return ENFILE;
	}
	nfiles++;
	MALLOC(fp, struct file *, sizeof(struct file), M_FILE, M_WAITOK);
	/* Don't link on any chain */
	fp->f_filef = 0;
	fp->f_fileb = 0;
	fp->f_count = 1;
	fp->f_msgcount = 0;
	fp->f_offset = 0;
	fp->f_cred = p->p_ucred;
	crhold(fp->f_cred);
	mutex_init(&fp->f_lock);
	mutex_set_name(&fp->f_lock, "file_p_handle_lock");
	kr = port_object_allocate_receive(&fp->f_port, POT_FILE_HANDLE, fp);
	if (kr)
	    return kr;
	/* 
	 * Add send right to be provided for user in after_exec.
	 * Activates no senders garbage collector.
	 */
	kr = port_object_make_send(fp->f_port);
	assert(kr == KERN_SUCCESS);

	/* Get ns notifications to port and receive from it */
	ux_server_add_port(fp->f_port);
	if (resultfp)
		*resultfp = fp;
	*image_port = fp->f_port;
	return KERN_SUCCESS;
}

mach_error_t fadd_port(
	struct file	*fp,
	mach_port_t	*port)	/* OUT */
{
	mach_error_t kr;

	mutex_lock(&fp->f_lock);
	if (!MACH_PORT_VALID(fp->f_port)) {
		kr = port_object_allocate_receive(&fp->f_port,
						  POT_FILE_HANDLE, fp);
		if (kr != KERN_SUCCESS) {
			mutex_unlock(&fp->f_lock);
			return kr;
		}
		fp->f_count++;/* XXXlater automatic through file_ref */
		ux_server_add_port(fp->f_port);
	}
	/* Add send right for MOVE_SEND */
	kr = port_object_make_send(fp->f_port);
	assert(kr == KERN_SUCCESS);
	*port = fp->f_port;
	mutex_unlock(&fp->f_lock);
	return KERN_SUCCESS;
}

/* System call */
mach_error_t fd_to_file_port(
	struct proc	*p,
	int		fd,
	mach_port_t	*port)	/* OUT */
{
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	mach_error_t kr;

	/* XXX If the file is a VCHR do device_pager_create! e_mmap needs it */
	if (((u_int)fd) >= fdp->fd_nfiles)
	    return EBADF;
	fp = fdp->fd_ofiles[fd];
	if (!fp)
	    return EBADF;
	return fadd_port(fp, port);
}
