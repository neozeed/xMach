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
 * $Log: block_io.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/08/29  01:07:23  gback
 * gross hack to ensure ioctl()'s on block devices work
 *
 * Revision 1.1.1.2  1995/03/23  01:16:52  law
 * lites-950323 from jvh.
 *
 * Revision 2.4  93/05/11  12:02:50  rvb
 * 	From UX: Do not fragment vm map with mbufs unless requested
 * 	to do so.
 * 	[93/05/03  12:58:09  rvb]
 * 
 * Revision 2.3  93/03/12  10:54:53  rwd
 * 	Keep allocbufspace and freebufspace up to date.
 * 	[93/03/04            rwd]
 * 
 * Revision 2.2  93/02/26  12:55:40  rwd
 * 	Include sys/systm.h for printf prototypes.
 * 	[92/12/09            rwd]
 * 
 * Revision 2.1  92/04/21  17:11:18  rwd
 * BSDSS
 * 
 *
 */

/*
 * Block IO using MACH KERNEL interface.
 */

#include "osfmach3.h"

#include <serv/server_defs.h>

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/synch.h>
#include <sys/ucred.h>
#include <sys/systm.h>
#include <sys/assert.h>
#include <sys/conf.h>

#include <serv/import_mach.h>
#include <serv/device_utils.h>

#include <device/device.h>

/*
 * We store the block-io port in the hash table
 */
#define	bio_port_enter(dev, port) \
		dev_number_hash_enter(XDEV_BLOCK(dev), (char *)(port))
#define	bio_port_remove(dev)	\
		dev_number_hash_remove(XDEV_BLOCK(dev))
#define	bio_port_lookup(dev)	\
		((mach_port_t)dev_number_hash_lookup(XDEV_BLOCK(dev)))

kern_return_t	bio_read_reply(char *, kern_return_t, char *,
			       mach_msg_type_number_t);
kern_return_t	bio_write_reply(char *,	kern_return_t, int);
dev_t blktochr (dev_t);

extern int freebufspace;
extern int allocbufspace;

#if OSFMACH3
extern security_token_t security_id;
#endif

int debug_bio = 0;

/*
 * Open block device.
 */
int bdev_open(
	dev_t	dev,
	int	flag)
{
#if OSFMACH3
	dev_name_t	name;
#else
	char		name[32];
#endif
	kern_return_t	rc;
	mach_port_t	device_port;
	int		mode;

	/*
	 * See whether we have opened the device already.
	 */
	if (bio_port_lookup(dev)) {
	    return (0);
	}

	rc = bdev_name_string(dev, name);
	if (rc != 0)
	    return (rc);

#if OSFMACH3
	mode = D_READ|D_WRITE;
#else
	mode = 0;		/* XXX */
#endif
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

	bio_port_enter(dev, device_port);
	return (0);
}

int bdev_close(
	dev_t	dev,
	int	flag)
{
	mach_port_t	device_port;
	int		error;

	device_port = (mach_port_t)bio_port_lookup(dev);
	if (device_port == MACH_PORT_NULL)
	    return (0);	/* shouldn't happen */

	bio_port_remove(dev);
	error = dev_error_to_errno(device_close(device_port));
	(void) mach_port_deallocate(mach_task_self(), device_port);
	return (error);
}

int bdev_dump()
{
	printf("bdev_dump()----------\n"); return(0);
}

int bdev_size()
{
	printf("bdev_size()----------\n"); return(0);
}

int bdev_ioctl(
	dev_t dev,
	ioctl_cmd_t cmd,
	caddr_t arg,
	int mode,
	struct proc *p)
{
	dev_t cdev = blktochr(dev);
	/* 
	 * this is a hack so that the ioctl will find the correct
	 * device port in the hash chain - if anybody asks, I didn't do it 
	 */
	if (cdev != NODEV)
	  return cdevsw[major(cdev)].d_ioctl(XDEV_BLOCK(dev), 
				cmd, arg, mode, p);
	
	printf("bdev_ioctl(dev = %x, cmd = %x, arg = %x, mode = %d)---------\n",
			dev, cmd, arg, mode);
	return EIO;
}

int bio_strategy(
	struct buf *	bp)
{
	mach_port_t	device_port;
	kern_return_t	error;
	mach_msg_type_number_t	count;

	assert((bp->b_flags & (B_INVAL|B_BAD|B_ERROR|B_DONE)) == 0);
	assert((bp->b_flags & B_BUSY) == B_BUSY);
	/*
	 * Find the request port for the device.
	 */
	device_port = bio_port_lookup(bp->b_dev);
	if (device_port == MACH_PORT_NULL)
		panic("bio_strategy null port");

	/*
	 * Start the IO.  XXX What should we do in case of error???
	 * Calling bio_read_reply/bio_write_reply isn't correct,
	 * because they use interrupt_enter/interrupt_exit.
	 */

/* XXX There must be a more general way of doing the rounding */
#define round_sector(x) ((x + 511U) & ~511U)

	count = round_sector(bp->b_bcount);
	if (bp->b_flags & B_READ) {
	    error = device_read_request(device_port,
					bp->b_reply_port,
					0,
					bp->b_blkno,
					count);
	    if (error != KERN_SUCCESS)
		panic("bio_strategy read request", error);
	} else {
	    /* as there is no write flag, check that it anyway IS a write */
	    assert(bp->b_flags & (B_DIRTY|B_DELWRI|B_WRITEINPROG));
	    assert(bp->b_bcount <= MAXBSIZE);
	    error = device_write_request(device_port,
					 bp->b_reply_port,
					 0,
					 bp->b_blkno,
					 bp->b_un.b_addr,
					 count);
	    if (error != KERN_SUCCESS)
		panic("bio_strategy write request",error);
	}
}

int donotfragmentbuf = 1;

kern_return_t
bio_read_reply(
	char *			bp_ptr,
	kern_return_t		return_code,
	char			*data,
	mach_msg_type_number_t	data_count)
{
	register struct buf *bp = (struct buf *)bp_ptr;
	vm_offset_t dealloc_addr = 0;
	vm_size_t dealloc_size = 0;
	int s;
	kern_return_t kr;

	interrupt_enter(SPLBIO);
	if (return_code != D_SUCCESS) {
	    bp->b_flags |= B_ERROR;
	    bp->b_error = EIO;
	} else {
	    /*
	     * Deallocate old memory.  Actually do it later,
	     * after we have lowered IPL.
	     */
	    if (bp->b_bufsize > 0) {
		dealloc_addr = (vm_offset_t) bp->b_un.b_addr;
		dealloc_size = (vm_size_t) bp->b_bufsize;	
		if (!donotfragmentbuf) bp->b_bufsize = 0;
	    }

	    if (data_count < bp->b_bcount) {
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount - data_count;
	    }
	    if (donotfragmentbuf && (bp->b_bufsize > 0)) {
	        bcopy(data, bp->b_un.b_addr, data_count);
		dealloc_addr = (vm_offset_t) data;
		dealloc_size = (vm_size_t) round_page(data_count);
	    } else {
		bp->b_un.b_addr = data;
		bp->b_bufsize = round_page(data_count);
#if 0
		freebufspace += (dealloc_size - bp->b_bufsize);
		allocbufspace -= (dealloc_size - bp->b_bufsize);
#endif
	    }
	}

	assert((bp->b_flags & (B_DIRTY|B_INVAL|B_DELWRI|B_BUSY)) == B_BUSY);

	biodone(bp);
	interrupt_exit(SPLBIO);

	if (return_code == D_SUCCESS & debug_bio)
	    printf("r  allocate bp: %x addr: %x size: %x\n",
		   bp, data, bp->b_bufsize);
	if (dealloc_size != 0) {
		kr = vm_deallocate(mach_task_self(),
				   dealloc_addr, dealloc_size);
		assert(kr == KERN_SUCCESS);
		if (debug_bio)
		    printf("rdeallocate bp: %x addr: %x size: %x\n",
			   bp, dealloc_addr, dealloc_size);
	}
}

kern_return_t
bio_write_reply(
	char *		bp_ptr,
	kern_return_t	return_code,
	int		bytes_written)
{
	register struct buf *bp = (struct buf *)bp_ptr;

	interrupt_enter(SPLBIO);
	if (return_code != D_SUCCESS) {
	    bp->b_flags |= B_ERROR;
	    bp->b_error = EIO;
	} else if (bytes_written < bp->b_bcount) {
	    bp->b_flags |= B_ERROR;
	    bp->b_resid = bp->b_bcount - bytes_written;
	}
	biodone(bp);
	interrupt_exit(SPLBIO);
}
