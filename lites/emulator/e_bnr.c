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
 * 23-Oct-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Added "pad" to e_lite_lseek arguments.
 *
 * $Log: e_bnr.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:32  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	emulator/e_bnr_stubs.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 * 	BNR compat system call handler functions.
 */

#include <e_defs.h>
#include <sys/dirent.h>
#include <sys/ipc.h>

errno_t e_lite_truncate(const char *path, int pad, off_t length);
errno_t e_lite_ftruncate(int fd, int pad, off_t length);
errno_t e_lite_lseek(int fd, int pad, off_t offset, int sbase, off_t *pos);


errno_t e_bnr_truncate(const char *path, bnr_off_t length)
{
	int pad = 0;

	return e_lite_truncate(path, pad, (off_t) length);
}

errno_t e_bnr_ftruncate(int fd, bnr_off_t length)
{
	int pad = 0;

	return e_lite_ftruncate(fd, pad, (off_t) length);
}

errno_t e_bnr_lseek(int fd, bnr_off_t offset, int sbase, bnr_off_t *pos)
{
	off_t tmp_pos;
	errno_t err;
	int pad = 0;

	err = e_lite_lseek(fd, pad, (off_t) offset, sbase, &tmp_pos);
	if (!err && pos)
	    *pos = tmp_pos;
	return err;
}

struct bnr_dirent {
	unsigned int	d_fileno;
	unsigned short	d_reclen;
	unsigned short	d_namlen;
	char		d_name[255 + 1];
};

errno_t e_bnr_getdirentries(
	int	fd,
	char	*buf,
	int	nbytes,
	long	*basep,
	int	*nread)
{
	struct dirent *de;
	unsigned int namlen;
	int n;

	errno_t err = e_lite_getdirentries(fd, buf, nbytes, basep, nread);
	/* 
	 * Convert Lite style direntries to net2 style direntries. The
	 * size stays the same so the conversion is done in situ.
	 */
	if (!err && *nread >= sizeof(struct dirent)) {
		for (de = (struct dirent *) buf, n = 0;
		     de->d_reclen != 0 && n < *nread;
		     n += de->d_reclen, de = (struct dirent *)(buf + n) )
		{
			((struct bnr_dirent *)de)->d_namlen = de->d_namlen;
		}
	}
	return err;
}

#include <e_templates.h>
DEF_STAT(bnr)
DEF_LSTAT(bnr)
DEF_FSTAT(bnr)


#define BNR_PROT_READ 4
#define BNR_PROT_WRITE 2
#define BNR_PROT_EXEC 1

#define BNR_MAP_TYPE 0xf
#define BNR_MAP_FILE 1
#define BNR_MAP_ANON 2

#define BNR_MAP_SHARED 0x10
#define BNR_MAP_FIXED 0x100

/* Open fd as port and map it. Later cache the fd->port mapping */
errno_t e_bnr_mmap(
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

	prot = VM_PROT_NONE;
	if (bsd_prot & BNR_PROT_READ)
		prot |= VM_PROT_READ;
	if (bsd_prot & BNR_PROT_WRITE)
		prot |= VM_PROT_WRITE;
	if (bsd_prot & BNR_PROT_EXEC)
		prot |= VM_PROT_EXECUTE;

	anywhere = !(flags & BNR_MAP_FIXED);
	copy = !(flags & BNR_MAP_SHARED);
	maxprot = (flags & BNR_MAP_SHARED) ? prot : VM_PROT_ALL;

	/* 
	 * MAP_INHERIT is defined to affect exec not fork!!!
	 * Thus it can't affect the mapping here but exec should later
	 * be careful about not to unmap MAP_INHERIT regions. For now
	 * MAP_INHERIT will simply be ignored.
	 *
	 * To handle this properly the server must be informed.
	 * Also MAP_INHERIT seems like a security risk.
	 */
	inheritance = (flags & BNR_MAP_SHARED)
	    ? VM_INHERIT_SHARE : VM_INHERIT_COPY;
	/*
	 * Address (if FIXED) must be page aligned.
	 * Size is implicitly rounded to a page boundary.
	 */
	addr = (vm_offset_t) bsd_addr;
	if (!anywhere && (addr != trunc_page(addr)))
		return EINVAL;
	if (anywhere && addr < 0x4000000) {
		/* XXX For NetBSD shared libraries: leave some space for sbrk*/
		addr = 0x4000000;
	}
	size = (vm_size_t) round_page(len);

	switch (flags & BNR_MAP_TYPE) {
	      case BNR_MAP_FILE:
		kr = bsd_fd_to_file_port(process_self(), fd, &handle);
		if (kr)
		    return e_mach_error_to_errno(kr);
		kr = bsd_vm_map(process_self(), &addr, size, 0, anywhere,
				handle, offset, copy, prot, maxprot,
				inheritance);
		if (kr != KERN_SUCCESS)
		    return e_mach_error_to_errno(kr);
		*addr_out = (caddr_t) addr;
		break;
	      case BNR_MAP_ANON:
		/* XXX If the anon file is named it should really be a file */
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
		break;
	      default:
		return EINVAL;
	}
	return ESUCCESS;
}

struct bnr_rlimit {
	long	rlim_cur;
	long	rlim_max;
};

errno_t e_bnr_getrlimit(int resource, struct bnr_rlimit *rl)
{
	struct rlimit r;
	errno_t err;

	err = e_lite_getrlimit(resource, &r);
	if (err)
	    return err;
	rl->rlim_cur = r.rlim_cur;
	rl->rlim_max = r.rlim_max;
	return ESUCCESS;
}

errno_t e_bnr_setrlimit(int resource, struct bnr_rlimit *rl)
{
	struct rlimit r;
	errno_t err;

	r.rlim_cur = rl->rlim_cur;
	r.rlim_max = rl->rlim_max;

	return e_lite_setrlimit(resource, &r);
}

/* pid size differs */
errno_t e_bnr_wait4(
	short		pid,
	int		*status,
	int		options,
	struct rusage	*rusage,
	short		*wpid)
{
	pid_t tmp_wpid;
	errno_t err;

	err = e_lite_wait4((pid_t) pid, status, options, rusage,
			   wpid ? &tmp_wpid : 0);
	if (!err) {
		if (wpid)
		    *wpid = tmp_wpid;	/* The upper bits get stripped! */
					/* XXX little endian specific? */
	}
	return err;
}

errno_t e_bnr_shmget(key_t key, int size, int shmflg, int *id)
{
	int open_flags = 0;
	if (shmflg & IPC_CREAT)
	    open_flags |= O_CREAT;
	if (shmflg & IPC_EXCL)
	    open_flags |= O_EXCL;
	if (shmflg & IPC_NOWAIT)
	    open_flags |= O_NDELAY;
	open_flags |= O_RDWR;

	return e_shmget(key, size, open_flags, id);
}

errno_t e_bnr_shmat(int shmid, char *shmaddr, int shmflg, caddr_t *addr_out)
{
	vm_prot_t prot = VM_PROT_READ|VM_PROT_EXECUTE|VM_PROT_WRITE;

	return e_shmat(shmid, (vm_offset_t) shmaddr, prot, addr_out);
}


/* SysV IPC (most code now in e_sysvipc.c) */
errno_t e_bnr_shmsys(
	integer_t flavor,
	integer_t a2,
	integer_t a3,
	integer_t a4,
	integer_t *retval)
{
	switch (flavor) {
	      case 0:
		return e_bnr_shmat(a2, (char *) a3, a4, (caddr_t *) retval);
	      case 1:
		/* return e_shmctl(a2, a3, a4); */
		return EOPNOTSUPP;
	      case 2:
		return e_shmdt(a2);
	      case 3:
		return e_bnr_shmget(a2, a3, a4, retval);
	      default:
		return EINVAL;
	}
}

errno_t e_sigaction(int sig, const struct sigaction *act, struct sigaction *oact);

/* sigvec to sigaction conversion COMPAT_43 */
errno_t e_bnr_sigvec(
	int sig,
	struct sigvec *nsv,
	struct sigvec *osv)		/* OUT */
	/* The trampoline address that is passed in wierd ways is unused */
{
	struct sigaction nsa, osa;
	errno_t err;

	if (nsv) {
		nsa.sa_handler = nsv->sv_handler;
		nsa.sa_mask = nsv->sv_mask;
		nsa.sa_flags = 0;

		if (nsv->sv_flags & SV_ONSTACK)
		    nsa.sa_flags |= SA_ONSTACK;
		if ((nsv->sv_flags & SV_INTERRUPT) == 0)
		    nsa.sa_flags |= SA_RESTART;
		/* SunOS compat: SA_DISABLE, SA_USERTRAMP */
	}
	err = e_sigaction(sig,
			  (nsv ? &nsa : (struct sigaction *) 0),
			  (osv ? &osa : (struct sigaction *) 0));

	if (!err && osv) {
		osv->sv_handler = osa.sa_handler;
		osv->sv_mask = osa.sa_mask;
		osv->sv_flags = 0;

		if (osa.sa_flags & SA_ONSTACK)
		    osv->sv_flags |= SV_ONSTACK;
		if ((osa.sa_flags & SA_RESTART) == 0)
		    osv->sv_flags |= SV_INTERRUPT;
		/* SunOS compat: P_NOCLDSTOP */
	}
	return err;
}

/* SOCKET COMPAT */

void e_sockaddr_bnr_to_lite(struct osockaddr *o, struct sockaddr *n)
{
	bcopy(o->sa_data, n->sa_data, sizeof(n->sa_data));
	n->sa_family = o->sa_family;
	n->sa_len = 14;		/* XXX */
}

void e_sockaddr_lite_to_bnr(struct sockaddr *n, struct osockaddr *o)
{
	bcopy(n->sa_data, o->sa_data, sizeof(o->sa_data));
	o->sa_family = n->sa_family;
}

errno_t e_recv(
	int	fileno,
	caddr_t	data,
	int	count,
	int	flags,
	int	*nreceived)
{
	struct msghdr m;
	struct iovec iov;

	m.msg_name = 0;
	m.msg_namelen = 0;
	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	m.msg_control = 0;
	m.msg_controllen = 0;
	m.msg_flags = flags;

	iov.iov_base = data;
	iov.iov_len = count;

	return e_recvmsg(fileno, &m, flags, nreceived);
}

errno_t e_recvfrom(
	int fileno,
	void *data,
	int count,
	int flags,
	struct sockaddr *from,
	int *fromlen,
	int *nreceived)
{
	errno_t err;
	struct msghdr m;
	struct iovec iov;

	m.msg_name = (caddr_t) from;
	m.msg_namelen = fromlen ? *fromlen : 0;

	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	m.msg_control = 0;
	m.msg_controllen = 0;
	m.msg_flags = 0;

	iov.iov_base = data;
	iov.iov_len = count;

	err = e_recvmsg(fileno, &m, flags, nreceived);
	if (err)
	    return err;
	if (fromlen)
	    *fromlen = sizeof(*from);
	return ESUCCESS;
}

errno_t e_bnr_recvfrom(
	int fileno,
	void *data,
	int count,
	int flags,
	struct osockaddr *ofrom,
	int *ofromlen,
	int *nreceived)
{
	errno_t err;
	struct msghdr m;
	struct iovec iov;
	struct sockaddr from;

	if (ofromlen && ofrom) {
		m.msg_name = (caddr_t) &from;
		m.msg_namelen = sizeof(from);
	} else {
		m.msg_name = 0;
		m.msg_namelen = 0;
	}
	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	m.msg_control = 0;
	m.msg_controllen = 0;
	m.msg_flags = 0;

	iov.iov_base = data;
	iov.iov_len = count;

	err = e_recvmsg(fileno, &m, flags, nreceived);
	if (err)
	    return err;
	if (ofromlen && ofrom) {
		e_sockaddr_lite_to_bnr(&from, ofrom);
	}
	return ESUCCESS;
}

errno_t e_bnr_recvmsg(
	int fd,
	struct omsghdr *omsg,
	int flags,
	int *nreceived)
{
	errno_t err;
	struct msghdr m;
	struct sockaddr from;
	struct cmsghdr cm;

	if (omsg->msg_namelen && omsg->msg_name) {
		m.msg_name = (caddr_t) &from;
		m.msg_namelen = sizeof(from);
	} else {
		m.msg_name = 0;
		m.msg_namelen = 0;
	}
	m.msg_iov = omsg->msg_iov;
	m.msg_iovlen = omsg->msg_iovlen;
	m.msg_flags = 0;
	m.msg_control = 0;
	m.msg_controllen = 0;

#if 0
	if (omsg->msg_accrights && omsg->msg_accrightslen) {
		m.msg_control = &cm;
		m.msg_controllen = sizeof(cm);
	}
#endif

	err = e_recvmsg(fd, &m, flags, nreceived);
	if (err)
	    return err;

	if (omsg->msg_namelen && omsg->msg_name) {
		struct osockaddr *ofrom = (struct osockaddr *) omsg->msg_name;
		e_sockaddr_lite_to_bnr(&from, ofrom);
	}
#if 0	/* XXX need to figure this out some day */
	if (omsg->msg_controllen && omsg->msg_control) {
		struct ocmsghdr *oc = omsg->msg_;
	}
#endif
	return ESUCCESS;
}

errno_t e_send(
	int		fileno,
	char		*data,
	unsigned int	count,
	int		flags,
	int		*nsent)
{
	struct msghdr m;
	struct iovec iov;

	m.msg_name = 0;
	m.msg_namelen = 0;
	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	m.msg_control = 0;
	m.msg_controllen = 0;
	m.msg_flags = 0;

	iov.iov_base = data;
	iov.iov_len = count;

	return e_sendmsg(fileno, &m, flags, nsent);
}

errno_t e_sendto(
	int			fileno,
	const void		*data,
	int			count,
	int			flags,
	const struct sockaddr	*to,
	int			tolen,
	int			*nsent)
{
	struct msghdr m;
	struct iovec iov;

	m.msg_name = (caddr_t) to;
	m.msg_namelen = tolen;

	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	m.msg_control = 0;
	m.msg_controllen = 0;
	m.msg_flags = 0;

	iov.iov_base = data;
	iov.iov_len = count;

	return e_sendmsg(fileno, &m, flags, nsent);
}

errno_t e_bnr_sendto(
	int		fileno,
	char		*data,
	unsigned int	count,
	int		flags,
	struct osockaddr *oto,
	int		otolen,
	int		*nsent)
{
	struct msghdr m;
	struct iovec iov;
	struct sockaddr to;

	if (otolen && oto) {
		e_sockaddr_bnr_to_lite(oto, &to);
		m.msg_name = (caddr_t) &to;
		m.msg_namelen = sizeof(to);
	} else {
		m.msg_name = 0;
		m.msg_namelen = 0;
	}
	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	m.msg_control = 0;
	m.msg_controllen = 0;
	m.msg_flags = 0;

	iov.iov_base = data;
	iov.iov_len = count;

	return e_sendmsg(fileno, &m, flags, nsent);
}

errno_t e_bnr_sendmsg(int s, const struct omsghdr *omsg, int flags, int *nsent)
{
	struct msghdr m;
	struct sockaddr to;
	struct osockaddr *oto = (struct osockaddr *) omsg->msg_name;

	if (omsg->msg_namelen && omsg->msg_name) {
		e_sockaddr_bnr_to_lite(oto, &to);
		m.msg_name = (caddr_t) &to;
		m.msg_namelen = sizeof(to);
	} else {
		m.msg_name = 0;
		m.msg_namelen = 0;
	}
	m.msg_iov = omsg->msg_iov;
	m.msg_iovlen = omsg->msg_iovlen;

	/* XXX accesses discarded */
	m.msg_control = 0;
	m.msg_controllen = 0;
	m.msg_flags = 0;

	return e_sendmsg(s, &m, flags, nsent);
}

errno_t e_bnr_getpeername(int fd, struct osockaddr *oname, int *namelen)
{
	errno_t err;
	struct sockaddr name;
	int len;

	len = sizeof(name);
	err = e_getpeername(fd, &name, &len);
	if (err)
	    return err;
	/* XXX check namelen */
	e_sockaddr_lite_to_bnr(&name, oname);
	if (namelen)
	    *namelen = sizeof(*oname);
	return ESUCCESS;
}

errno_t e_bnr_getsockname(int fd, struct osockaddr *oname, int *namelen)
{
	errno_t err;
	struct sockaddr name;
	int len;

	len = sizeof(name);
	err = e_getsockname(fd, &name, &len);
	if (err)
	    return err;
	/* XXX check namelen */
	e_sockaddr_lite_to_bnr(&name, oname);
	if (namelen)
	    *namelen = sizeof(*oname);
	return ESUCCESS;
}

errno_t e_bnr_accept(int s, struct osockaddr *oaddr, int *oaddrlen, int *fd)
{
	errno_t err;
	struct sockaddr from;
	int fromlen;

	err = e_accept(s, &from, &fromlen, fd);
	if (err)
	    return err;

	/* XXX check lengths! */
	if (oaddr)
	    e_sockaddr_lite_to_bnr(&from, oaddr);
	return ESUCCESS;
}
