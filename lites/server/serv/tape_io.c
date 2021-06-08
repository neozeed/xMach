/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994 Ian Dall
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND IAN DALL ALLOW FREE USE OF THIS SOFTWARE IN ITS
 * "AS IS" CONDITION.  CARNEGIE MELLON AND IAN DALL DISCLAIM ANY
 * LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
 * USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: tape_io.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:52  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	serv/tape_io.c
 *	Author:	Ian Dall
 *	Date:	Sep 1994
 *
 *	Routines for char IO to tape devices. This is essentially like
 *	the disk_io routines except we have to handle file marks. Mach
 *	io can not return data *and* return an error (MIG
 *	restriction?). So on reaching a file mark, it must return the
 *	data and on the next read, return an D_FILEMARK code. At the
 *	unix level, we have to convert the EOF mark to a zero length
 *	read.  If we have already successfully read something (in
 *	iovec case), we need complete successfully but remember we got
 *	an EOF and then make the *next* read return zero.
 *
 * 	We impliment this by keeping a structure in the dev_hash_table
 *	with a per unit flag. The flag is set to indicate the next read
 *	should return with nothing read (EOF). On any operation, the
 *	flag is cleared.
 *
 *	Correct operation of this requires that the mach device driver
 *	also understands filemarks. Also, it is assumed that the
 *	device is exclusive access.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <serv/server_defs.h>
#include <serv/device_utils.h>

#include <serv/tape_io.h>	/* tape_io prototypes */

#include <device/tape_status.h>

struct tape_device {
  mach_port_t t_device_port;	/* port to device */
  int t_flags;
};

#define tape_hash_enter(dev, tp) \
  dev_number_hash_enter(XDEV_CHAR(dev), (char *)(tp))
#define	tape_hash_remove(dev) \
		dev_number_hash_remove(XDEV_CHAR(dev))
#define	tape_hash_lookup(dev) \
		((struct tape_device *)dev_number_hash_lookup(XDEV_CHAR(dev)))

/* The encoding of rewind and density bits is really machine dependent
 * Suggest machine/types.h as an appropriate place to put definitions
 */
#ifndef TAPE_UNIT
#define	TAPE_UNIT(dev)		(((dev) & ~0xff) | (((dev) >> 4) & 0xf))
#endif
#ifndef TAPE_REWINDS
#define	TAPE_REWINDS(dev)	(((dev)&0x1)==0)
#endif
#ifndef TAPE_DENS
#define TAPE_DENS(dev)		(((dev) >> 1) & 7)
#endif

#define T_EOF 1

#if defined(D_FILEMARK) && defined(D_NO_SPACE)
#define DEVICE_EOF(err) ((err) == D_FILEMARK || (err) == D_NO_SPACE)
#endif

tape_open(dev_t dev, int flag, int devtype, struct proc *p)
{
  char		name[32];
  kern_return_t	rc;
  mach_port_t	device_port;
  int		mode;
  struct tape_device *tp;
  dev_t unit = TAPE_UNIT(dev);
  /*
   * Check whether tape device is already open.
   */
  tp = tape_hash_lookup(unit);
  if (tp == 0) {
    /*
     * Create new tape_device structure.
     */
    tp = (struct tape_device *)malloc(sizeof(struct tape_device));
    tape_hash_enter(unit, tp);
    tp->t_device_port = MACH_PORT_NULL;
    tp->t_flags = 0;
  }
  else {
    return EBUSY;		/* Enforce exclusive open */
  }
  /*
   * Device is closed.  Try to open it.
   */
  rc = cdev_name_string(unit, name);
  if (rc != 0) {
    tape_hash_remove(unit);
    free((char *)tp);
    return (rc);
  }

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
    tape_hash_remove(unit);
    free((char *)tp);
    return (dev_error_to_errno(rc));
  }
  tp->t_device_port = device_port;
  
  if (TAPE_REWINDS(dev)) {
    struct tape_status ts;
    bzero(&ts, sizeof(ts));
    ts.flags = TAPE_FLG_REWIND;
    (void) device_set_status(device_port,
			     TAPE_STATUS,
			     (dev_status_t)&ts,
			     TAPE_STATUS_COUNT);
  }
  return 0;
}

tape_close(dev_t dev, int flag, int mode, struct proc *p)
{
	struct tape_device *tp;
	int	error;
	dev = TAPE_UNIT(dev);

	tp = tape_hash_lookup(dev);

	if (tp == 0)
	    return EIO;

	error = dev_error_to_errno(device_close(tp->t_device_port));
	(void) mach_port_deallocate(mach_task_self(), tp->t_device_port);
	tape_hash_remove(dev);
	free((char *)tp);

	return (dev_error_to_errno(error));
}

mach_port_t tape_port(dev_t dev)
{
	dev = TAPE_UNIT(dev);
	return (tape_hash_lookup(dev)->t_device_port);
}

int tape_read(dev_t dev, struct uio *uio, int flag)
{
  register struct iovec *iov;
  register int	c;
  register kern_return_t	rc;
  io_buf_ptr_t	data;
  unsigned int	count;
  natural_t	resid = uio->uio_resid;
  struct tape_device *tp;

  dev = TAPE_UNIT(dev);
  tp = tape_hash_lookup(dev);
  
  if(tp->t_flags & T_EOF) {
    /* Signal the eof as a zero length read */
    tp->t_flags &= ~T_EOF;
    return 0;
  }
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
    
    rc = device_read(tp->t_device_port,
		     0,	/* mode */
		     btodb(uio->uio_offset),
		     iov->iov_len,
		     &data,
		     &count);
    if (rc) {
#ifdef DEVICE_EOF
      /* Make this conditional on DEVICE_EOF to support old mk */
      if (DEVICE_EOF(rc)) {
	if (resid != uio->uio_resid)
	  tp->t_flags |= T_EOF; /* Mark an EOF for later */
	return (0);
      }
      else
#endif
	return (dev_error_to_errno(rc));
    }
    
    (void) moveout(data, iov->iov_base, count);
    /* deallocates data (eventually) */
    
    iov->iov_base += count;
    iov->iov_len -= count;
    uio->uio_resid -= count;
    uio->uio_offset += count;
#ifndef DEVICE_EOF
    /* If mk supports filemarks properly, this kludge isn't required */
    /* temp kludge for tape drives */
    if (count < c)
      break;
#endif
  }
  return (0);
}

tape_write(dev_t dev, struct uio *uio, int flag)
{
	register struct iovec *iov;
	register int	c;
	register kern_return_t	rc;
	vm_offset_t	kern_addr;
	vm_size_t	kern_size;
	vm_size_t	count;
	struct tape_device *tp;

	dev = TAPE_UNIT(dev);
	tp = tape_hash_lookup(dev);
	tp->t_flags &= ~T_EOF;

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

	    rc = device_write(tp->t_device_port,
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

	    uio->uio_iov++;
	    uio->uio_iovcnt--;
	}
	return (0);
}

tape_ioctl(dev_t dev, ioctl_cmd_t cmd, caddr_t data, int flag, struct proc *p)
{
	unsigned int	count;
	register int	error = KERN_SUCCESS;
	struct tape_device *tp;

	dev = TAPE_UNIT(dev);
	tp = tape_hash_lookup(dev);
	tp->t_flags &= ~T_EOF;


	count = (cmd & ~(IOC_INOUT|IOC_VOID)) >> 16; /* bytes */
	count = (count + 3) >> 2;		     /* ints */
	if (count == 0)
	    count = 1;

	if (cmd & (IOC_VOID|IOC_IN)) {
	    error = device_set_status(tp->t_device_port,
				      cmd,
				      (int *)data,
				      count);
	    if (error)
		return (dev_error_to_errno(error));
	}
	if (cmd & IOC_OUT) {
	    error = device_get_status(tp->t_device_port,
				      cmd,
				      (int *)data,
				      &count);
	}
	if (error)
	     return (dev_error_to_errno(error));
	else
	     return (0);
}
