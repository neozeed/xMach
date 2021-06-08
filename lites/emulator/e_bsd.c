/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994,1995 Johannes Helander
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
 * 25-Oct-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Word order fix in e_lite_lseek and sigprocmask fixes.
 *	From Mike Hibler <mike@cs.utah.edu>.
 *
 * 23-Oct-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Added e_lite_mmap with slightly different arguments.
 *
 *	Kludge to handle netbsd mount which gives us a string instead of
 *	an integer as a filesystem type.
 *
 *	Added "pad" to lseek arguments.  Offset is quad word aligned.
 *
 * $Log: e_bsd.c,v $
 * Revision 1.3  2000/11/07 00:41:24  welchd
 *
 * Added support for executing dynamically linked Linux ELF binaries
 *
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.6  1996/02/17  00:15:14  sclawson
 * e_mount() now checks for NetBSD-style mount strings by searching a table
 * of strings and mount numbers instead of assuming that the index into the
 * table is the mount number.
 *
 * Revision 1.5  1995/08/29  21:03:10  sclawson
 * return success if mmap is given a size of 0.
 *
 * Revision 1.4  1995/08/15  06:48:32  sclawson
 * modifications from lites-1.1-950808
 *
 * Revision 1.3  1995/03/23  01:38:14  law
 * Update to 950323 snapshot + utah changes
 *
 *
 *
 */
/* 
 *	File:	emulator/e_bsd_stubs.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	4.4 BSD system call handler functions.
 */

#include <e_defs.h>
#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <machine/stdarg.h>

/* GLOBALS */
mode_t saved_umask = 022;
pid_t getpid_cache = 0;
boolean_t XXX_enable_getpid_cache = TRUE;

/* I/O */
errno_t e_getdtablesize(int *size)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getdtablesize, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*size = rv[0];
	return err;
}

errno_t e_fsync(int fd)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	union {
		int fd;
		integer_t dummy;
	} a;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_fsync, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_dup2(int ofd, int nfd)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int ofd;
		int nfd;
	} a;
	a.nfd = nfd;
	a.ofd = ofd;

	kr = emul_generic(process_self(), SYS_dup2, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_fcntl(int fd, int cmd, int arg, int *whatever)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int 	  fd;
		int 	  cmd;
		integer_t arg;
	} a;
	a.arg = arg;
	a.cmd = cmd;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_fcntl, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*whatever = rv[0];
	return err;
}

errno_t e_open(const char *path, int flags, int mode, int *fd_out)
{
	errno_t err;
	kern_return_t kr;
	int fd;

	if (syscall_debug > 2)
	    e_emulator_error ("e_open(%s x%x x%x)", path, flags, mode);

#ifdef notyet
	mode &= saved_umask;
#endif

	kr = bsd_open(process_self(),
			flags,
			mode,
			path, strlen(path) + 1,
			&fd);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err)
	    return err;
	if (flags & O_PORT) {
		/* XXX Experimental and incomplete etc */
		mach_port_t port;
		kr = bsd_fd_to_file_port(process_self(), fd, &port);
		e_close(fd);
		err = kr ? e_mach_error_to_errno(kr) : 0;
		if (err)
		    return err;
		if (port < 200)
		    emul_panic("e_open: port name in fd name space. Fix me.");
		*fd_out = port;
	} else {
		*fd_out = fd;
	}
	return err;
}

errno_t e_close(int fd)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	union {
		int fd;
		integer_t dummy;
	} a;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_close, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_sync()
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_sync, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_ioctl(int fd, unsigned int cmd, char *argp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int fd;
		unsigned int cmd;
		char *argp;
	} a;
	a.argp = argp;
	a.cmd = cmd;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_ioctl, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* SOCKETS */
errno_t e_socket(int domain, int type, int protocol, int *fd)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int domain;
		int type;
		int protocol;
	} a;
	a.protocol = protocol;
	a.type = type;
	a.domain = domain;

	kr = emul_generic(process_self(), SYS_socket, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*fd = rv[0];
	return err;
}

errno_t e_connect(int fd, const struct sockaddr *name, int namelen)
{
	kern_return_t kr;

	kr = bsd_connect(process_self(), fd, (char *) name, namelen);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_bind(int fd, const struct sockaddr *name, int namelen)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	kr = bsd_bind(process_self(), fd, (char *) name, namelen);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_setsockopt(
	int fd,
	int level,
	int optname,
	const void *optval,
	int optlen)
{
	kern_return_t kr;

	kr = bsd_setsockopt(process_self(),
			    fd,
			    level,
			    optname,
			    optval,
			    (optval) ? optlen : 0);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_listen(int fd, int backlog)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int fd;
		int backlog;
	} a;
	a.backlog = backlog;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_listen, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_dup(int fd, int *nfd)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	union {
		int fd;
		integer_t dummy;
	} a;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_dup, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*nfd = rv[0];
	return err;
}

errno_t e_pipe(int *fds)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_pipe, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0) {
		fds[0] = rv[0];
		fds[1] = rv[1];
	}
	return err;
}

errno_t e_recvmsg_OLD(int fd, struct msghdr *msg, int flags, int *nreceived)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int fd;
		struct msghdr *msg;
		int flags;
	} a;
	a.flags = flags;
	a.msg = msg;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_recvmsg, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*nreceived = rv[0];
	return err;
}

errno_t e_sendmsg_OLD(int s, const struct msghdr *msg, int flags, int *nsent)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int s;
		const struct msghdr *msg;
		int flags;
	} a;
	a.flags = flags;
	a.msg = msg;
	a.s = s;

	kr = emul_generic(process_self(), SYS_sendmsg, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*nsent = rv[0];
	return err;
}

errno_t e_recvmsg(int s, struct msghdr *msg, int flags, size_t *nreceived)
{
	errno_t err;
	kern_return_t kr;
	int i;
	struct sockaddr *from = (struct sockaddr *) msg->msg_name;
	struct iovec *iov;
	vm_offset_t addr;
	vm_size_t sz = 0;
	boolean_t use_inline;

	/* 
	 * It would be better to let the server decide how to return
	 * the data.  Perhaps use the extend_reply_msg mechanism.
	 * But I'll do it this way for now.  The long term solution of
	 * course is to use mapped sockets.
	 */
	i = msg->msg_iovlen;
	if (i < 1) {
		sz = 0;
		addr = 0;
		use_inline = TRUE;
	} else if (i == 1 && msg->msg_iov->iov_len <= SMALL_ARRAY_LIMIT) {
		/* 
		 * exactly one iov and short enough.
		 * Use inline data transfer and let mig copy the data
		 * right into the user buffer.
		 */
		sz = msg->msg_iov->iov_len;
		addr = (vm_offset_t) msg->msg_iov->iov_base;
		use_inline = TRUE;
	} else {
		/* first calculate total maximum size */
		sz = 0;
		for (i = 0; i < msg->msg_iovlen; i++)
		    sz += msg->msg_iov[i].iov_len;
		use_inline = FALSE;
	}

	if (use_inline) {
	  	mach_msg_type_number_t msg_namelen    = msg->msg_namelen;
	  	mach_msg_type_number_t msg_controllen = msg->msg_controllen;

		kr = bsd_recvmsg_short(process_self(),
				       s, flags,
				       &msg->msg_flags,
				       msg->msg_namelen,
				       nreceived,
				       (char *) from, &msg_namelen,
				       msg->msg_controllen,
				       msg->msg_control, &msg_controllen,
				       sz,
				       (char *) addr, &sz);
		err = kr ? e_mach_error_to_errno(kr) : 0;
		if (err == 0) {
		  	emul_assert(*nreceived == sz);
			msg->msg_namelen    = msg_namelen;
			msg->msg_controllen = msg_controllen;
		}
	} else {
		vm_offset_t mem;
		vm_size_t memsz;
	  	mach_msg_type_number_t msg_namelen    = msg->msg_namelen;
	  	mach_msg_type_number_t msg_controllen = msg->msg_controllen;

		kr = bsd_recvmsg_long(process_self(),
				      s, flags,
				      &msg->msg_flags,
				      nreceived,
				      msg->msg_namelen,
				      (char *) from, &msg_namelen,
				      msg->msg_controllen,
				      msg->msg_control, &msg_controllen,
				      sz,
				      (char **) &addr, &memsz);
		err = kr ? e_mach_error_to_errno(kr) : 0;
		if (err)
		    return err;

		msg->msg_namelen    = msg_namelen;
	  	msg->msg_controllen = msg_controllen;

		mem = addr;
		/* second iov pass: copy data */
		for (i = 0; i < msg->msg_iovlen && sz > 0; i++) {
			size_t count = msg->msg_iov[i].iov_len;
			if (count > sz)
			    count = sz;
			bcopy((void *)addr, msg->msg_iov[i].iov_base, count);
			addr += count;
			sz -= count;
		}
		emul_assert(sz == 0);
		if (mem)
		    vm_deallocate(mach_task_self(), mem, round_page(memsz));
	}
	return err;
}

errno_t e_sendmsg(int s, const struct msghdr *msg, int flags, size_t *nsent)
{
	errno_t err;
	kern_return_t kr;
	int i;
	struct sockaddr *to = (struct sockaddr *) msg->msg_name;
	struct iovec *iov;
	vm_offset_t addr, mem = 0;
	vm_size_t sz = 0;

	i = msg->msg_iovlen;
	if (i < 1) {
		sz = 0;
		addr = 0;
	} else if (i > 1) {
		/* Do iov processing here */
		sz = 0;		/* first calculate total size */
		for (i = 0; i < msg->msg_iovlen; i++)
		    sz += msg->msg_iov[i].iov_len;
		kr = vm_allocate(mach_task_self(), &mem, round_page(sz), TRUE);
		if (kr)
		    return e_mach_error_to_errno(kr);
		addr = mem;
		/* second pass: copy data */
		for (i = 0; i < msg->msg_iovlen; i++) {
			bcopy(msg->msg_iov[i].iov_base,
			      (void *)addr,
			      msg->msg_iov[i].iov_len);
			addr += msg->msg_iov[i].iov_len;
		}
		addr = mem;		    
	} else {
		/* exactly one iov */
		sz = msg->msg_iov->iov_len;
		addr = (vm_offset_t) msg->msg_iov->iov_base;
	}

	kr = ((sz <= SMALL_ARRAY_LIMIT)
	      ? bsd_sendmsg_short
	      : bsd_sendmsg_long)(process_self(),
				  s, flags,
				  (char *) addr, sz,
				  (char *) to, msg->msg_namelen,
				  msg->msg_control, msg->msg_controllen,
				  nsent);

	err = kr ? e_mach_error_to_errno(kr) : 0;

	if (mem)
	    vm_deallocate(mach_task_self(), mem, round_page(sz));

	return err;
}

errno_t e_read(int fd, void *buf, size_t nbytes, size_t *nread)
{
	struct iovec iov;

	iov.iov_base = buf;
	iov.iov_len = nbytes;

	return e_readv(fd, &iov, 1, nread);
}

errno_t e_readv(int fd, struct iovec *iov, int iovlen, size_t *nread)
{
	errno_t err;
	kern_return_t kr;
	int i;
	vm_offset_t addr;
	vm_size_t sz = 0;
	boolean_t use_inline;

	/* 
	 * It would be better to let the server decide how to return
	 * the data.  Perhaps use the extend_reply_msg mechanism.
	 * But I'll do it this way for now.  The long term solution of
	 * course is to use mapped sockets.
	 */
	i = iovlen;
	if (i < 1) {
		sz = 0;
		addr = 0;
		use_inline = TRUE;
	} else if (i == 1 && iov->iov_len <= SMALL_ARRAY_LIMIT) {
		/* 
		 * exactly one iov and short enough.
		 * Use inline data transfer and let mig copy the data
		 * right into the user buffer.
		 */
		sz = iov->iov_len;
		addr = (vm_offset_t) iov->iov_base;
		use_inline = TRUE;
	} else {
		/* first calculate total maximum size */
		sz = 0;
		for (i = 0; i < iovlen; i++)
		    sz += iov[i].iov_len;
		use_inline = FALSE;
	}

	if (use_inline) {
		kr = bsd_read_short(process_self(),
				    fd,
				    sz,
				    nread,
				    (char *) addr, &sz);
		err = kr ? e_mach_error_to_errno(kr) : 0;
		if (err == 0) {
		  	emul_assert(*nread == sz);
		}
	} else {
		vm_offset_t mem;
		vm_size_t memsz;

		kr = bsd_read_long(process_self(),
				   fd,
				   sz,
				   nread,
				   (char **) &addr, &memsz);
		err = kr ? e_mach_error_to_errno(kr) : 0;
		if (err)
		    return err;

		mem = addr;
		/* second iov pass: copy data */
		for (i = 0; i < iovlen && sz > 0; i++) {
			size_t count = iov[i].iov_len;
			if (count > sz)
			    count = sz;
			bcopy((void *)addr, iov[i].iov_base, count);
			addr += count;
			sz -= count;
		}
		emul_assert(sz == 0);
		if (mem)
		    vm_deallocate(mach_task_self(), mem, round_page(memsz));
	}
	return err;
}

errno_t e_writev(int fd, const struct iovec *iov, int iovlen, size_t *nwritten)
{
	errno_t err;
	kern_return_t kr;
	int i;
	vm_offset_t addr, mem = 0;
	vm_size_t sz = 0;

	i = iovlen;
	if (i < 1) {
		sz = 0;
		addr = 0;
	} else if (i > 1) {
		/* Do iov processing here */
		sz = 0;		/* first calculate total size */
		for (i = 0; i < iovlen; i++)
		    sz += iov[i].iov_len;
		kr = vm_allocate(mach_task_self(), &mem, round_page(sz), TRUE);
		if (kr)
		    return e_mach_error_to_errno(kr);
		addr = mem;
		/* second pass: copy data */
		for (i = 0; i < iovlen; i++) {
			bcopy(iov[i].iov_base, (void *)addr, iov[i].iov_len);
			addr += iov[i].iov_len;
		}
		addr = mem;		    
	} else {
		/* exactly one iov */
		sz = iov->iov_len;
		addr = (vm_offset_t) iov->iov_base;
	}

	kr = ((sz <= SMALL_ARRAY_LIMIT)
	      ? bsd_write_short
	      : bsd_write_long)(process_self(),
				  fd,
				  (char *) addr, sz,
				  nwritten);

	err = kr ? e_mach_error_to_errno(kr) : 0;

	if (mem)
	    vm_deallocate(mach_task_self(), mem, round_page(sz));

	return err;
}

errno_t e_write(int fd, const void *buf, size_t nbytes, size_t *nwritten)
{
	struct iovec iov;

	iov.iov_base = buf;
	iov.iov_len = nbytes;

	return e_writev(fd, &iov, 1, nwritten);
}

errno_t e_accept(int s, struct sockaddr *addr, int *addrlen, int *fd)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int s;
		struct sockaddr *addr;
		int *addrlen;
	} a;
	a.addrlen = addrlen;
	a.addr = addr;
	a.s = s;

	kr = emul_generic(process_self(), SYS_accept, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*fd = rv[0];
	return err;
}

errno_t e_getpeername(int fd, struct sockaddr *name, int *namelen)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int	s;
		caddr_t	asa;
		int	*alen;
	} a;
	a.s = fd;
	a.asa = (caddr_t) name;
	a.alen = namelen;

	kr = emul_generic(process_self(), SYS_getpeername, &a,&rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getsockname(int s, struct sockaddr *name, int *namelen)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int s;
		struct sockaddr *name;
		int *namelen;
	} a;
	a.namelen = namelen;
	a.name = name;
	a.s = s;

	kr = emul_generic(process_self(), SYS_getsockname, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* ATTRIBUTES */

#include <e_templates.h>
DEF_STAT(lite)
DEF_LSTAT(lite)
DEF_FSTAT(lite)

errno_t e_chmod(const char *path, mode_t mode)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_mode = mode;
	kr = bsd_path_setattr(process_self(), TRUE,
			      path, strlen(path)+1, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_chown(const char *path, uid_t owner, gid_t group)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_uid = owner;
	va.va_gid = group;
	kr = bsd_path_setattr(process_self(), TRUE, path, strlen(path)+1, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_chflags(const char *path, int flags)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_flags = flags;
	kr = bsd_path_setattr(process_self(), TRUE, path, strlen(path)+1, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_fchflags(int fd, int flags)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_flags = flags;
	kr = bsd_setattr(process_self(), fd, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_fchown(int fd, uid_t owner, gid_t group)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_uid = owner;
	va.va_gid = group;
	kr = bsd_setattr(process_self(), fd, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_fchmod(int fd, mode_t mode)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_mode = mode;
	kr = bsd_setattr(process_self(), fd, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_lite_truncate(
	const char *path,
#ifndef alpha			/* XXX depends on pointer length */
	int pad,
#endif
	off_t length)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_size = length;
	kr = bsd_path_setattr(process_self(), TRUE, path, strlen(path)+1, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_lite_ftruncate(int fd, int pad, off_t length)
{
	kern_return_t kr;
	struct vattr va;

	VATTR_NULL(&va);
	va.va_size = length;
	kr = bsd_setattr(process_self(), fd, va);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_flock(int fd, int operation)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int fd;
		int operation;
	} a;
	a.operation = operation;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_flock, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_lite_lseek(int fd, int pad, off_t offset, int sbase, off_t *pos)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

#if 1
	struct {
		int fd;
		int pad;
		off_t offset;
		int sbase;
	} a;
	a.sbase = sbase;
	a.offset = offset;
	a.pad = 0;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_lseek, &a, &rv);
#else
	kr = bsd_lseek(process_self(), fd, offset, sbase, (off_t *)rv);
#endif
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0 && pos)
		*pos = *(off_t *)rv;
	return err;
}

/* DIRECTORIES */

errno_t e_lite_getdirentries(
	int	fd,
	char	*buf,
	int	nbytes,
	long	*basep,
	int	*nread)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int fd;
		char *buf;
		int nbytes;
		long *basep;
	} a;
	a.basep = basep;
	a.nbytes = nbytes;
	a.buf = buf;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_getdirentries, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
	    *nread = rv[0];
	return err;
}

errno_t e_mkdir(const char *path, mode_t mode)
{
	kern_return_t kr;

	kr = bsd_mkdir(process_self(),
			 mode,
			 path, strlen(path) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_rmdir(const char *path)
{
	kern_return_t kr;

	kr = bsd_rmdir(process_self(), path, strlen(path) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_mkfifo(const char *path, mode_t mode)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *path;
		unsigned int mode; /* mode_t */
	} a;
	a.mode = mode;
	a.path = path;

	kr = emul_generic(process_self(), SYS_mkfifo, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_chroot(const char *dirname)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	kr = bsd_chroot(process_self(), dirname, strlen(dirname) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_symlink(const char *oname, const char *nname)
{
	kern_return_t kr;

	kr = bsd_symlink(process_self(),
			 oname, strlen(oname) + 1,
			 nname, strlen(nname) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_readlink(const char *path, char *buf, int bufsize, int *nread)
{
	errno_t err;
	kern_return_t kr;
	mach_msg_type_number_t buflen;

	buflen = bufsize;
	kr = bsd_readlink(process_self(),
			bufsize,		/* max length */
			path, strlen(path) + 1,
			buf,
			&buflen);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*nread = buflen;
	return err;
}

errno_t e_access(const char *path, int amode)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	kr = bsd_access(process_self(),
			amode,
			path, strlen(path) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_rename(const char *old, const char *new)
{
	errno_t err;
	kern_return_t kr;

	kr = bsd_rename(process_self(),
			old, strlen(old) + 1,
			new, strlen(new) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_link(const char *target, const char *linkname)
{
	kern_return_t kr;

	kr = bsd_link(process_self(),
		      target, strlen(target) + 1,
		      linkname, strlen(linkname) + 1);
	
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_unlink(const char *path)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	kr = bsd_unlink(process_self(), path, strlen(path) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_chdir(const char *path)
{
	kern_return_t kr;

	kr = bsd_chdir(process_self(), path, strlen(path) + 1);

	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_fchdir(const char *path)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *path;
	} a;
	a.path = path;

	kr = emul_generic(process_self(), SYS_fchdir, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_mknod(const char *path, mode_t mode, dev_t dev)
{
	errno_t err;
	kern_return_t kr;

	kr = bsd_mknod(process_self(),
			 mode,
			 dev,
			 path, strlen(path) + 1);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}



errno_t e_getfsstat(struct statfs *buf, long bufsize, int flags, int *nstructs)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		struct statfs *buf;
		long bufsize;
		int flags;
	} a;
	a.flags = flags;
	a.bufsize = bufsize;
	a.buf = buf;

	kr = emul_generic(process_self(), SYS_getfsstat, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*nstructs = rv[0];
	return err;
}

errno_t e_statfs(const char *path, struct statfs *buf)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *path;
		struct statfs *buf;
	} a;
	a.buf = buf;
	a.path = path;

	kr = emul_generic(process_self(), SYS_statfs, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_fstatfs(int fd, struct statfs *buf)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int fd;
		struct statfs *buf;
	} a;
	a.buf = buf;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_fstatfs, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_mount(int type, const char *dir, int flags, void * data)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int type;
		const char *dir;
		int flags;
		caddr_t data;
	} a;

 	/* Kludge alert. The netbsd mount command gives us a string
 	 * in the type field
	 *
	 * Doesn't work with 64 bit pointers.
 	 */
#if !defined(alpha)
 	if ((u_long)type > MOUNT_MAXTYPE) {
		int i;
		const struct { 
			char *name; 
			int type; 
		} mountnames[] = INITMOUNTNAMES;
		int numnames = (sizeof(mountnames) / sizeof(mountnames[0]));

		for (i = 0; i < numnames; i++)
		    if(strcmp((char *) type, mountnames[i].name) == 0) {
			    type = mountnames[i].type;
			    break;
		    }
 	}
#endif /* !alpha */
	a.data = data;
	a.flags = flags;
	a.dir = dir;
	a.type = type;

	kr = emul_generic(process_self(), SYS_mount, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_unmount(const char *path, int flags)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *path;
		int flags;
	} a;
	a.flags = flags;
	a.path = path;

	kr = emul_generic(process_self(), SYS_unmount, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* PROCESSES */
/*
 * Copy zero-terminated string and return its length,
 * including the trailing zero.  If longer than max_len,
 * return -1.
 */
int
copystr(from, to, max_len)
	register char	*from;
	register char	*to;
	register int	max_len;
{
	register int	count;

	count = 0;
	while (count < max_len) {
	    count++;
	    if ((*to++ = *from++) == 0) {
		return (count);
	    }
	}
	return (-1);
}

int
copy_args(argp, arg_count, arg_addr, arg_size, char_count)
	char		**argp;
	int		*arg_count;	/* OUT */
	vm_offset_t	*arg_addr;	/* IN/OUT */
	vm_size_t	*arg_size;	/* IN/OUT */
	unsigned int	*char_count;	/* IN/OUT */
{
	register char		*ap;
	register int		len;
	register unsigned int	cc = *char_count;
	register char		*cp = (char *)*arg_addr + cc;
	register int		na = 0;

	while ((ap = *argp++) != 0) {
	    na++;
	    while ((len = copystr(ap, cp, *arg_size - cc)) < 0) {
		/*
		 * Allocate more
		 */
		vm_offset_t	new_arg_addr;

		if (vm_allocate(mach_task_self(),
				&new_arg_addr,
				(*arg_size) * 2,
				TRUE) != KERN_SUCCESS)
		    return E2BIG;
		(void) vm_copy(mach_task_self(),
				*arg_addr,
				*arg_size,
				new_arg_addr);
		(void) vm_deallocate(mach_task_self(),
				*arg_addr,
				*arg_size);
		*arg_addr = new_arg_addr;
		*arg_size *= 2;

		cp = (char *)*arg_addr + cc;
	    }
	    cc += len;
	    cp += len;
	}
	*arg_count = na;
	*char_count = cc;
	return (0);
}

boolean_t optimize_exec_call = TRUE;

errno_t e_execve(const char *fname, const char *argp[], const char *envp[])
{
	vm_offset_t	arg_addr;
	vm_size_t	arg_size;
	int		arg_count, env_count;
	unsigned int	char_count = 0;
	int		error;
	vm_offset_t	arg_start;
	errno_t		ret;

	if (!fname)
	    return EFAULT;

	if (syscall_debug > 1)
	    e_emulator_error ("e_execve(%s,[%v],[%v])", fname, argp, envp);

	/*
	 * wait a mo ... If this exec won't succeed don't
	 * bother with the drivel below.
	 */
	if (optimize_exec_call && (ret = e_access(fname,
						  1 /* execute access */)))
		return ret;
	/*
	 * Copy the argument and environment strings into
	 * contiguous memory.  Since most argument lists are
	 * small, we allocate a page to start, and add more
	 * if we need it.
	 */
	arg_size = vm_page_size;
	arg_addr = EMULATOR_BASE;
	error = vm_allocate(mach_task_self(), &arg_addr, arg_size, TRUE);
	if (error) {
		e_emulator_error("e_execve: vm_allocate x%x", error);
		return error;
	}

	if (argp) {
	    if (copy_args(argp, &arg_count,
			&arg_addr, &arg_size, &char_count) != 0)
		return E2BIG;
	} else {
	    arg_count = 0;
	}

	if (envp) {
	    if (copy_args(envp, &env_count,
			&arg_addr, &arg_size, &char_count) != 0)
		return E2BIG;
	} else {
	    env_count = 0;
	}

	/*
	 * Exec the program.  Get back the command file name (if any),
	 * and the entry information (machine-dependent).
	 */
	error = bsd_execve(process_self(),
			   arg_addr,
			   arg_size,
			   arg_count,
			   env_count,
			   fname, strlen(fname) + 1);
	if (error) {
	    (void) vm_deallocate(mach_task_self(), arg_addr, arg_size);
	    return (error);
	}
	/* Impossible */
	return EALREADY;	/* XXX */
}

noreturn exit(int status)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	union {
		int status;
		integer_t dummy;
	} a;
	a.status = status;

	(noreturn) emul_generic(process_self(), SYS_exit, &a, &rv);
	/*NOTREACHED*/
	task_suspend(mach_task_self());
	(noreturn) task_terminate(mach_task_self());
	exit(status);		/* avoid bogus warning about returning */
}

errno_t e_vfork(pid_t *pid)
{
	boolean_t ischild;
	errno_t err;
	getpid_cache = 0;
	err = e_fork_call(TRUE, pid, &ischild);
	if (ischild)
	    *pid = 0;
	return err;
}

errno_t e_fork(pid_t *pid)
{
	boolean_t ischild;
	errno_t err;
	getpid_cache = 0;
	err = e_fork_call(FALSE, pid, &ischild);
	if (ischild)
	    *pid = 0;
	return err;
}

errno_t e_lite_wait4(
	pid_t		pid,
	int		*status,
	int		options,
	struct rusage	*rusage,
	pid_t		*wpid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int pid;	/* pid_t */
		int *status;
		int options;
		struct rusage *rusage;
	} a;
	a.rusage = rusage;
	a.options = options;
	a.status = status;
	a.pid = pid;

	kr = emul_generic(process_self(), SYS_wait4, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*wpid = rv[0];
	return err;
}

errno_t e_setgid(gid_t gid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		unsigned int gid; /* gid_t */
	} a;
	a.gid = gid;

	kr = emul_generic(process_self(), SYS_setgid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_setegid(gid_t egid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		unsigned int egid; /* gid_t */
	} a;
	a.egid = egid;

	kr = emul_generic(process_self(), SYS_setegid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_seteuid(uid_t euid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		unsigned int euid; /* uid_t */
	} a;
	a.euid = euid;

	kr = emul_generic(process_self(), SYS_seteuid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getpid(pid_t *pid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	if (!XXX_enable_getpid_cache || getpid_cache == 0) {
		kr = emul_generic(process_self(), SYS_getpid, &a, &rv);
		err = kr ? e_mach_error_to_errno(kr) : 0;
		if (err == 0) {
			getpid_cache = rv[0];
			*pid = rv[0];
		}
	} else {
		*pid = getpid_cache;
		err = ESUCCESS;
	}
	return err;
}

errno_t e_getgroups(u_int gidsetsize, int *gidset, int *ngroups)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		u_int gidsetsize;
		int *gidset;
	} a;
	a.gidset = gidset;
	a.gidsetsize = gidsetsize;

	kr = emul_generic(process_self(), SYS_getgroups, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*ngroups = rv[0];
	return err;
}

errno_t e_setgroups(int gidsetsize, gid_t *gidset)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int gidsetsize;
		gid_t *gidset;
	} a;
	a.gidset = gidset;
	a.gidsetsize = gidsetsize;

	kr = emul_generic(process_self(), SYS_setgroups, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getpgrp(pid_t *pgrp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getpgrp, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*pgrp = rv[0];
	return err;
}

errno_t e_setpgid(pid_t pid, pid_t pgrp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int pid;	/* XXX pid_t but sign extend */
		int pgrp;	/* XXX pid_t but sign extend */
	} a;
	a.pgrp = pgrp;
	a.pid = pid;

	kr = emul_generic(process_self(), SYS_setpgid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_setuid(uid_t uid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		unsigned int uid; /* uid_t */
	} a;
	a.uid = uid;

	kr = emul_generic(process_self(), SYS_setuid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getuid(uid_t *uid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getuid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*uid = rv[0];
	return err;
}

errno_t e_geteuid(uid_t *euid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_geteuid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*euid = rv[0];
	return err;
}

#ifndef PT_TRACE_ME
#define PT_TRACE_ME 0
#endif

errno_t e_ptrace(int request, pid_t pid, int *addr, int data)
{
	errno_t err;
	kern_return_t kr = KERN_SUCCESS;

	if (request != PT_TRACE_ME)
	    return EINVAL;

#ifdef notyet
	kr = bsd_set_trace(process_self(), TRUE);
#endif
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_getppid(pid_t *pid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getppid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*pid = rv[0];
	return err;
}

errno_t e_getegid(gid_t *gid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getegid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*gid = rv[0];
	return err;
}

errno_t e_getgid(gid_t *gid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_getgid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*gid = rv[0];
	return err;
}

/* XXX The manual page interface differs */
errno_t e_getlogin(char *name, int namelen)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		char *namebuf;
		size_t namelen;
	} a;
	a.namelen = namelen;
	a.namebuf = name;

	kr = emul_generic(process_self(), SYS_getlogin, &a, &rv);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_setlogin(const char *name)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *name;
	} a;
	a.name = name;

	kr = emul_generic(process_self(), SYS_setlogin, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* SIGNALS */
errno_t e_kill(pid_t pid, int sig)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int pid;	/* pid_t */
		int sig;
	} a;
	a.sig = sig;
	a.pid = pid;

	kr = emul_generic(process_self(), SYS_kill, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* 
 * The manual says sigsuspend takes a pointer to mask but in fact the
 * library stub pushes the actual argument on the stack -- not the
 * pointer.
 */
errno_t e_sigsuspend(const sigset_t sigmask)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	union {
		sigset_t mask;
		integer_t dummy;
	} a;
	a.mask = sigmask;

	kr = emul_generic(process_self(), SYS_sigsuspend, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

struct sigstack	zero_ss = { 0, 0 };

errno_t e_sigstack(const struct sigstack *ss, struct sigstack *oss)
{
	errno_t err;
	kern_return_t kr;

	struct sigstack	old_sig_stack;

#ifdef	NMAP_UAREA 
    if (shared_enabled) {
	share_lock(&shared_base_rw->us_lock);
	if (oss)
	    *oss = shared_base_rw->us_sigstack;
	if (ss)
	    shared_base_rw->us_sigstack = *ss;
	share_unlock(&shared_base_rw->us_lock);
	return ESUCCESS;
    } else {
#endif	NMAP_UAREA
	kr = bsd_sigstack(process_self(),
			(ss != 0),
			(ss) ? *ss : zero_ss,
			&old_sig_stack);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0 && oss)
	    *oss = old_sig_stack;
	return err;
#ifdef	NMAP_UAREA
    }
#endif	NMAP_UAREA
}

errno_t e_shutdown(int fd, int how)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int fd;
		int how;
	} a;
	a.how = how;
	a.fd = fd;

	kr = emul_generic(process_self(), SYS_shutdown, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_socketpair(int domain, int type, int protocol, int *sv)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int domain;
		int type;
		int protocol;
		int *sv;
	} a;
	a.sv = sv;
	a.protocol = protocol;
	a.type = type;
	a.domain = domain;

	kr = emul_generic(process_self(), SYS_socketpair, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getsockopt(
	int fd,
	int level,
	int optname,
	void *optval,
	int *optlen)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	integer_t	valsize = sizeof(sockarg_t);
	sockarg_t	val_buf;

	kr = bsd_getsockopt(process_self(),
			    fd,
			    level,
			    optname,
			    val_buf,
			    &valsize);
	if (kr)
	    return e_mach_error_to_errno(kr);

	if (optval) {
	    if (valsize > *optlen)
		valsize = *optlen;
	    bcopy(val_buf, optval, valsize);
	    *optlen = valsize;
	}
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int sig;
		const struct sigaction *act;
		struct sigaction *oact;
	} a;
	a.oact = oact;
	a.act = act;
	a.sig = sig;

	kr = emul_generic(process_self(), SYS_sigaction, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;

#if ASYNCH_SIGNALS
	/* 
	 * When installing a signal handler for the first time start
	 * another thread for doing signal wakeup.
	 */
	if (!err && act && (act->sa_handler != SIG_DFL)
	    && (act->sa_handler != SIG_IGN))
	{
		e_enable_asynch_signals();
	}
#endif
	return err;
}

/*
 * Supports the documented sigprocmask interface.
 * Must be converted to the "fast" interface actually used by the server
 * (see e_bsd_sigprocmask).
 */
errno_t e_sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	sigset_t nset;
	int nhow;

	/*
	 * No new mask specified.  Since we don't pass a pointer the server
	 * cannot distinguish this from a 0 mask value, so convert this into
	 * a SIGBLOCK with 0 mask (we must still make the call since the old
	 * mask value might be desired).
	 */
	if (set == 0) {
		nhow = SIG_BLOCK;
		nset = 0;
	} else {
		nhow = how;
		nset = *set;
	}
	return e_bsd_sigprocmask(nhow, nset, oset);
}

/*
 * Supports the "fast" (two argument) sigprocmask interface.
 * Use this routine in sysent for emulation of existing 4.4 systems.
 * Here the actual mask is passed in instead of a pointer and the old mask
 * is returned as a return value instead of copied out via another pointer.
 */
errno_t e_bsd_sigprocmask(int how, const sigset_t set, sigset_t *oset)
{
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int how;
		sigset_t set;
	} a;
	a.set = set;
	a.how = how;

	kr = emul_generic(process_self(), SYS_sigprocmask, &a, rv);
	/*
	 * Copy the old mask value to the expected place
	 */
#ifdef alpha			/* XXX */
	if ((vm_offset_t)oset & (sizeof(oset)-1)) {
		e_emulator_error("e_bsd_sigprocmask:oset=x%lx --> 0", oset);
		oset = 0;	/* XXX. tmp hack to make zsh work on alpha */
	}
#endif
	if (kr == KERN_SUCCESS && oset)
	    *oset = rv[0];
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_sigpending(sigset_t *set)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		sigset_t *set;
	} a;
	a.set = set;

	kr = emul_generic(process_self(), SYS_sigpending, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_revoke(char *fname)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		char *fname;
	} a;
	a.fname = fname;

	kr = emul_generic(process_self(), SYS_revoke, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_umask(mode_t mask, mode_t *old_mask)
{
	/* XXX locking for strict consistency */
	*old_mask = saved_umask;
	saved_umask = mask;

	{
		/* For now tell the server about it too. */
		integer_t rv[2];
		union {
			unsigned int mask; /* mode_t */
			integer_t dummy;
		} a;
		a.mask = mask;

		emul_generic(process_self(), SYS_umask, &a, &rv);
		*old_mask = rv[0];
	}
	return ESUCCESS;
}

/* MEMORY */
extern vm_size_t vm_page_size;

errno_t e_getpagesize(int *size)
{
	/* Set by init code from vm_statistics */
	*size = vm_page_size;
	return ESUCCESS;
}

errno_t e_msync(caddr_t addr, int len)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		caddr_t addr;
		int len;
	} a;
	a.len = len;
	a.addr = addr;

	kr = emul_generic(process_self(), SYS_msync, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* Open fd as port and map it. Later cache the fd->port mapping */
errno_t e_lite_mmap(
	caddr_t		bsd_addr,
	size_t		len,
	int		bsd_prot,
	int		flags,
	int		fd,
	int		pad,
	off_t		offset,
	caddr_t		*addr_out)
{
	return e_mmap(bsd_addr, len, bsd_prot, flags, fd, (vm_offset_t) offset,
		      addr_out);
}

/* Open fd as port and map it. Later cache the fd->port mapping */
errno_t e_mmap(
	caddr_t		bsd_addr,
	size_t		len,
	int		bsd_prot,
	int		flags,
	int		fd,
	vm_offset_t	offset,	/* XXX was off_t */
	caddr_t		*addr_out)
{
	kern_return_t kr;

	vm_offset_t addr;
	vm_size_t size;
	vm_prot_t prot;
	mach_port_t handle;
	boolean_t anywhere;
	boolean_t copy;
	vm_prot_t maxprot;
	vm_inherit_t inheritance;

	/*
	 * Do nothing if given zero size. This happens at least with
	 * NetBSD-current shared libraries (zero bss).
	 */
	if (!len)
		return ESUCCESS;

	prot = VM_PROT_NONE;
	if (bsd_prot & PROT_READ)
		prot |= VM_PROT_READ;
	if (bsd_prot & PROT_WRITE)
		prot |= VM_PROT_WRITE;
	if (bsd_prot & PROT_EXEC)
		prot |= VM_PROT_EXECUTE;

	anywhere = !(flags & MAP_FIXED);
	copy = !(flags & MAP_SHARED);
	maxprot = (flags & MAP_SHARED) ? prot : VM_PROT_ALL;

	/* 
	 * MAP_INHERIT is defined to affect exec not fork!!!
	 * Thus it can't affect the mapping here but exec should later
	 * be careful about not to unmap MAP_INHERIT regions. For now
	 * MAP_INHERIT will simply be ignored.
	 *
	 * To handle this properly the server must be informed.
	 * Also MAP_INHERIT seems like a security risk.
	 */
	inheritance = (flags & MAP_SHARED)
	    ? VM_INHERIT_SHARE : VM_INHERIT_COPY;
	/*
	 * Address (if FIXED) must be page aligned.
	 * Size is implicitly rounded to a page boundary.
	 */
	addr = (vm_offset_t) bsd_addr;
	if (!anywhere && (addr != trunc_page(addr)))
		return EINVAL;
	if (anywhere && addr == 0) {
		/* XXX For NetBSD shared libraries: leave some space for sbrk*/
		addr = 0x4000000;
	}
	size = (vm_size_t) round_page(len);

	if (flags & MAP_ANON) {
		/* XXX If the anon file is named it should really be a file */
		/* XXX Handle if(anywhere) case */
		if (!anywhere)
		    vm_deallocate(mach_task_self(), addr, size);
		/* 
		 * Use vm_map instead of vm_allocate as vm_map does
		 * not sabotage the addr hint before calling
		 * vm_map_enter as vm_allocate does (in the anywhere case).
		 */
		kr = vm_map(mach_task_self(), &addr, size, 0, anywhere,
			    MEMORY_OBJECT_NULL, 0, FALSE, VM_PROT_DEFAULT,
			    VM_PROT_ALL, VM_INHERIT_DEFAULT);
		if (kr != KERN_SUCCESS)
		    return e_mach_error_to_errno(kr);
		*addr_out = (caddr_t) addr;
	} else {
		kr = bsd_fd_to_file_port(process_self(), fd, &handle);
		if (kr)
		    return e_mach_error_to_errno(kr);
		kr = bsd_vm_map(process_self(), &addr, size, 0, anywhere,
				handle, offset, copy, prot, maxprot,
				inheritance);
		if (kr != KERN_SUCCESS)
		    return e_mach_error_to_errno(kr);
		*addr_out = (caddr_t) addr;
	}
	return ESUCCESS;
}

errno_t e_munmap(caddr_t addr, int len)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		caddr_t addr;
		int len;
	} a;
	a.len = len;
	a.addr = addr;

	kr = emul_generic(process_self(), SYS_munmap, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_mprotect(caddr_t addr, int len, int prot)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		caddr_t addr;
		int len;
		int prot;
	} a;
	a.prot = prot;
	a.len = len;
	a.addr = addr;

	kr = emul_generic(process_self(), SYS_mprotect, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_madvise(caddr_t addr, int len, int behav)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		caddr_t addr;
		int len;
		int behav;
	} a;
	a.behav = behav;
	a.len = len;
	a.addr = addr;

	kr = emul_generic(process_self(), SYS_madvise, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_mincore(caddr_t addr, int len, char *vec)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		caddr_t addr;
		int len;
		char *vec;
	} a;
	a.vec = vec;
	a.len = len;
	a.addr = addr;

	kr = emul_generic(process_self(), SYS_mincore, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* TIME */
struct itimerval zero_itv = { 0, 0, 0, 0 };

errno_t e_setitimer(
	int which,
	struct itimerval *value,
	struct itimerval *ovalue)
{
	kern_return_t kr;

	struct itimerval old_itimer_val;

	kr = bsd_setitimer(process_self(),
			   which,
			   (value != 0),
			   &old_itimer_val,
			   (value) ? *value : zero_itv);
	if (kr == 0 && ovalue)
	    *ovalue = old_itimer_val;
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_getitimer(int which, struct itimervalue *value)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		natural_t which;
		struct itimervalue *value;
	} a;
	a.value = value;
	a.which = which;

	kr = emul_generic(process_self(), SYS_getitimer, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

struct timeval	zero_time = { 0, 0 };
errno_t e_select(
	int nfds,
	fd_set *readfds,
	fd_set *writefds,
	fd_set *exceptfds,
	struct timeval *timeout,
	int *nready)
{
	kern_return_t kr;
	integer_t rv[2];

	int		ni_size;
	fd_set		zeros;
	fd_set		in_set, ou_set, ex_set;

	FD_ZERO(&zeros);

	if (nfds < 0)
	    return EINVAL;
	if (nfds > NOFILE)
	    nfds = NOFILE;
	ni_size = howmany(nfds, NFDBITS) * sizeof(fd_mask);

	if (readfds)
	    bcopy((char *)readfds, (char *)&in_set, (unsigned)ni_size);
	if (writefds)
	    bcopy((char *)writefds, (char *)&ou_set, (unsigned)ni_size);
	if (exceptfds)
	    bcopy((char *)exceptfds, (char *)&ex_set, (unsigned)ni_size);

	kr = bsd_select(process_self(),
			nfds,
			(readfds) ? &in_set : &zeros,
			(writefds) ? &ou_set : &zeros,
			(exceptfds) ? &ex_set : &zeros,
			(readfds != 0),
			(writefds != 0),
			(exceptfds != 0),
			(timeout != 0),
			(timeout) ? *timeout : zero_time,
			rv);

	if (kr)
	    return e_mach_error_to_errno(kr);

	if (readfds)
	    bcopy((char *)&in_set, (char *)readfds, (unsigned)ni_size);
	if (writefds)
	    bcopy((char *)&ou_set, (char *)writefds, (unsigned)ni_size);
	if (exceptfds)
	    bcopy((char *)&ex_set, (char *)exceptfds, (unsigned)ni_size);

	*nready = rv[0];
	return ESUCCESS;
}

errno_t e_setpriority(int which, int who, int prio)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int which;
		int who;
		int prio;
	} a;
	a.prio = prio;
	a.who = who;
	a.which = which;

	kr = emul_generic(process_self(), SYS_setpriority, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getpriority(int which, int who, int *prio)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int which;
		int who;
	} a;
	a.who = who;
	a.which = which;

	kr = emul_generic(process_self(), SYS_getpriority, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*prio = rv[0];
	return err;
}

errno_t e_gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		struct timeval *tp;
		struct timezone *tzp;
	} a;
	a.tzp = tzp;
	a.tp = tp;

	kr = emul_generic(process_self(), SYS_gettimeofday, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getrusage(int who, struct rusage *rusage)
{
	kern_return_t kr;
	struct thread_basic_info bi;
	mach_msg_type_number_t bi_count;

	kr = bsd_getrusage(process_self(), who, rusage);
	if (kr)
	    return e_mach_error_to_errno(kr);

#if 0
	bi_count = THREAD_BASIC_INFO_COUNT;
	(void) thread_info(mach_thread_self(),
			THREAD_BASIC_INFO,
			(thread_info_t)&bi,
			&bi_count);

	rusage->ru_utime.tv_sec  = bi.user_time.seconds;
	rusage->ru_utime.tv_usec = bi.user_time.microseconds;
	rusage->ru_stime.tv_sec  = bi.system_time.seconds;
	rusage->ru_stime.tv_usec = bi.system_time.microseconds;
#endif
	return 0;
}

struct timeval	zero_tv = { 0, 0 };
struct timezone	zero_tz = { 0, 0 };

errno_t e_settimeofday(struct timeval *tv, struct timezone *tz)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	kr = bsd_settimeofday(process_self(),
			      (tv != 0),
			      (tv) ? *tv : zero_tv,
			      (tz != 0),
			      (tz) ? *tz : zero_tz);

	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_utimes(const char *file, const struct timeval *times)
{
	errno_t err;
	kern_return_t kr;

        kr = bsd_utimes(process_self(),
			times,
			file, strlen(file) + 1);
	return kr ? e_mach_error_to_errno(kr) : 0;
}

errno_t e_adjtime(struct timeval *delta, struct timeval *olddelta)
{
	errno_t err;
	kern_return_t kr;

	kr = bsd_adjtime(process_self(), *delta, olddelta);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_lite_getrlimit(int resource, struct rlimit *rlp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int resource;
		struct rlimit *rlp;
	} a;
	a.rlp = rlp;
	a.resource = resource;

	kr = emul_generic(process_self(), SYS_getrlimit, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_lite_setrlimit(int resource, struct rlimit *rlp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	kr = bsd_setrlimit(process_self(), resource, rlp);
#if 0
	struct {
		int resource;
		struct rlimit *rlp;
	} a;
	a.rlp = rlp;
	a.resource = resource;

	kr = emul_generic(process_self(), SYS_setrlimit, &a, &rv);
#endif
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_setsid(pid_t *pgrp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_setsid, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0 && pgrp)
		*pgrp = rv[0];
	return err;
}

errno_t e_quotactl(const char *path, int cmd, int id, char *addr)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *path;
		int cmd;
		int id;
		char *addr;
	} a;
	a.addr = addr;
	a.id = id;
	a.cmd = cmd;
	a.path = path;

	kr = emul_generic(process_self(), SYS_quotactl, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

/* NFS */
errno_t e_nfssvc(int sock, struct sockaddr *mask, int mask_length, struct sockaddr *match, int match_length)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int sock;
		struct sockaddr *mask;
		int mask_length;
		struct sockaddr *match;
		int match_length;
	} a;
	a.match_length = match_length;
	a.match = match;
	a.mask_length = mask_length;
	a.mask = mask;
	a.sock = sock;

	kr = emul_generic(process_self(), SYS_nfssvc, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_async_daemon()
{
#if 0
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		integer_t dummy;
	} a;

	kr = emul_generic(process_self(), SYS_async_daemon, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
#else
	return ENOSYS;
#endif
}

errno_t e_getfh(const char *path, struct fhandle *fhp)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *path;
		struct fhandle *fhp;
	} a;
	a.fhp = fhp;
	a.path = path;

	kr = emul_generic(process_self(), SYS_getfh, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getdomainname(char *name, int namelen)
{
	int sysname[] = { CTL_KERN, KERN_DOMAINNAME };
	size_t oldlen = namelen;

	return e_sysctl(sysname, 2, name, &oldlen, 0, 0, 0);
}

errno_t e_setdomainname(const char *name, int namelen)
{
	int sysname[] = { CTL_KERN, KERN_DOMAINNAME };

	return e_sysctl(sysname, 2, 0, 0, name, namelen, 0);
}

/* MISC */
errno_t e_profil(void *bufbase, size_t bufsize, unsigned pcoffset, unsigned pcscale)
{
	return ENOSYS;
}

errno_t e_sysacct(const char *file)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	if ((integer_t)file == 1)
	    file = "/var/account/acct";
	if (file) {
		kr = bsd_acct(process_self(),
			      TRUE,
			      file, strlen(file) + 1);
	} else {
		kr = bsd_acct(process_self(),
			      FALSE,
			      "", 1);
	}

	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_reboot(int howto)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	union {
		int howto;
		integer_t dummy;
	} a;
	a.howto = howto;

	kr = emul_generic(process_self(), SYS_reboot, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_getkerninfo(int op, char *where, int *size, int arg, int *retval)
{
#if 1
	return ENOSYS;
#else
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		int op;
		char *where;
		int *size;
		int arg;
	} a;
	a.arg = arg;
	a.size = size;
	a.where = where;
	a.op = op;

	kr = emul_generic(process_self(), SYS_getkerninfo, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*retval = rv[0];
	return err;
#endif
}

errno_t e_swapon(const char *special)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	struct {
		const char *special;
	} a;
	a.special = special;

	kr = emul_generic(process_self(), SYS_swapon, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_gethostname(char *name, int namelen)
{
	int    mib[2];
	size_t size;

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	size = namelen;
	return e_sysctl(mib, 2, (void *)name, &size, NULL, 0, 0);
}

errno_t e_sethostname(const char *name, int namelen)
{
	int mib[2];

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	return e_sysctl(mib, 2, NULL, NULL, (void *)name, (size_t)namelen, 0);
}

errno_t e_gethostid(int *id)
{
	int    mib[2];
	size_t size;

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTID;
	size = sizeof(int);
	return e_sysctl(mib, 2, (void *)id, &size, NULL, 0, 0);
}

errno_t e_sethostid(int hostid)
{
	int mib[2];

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTID;
	return e_sysctl(mib, 2, NULL, NULL, (void *)&hostid, sizeof (int), 0);
}

errno_t e_table(int id, int index, char *addr, int nel, u_int lel, int *cel)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	int		nel_done;

	if (nel < 0) {
	    /*
	     * Set.
	     */
	    kr = bsd_table_set(process_self(),
			       id, index, lel, nel,
			       addr, -nel*lel,
			       &nel_done);
	} else {
	    char *		out_addr;
	    natural_t		out_count;

	    kr = bsd_table_get(process_self(),
			id, index, lel, nel,
			&out_addr, &out_count,
			&nel_done);

	    if (kr == KERN_SUCCESS) {
		/*
		 * Copy table to addr
		 */
		bcopy(out_addr, addr, lel * nel_done);
		(void) vm_deallocate(mach_task_self(),
				     (vm_offset_t)out_addr,
				     (vm_size_t) out_count);
	    }
	}
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0)
		*cel = nel_done;
	return err;
}

errno_t e_sysctrace(pid_t pid)
{
	errno_t err;
	kern_return_t kr;
	integer_t rv[2];

	union {
		int pid;	/* pid_t */
		integer_t dummy;
	} a;
	a.pid = pid;

	kr = emul_generic(process_self(), SYS_sysctrace, &a, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	return err;
}

errno_t e_sysctl(int *name, unsigned int namelen, void *old, size_t *oldlenp,
		   void *new, size_t newlen, int *retlen)
{
	errno_t err;
	kern_return_t kr;
	void                   *ret;
	mach_msg_type_number_t  ret_count;
	size_t                  oldlen;
	int                     rv;

	oldlen = oldlenp ? *oldlenp : 0;
	/*
	 * XXX If the user gives NULL old, and non-NULL oldlenp and *oldlenp
	 * is not zero, fix the actual parameter given to the server.
	 */
	if (old == 0 && oldlenp && *oldlenp != 0)
		oldlen = 0;
	kr = bsd_sysctl(process_self(), name, namelen, namelen,
			&ret, &ret_count, &oldlen, new, newlen, newlen, &rv);
	err = kr ? e_mach_error_to_errno(kr) : 0;
	if (err == 0) {
		if (retlen)
			*retlen = rv;
		if (old && rv)
			bcopy(ret, old, rv);
		if (oldlenp)
			*oldlenp = oldlen;
		if (ret_count)
			(void)vm_deallocate(mach_task_self(),
					    (vm_offset_t)ret,
					    (vm_size_t)ret_count);
	}
	return err;
}

/* CMU CALLS */
errno_t e_task_by_pid(pid_t pid, task_t *task)
{
	mach_error_t kr;

	kr = bsd_task_by_pid(process_self(), pid, task);
	return e_mach_error_to_errno(kr);
}

errno_t e_pid_by_task(
	task_t		task,
	pid_t		*pid,
	char		*comm,
	int		*commcnt) /* IN/OUT */
{
	mach_error_t kr;
	int tmp_pid;
	integer_t tmp_commcnt = *commcnt;

	kr = bsd_pid_by_task(process_self(),
			     task,
			     &tmp_pid,
			     comm, &tmp_commcnt);
	if (kr == KERN_SUCCESS) {
		*pid = tmp_pid;
		*commcnt = (int) tmp_commcnt;
	}
	return e_mach_error_to_errno(kr);
}

errno_t e_init_process()
{
	mach_error_t kr;

	kr = bsd_init_process(process_self());
	return e_mach_error_to_errno(kr);
}

/**** SUPPORT ROUTINES *****/

#define MAXPRINT 128
#define putchar(x) if (pos<MAXPRINT) error[pos++] = (x); else goto done
int e_emulator_error(char *fmt, ...)
{
	register int b, c, i;
	char *s;
	char **v;
	u_long	value;
	char prbuf[24];
	register char *cp;
	char error[MAXPRINT];
	int pos = 0;
	va_list adx;
	boolean_t longfmt;

	va_start(adx, fmt);
loop:
	while ((c = *fmt++) != '%') {
	    if (c == '\0') {
		goto done;
	    }
	    putchar(c);
	}
	longfmt = FALSE;
long_fmt:
	c = *fmt++;
	switch (c) {
	    case 'l':
	  	longfmt = TRUE;
		goto long_fmt;
	    case 'x': case 'X':
		b = 16;
		goto number;
	    case 'd': case 'D':
	    case 'u':
		b = 10;
		goto number;
	    case 'o': case 'O':
		b = 8;
number:
		if (longfmt)
		    value = va_arg(adx, unsigned long);
		else
		    value = va_arg(adx, unsigned int);

		if (b == 10 && (long)value < 0) {
		    putchar('-');
		    value = -value;
		}
		cp = prbuf;
		do {
		    *cp++ = "0123456789abcdef"[value%b];
		    value /= b;
		} while (value);
		do {
		    putchar(*--cp);
		} while (cp > prbuf);
		break;
	    case 'c':
		value = va_arg(adx, unsigned int);
		for (i = 24; i >= 0; i -= 8)
			if (c = (value >> i) & 0x7f)
				putchar(c);
		break;
	    case 's':
		s = va_arg(adx, char *);
		while (c = *s++) {
		    putchar(c);
		}
		break;
	    case 'v':			/* vector of strings */
		v = va_arg(adx, char **);
		while (s = *v)
		  {
		    while (c = *s++) {
		      putchar(c);
		    }
		    v++;
		    if (*v)
		      putchar(',');
		  }
		break;
	    case '%':
		putchar('%');
		break;
	}
	goto loop;
done:
	error[pos]='\0';
	va_end(adx);
	return bsd_emulator_error(process_self(), error, pos+1);
}

errno_t e_nosys()
{
	return ENOSYS;
}
