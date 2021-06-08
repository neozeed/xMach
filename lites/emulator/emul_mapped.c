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
/*
 * HISTORY
 * $Log: emul_mapped.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/09/01  23:27:54  mike
 * HP-UX compatibility support from Jeff F.
 *
 * Revision 1.1.1.2  1995/03/23  01:15:30  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	emulator/emul_generic.c
 *	Author:	Randall W. Dean
 *	Date:	1992
 *
 * Routines which use mapped area of uarea instead of sending message
 * to server
 */

#ifdef MAP_UAREA
#include <serv/import_mach.h>
#include <serv/bsd_msg.h>
#include <mach/machine/vm_param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/ushared.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>

int enable_sharing = 1;
int shared_enabled = 0;
struct ushared_ro *shared_base_ro;
struct ushared_rw *shared_base_rw;
char *shared_readwrite;
int readwrite_inuse = 0;
spin_lock_t readwrite_lock = SPIN_LOCK_INITIALIZER;

#define ESUCCESS 0
#define	DEBUG 0

#if	DEBUG
#define	EPRINT(a) e_emulator_error a
#else	DEBUG
#define	EPRINT(a)
#endif	DEBUG

extern  mach_port_t	our_bsd_server_port;

void emul_mapped_init()
{
	kern_return_t	result;
	vm_address_t	address;
	vm_size_t	size;
	vm_prot_t	prot;
	vm_prot_t	max_prot;
	vm_inherit_t	inherit;
	boolean_t	shared;
	mach_port_t	object_name;
	vm_offset_t	offset;
	vm_statistics_data_t	vm_stat;

	if (!enable_sharing)
		return;

	shared_base_ro = (struct ushared_ro *)(EMULATOR_END - vm_page_size);
	shared_base_rw = (struct ushared_rw *)(EMULATOR_END - 2*vm_page_size);
	shared_readwrite = (char *) (EMULATOR_END - 4*vm_page_size);

	address = (vm_address_t) shared_base_ro;
#if OSFMACH3
	{
	struct vm_region_basic_info r;
	mach_msg_type_number_t icount;

	icount = VM_REGION_BASIC_INFO_COUNT;
	result = vm_region(mach_task_self(),
		      &address,
		      &size,
		      VM_REGION_BASIC_INFO,
		      (vm_region_info_t) &r,
		      &icount,
		      &object_name);
	if (result == KERN_SUCCESS) {
		prot = r.protection;
		max_prot = r.max_protection;
		inherit = r.inheritance;
		shared = r.shared;
		offset = r.offset;
	}
	}
#else
	result = vm_region(mach_task_self(), &address, &size,
			   &prot, &max_prot, &inherit, &shared,
			   &object_name, &offset);
#endif
	if (result != KERN_SUCCESS) {
		e_emulator_error("vm_region ret = %x",result);
		return;
	}
	if (!(prot & VM_PROT_READ)) {
		e_emulator_error("shared region not readable");
		return;
	}
	if (shared_base_ro->us_version != USHARED_VERSION) {
		e_emulator_error("shared region mismatch %x/%x",
			shared_base_ro->us_version, USHARED_VERSION);
		return;
	}

	shared_enabled = 1;
	shared_base_rw->us_inuse = 1;
	shared_base_rw->us_debug = 0;
	readwrite_inuse = 0;
	spin_lock_init(&readwrite_lock);
#if 0
	if (shared_base_ro->us_mach_nbc)
		shared_base_rw->us_map_files = 1;
#endif 0
}

errno_t e_sigblock(int mask, int *omask)
{
	if (shared_enabled) {
		share_lock(&shared_base_rw->us_siglock, 0);
		*omask = shared_base_rw->us_sigmask;
		shared_base_rw->us_sigmask |= mask &~ sigcantmask;
		share_unlock(&shared_base_rw->us_siglock, 0);
		return ESUCCESS;
	} else {
		e_emulator_error("sigblock but shared not enabled");
		return ENOSYS;
	}
}


#define E_DECLARE(routine) \
int routine(mach_port_t	serv_port, int syscode, \
	    integer_t	*argp, integer_t *rvalp) \
{ \

#define E_IF \
    if (shared_enabled) {

#define E_END(status) \
end:\
	e_checksignals(interrupt); \
	return (status); \
    } else { \
server: \
	return emul_generic(serv_port, interrupt, syscode, argp, rvalp); \
    } \
} \

#if 0
errno_t e_obreak(const char *addr)
{
	struct vmspace *vm = &shared_base_rw->us_vmspace;
	vm_offset_t new, old;
	integer_t rv, diff;

	if (shared_enabled) {
		old = (vm_offset_t)vm->vm_daddr;
		new = round_page((vm_offset_t)addr);
		if ((integer_t)(new - old) > shared_base_ro->us_limit.pl_rlimit[RLIMIT_DATA].rlim_cur)
		    return(ENOMEM);
		old = round_page(old + ctob(vm->vm_dsize));
		diff = new - old;
		if (diff > 0) {
			rv = vm_allocate(mach_task_self(), &old, diff, FALSE);
			if (rv != KERN_SUCCESS) {
				e_emulator_error("obrk: grow failed: x%x -> x%x = x%x",
						 old, new, rv);
				return(ENOMEM);
			}
			vm->vm_dsize += btoc(diff);
		} else if (diff < 0) {
			diff = -diff;
			rv = vm_deallocate(mach_task_self(), new, diff);
			if (rv != KERN_SUCCESS) {
				e_emulator_error("obrk: shrink failed: x%x -> x%x = x%x",
						 old, new, rv);
				return(ENOMEM);
			}
			vm->vm_dsize -= btoc(diff);
		}
	} else {
		return ENOSYS;
	}
}
#else 0
#endif 0

errno_t e_sbrk(int incr, caddr_t *addr)
{
	struct vmspace *vm = &shared_base_rw->us_vmspace;
	vm_offset_t new, old;
	integer_t rv, diff;

	if (shared_enabled) {
		old = (vm_offset_t)vm->vm_daddr;
		new = round_page(old + incr);
		if ((integer_t)(new - old) > shared_base_ro->us_limit.pl_rlimit[RLIMIT_DATA].rlim_cur)
		    return(ENOMEM);
		old = round_page(old + ctob(vm->vm_dsize));
		diff = new - old;
		if (diff > 0) {
			rv = vm_allocate(mach_task_self(), &old, diff, FALSE);
			if (rv != KERN_SUCCESS) {
				e_emulator_error("sbrk: grow failed: x%x -> x%x = x%x",
						 old, new, rv);
				return(ENOMEM);
			}
			vm->vm_dsize += btoc(diff);
		} else if (diff < 0) {
			diff = -diff;
			rv = vm_deallocate(mach_task_self(), new, diff);
			if (rv != KERN_SUCCESS) {
				e_emulator_error("sbrk: shrink failed: x%x -> x%x = x%x",
						 old, new, rv);
				return(ENOMEM);
			}
			vm->vm_dsize -= btoc(diff);
		}
		return ESUCCESS;
	} else {
		return ENOSYS;
	}
}

#ifdef notyet
#if 0
E_DECLARE(e_getdtablesize)
E_IF
	rvalp[0] = shared_base_ro->us_nofile;
E_END(ESUCCESS)


E_DECLARE(e_getuid)
E_IF
	rvalp[0] = shared_base_ro->us_ruid;
	rvalp[1] = shared_base_ro->us_uid;
E_END(ESUCCESS)


E_DECLARE(e_getpid)
E_IF
	rvalp[0] = shared_base_ro->us_pid;
	rvalp[1] = shared_base_ro->us_ppid;
E_END(ESUCCESS)


E_DECLARE(e_getgid)
E_IF
	rvalp[0] = shared_base_ro->us_rgid;
	rvalp[1] = shared_base_ro->us_gid;
E_END(ESUCCESS)


E_DECLARE(e_getgroups)
	register gid_t *gp;
	int *groups = (int *)argp[1];
	int size,i;
E_IF
	for (gp = &shared_base_ro->us_groups[NGROUPS];
	     gp > shared_base_ro->us_groups; gp--)
		if (gp[-1] != NOGROUP)
			break;
	size = gp - shared_base_ro->us_groups;
	if (argp[0] < size) return EINVAL;
	for (i=0; i < size; i++)
		groups[i]=shared_base_ro->us_groups[i];
	rvalp[0] = size;
E_END(ESUCCESS)

#endif 0
E_DECLARE(e_getrlimit)
	struct rlimit *rlp = (struct rlimit *)argp[1];
E_IF
	if (argp[0] >= RLIM_NLIMITS) return EINVAL;
	*rlp = shared_base_ro->us_limit.pl_rlimit[argp[0]];
E_END(ESUCCESS)

#if 0
E_DECLARE(e_umask)
E_IF
	rvalp[0] = shared_base_rw->us_cmask;
	shared_base_rw->us_cmask = argp[0];
E_END(ESUCCESS)


#endif 0
E_DECLARE(e_sigpending)
E_IF
        sigset_t *set = (sigset_t *)argp[0];
        *set = shared_base_rw->us_sig;
E_END(ESUCCESS)

E_DECLARE(e_sigprocmask)
E_IF
        int how = (int) argp[0];
        sigset_t mask = (sigset_t) argp[1];
        share_lock(&shared_base_rw->us_siglock, (struct proc *)0);
        rvalp[0] = shared_base_rw->us_sigmask;
        switch (how) {
	      case SIG_BLOCK:
		shared_base_rw->us_sigmask |= mask &~ sigcantmask;
		break;
	      case SIG_UNBLOCK:
		shared_base_rw->us_sigmask &= ~mask;
		break;
	      case SIG_SETMASK:
		shared_base_rw->us_sigmask = mask &~ sigcantmask;
		break;
	      default:
		share_unlock(&shared_base_rw->us_siglock, (struct proc *)0);
		return EINVAL;
	}
        share_unlock(&shared_base_rw->us_siglock, (struct proc *)0);
E_END(ESUCCESS)

#if 0
#ifdef	MAP_FILE

E_DECLARE(e_lseek)
	register struct file_info *fd;
E_IF
	if (argp[0] < 0 || argp[0] > shared_base_ro->us_nofile)
	    return (EBADF);
	fd = &shared_base_rw->us_file_info[argp[0]];

	share_lock(&fd->lock);
	if (fd->mapped && fd->open) {
	    get_it(argp[0], interrupt);
	    switch (argp[2]) {

	    case L_INCR:
		fd->offset += argp[1];
		break;

	    case L_XTND:
		fd->offset = argp[1] + fd->map_info.inode_size;
		break;

	    case L_SET:
		fd->offset = argp[1];
		break;

	    default:
		rel_it(argp[0], interrupt);
		return (EINVAL);
	    }
	    rvalp[0] = fd->offset;
	    rel_it(argp[0], interrupt);
	} else {
	    share_unlock(&fd->lock);
	    goto server;
	}
E_END(ESUCCESS)

#endif	MAP_FILE
#endif 0

E_DECLARE(e_read)
	int result;
E_IF
#ifdef	MAP_FILE
	if (!e_maprw(serv_port, interrupt, argp[0], argp[1], argp[2],
			     rvalp, 0, &result)) {
#endif	MAP_FILE
	if (argp[2] > 2*vm_page_size) goto server;
	spin_lock(&readwrite_lock);
	if (readwrite_inuse) {
		spin_unlock(&readwrite_lock);
		goto server;
	}
	readwrite_inuse = 1;
	spin_unlock(&readwrite_lock);
	result = e_readwrite(serv_port, interrupt, argp[0], argp[1], argp[2],
			     rvalp, 0, 1);
#ifdef	MAP_FILE
        }
#endif	MAP_FILE
E_END(result)

E_DECLARE(e_sigsetmask)

E_IF
	share_lock(&shared_base_rw->us_siglock, (struct proc *)0);

	rvalp[0] = shared_base_rw->us_sigmask;
	shared_base_rw->us_sigmask = argp[0] &~ sigcantmask;

	share_unlock(&shared_base_rw->us_siglock, (struct proc *)0);

E_END(ESUCCESS)
#endif notyet

void share_lock(struct shared_lock *x, struct proc *p)
{
	int i=0;
	boolean_t y;
	for (y = spin_try_lock(&(x)->lock); !y; y = spin_try_lock(&(x)->lock)) {
	    if (++i % 1024 == 0)
		e_emulator_error("share_lock failure %d", i);
	    swtch_pri(0);
	}
	(x)->who = shared_base_ro->us_proc_pointer & ~KERNEL_USER;
}

void share_unlock(struct shared_lock *x, struct proc *p)
{
    (x)->who = 0;
    spin_unlock(&(x)->lock);
    if ((x)->need_wakeup)
	bsd_share_wakeup(our_bsd_server_port,
			 (integer_t)x - (integer_t)shared_base_rw);
}

#if 0
void e_shared_sigreturn(proc_port, interrupt, old_on_stack, old_sigmask)
	mach_port_t	proc_port;
	boolean_t	*interrupt;
	int		old_on_stack;
	int		old_sigmask;
{
	share_lock(&shared_base_rw->us_lock);
	shared_base_rw->us_sigstack.ss_onstack = old_on_stack & 01;
	share_unlock(&shared_base_rw->us_lock);

	share_lock(&shared_base_rw->us_siglock);
	shared_base_rw->us_sigmask = old_sigmask & ~cantmask;
	share_unlock(&shared_base_rw->us_siglock);

	e_checksignals(interrupt);
}

#endif 0

void e_checksignals(interrupt)
	boolean_t	*interrupt;
{
	if (shared_enabled) {
		/*
		 *	This is really just a hint; so the lack
		 *	of locking isn't important.
		 */

		if (shared_base_ro->us_cursig ||
		    (shared_base_rw->us_sig &~
		     (shared_base_rw->us_sigmask |
		      ((shared_base_ro->us_flag & P_TRACED) ?
		       0 : shared_base_rw->us_sigignore))))
			*interrupt = TRUE;
	}
}

#if 0
E_DECLARE(e_close)
	struct file_info *fd;
E_IF
	if (argp[0] < 0 || argp[0] > shared_base_ro->us_lastfile)
	    return (EBADF);
	fd = &shared_base_rw->us_file_info[argp[0]];

	if (shared_base_rw->us_closefile != -1)
	    goto server;
	share_lock(&fd->lock);
	if (fd->mapped && fd->open) {
	    shared_base_rw->us_closefile = argp[0];
	    fd->open = FALSE;
	    share_unlock(&fd->lock);
	    return (ESUCCESS);
	}
	share_unlock(&fd->lock);
	goto server;
E_END(ESUCCESS)
#endif 0

#if 0
int e_readwrite(mach_port_t serv_port, boolean_t *interrupt, int fileno,
		char *data, unsigned int count, int *rval, int which,
		int copy)
{
	extern int bsd_readwrite();
	int result;

	rval[0] = -1;
	EPRINT(("e_readwrite: %s(%d,%x,%d)",(which?"write":"read"),fileno,data,count));
#if 0
	if (fileno < 0 || fileno > shared_base_ro->us_lastfile)
	    return (EBADF);
#endif 0
	if (which && copy)
	    bcopy(data, shared_readwrite, count);
	result = bsd_readwrite(serv_port, interrupt, which, fileno,
			       count, rval);
	if (!which && rval[0] > 0 && copy)
	    bcopy(shared_readwrite, data, rval[0]);
	if (copy) {
	    spin_lock(&readwrite_lock);
	    readwrite_inuse = 0;
	    spin_unlock(&readwrite_lock);
	}
	EPRINT(("e_readwrite: returns %d",result));
	return (result);
}
#endif

#ifdef	MAP_FILE

int e_maprw(mach_port_t serv_port, boolean_t *interrupt, int fileno,
		char *data, unsigned int count, int *rval, int which,
		int *result)
{
	register struct file_info *fd;
	register struct map_info *mi;
	char *from,*to;	
	char *wdata;
	int size,tsize;

	if (fileno < 0 || fileno > shared_base_ro->us_lastfile) {
	    EPRINT(("e_maprw:%d badfile",which));
	    *result = EBADF;
	    return TRUE;
	}
	fd = &shared_base_rw->us_file_info[fileno];
	mi = &fd->map_info;

	share_lock(&fd->lock);
	if (fd->mapped && fd->open) {
	  if(which?fd->write:fd->read) {
	    get_it(fileno, interrupt);
	    wdata = data;
	    tsize = size = count;
	    while (tsize > 0 && size > 0) {
		size = tsize;
		if (which & fd->append)
		    fd->offset = mi->inode_size;
		if (fd->offset < mi->offset ||
		    fd->offset >= mi->offset + mi->size)
		    bsd_maprw_remap(serv_port, interrupt, fileno, fd->offset, 
				    size);
		if (fd->offset + size > mi->offset + mi->size)
		    size = mi->offset + mi->size - fd->offset;
		from =(char * )( mi->address + fd->offset - mi->offset);
		if (which) {
		    if (fd->offset + size > mi->inode_size)
			mi->inode_size = fd->offset + size;
		    fd->dirty = TRUE;
		    to = from;
		    from = wdata;
		} else {
		    if (fd->offset + size > mi->inode_size)
			size = mi->inode_size - fd->offset;
		    if (size <= 0) size = 0;
		    to = wdata;
		}
		if (!fd->inuse) goto done;
		if (size > 0) bcopy(from, to, size);
		fd->offset += size;
		tsize-=size;
		wdata += size;
	    }
	    rel_it(fileno, interrupt);
done:
	    rval[0] = count - tsize;
	    *result = ESUCCESS;
	    return TRUE;
	  }
	  share_unlock(&fd->lock);
	  *result = EBADF;
	  e_emulator_error("e_maprw:%d (r/w) %d/%d",which,fd->read,fd->write);
	  return TRUE;
	}
	share_unlock(&fd->lock);
	return FALSE;
}

/* called with share_lock(fd->lock) held */
void get_it(fileno, interrupt)
	int fileno;
	int *interrupt;
{
	register struct file_info *fd = &shared_base_rw->us_file_info[fileno];

	if (!fd->control || fd->inuse ) {
	    share_unlock(&fd->lock);
	    bsd_maprw_request_it(our_bsd_server_port, interrupt, fileno);
	    return;
	}
	fd->inuse = TRUE;
	share_unlock(&fd->lock);
}

rel_it(fileno, interrupt)
	int fileno;
	int *interrupt;
{
	register struct file_info *fd = &shared_base_rw->us_file_info[fileno];

	share_lock(&fd->lock);
	if (fd->wants) {
		share_unlock(&fd->lock);
		bsd_maprw_release_it(our_bsd_server_port, interrupt, fileno);
	} else {
		fd->inuse = FALSE;
		share_unlock(&fd->lock);
	}
}

#endif	MAP_FILE

/*
 * For HP-UX procs, we propagate misc emulator state across exec.
 * The advantage being, we need not maintain it in the server.
 *
 * This hackery then, is how we do it.  We should probably check
 * that the "size" is sane, but for now, let's not bother.
 */

#define	__HPUX_SAVEMAGIC	0xABBADABA

void __e_hpux_exrest(void *addr, int size)
{
	unsigned int *savaddr = (int *)(shared_base_rw + 1);

	if (shared_enabled && *savaddr++ == __HPUX_SAVEMAGIC)
		bcopy(savaddr, addr, size);
}

void __e_hpux_exsave(void *addr, int size)
{
	unsigned int *savaddr = (int *)(shared_base_rw + 1);

	if (shared_enabled) {
		*savaddr++ = __HPUX_SAVEMAGIC;
		bcopy(addr, savaddr, size);
	}
}
#else	/* !MAP_UAREA */
void __e_hpux_exrest(void *addr, int size) {}
void __e_hpux_exsave(void *addr, int size) {}
#endif	MAP_UAREA
