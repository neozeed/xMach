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
 * $Log: e_mapped_socket.c,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:31  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_mapped_socket.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	June 1994
 *
 *	Mapped pipes user side.
 *
 *	EXPERIMENTAL AND INCOMPLETE.
 */

#include <e_defs.h>

typedef struct mapped_pipe_ctl {
	int version;			/* Protocol version number */
	spin_lock_t lock;		/* synch between readers (writers) */
	int gen;			/* generation number */
	vm_offset_t done_off;		/* data is valid until here */
	vm_offset_t busy_off;		/* data is being written until here */
	int waiters;			/* waiting readers (writers) count */
	int synch_waiters;		/* waiting for done_off count */
	int gen_check;			/* generation number duplicate */
} *mapped_pipe_ctl_t;

typedef struct e_pipe {
	vm_prot_t prot;		/* reader, writer, or both */
	vm_address_t buf;	/* read buf addr */
	vm_size_t size;
	mapped_pipe_ctl_t rctl;
	mapped_pipe_ctl_t wctl;
} *e_pipe_t;

e_pipe_t fileno_to_pipe(int fd);

mach_error_t e_mapped_pipe(int *fds)
{
	mach_error_t kr;
	mach_port_t pipe;
	int rfd, wfd;

	kr = bsd_pipe_create(process_self(), &pipe);
	if (kr)
	    emul_panic("e_mapped_pipe: bsd_pipe_create");
	kr = bsd_pipe_make_file(process_self(), pipe, FREAD, &rfd);
	if (kr)
	    emul_panic("e_mapped_pipe: bsd_pipe_make_file FREAD");
	kr = bsd_pipe_make_file(process_self(), pipe, FWRITE, &wfd);
	if (kr)
	    emul_panic("e_mapped_pipe: bsd_pipe_make_file FREAD");
	fds[0] = rfd;
	fds[1] = wfd;
	kr = mach_port_deallocate(mach_task_self(), pipe);
	if (kr)
	    emul_panic("e_mapped_pipe: mach_port_deallocate");
	return KERN_SUCCESS;
}

mach_error_t e_mapped_socketpair(int domain, int type, int protocol, int *fds)
{
	mach_error_t kr;
	mach_port_t pipe;
	int fd1, fd2;

	if (domain != AF_UNIX || type != SOCK_STREAM || protocol != 0)
	    emul_panic("e_mapped_socketpair: type check");

	kr = bsd_pipe_create(process_self(), &pipe);
	if (kr)
	    emul_panic("e_mapped_socketpair: bsd_pipe_create");
	kr = bsd_pipe_make_file(process_self(), pipe, FREAD|FWRITE, &fd1);
	if (kr)
	    emul_panic("e_mapped_socketpair: bsd_pipe_make_file FREAD");
	kr = bsd_pipe_make_file(process_self(), pipe, FREAD|FWRITE, &fd2);
	if (kr)
	    emul_panic("e_mapped_socketpair: bsd_pipe_make_file FREAD");
	fds[0] = fd1;
	fds[1] = fd2;
	kr = mach_port_deallocate(mach_task_self(), pipe);
	if (kr)
	    emul_panic("e_mapped_socketpair: mach_port_deallocate");
	return KERN_SUCCESS;
}

mach_error_t e_mapped_pipe_map(int fd, vm_prot_t mode)
{
	kern_return_t kr;
	mach_port_t handle;
	vm_address_t addr;
	vm_size_t size;
	e_pipe_t pipe;


	pipe = fileno_to_pipe(fd);

	kr = bsd_fd_to_file_port(process_self(), fd, &handle);
	if (kr)
	    emul_panic("e_mapped_pipe_map: bsd_fd_to_file_port");

	/* 
	 * READER, mode=VM_PROT_READ: map readers control page RW,
	 * writers control page RO and ring buffer RO.
	 * WRITER, mode=VM_PROT_WRITE: reader page RO, writer page RW,
	 * buffer RW.
	 * BOTH, mode=VM_PROT_READ|VM_PROT_WRITE: all RW. (potentially
	 * both ways).
	 */
	kr = bsd_pipe_map(process_self(), &addr, &size, TRUE, handle,
			  mode);
	if (kr)
	    emul_panic("e_mapped_pipe_map_read: bsd_fd_to_file_port");
	pipe->prot = mode;
	pipe->buf = addr + 2*vm_page_size;
	pipe->size = size - 2*vm_page_size;
	pipe->rctl = (mapped_pipe_ctl_t) addr;
	pipe->wctl = (mapped_pipe_ctl_t) addr + vm_page_size;
	return KERN_SUCCESS;
}

errno_t e_mapped_pipe_wait(int fd, boolean_t reader)
{
	errno_t err;
	fd_set set;
	int nready;

	FD_ZERO(&set);
	FD_SET(fd, &set);

	if (reader)
	    err = e_select(fd, &set, 0, 0, 0, &nready);
	else
	    err = e_select(fd, 0, &set, 0, 0, &nready);
	if (err)
	    emul_panic("e_mapped_pipe_wait");
	return err;
}

mach_error_t e_mapped_pipe_signal(int fd, boolean_t readers)
{
	kern_return_t kr;
	mach_port_t handle;

	kr = bsd_fd_to_file_port(process_self(), fd, &handle);
	if (kr)
	    emul_panic("e_mapped_pipe_signal: bsd_fd_to_file_port");

	kr = bsd_pipe_signal(process_self(), handle, readers);
	if (kr)
	    emul_panic("e_mapped_pipe_signal: bsd_pipe_signal");
}

mach_error_t e_mapped_pipe_send(
	int		fileno,
	vm_address_t	data,
	unsigned int	count,
	int		flags,
	int		*nsent)
{
	e_pipe_t pipe;
	mapped_pipe_ctl_t rctl;
	mapped_pipe_ctl_t wctl;
	int gen;
	vm_offset_t end, start;
	int space, amount, n1, n2, swaiters, waiters;

	/* Lookup from fd table */
	pipe = fileno_to_pipe(fileno);
	/* Check that this is a write end */
	if (pipe->prot & VM_PROT_WRITE == 0)
	    return EINVAL;

	rctl = pipe->rctl;
	wctl = pipe->wctl;

	spin_lock(&wctl->lock);
	while (TRUE) {
		do {
			gen = rctl->gen;
			end = rctl->done_off;
		} while (gen != rctl->gen);
		start = wctl->busy_off;
		space = end - start;
		if (space < 0)
		    space += pipe->size;
		if (space > 0)
		    break;
		/* no space (0) */
		wctl->waiters++;
		spin_unlock(&wctl->lock);
		e_mapped_pipe_wait(fileno, FALSE);
		spin_lock(&wctl->lock);
		wctl->waiters--;
	}

	amount = (space < count) ? space : count;
	end = wctl->busy_off;
	end += amount;
	n1 = amount;
	n2 = 0;
	if (end > pipe->buf + pipe->size) {
		n1 = pipe->buf + pipe->size - end; /* fits in end */
		end -= pipe->size;
		n2 = amount - n1;	/* goes to beginning */
	}
	wctl->busy_off = end;
	wctl->gen++;
	spin_unlock(&wctl->lock);

	if (n1 > 0)
	    bcopy(data, start, n1);
	if (n2 > 0)
	    bcopy(data + n1, pipe->buf, n2);

	spin_lock(&wctl->lock);
	while (wctl->done_off != start) {
		wctl->synch_waiters++;
		spin_unlock(&wctl->lock);
		e_mapped_pipe_wait(fileno, FALSE);
		spin_lock(&wctl->lock);
		wctl->synch_waiters--;
	}
	wctl->done_off = end;
	wctl->gen++;

	/* See if anyone needs to be signalled but get rid of lock */
	swaiters = wctl->synch_waiters;
	waiters = wctl->waiters;
	spin_unlock(&wctl->lock);

	if (swaiters < 0 || waiters < 0)
	    emul_panic("waiters count messup");
	/* Wake up writers trying to synch */
	if (swaiters)
	    e_mapped_pipe_signal(fileno, FALSE);
	/* Wake up readers as data is available. XXX watermarks! */
	if (waiters)
	    e_mapped_pipe_signal(fileno, TRUE);

	*nsent = amount;

}

mach_error_t e_mapped_pipe_recv(
	int		fileno,
	vm_address_t	data,
	unsigned int	count,
	int		flags,
	int		*nreceived)
{
	e_pipe_t pipe;
	mapped_pipe_ctl_t rctl;
	mapped_pipe_ctl_t wctl;
	int gen;
	vm_offset_t end, start;
	int space, amount, n1, n2, swaiters, waiters;

	/* Lookup from fd table */
	pipe = fileno_to_pipe(fileno);
	/* Check that this is a write end */
	if (pipe->prot & VM_PROT_READ == 0)
	    return EINVAL;

	rctl = pipe->rctl;
	wctl = pipe->wctl;

	spin_lock(&rctl->lock);
	while (TRUE) {
		do {
			gen = wctl->gen;
			end = wctl->done_off;
		} while (gen != wctl->gen);
		start = rctl->busy_off;
		space = end - start;
		if (space < 0)
		    space += pipe->size;
		if (space > 0)
		    break;
		/* no space (0) */
		rctl->waiters++;
		spin_unlock(&rctl->lock);
		e_mapped_pipe_wait(fileno, TRUE);
		spin_lock(&rctl->lock);
		rctl->waiters--;
	}

	amount = (space < count) ? space : count;

	end = rctl->busy_off;
	end += amount;
	n1 = amount;
	n2 = 0;
	if (end > pipe->buf + pipe->size) {
		n1 = pipe->buf + pipe->size - end; /* fits in end */
		end -= pipe->size;
		n2 = amount - n1;	/* goes to beginning */
	}
	rctl->busy_off = end;
	rctl->gen++;
	spin_unlock(&rctl->lock);

	if (n1 > 0)
	    bcopy(start, data, n1);
	if (n2 > 0)
	    bcopy(pipe->buf, data + n1, n2);

	spin_lock(&rctl->lock);
	while (rctl->done_off != start) {
		rctl->synch_waiters++;
		spin_unlock(&rctl->lock);
		e_mapped_pipe_wait(fileno, TRUE);
		spin_lock(&rctl->lock);
		rctl->synch_waiters--;
	}
	rctl->done_off = end;
	rctl->gen++;

	/* See if anyone needs to be signalled but get rid of lock */
	swaiters = rctl->synch_waiters;
	waiters = rctl->waiters;
	spin_unlock(&rctl->lock);

	if (swaiters < 0 || waiters < 0)
	    emul_panic("waiters count messup");
	/* Wake up readers trying to synch */
	if (swaiters)
	    e_mapped_pipe_signal(fileno, TRUE);
	/* Wake up writers as there is room. XXX watermarks! */
	if (waiters)
	    e_mapped_pipe_signal(fileno, FALSE);

	*nreceived = amount;

}
