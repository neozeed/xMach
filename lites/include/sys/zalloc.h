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
 * 15-Jan-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Ansified prototypes.
 *
 * $Log: zalloc.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:36  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.1  92/04/21  17:15:27  rwd
 * BSDSS
 * 
 *
 */

#ifndef	_ZALLOC_
#define	_ZALLOC_

#include <serv/import_mach.h>

#include <sys/macro_help.h>

/*
 *	A zone is a collection of fixed size blocks for which there
 *	is fast allocation/deallocation access.  Kernel routines can
 *	use zones to manage data structures dynamically, creating a zone
 *	for each type of data structure to be managed.
 *
 */

typedef struct zone {
	struct mutex	lock;		/* generic lock */
	int		count;		/* Number of elements used now */
	vm_offset_t	free_elements;
	vm_size_t	cur_size;	/* current memory utilization */
	vm_size_t	max_size;	/* how large can this zone grow */
	vm_size_t	elem_size;	/* size of an element */
	vm_size_t	alloc_size;	/* size used for more memory */
	boolean_t	doing_alloc;	/* is zone expanding now? */
	char		*zone_name;	/* a name for the zone */
	unsigned int
	/* boolean_t */	pageable :1,	/* zone pageable? */
	/* boolean_t */	sleepable :1,	/* sleep if empty? */
	/* boolean_t */ exhaustible :1;	/* merely return if empty? */

	struct zone	*next_zone;	/* link for all-zones list */
} *zone_t;

#define		ZONE_NULL	((zone_t) 0)

vm_offset_t	zalloc(zone_t zone);
vm_offset_t	zget(zone_t zone);
zone_t		zinit(vm_size_t size, vm_size_t max, vm_size_t alloc, 
		      boolean_t pageable, char *name);
void		zfree(zone_t zone, vm_offset_t elem);
void		zchange(zone_t zone, boolean_t pageable, boolean_t sleepable,
			boolean_t exhaustible);

#define ADD_TO_ZONE(zone, element) \
	MACRO_BEGIN							\
		*((vm_offset_t *)(element)) = (zone)->free_elements;	\
		(zone)->free_elements = (vm_offset_t) (element);	\
		(zone)->count--;					\
	MACRO_END

#define REMOVE_FROM_ZONE(zone, ret, type)				\
	MACRO_BEGIN							\
	(ret) = (type) (zone)->free_elements;				\
	if ((ret) != (type) 0) {					\
		(zone)->count++;					\
		(zone)->free_elements = *((vm_offset_t *)(ret));	\
	}								\
	MACRO_END

#define ZFREE(zone, element)		\
	MACRO_BEGIN			\
	register zone_t	z = (zone);	\
					\
	mutex_lock(&z->lock);		\
	ADD_TO_ZONE(z, element);	\
	mutex_unlock(&z->lock);		\
	MACRO_END

#define	ZALLOC(zone, ret, type)			\
	MACRO_BEGIN				\
	register zone_t	z = (zone);		\
						\
	mutex_lock(&z->lock);			\
	REMOVE_FROM_ZONE(zone, ret, type);	\
	mutex_unlock(&z->lock);			\
	if ((ret) == (type)0)			\
		(ret) = (type)zalloc(z);	\
	MACRO_END

#define	ZGET(zone, ret, type)			\
	MACRO_BEGIN				\
	register zone_t	z = (zone);		\
						\
	mutex_lock(&z->lock);			\
	REMOVE_FROM_ZONE(zone, ret, type);	\
	mutex_unlock(&z->lock);			\
	MACRO_END

void		zcram(zone_t zone, vm_offset_t newmem, vm_size_t size);
void		zone_init(void);

#endif	_ZALLOC_
