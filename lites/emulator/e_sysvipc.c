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
 * $Log: e_sysvipc.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:28  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_sysvipc.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	November 1994
 *
 *	System V IPC (semaphores, shared memory, messages)
 */

#include <e_defs.h>
#include <sys/ipc.h>

#define NSHM_SEGS 64

struct shm_info {
	int fd;
	vm_address_t where;	/* one might be mapped into several places */
	vm_size_t size;
	key_t key;
} shm_segs[NSHM_SEGS] = {{0, 0}, };

/* Like ftruncate but reserves disk space */
errno_t e_file_set_size(int fd, off_t size)
{
	return e_lite_ftruncate(fd, size);
	/* XXX need to reserve disk space! */
}

errno_t e_shmget(key_t key, int size, int open_flags, int *id)
{
	char path[PATH_MAX];
	int open_mode = 0xfff;
	int new_id;
	errno_t err;
	int fd;

	/* find free id */
	e_mig_lock();
	for (new_id = 0; new_id < NSHM_SEGS; new_id++) {
		if (shm_segs[new_id].fd == 0) {
			shm_segs[new_id].fd = -1;
			break;
		}
	}
	e_mig_unlock();
	if (new_id >= NSHM_SEGS)
	    return ENOSPC;

	/* Open it */
	if (key == IPC_PRIVATE) {
		int foo = 0;
		open_mode = 0;
		open_flags |= O_EXCL | O_CREAT;
		/* find an unused key and open it (tmpnam) */
		do {
			sprintf(path, "/tmp/shm/p%d", foo++);
			err = e_open(path, open_flags, open_mode, &fd);
			
		} while (err == EEXIST);
		if (err == ESUCCESS) {
new_file:
			/* a new one was created */
			err = e_file_set_size(fd, size);
			if (err == ENOSPC) {
				err = ENOMEM;
				e_close(fd);
				e_unlink(path);
			}
		}
	} else {
		sprintf(path, "/tmp/shm/%d", key);

		if (open_flags & O_CREAT) {
			/* 
			 * When CREAT without EXCL we need to figure
			 * out whether the file was created or not.
			 */
			err = e_open(path, open_flags|O_EXCL, open_mode, &fd);
			if (err == ESUCCESS) {
				goto new_file;
			}
		}
		/* Open existing file */
		err = e_open(path, open_flags, open_mode, &fd);
	}
	if (err != ESUCCESS) {
		shm_segs[new_id].fd = 0;
		return err;
	}
	if (fd == 0)
	    e_dup(fd, &fd);
	shm_segs[new_id].fd = fd;
	if (id)
	    *id = new_id;
	return ESUCCESS;
}

/* XXX use ?_... instead of e_... because of non-Posix interface */
errno_t e_shmat(int shmid, vm_offset_t addr, vm_prot_t prot, caddr_t *addr_out)
{
	mach_error_t kr;
	int fd;
	struct vattr va;
	mach_port_t handle;

	boolean_t anywhere;
	vm_size_t size;

	if (shmid < 0 || shmid >= NSHM_SEGS)
	    return EINVAL;

	e_mig_lock();
	fd = shm_segs[shmid].fd;
	e_mig_unlock();

	if (fd <= 0)
	    return EINVAL;

	if (addr) {
		anywhere = FALSE;
	} else {
		addr = 0x4000000;
		anywhere = TRUE;
	}
	addr = trunc_page(addr);

	/* Get size from vnode */
	kr = bsd_getattr(process_self(), fd, &va);
	if (kr != KERN_SUCCESS)
	    return e_mach_error_to_errno(kr);
	size = round_page(va.va_size);

#if 0
	if ((shmflg & SHM_RDONLY) == 0)
	    prot |= VM_PROT_WRITE;
#else
	prot |= VM_PROT_WRITE;
#endif

	kr = bsd_fd_to_file_port(process_self(), fd, &handle);
	if (kr)
	    return e_mach_error_to_errno(kr);
	kr = bsd_vm_map(process_self(), &addr, size, 0, anywhere,
			handle, 0, FALSE, prot, prot,
			VM_INHERIT_SHARE);
	if (kr != KERN_SUCCESS)
	    return e_mach_error_to_errno(kr);
	*addr_out = (caddr_t) addr;

	return ESUCCESS;
}

errno_t e_shmdt(caddr_t shmaddr)
{
	return ENOSYS;
}

errno_t e_shmid_to_fd(
	int	shmid,
	int	*fd_out)
{
	int fd;
	errno_t err;

	if (shmid < 0 || shmid >= NSHM_SEGS)
	    return EINVAL;

	e_mig_lock();
	fd = shm_segs[shmid].fd;
	e_mig_unlock();

	if (fd <= 0)
	    return EINVAL;

	*fd_out = fd;
	return ESUCCESS;
}

struct sembuf;
struct semid_ds;
union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
};

errno_t e_semop(
	int	semid,
	struct	sembuf *sops,
	int	nsops)
{
	return ENOSYS;
}

errno_t e_semget(
	key_t	key,
	int	nsems,
	int	semflg,
	int	*id)
{
	return ENOSYS;
}

errno_t e_semctl(
	int	semid,
	int	semnum,
	int	cmd,
	union semun arg)
{
	return ENOSYS;
}
