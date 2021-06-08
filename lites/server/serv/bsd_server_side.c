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
 * 17-Sep-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *    Make bsd_mon_switch and bsd_mon_dump call libprof1 functions.
 *
 * $Log: bsd_server_side.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.5  1995/08/30  22:15:25  mike
 * hack rpc for GDB support
 *
 * Revision 1.4  1995/08/15  06:49:28  sclawson
 * modifications from lites-1.1-950808
 *
 * Revision 1.3  1995/03/23  01:44:03  law
 * Update to 950323 snapshot + utah changes
 *
 *
 */
/* 
 *	File: serv/bsd_server_side.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 * 	bsd_1.defs server side routines.
 */

#include "map_uarea.h"
#include "compat_43.h"
#include "syscalltrace.h"

#include <serv/server_defs.h>
#include <serv/syscall_subr.h>
#include <serv/import_mach.h>
#include <serv/bsd_types.h>
#include <ufs/ufs/quota.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <ufs/ufs/inode.h>
#include <sys/syscall.h>
#include <bsd_1_server.h>

#include <serv/syscalltrace.h>

/*
 * in sys_generic
 */
kern_return_t
bsd_write_short(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	char			*data,
	mach_msg_type_number_t	data_count,
	size_t			*nwritten)	/* OUT */
{
	START_SERVER(SYS_write, 3)

	arg[0] = fileno;
	arg[1] = (integer_t)data;
	arg[2] = (integer_t)data_count;
	error = s_write(p, arg, nwritten);

	END_SERVER(SYS_write)
}

kern_return_t
bsd_write_long(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	char			*data,
	mach_msg_type_number_t	data_count,
	size_t			*nwritten)	/* OUT */
{
	START_SERVER(SYS_write, 3)

	arg[0] = fileno;
	arg[1] = (integer_t)data;
	arg[2] = (integer_t)data_count;
	error = s_write(p, arg, nwritten);

	END_SERVER_DEALLOC(SYS_write, data, data_count)
}

kern_return_t
bsd_read_short(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	int			datalen,
	size_t			*nread,		/* OUT */
	char 			*data,		/* OUT */
	mach_msg_type_number_t	*data_count)	/* max in/OUT */
{
	START_SERVER(SYS_read, 3);

	error = KERN_SUCCESS;
	if (datalen < 0 || datalen > *data_count) {
		error = EINVAL;
	} else {
		arg[0] = fileno;
		arg[1] = (integer_t) data;
		arg[2] = (integer_t) datalen;
		error = s_read(p, arg, nread);
	}
	if (error == KERN_SUCCESS)
	    *data_count = *nread;

	END_SERVER(SYS_read)
}

kern_return_t
bsd_read_long(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	int			datalen,
	size_t			*nread,		/* OUT */
	char			**data,		/* OUT dealloc */
	mach_msg_type_number_t	*data_count)	/* OUT */
{
	START_SERVER(SYS_read, 3);

	*data_count = round_page(datalen);
	error = vm_allocate(mach_task_self(), (vm_offset_t *) data,
			    *data_count, TRUE);
	if (error == KERN_SUCCESS) {
		arg[0] = fileno;
		arg[1] = (integer_t) *data;
		arg[2] = (integer_t) datalen;
		error = s_read(p, arg, nread);
	}
	END_SERVER_DEALLOC_ON_ERROR(SYS_read, (*data), (*data_count));
}

kern_return_t
bsd_select(
	mach_port_t	proc_port,
	mach_port_seqno_t	seqno,
	int		nd,
	fd_set		*in_set,	/* in/out */
	fd_set		*ou_set,	/* in/out */
	fd_set		*ex_set,	/* in/out */
	boolean_t	in_valid,
	boolean_t	ou_valid,
	boolean_t	ex_valid,
	boolean_t	do_timeout,
	timeval_t	tv,
	integer_t	*rval)		/* out */
{
	START_SERVER(SYS_select, 5)

	error = s_select(p,
			 nd,
			 (in_valid) ? in_set : 0,
			 (ou_valid) ? ou_set : 0,
			 (ex_valid) ? ex_set : 0,
			 (do_timeout) ? &tv : 0,
			 rval);

	END_SERVER(SYS_select)
}

/*
 * in ufs_syscalls
 */
kern_return_t
bsd_chdir(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	char			*fname,
	mach_msg_type_number_t	fname_count)
{
	START_SERVER(SYS_chdir, 1)

	TRACE(("(%s)",fname));
	arg[0] = (integer_t)fname;
	error = chdir(p, arg, 0);

	END_SERVER(SYS_chdir)
}

kern_return_t
bsd_chroot(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	char			*fname,
	mach_msg_type_number_t	fname_count)
{
	START_SERVER(SYS_chroot, 1)

	TRACE(("(%s)",fname));
	arg[0] = (integer_t)fname;
	error = chroot(p, arg, 0);

	END_SERVER(SYS_chroot)
}

kern_return_t
bsd_open(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			mode,
	int			crtmode,
	char			*fname,
	mach_msg_type_number_t	fname_count,
	int			*fileno)	/* OUT */
{
	START_SERVER(SYS_open, 3)

	TRACE(("(%s)",fname));
	arg[0] = (integer_t)fname;
	arg[1] = mode;
	arg[2] = crtmode;
	error = open(p, arg, fileno);

	END_SERVER(SYS_open)
}

kern_return_t
bsd_mknod(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fmode,
	int			dev,
	char			*fname,
	mach_msg_type_number_t	fname_count)
{
	START_SERVER(SYS_mknod, 3)

	TRACE(("(%s)",fname));
	arg[0] = (integer_t)fname;
	arg[1] = fmode;
	arg[2] = dev;
	error = mknod(p, arg, 0);

	END_SERVER(SYS_mknod)
}

kern_return_t
bsd_link(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	char			*target,
	mach_msg_type_number_t	target_count,
	char			*linkname,
	mach_msg_type_number_t	linkname_count)
{
	START_SERVER(SYS_link, 2)

	TRACE(("(%s,%s)",target,linkname));
	arg[0] = (integer_t)target;
	arg[1] = (integer_t)linkname;
	error = link(p, arg, 0);

	END_SERVER(SYS_link)
}

kern_return_t
bsd_symlink(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	char			*target,
	mach_msg_type_number_t	target_count,
	char			*linkname,
	mach_msg_type_number_t	linkname_count)
{
	START_SERVER(SYS_symlink, 2)

	TRACE(("(%s,%s)",target,linkname));
	arg[0] = (integer_t)target;
	arg[1] = (integer_t)linkname;
	error = symlink(p, arg, 0);

	END_SERVER(SYS_symlink)
}

kern_return_t
bsd_unlink(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	char			*fname,
	mach_msg_type_number_t	fname_count)
{
	START_SERVER(SYS_unlink, 1)

	TRACE(("(%s)",fname));
	arg[0] = (integer_t)fname;
	error = unlink(p, arg, 0);

	END_SERVER(SYS_unlink)
}

kern_return_t
bsd_access(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fmode,
	char			*fname,
	mach_msg_type_number_t	fname_count)
{
	START_SERVER(SYS_access, 2)

	TRACE(("(%s,%o)",fname,fmode));
	arg[0] = (integer_t)fname;
	arg[1] = fmode;
	error = saccess(p, arg, 0);

	END_SERVER(SYS_access)
}

kern_return_t
bsd_readlink(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			count,
	path_name_t		name,
	mach_msg_type_number_t	name_count,
	char			*buf,		/* pointer to OUT array */
	mach_msg_type_number_t	*bufCount)	/* out */
{
	START_SERVER(SYS_readlink, 3)

	TRACE(("(%s)",name));
	arg[0] = (integer_t)name;
	arg[1] = (integer_t)buf;
	arg[2] = count;
	error =	readlink(p, arg, bufCount);

	END_SERVER(SYS_readlink)
}

kern_return_t
bsd_utimes(
	mach_port_t	proc_port,
	mach_port_seqno_t	seqno,
	struct timeval	*times,
	char		*fname,
	mach_msg_type_number_t	fname_count)
{
	START_SERVER(SYS_utimes, 2)

	TRACE(("(%s)",fname));
	error = s_utimes(p, fname, times, 0);
	END_SERVER(SYS_utimes)
}

kern_return_t
bsd_rename(
	mach_port_t	proc_port,
	mach_port_seqno_t	seqno,
	char		*from,
	mach_msg_type_number_t	from_count,
	char		*to,
	mach_msg_type_number_t	to_count)
{
	START_SERVER(SYS_rename, 2)

	TRACE(("(%s,%s)",from,to));
	arg[0] = (integer_t)from;
	arg[1] = (integer_t)to;
	error = rename(p, arg, 0);

	END_SERVER(SYS_rename)
}

kern_return_t
bsd_mkdir(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			dmode,
	char			*name,
	mach_msg_type_number_t	name_count)
{
	START_SERVER(SYS_mkdir, 2)

	TRACE(("(%s,%o)",name,dmode));
	arg[0] = (integer_t)name;
	arg[1] = dmode;
	error = mkdir(p, arg, 0);

	END_SERVER(SYS_mkdir)
}

kern_return_t
bsd_rmdir(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	char			*name,
	mach_msg_type_number_t		name_count)
{
	START_SERVER(SYS_rmdir, 1)

	TRACE(("(%s)",name));
	arg[0] = (integer_t)name;
	error = rmdir(p, arg, 0);

	END_SERVER(SYS_rmdir)
}

kern_return_t
bsd_fork(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	thread_state_t		new_state,
	mach_msg_type_number_t	new_state_count,
	int			*child_pid)	/* OUT */
{
	START_SERVER(SYS_fork, 2)

	TRACE(("(%s)","fork"));
	error = s_fork(p, new_state, new_state_count, child_pid, FALSE);

	END_SERVER(SYS_fork)
}
kern_return_t
bsd_vfork(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	thread_state_t		new_state,
	mach_msg_type_number_t		new_state_count,
	int			*child_pid)	/* OUT */
{
	START_SERVER(SYS_vfork, 2)

	TRACE(("(%s)","vfork"));
	error = s_fork(p, new_state, new_state_count, child_pid, TRUE);

	END_SERVER(SYS_fork)
}

/*
 * in kern_acct
 */

kern_return_t
bsd_acct(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	boolean_t		acct_on,
	char			*fname,
	mach_msg_type_number_t		fname_count)
{
	START_SERVER(SYS_acct, 1)

	arg[0] = (acct_on) ? (integer_t)fname : 0;
	error = sysacct(p, arg, 0);

	END_SERVER(SYS_acct)
}

/*
 * More glue
 */
kern_return_t
bsd_setgroups(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			gidsetsize,
	gidset_t		gidset)
{
	START_SERVER(SYS_setgroups, 2)

	arg[0] = gidsetsize;
	arg[1] = (integer_t)gidset;

	error = setgroups(p, arg, 0);

	END_SERVER(SYS_setgroups)
}

kern_return_t
bsd_setrlimit(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			which,
	struct rlimit		*lim)
{
	START_SERVER(SYS_setrlimit, 2)

	arg[0] = which;
	arg[1] = (integer_t)lim;

	error = s_setrlimit(p, arg, 0);

	END_SERVER(SYS_setrlimit)
}

kern_return_t
bsd_sigstack(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	boolean_t		have_nss,
	struct sigstack		nss,
	struct sigstack		*oss)		/* OUT */
{
	START_SERVER(SYS_osigstack, 2)

	arg[0] = (have_nss) ? (integer_t)&nss : 0;
	arg[1] = (integer_t)oss;

#if COMPAT_43
	error = osigstack(p, arg, 0);
#else
	return ENOSYS;
#endif

	END_SERVER(SYS_osigstack)
}

kern_return_t
bsd_settimeofday(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	boolean_t		have_tv,
	struct timeval		tv,
	boolean_t		have_tz,
	struct timezone		tz)
{
	START_SERVER(SYS_settimeofday, 2)

	arg[0] = (have_tv) ? (integer_t)&tv : 0;
	arg[1] = (have_tz) ? (integer_t)&tz : 0;

	error = s_settimeofday(p, arg, 0);

	END_SERVER(SYS_settimeofday)
}

kern_return_t
bsd_adjtime(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	struct timeval		delta,
	struct timeval		*olddelta)
{
	START_SERVER(SYS_adjtime, 2)

	arg[0] = (integer_t)&delta;
	arg[1] = (integer_t)olddelta;

	error = s_adjtime(p, arg, 0);

	END_SERVER(SYS_adjtime)
}

kern_return_t
bsd_setitimer(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			which,
	boolean_t		have_itv,
	struct itimerval	*oitv,		/* OUT */
	struct itimerval	itv)
{
	START_SERVER(SYS_setitimer, 3)

	arg[0] = which;
	arg[1] = (have_itv) ? (integer_t)&itv : 0;
	arg[2] = (integer_t)oitv;

	error = setitimer(p, arg, 0);

	END_SERVER(SYS_setitimer)
}

kern_return_t
bsd_bind(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			s,
	sockarg_t		name,
	mach_msg_type_number_t	namelen)
{
	START_SERVER(SYS_bind, 3)

	error = s_bind(p, s, name, namelen);

	END_SERVER(SYS_bind)
}

kern_return_t
bsd_connect(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			s,
	sockarg_t		name,
	mach_msg_type_number_t	namelen)
{
	START_SERVER(SYS_connect, 3)

	error = s_connect(p, s, name, namelen);

	END_SERVER(SYS_connect)
}

kern_return_t
bsd_setsockopt(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			s,
	int			level,
	int			name,
	sockarg_t		val,
	mach_msg_type_number_t	valsize)
{
	START_SERVER(SYS_setsockopt, 5)

	error = s_setsockopt(p, s, level, name,
			     (valsize > 0) ? val : 0,
			     valsize);

	END_SERVER(SYS_setsockopt)
}

kern_return_t
bsd_getsockopt(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			s,
	int			level,
	int			name,
	sockarg_t		val,		/* OUT */
	mach_msg_type_number_t	*avalsize)	/* IN/OUT */
{
	START_SERVER(SYS_getsockopt, 5)

	*avalsize = sizeof(sockarg_t);

	error = s_getsockopt(p, s, level, name, val, avalsize);

	END_SERVER(SYS_getsockopt)
}

kern_return_t
bsd_lseek(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	off_t			offset,
	int			sbase,
	off_t			*ooffset)	/* OUT */
{
	struct lseek_args {
		int	fd;
		int	pad;
		off_t	offset;
		int	whence;
	} *uap;

	START_SERVER(SYS_lseek, 5)

	uap = (struct lseek_args *) arg;
	uap->fd = fileno;
	uap->pad = 0;
	uap->offset = offset;
	uap->whence = sbase;
	error =	lseek(p, arg, (int *)ooffset);

	END_SERVER(SYS_lseek)
}

kern_return_t
bsd_table_set(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			id,
	int			index,
	int			lel,
	int			nel,
	char			*addr,
	mach_msg_type_number_t		count,
	int			*nel_done)	/* out */
{
	START_SERVER(SYS_table, 5)

	arg[0] = id;
	arg[1] = index;
	arg[2] = (integer_t)addr;
	arg[3] = nel;
	arg[4] = lel;

	error = table(p, arg, nel_done);

	END_SERVER(SYS_table)
}

kern_return_t
bsd_table_get(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			id,
	int			index,
	int			lel,
	int			nel,
	char			**addr,		/* out */
	mach_msg_type_number_t		*count,		/* out */
	int			*nel_done)	/* out */
{
	START_SERVER(SYS_table, 5)

	*count = nel * lel;
	if (vm_allocate(mach_task_self(),
			(vm_offset_t *)addr,
			*count,
			TRUE)
	    != KERN_SUCCESS)
	{
	    error = ENOBUFS;
	}
	else {
	    arg[0] = id;
	    arg[1] = index;
	    arg[2] = (integer_t)*addr;
	    arg[3] = nel;
	    arg[4] = lel;

	    error = table(p, arg, nel_done);
    	}


	/*
	 * Special end code to deallocate data if any error
	 */
	/* { for electric-c */
	}
        unix_release();
	error = end_server_op(p, error, (boolean_t *)0);
	if (error) {
	    (void) vm_deallocate(mach_task_self(),
				 (vm_offset_t) *addr,
				 *count);
	    *count = 0;
	}

	return (error);
}

kern_return_t
bsd_emulator_error(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	small_char_array	error_message,
	mach_msg_type_number_t	size)
{
	int pid;
	struct proc *p = proc_receive_lookup(proc_port, seqno);
	pid = p ? p->p_pid : 0;
	if (p)
	    mutex_unlock(&p->p_lock);

	printf("emulator [%d] %s\n", pid, error_message);
	return KERN_SUCCESS;
}

kern_return_t
bsd_readwrite(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	boolean_t		which,
	int			fileno,
	int			size,
	int			*amount)
{
	panic("bsd_readwrite");
#if 0
#if	MAP_UAREA
	if (which != 0 && which != 1)
		return (EINVAL);
	else {
		START_SERVER(which?SYS_write:SYS_read, 3)

		arg[0] = fileno;
		arg[1] = (integer_t)p->p_readwrite;
		arg[2] = (integer_t)size;
		if (which)
			error = write(p,
				      arg, amount);
		else
			error = read(p,
				     arg, amount);

		END_SERVER(which?SYS_write:SYS_read)
	}
#else	MAP_UAREA
	return (EINVAL);
#endif	MAP_UAREA
#endif
}

kern_return_t
bsd_share_wakeup(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			offset)
{
#if	MAP_UAREA
	register struct proc	*p;

	if ((p = proc_receive_lookup(proc_port, seqno)) == (struct proc *)0)
		return (ESRCH);
	mutex_unlock(&p->p_lock); /* XXX should replace master lock */
#if	SYSCALLTRACE
	if (syscalltrace &&
		(syscalltrace == p->p_pid || syscalltrace < 0)) {
	    printf("[%d]bsd_share_wakeup(%lx, %x)",
		   p->p_pid, (integer_t)p, offset);
	}
#endif
	mutex_lock(&master_mutex);
	wakeup((caddr_t)(p->p_shared_rw) + offset);
	mutex_unlock(&master_mutex);
	return (0);
#else MAP_UAREA
	return (EINVAL);
#endif MAP_UAREA
}

kern_return_t
bsd_maprw_request_it(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	register int		fileno)
{
#if	MACH_NBC && MAP_UAREA
	register struct inode *ip;

	START_SERVER_PARALLEL(1005, 1)
	ip = VTOI((struct vnode *)u.u_ofile[fileno]->f_data);
	user_request_it(ip, fileno, 0);
	END_SERVER_PARALLEL
#else	MACH_NBC && MAP_UAREA
	return (EINVAL);
#endif	MACH_NBC && MAP_UAREA
}

kern_return_t
bsd_maprw_release_it(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	register int		fileno)
{
#if	MACH_NBC && MAP_UAREA
	register struct inode *ip;
	START_SERVER_PARALLEL(1006, 1)
	ip = VTOI((struct vnode *)u.u_ofile[fileno]->f_data);
	user_release_it(ip);
	END_SERVER_PARALLEL
#else	MACH_NBC && MAP_UAREA
	return (EINVAL);
#endif	MACH_NBC && MAP_UAREA
}

kern_return_t
bsd_maprw_remap(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	register int		fileno,
	int			offset,
	int			size)
{
#if	MACH_NBC && MAP_UAREA
	register struct inode *ip;
	START_SERVER_PARALLEL(1007, 3)
	ip = VTOI((struct vnode *)u.u_ofile[fileno]->f_data);
	user_remap_inode(ip, offset, size);
	END_SERVER_PARALLEL
#else	MACH_NBC && MAP_UAREA
	return (EINVAL);
#endif	MACH_NBC && MAP_UAREA
}

/*
 * The bsd front wrappers to gprof routines are necessary to allow us
 * to access the gprof routines through the client's bootstrap port.
 */

/*ARGSUSED*/
kern_return_t
bsd_mon_switch(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			*sample_flavor)
{
	struct proc *p;
	if ((p = proc_receive_lookup(proc_port, seqno)) == (struct proc *)0)
	    return ESRCH;
	mutex_unlock(&p->p_lock);  
#if defined(GPROF) && ((defined(i386) || defined(mips) || defined(ns532)))
	return do_gprof_mon_switch(proc_port, sample_flavor);
#else
	return EINVAL;
#endif
}

/*ARGSUSED*/
kern_return_t
bsd_mon_dump(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	char_array		*mon_data,
	mach_msg_type_number_t	*mon_data_cnt)
{
	struct proc *p;
	if ((p = proc_receive_lookup(proc_port, seqno)) == (struct proc *)0)
	    return ESRCH;
	mutex_unlock(&p->p_lock);  
#if defined(GPROF) && ((defined(i386) || defined(mips) || defined(ns532)))
	return do_gprof_mon_dump(proc_port, mon_data, mon_data_cnt);
#else
	return EINVAL;
#endif
}

kern_return_t
bsd_setattr(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	vattr_t			vattr)
{
	START_SERVER(1008, 2);
	TRACE(("(x%x)", fileno));
	error = setattr(p, fileno, &vattr);
	END_SERVER(1008);
}

kern_return_t
bsd_getattr(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	vattr_t			*vattr)		/* OUT */
{
	START_SERVER(1009, 2);
	TRACE(("(x%x)", fileno));
	error = getattr(p, fileno, vattr);
	END_SERVER(1009);
}

kern_return_t
bsd_path_setattr(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	boolean_t		follow,
	char			*path,
	mach_msg_type_number_t	path_count,
	vattr_t			vattr)
{
	START_SERVER(1010, 2);
	TRACE(("(%s,%d)", path, follow));
	error = path_setattr(p, path, follow, &vattr);
	END_SERVER(1010);
}

kern_return_t
bsd_path_getattr(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	boolean_t		follow,
	char			*path,
	mach_msg_type_number_t	path_count,
	vattr_t			*vattr)		/* OUT */
{
	START_SERVER(1011, 2);
	TRACE(("(%s,%d)", path, follow));

	error = path_getattr(p, path, follow, vattr);
	END_SERVER(1011);
}

kern_return_t
bsd_execve(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	vm_address_t		arg_addr,
	vm_size_t		arg_size,
	int			arg_count,
	int			env_count,
	char			*fname,
	mach_msg_type_number_t	fname_count)
{
	START_SERVER(1012, 5)

	TRACE(("(%s)",fname));

	error = s_execve(p, fname, arg_addr, arg_size, arg_count, env_count);
	if (error == KERN_SUCCESS) {
		/* No one to return to as the task was changed */
		error = MIG_NO_REPLY;
	}
	END_SERVER(1012)
}

kern_return_t
bsd_after_exec(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	vm_address_t		*arg_addr,	  /* OUT */
	vm_size_t		*arg_size,	  /* OUT */
	int			*arg_count,	  /* OUT */
	int			*env_count,	  /* OUT */
	mach_port_t		*image_port,	  /* OUT move_send */
	char			*emul_name,	  /* OUT */
	mach_msg_type_number_t	*emul_name_count, /* OUT */
	char			*fname,		  /* OUT */
	mach_msg_type_number_t	*fname_count,	  /* OUT */
	char			*cfname,	  /* OUT */
	mach_msg_type_number_t	*cfname_count,	  /* OUT */
	char			*cfarg,		  /* OUT */
	mach_msg_type_number_t	*cfarg_count)	  /* OUT */
{
	START_SERVER(1013, 4)

	error = after_exec(p,
			   arg_addr, arg_size, arg_count, env_count,
			   emul_name, emul_name_count,
			   fname, fname_count,
			   cfname, cfname_count,
			   cfarg, cfarg_count,
			   image_port);
	END_SERVER(1013)
}

kern_return_t
bsd_exec_done(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno)
{
	START_SERVER(1019, 1);
	error = 0;
	END_SERVER(1019);
}

kern_return_t
bsd_init_process(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno)
{
	register int	error;
	register struct proc *p;

	p = proc_receive_lookup(proc_port, seqno);
	if (error = start_server_op(p, 1002))
	    return (error);

	init_process(p);

	return (end_server_op(p, error, (boolean_t *)0));
}

/* 
 * Like vm_map but applies representative object on process.
 */
kern_return_t
bsd_vm_map(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	vm_address_t		*address,	/* IN/OUT */
	vm_size_t		size,
	vm_address_t		mask,
	boolean_t		anywhere,
	mach_port_t		memory_object_rep,
	vm_offset_t		offset,
	boolean_t		copy,
	vm_prot_t		cur_prot,
	vm_prot_t		max_prot,
	vm_inherit_t		inheritance)
{
	struct proc *p;
	struct file *file;
	mach_error_t error;
	proc_invocation_t pk = get_proc_invocation();

	p = port_object_receive_lookup(proc_port, seqno, POT_PROCESS);
	error = start_server_op(p, 1014);
	if (error)
	    return error;
	pk->k_reply_msg = 0;
	file = port_object_send_lookup(memory_object_rep, POT_FILE_HANDLE);
	if (file) {
		unix_master();
		error = file_vm_map(p, address, size, mask, anywhere, file,
				    offset, copy, cur_prot, max_prot,
				    inheritance);
		unix_release();
	} else {
		error = EBADF;
	}

	return (end_server_op(p, error, (boolean_t *)0));
}

/* 
 * "dup" file descriptor as port.
 */
kern_return_t
bsd_fd_to_file_port(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fd,
	mach_port_t		*port)		/* OUT move_send */
{
	START_SERVER(1015, 2);
	TRACE(("(x%x)", fd));
	error = fd_to_file_port(p, fd, port);
	END_SERVER(1015);
}

/* 
 * Open file as port.
 */

kern_return_t
bsd_file_port_open(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			mode,
	int			crtmode,
	char			*fname,
	mach_msg_type_number_t	fname_count,
	mach_port_t		*port)		/* OUT move_send */
{
	START_SERVER(1016, 4);
	TRACE(("(%s)", fname));
	panic("file_port_open unimplemented");
	/* error = file_port_open(?); */
	END_SERVER(1016);
}

/* bsd_zone_info is in zalloc.c */

kern_return_t
bsd_signal_port_register(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	mach_port_t		sigport)
{
	START_SERVER(1018, 1);
	mutex_lock(&p->p_lock);
	/* 
	 * A second checkin is ok. it will go away when the task dies
	 * and a dead port notification is received
	 */
	error = port_object_enter_send(&sigport, POT_SIGPORT, p);
	if (error == KERN_SUCCESS) {
		p->p_sigport = sigport;
		proc_ref(p);
	}
	mutex_unlock(&p->p_lock);
	END_SERVER(1018);
}

/* Set @bin expansion for namei */
kern_return_t
bsd_set_atexpansion(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	path_name_t		what_to_expand,
	mach_msg_type_number_t	what_count,
	path_name_t		expansion,
	mach_msg_type_number_t	expansion_count)
{
	char *o, *n;
	struct proc *p;

	p = proc_receive_lookup(proc_port, seqno);
	if (!p)
	    return ESRCH;

	TRACE(("\n[%d] bsd_set_atexpansion", p->p_pid));

	if ((what_count < 4) || bcmp("@bin", what_to_expand, 4)) {
		mutex_unlock(&p->p_lock);
		return EOPNOTSUPP;
	}
	what_to_expand[what_count - 1] = '\0';
	TRACE(("(\"%s\", ", what_to_expand));
	o = p->p_atbin;
	if (expansion_count > 1) {
		expansion[expansion_count - 1] = '\0';
		n = malloc(strlen(expansion) + 1);
		if (!n) {
			panic("bsd_set_atexpansion");
		} else {
			strcpy(n, expansion);
			p->p_atbin = n;
			TRACE(("\"%s\")", n));
		}
	} else {
		p->p_atbin = 0;
		TRACE(("<null>)"));
	}
	if (o)
	    free(o);
	mutex_unlock(&p->p_lock);
	return KERN_SUCCESS;
}

kern_return_t
bsd_sendmsg_short(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	int			flags,
	char			*data,
	mach_msg_type_number_t	data_count,
	sockarg_t		to,
	mach_msg_type_number_t	tolen,
	sockarg_t		cmsg,
	mach_msg_type_number_t	cmsg_length,
	size_t			*nsent)	/* OUT */
{
	START_SERVER(SYS_sendmsg, 6);

	error = s_sendmsg(p, fileno, flags,
			  to, tolen,
			  cmsg, cmsg_length,
			  data, data_count,
			  nsent);
	END_SERVER(SYS_sendmsg)
}

kern_return_t
bsd_sendmsg_long(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	int			flags,
	char			*data,
	mach_msg_type_number_t	data_count,
	sockarg_t		to,
	mach_msg_type_number_t	tolen,
	sockarg_t		cmsg,
	mach_msg_type_number_t	cmsg_length,
	size_t			*nsent)	/* OUT */
{
	START_SERVER(SYS_sendmsg, 6);

	error = s_sendmsg(p, fileno, flags,
			  to, tolen,
			  cmsg, cmsg_length,
			  data, data_count,
			  nsent);
	END_SERVER_DEALLOC(SYS_sendmsg, data, data_count);
}

kern_return_t
bsd_recvmsg_short(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	int			flags,
	int			*outflags,	/* OUT */
	int			fromlen,
	size_t			*nreceived,	/* OUT */
	sockarg_t		from, 		/* OUT */
	mach_msg_type_number_t	*from_count,	/* max in/OUT */
	int			cmsglen,
	sockarg_t		cmsg,		/* OUT */
	mach_msg_type_number_t	*cmsg_count,	/* max in/OUT */
	int			datalen,
	small_char_array	data,		/* OUT */
	mach_msg_type_number_t	*data_count)	/* max in/OUT */
{
	START_SERVER(SYS_recvmsg, 6);

	error = KERN_SUCCESS;
	if (fromlen < 0 || fromlen > *from_count)
	    error = EINVAL;
	if (cmsglen < 0 || cmsglen > *cmsg_count)
	    error = EINVAL;
	if (datalen < 0 || datalen > *data_count)
	    error = EINVAL;
	*from_count = fromlen;
	*cmsg_count = cmsglen;
	*data_count = datalen;
	if (error == KERN_SUCCESS) {
		error = s_recvmsg(p, fileno, flags, &outflags,
				  from, from_count,
				  cmsg, cmsg_count,
				  data, datalen, nreceived);
		if (error == KERN_SUCCESS)
		    *data_count = *nreceived;
	}
	END_SERVER(SYS_recvmsg)
}

kern_return_t
bsd_recvmsg_long(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	int			fileno,
	int			flags,
	int			*outflags,	/* OUT */
	size_t			*nreceived,	/* OUT */
	int			fromlen,
	sockarg_t		from, 		/* OUT */
	mach_msg_type_number_t	*from_count,	/* max in/OUT */
	int			cmsglen,
	sockarg_t		cmsg,		/* OUT */
	mach_msg_type_number_t	*cmsg_count,	/* max in/OUT */
	int			datalen,
	char_array		*data,		/* OUT dealloc */
	mach_msg_type_number_t	*data_count)	/* OUT */
{
	START_SERVER(SYS_recvmsg, 6);

	error = KERN_SUCCESS;
	if (fromlen < 0 || fromlen > *from_count)
	    error = EINVAL;
	if (cmsglen < 0 || cmsglen > *cmsg_count)
	    error = EINVAL;
	*from_count = fromlen;
	*cmsg_count = cmsglen;
	*data_count = round_page(datalen);

	if (error == KERN_SUCCESS) {
		error = vm_allocate(mach_task_self(),
				    (vm_offset_t *) data, *data_count, TRUE);
		if (error == KERN_SUCCESS) {
			error = s_recvmsg(p, fileno, flags, &outflags,
					  from, from_count, cmsg, cmsg_count,
					  *data, datalen, nreceived);
			if (error)
				(void)vm_deallocate(mach_task_self(),
						    (vm_offset_t)*data,
						    (vm_size_t)*data_count);
		}
	}
	END_SERVER(SYS_recvmsg);
}

kern_return_t
bsd_sysctl(
	mach_port_t		proc_port,
	mach_port_seqno_t	seqno,
	mib_t			name,
	mach_msg_type_number_t  name_count,
	int                     namelen,
	char_array              *old,		/* OUT dealloc */
	mach_msg_type_number_t  *old_count,
	size_t                  *oldlen, 	/* IN/OUT */
	char_array              new,		/* IN */
	mach_msg_type_number_t  new_count,
	size_t                  newlen,         /* IN */
	int                     *retlen)        /* OUT */
{
	START_SERVER(SYS___sysctl, 6);

	error = KERN_SUCCESS;
	if (namelen < 2 || namelen > name_count)
		error = EINVAL;
	if (error == KERN_SUCCESS) {
		if (oldlen && *oldlen) {
			*old_count = round_page(*oldlen);
			error = vm_allocate(mach_task_self(),
					    (vm_offset_t *)old,
					    (vm_size_t)*old_count, TRUE);
		} else {
			*old_count = 0;
			*old       = 0;
		}
		if (error == KERN_SUCCESS) {
			error = s___sysctl(p,
					   (int *)name, namelen,
					   (void *)*old, oldlen,
					   (void *)new, newlen,
					   retlen);
			if (error && *old_count)
				(void)vm_deallocate(mach_task_self(),
						    (vm_offset_t)*old,
						    (vm_size_t)*old_count);
		}
	}
	if (new_count)
		(void)vm_deallocate(mach_task_self(),
				    (vm_offset_t)new, (vm_size_t)new_count);
	END_SERVER(SYS___sysctl);
}
