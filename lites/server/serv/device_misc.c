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
 * $Log: device_misc.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.6  1996/03/14  19:59:50  sclawson
 * Now #include "slice.h".
 *
 * Revision 1.5  1996/03/09  00:52:24  goel
 * parse_root_device: Don't use C_BLOCK_GET until major_num is initialized.
 * Fixed typo in slice number calculation.
 *
 * Revision 1.4  1996/02/17  02:09:01  sclawson
 * bug fix in parse_root_device() (missing quotes).
 *
 * Revision 1.3  1996/02/17  01:07:53  sclawson
 * bdev_name_string(), cdev_name_string() (through disk_name_string()), and
 * parse_root_device() understand slice names if DISKSLICE is defined.
 *
 * Revision 1.2  1995/08/30  22:54:08  sclawson
 * added support for /dev/zero.
 *
 * Revision 1.1.1.2  1995/03/23  01:16:54  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File: 	server/serv/device_misc.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Device interface for out-of-kernel kernel.
 */

#include "osfmach3.h"
#include "slice.h"

#include <serv/server_defs.h>

#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/synch.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/synch.h>

#include <serv/device.h>
#include <serv/device_utils.h>
#include <mach/kern_return.h>
#include <sys/parallel.h>

#ifdef 	DISKSLICE
#include <sys/diskslice.h> 		/* XXX */
#endif	/* DISKSLICE */

void itoa(int num, char	str[]);

/*
 * Character device support.
 */
struct char_device {
	mach_port_t	c_device_port;	/* port to device */
	mach_port_t	c_select_port;	/* reply port for select */
	struct selinfo	c_read_select;
	int		c_read_sel_state; /* message sent/recvd for r-select */
#define CD_SEL_MSENT 1
#define CD_SEL_MRCVD 2
	struct selinfo	c_write_select;
	int		c_write_sel_state;
					/* message sent/recvd for select */
	int		c_mode;		/* IO modes: */
#define	C_NBIO		0x01		/* non-blocking IO */
#define	C_ASYNC		0x02		/* asynchronous notification */
	struct pgrp	*c_pgrp;
};

#define	char_hash_enter(dev, cp) \
		dev_number_hash_enter(XDEV_CHAR(dev), (char *)(cp))
#define	char_hash_remove(dev) \
		dev_number_hash_remove(XDEV_CHAR(dev))
#define	char_hash_lookup(dev) \
		((struct char_device *)dev_number_hash_lookup(XDEV_CHAR(dev)))

int	char_select_read_reply();	/* forward */
int	char_select_write_reply();	/* forward */

/*
 * Open device (not TTY).
 * Call device server, then enter device and port in hash table.
 */
mach_error_t char_open(dev_t dev, int flag, int devtype, struct proc *p)
{
#if OSFMACH3
	dev_name_t	name;
#else
	char		name[32];
#endif
	kern_return_t	rc;
	mach_port_t	device_port;
	int		mode;
	boolean_t	new_char_dev;

	register struct char_device *cp;

	/*
	 * Check whether character device is already open.
	 */
	cp = char_hash_lookup(dev);
	if (cp == 0) {
	    /*
	     * Create new char_device structure.
	     */
	    cp = (struct char_device *)malloc(sizeof(struct char_device));
	    char_hash_enter(dev, cp);
	    cp->c_device_port = MACH_PORT_NULL;
	    cp->c_select_port = MACH_PORT_NULL;
	    selinfo_init(&cp->c_read_select);
	    selinfo_init(&cp->c_write_select);
#if 0
	    queue_init(&cp->c_read_select);
	    queue_init(&cp->c_write_select);
#endif
	    cp->c_read_sel_state = 0;
	    cp->c_write_sel_state = 0;
	    cp->c_mode = 0;
	    cp->c_pgrp = p->p_pgrp;

	    new_char_dev = TRUE;
	}
	else {
	    new_char_dev = FALSE;
	}

	if (cp->c_device_port == MACH_PORT_NULL) {
	    /*
	     * Device is closed.  Try to open it.
	     */
	    rc = cdev_name_string(dev, name);
	    if (rc != 0)
		return (rc);	/* bad name */

	    mode = D_READ|D_WRITE;
	    rc = device_open(device_server_port,
#if OSF_LEDGERS
			     MACH_PORT_NULL,
#endif
			     mode,
#if OSFMACH3
			     security_id,
#endif
			     name,
			     &device_port);
	    if (rc != D_SUCCESS) {
		if (new_char_dev) {
		    char_hash_remove(dev);
		    free((char *)cp);
		}
		return (dev_error_to_errno(rc));
	    }
	    cp->c_device_port = device_port;
	}
	return (0);
}

mach_error_t char_close(dev_t dev, int flag, int mode, struct proc *p)
{
	register struct char_device *cp;
	register int	error;

	cp = char_hash_lookup(dev);

	if (!cp)
	    return EIO;

	error = dev_error_to_errno(device_close(cp->c_device_port));
	(void) mach_port_deallocate(mach_task_self(), cp->c_device_port);
	cp->c_device_port = MACH_PORT_NULL;

#if 0
	reply_hash_remove(cp->c_select_port);
	(void) mach_port_mod_refs(mach_task_self(), cp->c_select_port,
				  MACH_PORT_RIGHT_RECEIVE, -1);
#else
	    /* This could be more elegant and lazy */
	    port_object_shutdown(cp->c_select_port, TRUE);
#endif
	cp->c_select_port = MACH_PORT_NULL;

	selinfo_init(&cp->c_read_select);
	selinfo_init(&cp->c_write_select);
	cp->c_read_sel_state = 0;
	cp->c_write_sel_state = 0;

	char_hash_remove(dev);
	free((char *)cp);

	return (dev_error_to_errno(error));
}

mach_port_t char_port(dev)
	dev_t dev;
{
	return (char_hash_lookup(dev)->c_device_port);
}

mach_error_t char_read(dev_t	dev, struct uio *uio, int flag)
{
	register int	c;
	register kern_return_t	rc;
	io_buf_ptr_inband_t	data;	/* inline array */
	mach_msg_type_number_t	count;
	register struct char_device *cp;
	boolean_t	first = TRUE;
	boolean_t nonblocking, async;
	proc_invocation_t pk = get_proc_invocation();
	boolean_t on_master = pk->k_master_lock; /* XXX */

	cp = char_hash_lookup(dev);

	nonblocking = cp->c_mode & C_NBIO;
	async = cp->c_mode & C_ASYNC;

	while (uio->uio_iovcnt > 0) {

	    c = uio->uio_iov->iov_len;

	    if (c > IO_INBAND_MAX)
		c = IO_INBAND_MAX;

	    if (on_master)
		unix_release();
	    /* 
	     * Cant' have ipl since either locking order is violated
	     * or everybody waiting for spl lock are getting stuck.
	     * (D_NOWAIT would be ok though).
	     */
	    if (pk->k_ipl != 0)
		panic("char_read: ipl");

	    count = IO_INBAND_MAX;
	    rc = device_read_inband(cp->c_device_port,
				(nonblocking || !first)
					? D_NOWAIT
					: 0,
				0,
				c,
				&data[0],
				&count);
	    if (on_master)
		unix_master();

	    if (count == 0 && !first)
		break;
	    if (rc != 0) {
		if ((rc == D_WOULD_BLOCK) && !first) /* XXX never returned */
		    break;
		return (dev_error_to_errno(rc));
	    }
	    first = FALSE;

	    uiomove(&data[0], count, uio);
	}

	if (async) {
	    /*
	     * Post read request if not already posted.
	     */
	    if ((cp->c_read_sel_state & CD_SEL_MSENT) == 0) {
		cp->c_read_sel_state |= CD_SEL_MSENT;
		(void) device_read_request_inband(cp->c_device_port,
						  cp->c_select_port,
						  0,	/* mode */
						  0,	/* recnum */
						  0);	/* bytes wanted */
	    }
	}
	return (0);
}

mach_error_t char_write(dev_t dev, struct uio *uio, int flag)
{
	/* break up the uio into individual device_write calls */
	panic("char_write called");
	return EIO;
}

mach_error_t char_ioctl(
	dev_t	dev,
	ioctl_cmd_t	cmd,
	caddr_t	data,
	int	flag,
	struct proc *p)
{
	register struct char_device *cp;
	mach_msg_type_number_t	count;
	register int	error;
	proc_invocation_t pk = get_proc_invocation();
	kern_return_t kr;

	cp = char_hash_lookup(dev);

	if (cmd == FIONBIO) {
	    if (*(int *)data)
		cp->c_mode |= C_NBIO;
	    else
		cp->c_mode &= ~C_NBIO;
	    return (0);
	}

	if (cmd == FIOASYNC) {
	    if (*(int *)data) {
		cp->c_mode |= C_ASYNC;

		/*
		 * If select reply port does not exist, create it.
		 */
		if (cp->c_select_port == MACH_PORT_NULL) {
#if 0
		    cp->c_select_port = mach_reply_port();
		    reply_hash_enter(cp->c_select_port,
				     (char *)cp,
				     char_select_read_reply,
				     char_select_write_reply);
#else
		    kr = port_object_allocate_receive(&cp->c_select_port,
						      POT_CHAR_DEV,
						      cp);
		    add_to_reply_port_set(cp->c_select_port);
		    assert(kr == KERN_SUCCESS);
#endif
		}
		/*
		 * Post read request if not already posted.
		 */
		if ((cp->c_read_sel_state & CD_SEL_MSENT) == 0) {
		    cp->c_read_sel_state |= CD_SEL_MSENT;
		    (void) device_read_request_inband(cp->c_device_port,
					cp->c_select_port,
					0,	/* mode */
					0,	/* recnum */
					0);	/* bytes wanted */
		}
	    }
	    else {
		cp->c_mode &= ~C_ASYNC;
	    }
	    return (0);
	}

#ifdef	i386
    {
	struct X_kdb {
	    u_int *ptr;
	    u_int size;
	};

#define K_X_KDB_ENTER	_IOW('K', 16, struct X_kdb)
#define K_X_KDB_EXIT	_IOW('K', 17, struct X_kdb)

	if ((cmd == K_X_KDB_ENTER) ||
	    (cmd == K_X_KDB_EXIT)) {
	    u_int X_kdb_buffer[512];
	    struct X_kdb *X_kdb = (struct X_kdb *) data;

	    /* make sure that copyin will do the right thing */
	    if (pk->k_reply_msg == 0)
		panic("X_K_KDB ioctl");

	    if (X_kdb->size > sizeof X_kdb_buffer)
		return (ENOENT);

	    if (copyin(X_kdb->ptr, X_kdb_buffer, X_kdb->size))
		return (EFAULT);

	    error = device_set_status(cp->c_device_port, cmd,
				      (int *)X_kdb_buffer, X_kdb->size>>2);
	    if (error)
		return (ENOTTY);

	    return (0);
	}
    }
#endif	i386

	count = (cmd & ~(IOC_INOUT|IOC_VOID)) >> 16; /* bytes */
	count = (count + 3) >> 2;		     /* ints */
	if (count == 0)
	    count = 1;

	if (cmd & (IOC_VOID|IOC_IN)) {
	    error = device_set_status(cp->c_device_port,
				      cmd,
				      (int *)data,
				      count);
	    if (error)
		return (ENOTTY);
	}
	if (cmd & IOC_OUT) {
	    error = device_get_status(cp->c_device_port,
				      cmd,
				      (int *)data,
				      &count);
	    if (error)
		return (ENOTTY);
	}
	return (0);
}

boolean_t char_select(
	dev_t	dev,
	int	rw,
	struct proc *p)
{
	register struct char_device *cp;
	register int s;
	kern_return_t kr;

	cp = char_hash_lookup(dev);

	/*
	 * If select reply port does not exist, create it.
	 */
	if (cp->c_select_port == MACH_PORT_NULL) {
#if 0
	    cp->c_select_port = mach_reply_port();
	    reply_hash_enter(cp->c_select_port,
			     (char *)cp,
			     char_select_read_reply,
			     char_select_write_reply);
#else
	    kr = port_object_allocate_receive(&cp->c_select_port,
					      POT_CHAR_DEV,
					      cp);
	    add_to_reply_port_set(cp->c_select_port);
	    assert(kr == KERN_SUCCESS);
#endif
	}

	s = spltty();

	switch (rw) {
	    case FREAD:
		/*
		 * If reply available, consume it
		 */
		if (cp->c_read_sel_state & CD_SEL_MRCVD) {
		    cp->c_read_sel_state = 0;
		    splx(s);
		    return (1);
		}
		/*
		 * Post read request if not already posted.
		 */
		if ((cp->c_read_sel_state & CD_SEL_MSENT) == 0) {
		    cp->c_read_sel_state |= CD_SEL_MSENT;
		    (void) device_read_request_inband(cp->c_device_port,
					cp->c_select_port,
					0,	/* mode */
					0,	/* recnum */
					0);	/* bytes wanted */
		}
		selrecord(p, &cp->c_read_select);
		break;

	    case FWRITE:
		/*
		 * If reply available, consume it
		 */
		if (cp->c_write_sel_state & CD_SEL_MRCVD) {
		    cp->c_write_sel_state = 0;
		    splx(s);
		    return (1);
		}
		/*
		 * Post write request if not already posted.
		 */
		if ((cp->c_write_sel_state & CD_SEL_MSENT) == 0) {
		    cp->c_write_sel_state |= CD_SEL_MSENT;
		    (void) device_write_request_inband(cp->c_device_port,
					cp->c_select_port,
					0,	/* mode */
					0,	/* recnum */
					0,	/* data */
					0);	/* bytes wanted */
		}
		selrecord(p, &cp->c_write_select);
		break;
	}

	splx(s);

	return (0);
}

/*
 * Handler for c_select_port replies.
 */
char_select_read_reply(cp, error, data, size)
	register struct char_device *cp;
	int		error;
	char		*data;
	unsigned int	size;
{
	interrupt_enter(SPLTTY);
	cp->c_read_sel_state = CD_SEL_MRCVD;
	selwakeup(&cp->c_read_select);
	if (cp->c_mode & C_ASYNC)
	    gsignal(cp->c_pgrp->pg_id, SIGIO);
	interrupt_exit(SPLTTY);
}

char_select_write_reply(cp, error, size)
	register struct char_device *cp;
	int		error;
	unsigned int	size;
{
	interrupt_enter(SPLTTY);
	cp->c_write_sel_state = CD_SEL_MRCVD;
	selwakeup(&cp->c_write_select);
	interrupt_exit(SPLTTY);
}

/*

 */

/*
 * Device memory object support.
 */
memory_object_t
device_pager_create(dev, offset, size, protection)
	dev_t		dev;
	vm_offset_t	offset;
	vm_size_t	size;
	vm_prot_t	protection;
{
	mach_port_t	(*d_port_routine)(dev_t);
	mach_port_t	device_port;
	memory_object_t	pager;
	int		status;

	d_port_routine = cdevsw[major(dev)].d_port;
	if (d_port_routine == (mach_port_t (*)(dev_t))0)
		return (MACH_PORT_NULL);
	device_port = (*d_port_routine)(dev);
#if i386 /* XXX for X11 binary compat on second server	   XXX */
	{						/* XXX */
		extern mach_port_t dprintf_console_port;/* XXX */
		extern mach_port_t cons_port(dev_t);	/* XXX */
		if ((d_port_routine == cons_port)	/* XXX */
		    && (device_port == MACH_PORT_NULL)) /* XXX */
		    device_port = dprintf_console_port;	/* XXX */
	}						/* XXX */
#endif							/* XXX */
	if (device_port == MACH_PORT_NULL)
		return (MACH_PORT_NULL);

	status = (*cdevsw[major(dev)].d_mmap)(device_port, protection, offset, 
					      size, &pager, 0); 
	if (status != KERN_SUCCESS) {
		return (MACH_PORT_NULL);
	}
	return (pager);
}

device_pager_release(mem_obj)
	memory_object_t	mem_obj;
{
	kern_return_t kr;

	kr = mach_port_deallocate(mach_task_self(), mem_obj);
	if (kr != KERN_SUCCESS)
		panic("device_pager_release");
}

/*
 * Memory device.
 */
#define M_KMEM		1	/* /dev/kmem - virtual kernel memory & I/O */
#define M_NULL		2	/* /dev/null - EOF & Rathole */
#define M_ZERO	       12	/* /dev/zero - fun for dd */

mach_error_t mmopen(dev_t dev, int flag)
{
	switch (minor(dev)) {
	    case M_KMEM:
	    case M_NULL:
	    case M_ZERO:
		return (0);
	}
	return (ENXIO);
}

mach_error_t mmread(dev_t dev, struct uio *uio)
{
	return (mmrw(dev, uio, UIO_READ));
}

mach_error_t mmwrite(dev_t dev, struct uio *uio)
{
	return (mmrw(dev, uio, UIO_WRITE));
}	

mach_error_t mmrw(
	dev_t dev,
	struct uio *uio,
	enum uio_rw rw)

{
	register u_int c;
	register struct iovec *iov;
	int error = 0;
	caddr_t zbuf = NULL;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		case M_KMEM:
			c = iov->iov_len;
			if (mmca((vm_address_t)uio->uio_offset, (vm_size_t)c,
			    rw == UIO_READ ? B_READ : B_WRITE)) {
			  	/* uio_offset is 64 bit wide */
			  	vm_offset_t offset =
				  (vm_offset_t)(uio->uio_offset);
				error = uiomove((caddr_t)offset,
					(int)c, uio);
				continue;
			}
			return (EFAULT);

		case M_NULL:
			if (rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_offset += c;
			uio->uio_resid -= c;
			break;
			
		case M_ZERO:
			if (rw == UIO_WRITE) {
                                c = iov->iov_len;
                                break;
                        }
                        if (zbuf == NULL) {
                                zbuf = (caddr_t)
                                    bsd_malloc(CLBYTES, M_TEMP, M_WAITOK);
                                bzero(zbuf, CLBYTES);
                        }
                        c = min(iov->iov_len, CLBYTES);
                        error = uiomove(zbuf, (int)c, uio);
                        continue;
		}
	}
	if (zbuf)
                bsd_free(zbuf, M_TEMP);
	return (error);
}

/*
 *	Returns true if the region is readable.
 */

boolean_t mmca(
	vm_address_t address,
	vm_size_t count)
{
    register vm_offset_t	addr;
    vm_offset_t			r_addr;
    vm_size_t			r_size;
    vm_prot_t			r_protection,
				r_max_protection;
    vm_inherit_t		r_inheritance;
    boolean_t			r_is_shared;
    memory_object_name_t	r_object_name;
    vm_offset_t			r_offset;

    addr = address;
    while (addr < address + count) {
#if OSFMACH3
	struct vm_region_basic_info r;
	mach_msg_type_number_t icount;

	r_addr = addr;
	icount = VM_REGION_BASIC_INFO_COUNT;
	if (vm_region(mach_task_self(),
		      &r_addr,
		      &r_size,
		      VM_REGION_BASIC_INFO,
		      (vm_region_info_t) &r,
		      &icount,
		      &r_object_name) != KERN_SUCCESS)
	    return (0);
	r_protection = r.protection;
	r_max_protection = r.max_protection;
	r_inheritance = r.inheritance;
	r_is_shared = r.shared;
	r_offset = r.offset;
#else
	r_addr = addr;
	if (vm_region(mach_task_self(),
		      &r_addr,
		      &r_size,
		      &r_protection,
		      &r_max_protection,
		      &r_inheritance,
		      &r_is_shared,
		      &r_object_name,
		      &r_offset) != KERN_SUCCESS)
	    return (0);
#endif /* OSFMACH3 */

	if (MACH_PORT_VALID(r_object_name))
	    (void) mach_port_deallocate(mach_task_self(), r_object_name);

	/* is there a gap? */
	if (r_addr > addr)
	    return (0);

	/* is this region not readable? */
	if ((r_protection & VM_PROT_READ) != VM_PROT_READ)
	    return (0);

	/* continue to next region */
	addr = r_addr + r_size;
    }
    return (1);
}

void
disk_name_string(dev, part_count, str)
	dev_t 	dev;
	int	part_count;
	char	*str;		/* REF OUT */
{
	int minor_num = minor(dev);
	char num[4];
	int part;
#if 	DISKSLICE
	int slice;
#endif 	/* DISKSLICE */
	
	/* 
	 * We mask out the upper 16 bits to remove any slice number
	 * that could be lurking up there.  Alpha minor numbers under
	 * lites (or at least in sys/types.h) seem to include the
	 * lower 20 bits of a dev_t, which means that they've got 17
	 * bits for a unit number??  If those upper 4 bits really are
	 * used, then this will have to be fixed.  
	 */
	itoa((minor_num & 0xffff)/part_count, num);
	strcat(str, num);

	/* 
	 * This assumes that the only devices that have bits set in
	 * the upper 16 bits of a dev_t are those that support slices.
	 * If this isn't true, this will cause trouble.  On the alpha
	 * dkslice() always returns COMPATIBILITY_SLICE.
	 */
	part = minor_num % part_count;
#if 	DISKSLICE
	slice = dkslice(dev);
	if (slice != WHOLE_DISK_SLICE) {
	    if (slice != COMPATIBILITY_SLICE) {
	        sprintf(num, "s%d", slice);
		strcat(str, num);
	    }
	   if (part != RAW_PART || slice == COMPATIBILITY_SLICE) {
#endif 	/* DISKSLICE */
		num[0] = 'a' + part;
		num[1] = 0;
		strcat(str, num);
#if 	DISKSLICE
	    }
	}
#endif 	/* DISKSLICE */
}


int
cdev_name_string(dev, str)
	dev_t	dev;
	char	str[];	/* REF OUT */
{
	int	major_num = major(dev);
	int	d_flag;

	if (major_num < 0 || major_num >= nchrdev)
	    return (ENXIO);

	d_flag = cdevsw[major_num].d_flags;
	if (d_flag & C_MINOR) {
	    /*
	     * Must check minor device number -
	     * not all minors for this device translate to
	     * the same name
	     */
	    return (check_dev(dev, str));
	}
	else {
   	    strcpy(str, cdevsw[major_num].d_name);
	    if (d_flag & C_BLOCK(0)) {
		/*
		 * Disk device - break minor into dev/partition
		 */
		disk_name_string(dev, C_BLOCK_GET(d_flag), str);
	}
	    else {
		/*
		 * Add minor number
		 */
		char num[4];
		itoa(minor(dev), num);
		strcat(str, num);
	    }
	}
	return (0);
}

mach_error_t
bdev_name_string(
	dev_t	dev,
	char	str[])	/* REF OUT */
{
	int	major_num = major(dev);

	if (major_num < 0 || major_num >= nblkdev)
	    return (ENXIO);

	strcpy(str, bdevsw[major_num].d_name);

	disk_name_string(dev, C_BLOCK_GET(bdevsw[major_num].d_flags), str);

	return (0);
}

void itoa(
	int	num,
	char	str[])
{
	char	digits[11];
	register char *dp;
	register char *cp = str;

	if (num == 0) {
	    *cp++ = '0';
	}
	else {
	    dp = digits;
	    while (num) {
		*dp++ = '0' + num % 10;
		num /= 10;
	    }
	    while (dp != digits) {
		*cp++ = *--dp;
	    }
	}
	*cp++ = '\0';
}

/*
 * Parse root device name into a block device number.
 */
dev_t
parse_root_device(char *str)
{
	register char c;
	register char *cp = str;
	char *name_end;
	register int minor_num = 0;
	register int major_num;
	register struct bdevsw *bdp;
#ifdef	DISKSLICE 
	int slice = 0;
#endif	/* DISKSLICE */

	/*
	 * Find device type name (characters before digit)
	 */
	while ((c = *cp) != '\0' &&
		!(c >= '0' && c <= '9'))
	    cp++;
	name_end = cp;

	if (c != '\0') {
	    /*
	     * Find minor_num number
	     */
	    while ((c = *cp) != '\0' &&
		    c >= '0' && c <= '9') {
		minor_num = minor_num * 10 + (c - '0');
		cp++;
	    }
	}
	*name_end = 0;

	for (major_num = 0, bdp = bdevsw;
	     major_num < nblkdev;
	     major_num++, bdp++) {
	    if (!strcmp(str, bdp->d_name))
		break;
	}
	if (major_num == nblkdev) {
	    /* not found */
	    return ((dev_t)-1);
        }     
	minor_num *= C_BLOCK_GET(bdevsw[major_num].d_flags);
#ifdef 	DISKSLICE
	if (c == 's') {
	    cp++;
	    while ((c = *cp) != '\0' && c >= '0' && c <= '9') {
	        slice = slice * 10 + (c - '0');
		cp++;
	    }
    	}
#endif	/* DISKSLICE */
	if (c >= 'a' && c <= 'h') {
	    /*
	     * Disk minor number is 8*minor_num + partition.
	     */
	    minor_num += (c - 'a');
	}
#ifdef	DISKSLICE
	return (makedev(major_num, dkmodslice(minor_num, slice)));
#else	/* !DISKSLICE */
	return (makedev(major_num, minor_num));
#endif	/* !DISKSLICE */
}
