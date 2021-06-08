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
 * $Log: device_utils.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/03/23  01:44:04  law
 * Update to 950323 snapshot + utah changes
 *
 * Revision 1.1.1.1  1995/03/02  21:49:47  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.1  92/04/21  17:10:54  rwd
 * BSDSS
 * 
 *
 */

/*
 * Support routines for device interface in out-of-kernel kernel.
 */

#include <serv/device_utils.h>
#include <sys/errno.h>

#include <sys/cmu_queue.h>
#include <sys/zalloc.h>

#include <serv/import_mach.h>


/*
 * device_number to pointer hash table.
 */
#define	NDEVHASH	8
#define	DEV_NUMBER_HASH(dev)	\
		((major(dev) ^ minor(dev)) & (NDEVHASH-1))

struct dev_number_hash {
	struct mutex lock;
	queue_head_t queue;
	int count;
} dev_number_hash_table[NDEVHASH];

struct dev_entry {
	queue_chain_t	chain;
	void		*object;	/* anything */
	xdev_t		dev;
};

zone_t	dev_entry_zone;

void dev_utils_init()
{
	int	i;

	for (i = 0; i < NDEVHASH; i++) {
	    mutex_init(&dev_number_hash_table[i].lock);
	    queue_init(&dev_number_hash_table[i].queue);
	    dev_number_hash_table[i].count = 0;
	}

	dev_entry_zone =
		zinit((vm_size_t)sizeof(struct dev_entry),
		      (vm_size_t)sizeof(struct dev_entry) * 4096,
		      vm_page_size,
		      TRUE,
		      "device to device_request port");
}

/*
 * Enter a device in the devnumber hash table.
 */
void dev_number_hash_enter(xdev_t dev, void *object)
{
	struct dev_entry *de;
	struct dev_number_hash *b;

	de = (struct dev_entry *)zalloc(dev_entry_zone);
	de->dev = dev;
	de->object = object;

	b = &dev_number_hash_table[DEV_NUMBER_HASH(dev)];
	mutex_lock(&b->lock);
	enqueue_tail(&b->queue, (queue_entry_t)de);
	b->count++;
	mutex_unlock(&b->lock);
}

/*
 * Remove a device from the devnumber hash table.
 */
void dev_number_hash_remove(xdev_t dev)
{
	struct dev_entry *de;
	struct dev_number_hash *b;

	b = &dev_number_hash_table[DEV_NUMBER_HASH(dev)];

	mutex_lock(&b->lock);

	for (de = (struct dev_entry *)queue_first(&b->queue);
	     !queue_end(&b->queue, (queue_entry_t)de);
	     de = (struct dev_entry *)queue_next(&de->chain)) {
	    if (de->dev == dev) {
		queue_remove(&b->queue, de, struct dev_entry *, chain);
		b->count--;
		zfree(dev_entry_zone, (vm_offset_t)de);
		break;
	    }
	}

	mutex_unlock(&b->lock);
}

/*
 * Map a device to an object.
 */
char *
dev_number_hash_lookup(xdev_t dev)
{
	struct dev_entry *de;
	struct dev_number_hash *b;
	char *	object = 0;

	b = &dev_number_hash_table[DEV_NUMBER_HASH(dev)];
	mutex_lock(&b->lock);

	for (de = (struct dev_entry *)queue_first(&b->queue);
	     !queue_end(&b->queue, (queue_entry_t)de);
	     de = (struct dev_entry *)queue_next(&de->chain)) {
	    if (de->dev == dev) {
		object = de->object;
		break;
	    }
	}

	mutex_unlock(&b->lock);
	return (object);
}

/*
 * Map kernel device error codes to BSD error numbers.
 */
int
dev_error_to_errno(err)
	int	err;
{
	/* Unnecessary: the emulator does the mapping */
	return err;

	switch (err) {
	    case D_SUCCESS:
		return (0);

	    case D_IO_ERROR:
		return (EIO);

	    case D_WOULD_BLOCK:
		return (EWOULDBLOCK);

	    case D_NO_SUCH_DEVICE:
		return (ENXIO);

	    case D_ALREADY_OPEN:
		return (EBUSY);

	    case D_DEVICE_DOWN:
		return (ENETDOWN);

	    case D_INVALID_OPERATION:
		return (ENOTTY);    /* weird, but ioctl() callers expect it */

	    case D_INVALID_RECNUM:
	    case D_INVALID_SIZE:
		return (EINVAL);

#ifdef D_NO_SPACE
	      case D_NO_SPACE:
		return (ENOSPC);
#endif

	    default:
		return (EIO);
	}
	/*NOTREACHED*/
}
