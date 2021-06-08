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
 * 16-Mar-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Make no D_NO_SPACE appear as a EOF for disk_read.
 *
 * 05-Oct-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *    Remove "temp kludge for tape drives" in disk_read since there are
 *    now seperate tape routines in tape_io.c.
 *
 * $Log: disk_io.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.3  1996/03/27  07:26:59  sclawson
 * added back in ``temp kludge for tape drives'' that Ian Dall removed in '94.
 *
 * Revision 1.2  1995/03/23  01:44:04  law
 * Update to 950323 snapshot + utah changes
 *
 * Revision 1.1.1.2  1995/03/22  23:26:30  law
 * Pure lites-950316 snapshot.
 *
 *
 */
/* 
 *	File:	serv/disk_io.c
 *	Author:	Randall W. Dean
 *	Date:	1992
 *
 *	Routines for char IO to block devices.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <serv/server_defs.h>
#include <serv/device_utils.h>

#define	disk_port_enter(dev, port) \
		dev_number_hash_enter(XDEV_CHAR(dev), (char *)(port))
#define	disk_port_remove(dev) \
		dev_number_hash_remove(XDEV_CHAR(dev))
#define	disk_port_lookup(dev) \
		((mach_port_t) dev_number_hash_lookup(XDEV_CHAR(dev)))

int
disk_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	char		name[32];
	kern_return_t	rc;
	mach_port_t	device_port;
	int		mode;

	rc = cdev_name_string(dev, name);
	if (rc != 0)
	    return (rc);	/* bad name */

	/* fix modes */
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
	if (rc != D_SUCCESS)
	    return (dev_error_to_errno(rc));

	disk_port_enter(dev, device_port);
	return (0);
}

int
disk_close(dev_t dev, int flag, int mode, struct proc *p)
{
	mach_port_t	device_port;
	int		error;

	device_port = disk_port_lookup(dev);
	if (device_port == MACH_PORT_NULL)
	    return (0);		/* should not happen */

	disk_port_remove(dev);
	error = dev_error_to_errno(device_close(device_port));
	(void) mach_port_deallocate(mach_task_self(), device_port);
	return (error);
}

int
disk_read(dev_t	dev, struct uio *uio, int flag)
{
	register struct iovec *iov;
	register int	c;
	register kern_return_t	rc;
	io_buf_ptr_t	data;
	unsigned int	count;

	natural_t	resid = uio->uio_resid;

	while (uio->uio_iovcnt > 0) {

	    iov = uio->uio_iov;
	    if (iov->iov_len == 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		continue;
	    }

	    if (useracc(iov->iov_base, (u_int)iov->iov_len, 0) == 0)
		return (EFAULT);

	    /*
	     * Can read entire block here - device handler
	     * breaks into smaller pieces.
	     */

	    c = iov->iov_len;

	    rc = device_read(disk_port_lookup(dev),
			     0,	/* mode */
			     btodb(uio->uio_offset),
			     iov->iov_len,
			     &data,
			     &count);
	    if (rc != 0) {
#ifdef D_NO_SPACE
		    if (rc == D_NO_SPACE)
			/* Treat as EOF */
			return(0);
#endif
		    return (dev_error_to_errno(rc));
	    }

	    (void) moveout(data, iov->iov_base, count);

	    iov->iov_base += count;
	    iov->iov_len -= count;
	    uio->uio_resid -= count;
	    uio->uio_offset += count;

	    /* XXX check for EOF */
	    if (count < c)
                break;
	}
	return (0);
}

int
disk_write(dev_t dev, struct uio *uio, int flag)
{
	register struct iovec *iov;
	register int	c;
	register kern_return_t	rc;
	vm_offset_t	kern_addr;
	vm_size_t	kern_size;
	vm_size_t	count;

	while (uio->uio_iovcnt > 0) {
	    iov = uio->uio_iov;

	    kern_size = iov->iov_len;
	    (void) vm_allocate(mach_task_self(), &kern_addr, kern_size, TRUE);
	    if (copyin(iov->iov_base, kern_addr, (u_int)iov->iov_len)) {
		(void) vm_deallocate(mach_task_self(), kern_addr, kern_size);
		return (EFAULT);
	    }

	    /*
	     * Can write entire block here - device handler
	     * breaks into smaller pieces.
	     */

	    c = iov->iov_len;

	    rc = device_write(disk_port_lookup(dev),
			      0,	/* mode */
			      btodb(uio->uio_offset),
			      (void *)kern_addr,
			      iov->iov_len,
			      &count);

	    (void) vm_deallocate(mach_task_self(), kern_addr, kern_size);

	    if (rc != 0)
		return (dev_error_to_errno(rc));

	    iov->iov_base += count;
	    iov->iov_len -= count;
	    uio->uio_resid -= count;
	    uio->uio_offset += count;

	    /* temp kludge for tape drives */
	    if (count < c)
		break;

	    uio->uio_iov++;
	    uio->uio_iovcnt--;
	}
	return (0);
}

mach_error_t disk_ioctl(
	dev_t		dev,
	ioctl_cmd_t	cmd,
	caddr_t		data,
	int		flag)
{
	mach_port_t	device_port = disk_port_lookup(dev);
	unsigned int	count;
	register int	error = KERN_SUCCESS;

	count = (cmd & ~(IOC_INOUT|IOC_VOID)) >> 16; /* bytes */
	count = (count + 3) >> 2;		     /* ints */
	if (count == 0)
	    count = 1;

	if (cmd & (IOC_VOID|IOC_IN)) {
	    error = device_set_status(device_port,
				      cmd,
				      (int *)data,
				      count);
	    if (error)
		return (dev_error_to_errno(error));
	}
	if (cmd & IOC_OUT) {
	    error = device_get_status(device_port,
				      cmd,
				      (int *)data,
				      &count);
	}
	if (error)
	     return (dev_error_to_errno(error));
	else
	     return (0);
}

mach_port_t
disk_port(dev)
	dev_t	dev;
{
	return (disk_port_lookup(dev));
}
