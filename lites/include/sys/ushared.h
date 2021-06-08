/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

#ifndef	_SYS_USHARED_H_
#define	_SYS_USHARED_H_

#ifdef	KERNEL
#include <serv/import_mach.h>
#else	KERNEL
#include <cthreads.h>
#endif	KERNEL
#include <sys/shared_lock.h>
#include <vm/vm.h>
#include <sys/resourcevar.h>

#define USHARED_VERSION 1

#define KERNEL_USER 0x80000000

struct ushared_ro {
	vm_offset_t	us_proc_pointer;
	int		us_version;
	struct plimit	us_limit;
	int		us_cursig;
	int		us_flag;
};

struct ushared_rw {
	int		us_inuse;
	int		us_debug;
	shared_lock_t	us_lock;
	struct vmspace	us_vmspace;
	sigset_t	us_sigmask;
	shared_lock_t	us_siglock;
	int		us_sig;
	int		us_sigignore;
};

#endif	_SYS_USHARED_H_
