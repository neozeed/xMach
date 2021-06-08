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
 * $Log: zalloc.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	serv/zalloc.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 */

#include <sys/zalloc.h>
#include <sys/types.h>
#include <sys/time.h>		/* systm.h expects it */
#include <sys/systm.h>
#include <serv/import_mach.h>
#include <sys/assert.h>

zone_t		zone_zone;	/* this is the zone containing other zones */

boolean_t	zone_ignore_overflow = FALSE;

vm_offset_t	zdata;
vm_size_t	zdata_size;

/*
 *	Protects first_zone, last_zone, num_zones,
 *	and the next_zone field of zones.
 */
struct mutex	all_zones_lock = MUTEX_NAMED_INITIALIZER("all_zones_lock");
zone_t		first_zone = ZONE_NULL;
zone_t		*last_zone = &first_zone;
int		num_zones = 0;

#define	lock_zone(zone)		mutex_lock(&zone->lock)

#define	unlock_zone(zone)	mutex_unlock(&zone->lock)

#define	lock_zone_init(zone)	mutex_init(&zone->lock)

/*
 *	Initialize the "zone of zones."
 */
void zone_init()
{
	vm_offset_t	p;

	zdata_size = round_page(64 * sizeof(struct zone));
	(void) vm_allocate(mach_task_self(), &zdata, zdata_size, TRUE);
#if	0
	(void) vm_pageable(mach_task_self(), zdata, zdata_size,
			   VM_PROT_READ|VM_PROT_WRITE);
#endif

	zone_zone = ZONE_NULL;
	zone_zone = zinit(sizeof(struct zone), sizeof(struct zone), 0,
					FALSE, "zones");
	p = (vm_offset_t)(zone_zone + 1);
	zcram(zone_zone, p, (zdata + zdata_size) - p);
}

/*
 *	zinit initializes a new zone.  The zone data structures themselves
 *	are stored in a zone, which is initially a static structure that
 *	is initialized by zone_init.
 */
zone_t zinit(size, max, alloc, pageable, name)
	vm_size_t	size;		/* the size of an element */
	vm_size_t	max;		/* maximum memory to use */
	vm_size_t	alloc;		/* allocation size */
	boolean_t	pageable;	/* is this zone pageable? */
	char		*name;		/* a name for the zone */
{
	register zone_t		z;

	if (zone_zone == ZONE_NULL)
		z = (zone_t) zdata;
	else if ((z = (zone_t) zalloc(zone_zone)) == ZONE_NULL)
		return(ZONE_NULL);

	/*
	 *	Round off all the parameters appropriately.
	 */

	if ((max = round_page(max)) < (alloc = round_page(alloc)))
		max = alloc;

	z->free_elements = 0;
	z->cur_size = 0;
	z->max_size = max;
	z->elem_size = size;
	z->alloc_size = alloc;
	z->pageable = pageable;
	z->zone_name = name;
	z->count = 0;
	z->doing_alloc = FALSE;
	z->exhaustible = z->sleepable = FALSE;
	lock_zone_init(z);

	/*
	 *	Add the zone to the all-zones list.
	 */

	z->next_zone = ZONE_NULL;
	mutex_lock(&all_zones_lock);
	*last_zone = z;
	last_zone = &z->next_zone;
	num_zones++;
	mutex_unlock(&all_zones_lock);

	return(z);
}

/*
 *	Cram the given memory into the specified zone.
 */
void zcram(zone, newmem, size)
	register zone_t		zone;
	vm_offset_t		newmem;
	vm_size_t		size;
{
	register vm_size_t	elem_size;

	elem_size = zone->elem_size;

	lock_zone(zone);
	zone->cur_size += size;
	while (size >= elem_size) {
		ADD_TO_ZONE(zone, newmem);
		zone->count++;	/* compensate for ADD_TO_ZONE */
		size -= elem_size;
		newmem += elem_size;
	}
	unlock_zone(zone);
}

/*
 *	zalloc returns an element from the specified zone.
 */
vm_offset_t zalloc(zone)
	register zone_t	zone;
{
	register vm_offset_t	addr;

	if (zone == ZONE_NULL)
		panic ("zalloc: null zone");

	lock_zone(zone);
	REMOVE_FROM_ZONE(zone, addr, vm_offset_t);
	while (addr == 0) {
		/*
		 *	If nothing was there, try to get more
		 */
		vm_offset_t	alloc_addr;

		if ((zone->cur_size + zone->alloc_size) > zone->max_size) {
			if (zone->exhaustible)
				break;

			if (!zone_ignore_overflow) {
				printf("zone \"%s\" empty.\n", zone->zone_name);
				panic("zalloc");
			}
		}
		unlock_zone(zone);

		if (vm_allocate(mach_task_self(),
				&alloc_addr,
				zone->alloc_size,
				TRUE)
			!= KERN_SUCCESS) {
		    if (zone->exhaustible)
			break;
		    panic("zalloc");
		}
#if	0
		if (!zone->pageable) {
		    (void) vm_pageable(mach_task_self(),
					alloc_addr,
					zone->alloc_size,
					VM_PROT_READ|VM_PROT_WRITE);
		}
#endif
		zcram(zone, alloc_addr, zone->alloc_size);

		lock_zone(zone);

		REMOVE_FROM_ZONE(zone, addr, vm_offset_t);
	}

	unlock_zone(zone);
	return(addr);
}

/*
 *	zget returns an element from the specified zone
 *	and immediately returns nothing if there is nothing there.
 *
 *	This form should be used when you can not block (like when
 *	processing an interrupt).
 */
vm_offset_t zget(zone)
	register zone_t	zone;
{
	register vm_offset_t	addr;

	if (zone == ZONE_NULL)
		panic ("zalloc: null zone");

	lock_zone(zone);
	REMOVE_FROM_ZONE(zone, addr, vm_offset_t);
	unlock_zone(zone);

	return(addr);
}

void zfree(zone, elem)
	register zone_t	zone;
	vm_offset_t	elem;
{
	lock_zone(zone);
	ADD_TO_ZONE(zone, elem);
	unlock_zone(zone);
}


void zchange(zone, pageable, sleepable, exhaustible)
	zone_t		zone;
	boolean_t	pageable;
	boolean_t	sleepable;
	boolean_t	exhaustible;
{
	zone->pageable = pageable;
	zone->sleepable = sleepable;
	zone->exhaustible = exhaustible;
}

#include <mach_debug/zone_info.h>
#include <serv/server_defs.h>

/* System call. Grabbed from kernel and massaged to fit here --jvh */
kern_return_t bsd_zone_info(
	mach_port_t		proc,
	mach_port_seqno_t	seqno,
	zone_name_array_t	*namesp,
	unsigned int		*namesCntp,
	zone_info_array_t	*infop,
	unsigned int		*infoCntp)
{
	zone_name_t	*names;
	vm_offset_t	names_addr;
	vm_size_t	names_size = 0; /*'=0' to quiet gcc warnings */
	zone_info_t	*info;
	vm_offset_t	info_addr;
	vm_size_t	info_size = 0; /*'=0' to quiet gcc warnings */
	unsigned int	max_zones, i;
	zone_t		z;
	kern_return_t	kr;
	struct proc	*p;

	/* Just increment seqno */
	p = (struct proc *) port_object_receive_lookup(proc, seqno,
						       POT_PROCESS);
	mutex_unlock(&p->p_lock);

/*	if (host == HOST_NULL)
		return KERN_INVALID_HOST;
*/
	/*
	 *	We assume that zones aren't freed once allocated.
	 *	We won't pick up any zones that are allocated later.
	 */

	mutex_lock(&all_zones_lock);
	max_zones = num_zones;
	z = first_zone;
	mutex_unlock(&all_zones_lock);

	if (max_zones <= *namesCntp) {
		/* use in-line memory */

		names = *namesp;
	} else {
		names_size = round_page(max_zones * sizeof *names);
		kr = vm_allocate(mach_task_self(), &names_addr, names_size,
				 TRUE);
		if (kr != KERN_SUCCESS)
			return kr;

		names = (zone_name_t *) names_addr;
	}

	if (max_zones <= *infoCntp) {
		/* use in-line memory */

		info = *infop;
	} else {
		info_size = round_page(max_zones * sizeof *info);
		kr = vm_allocate(mach_task_self(), &info_addr, info_size,
				 TRUE);
		if (kr != KERN_SUCCESS) {
			if (names != *namesp)
				vm_deallocate(mach_task_self(),
					      names_addr, names_size);
			return kr;
		}

		info = (zone_info_t *) info_addr;
	}

	for (i = 0; i < max_zones; i++) {
		zone_name_t *zn = &names[i];
		zone_info_t *zi = &info[i];
		struct zone zcopy;

		assert(z != ZONE_NULL);

		lock_zone(z);
		zcopy = *z;
		unlock_zone(z);

		mutex_lock(&all_zones_lock);
		z = z->next_zone;
		mutex_unlock(&all_zones_lock);

		/* assuming here the name data is static */
		(void) strncpy(zn->zn_name, zcopy.zone_name,
			       sizeof zn->zn_name);

		zi->zi_count = zcopy.count;
		zi->zi_cur_size = zcopy.cur_size;
		zi->zi_max_size = zcopy.max_size;
		zi->zi_elem_size = zcopy.elem_size;
		zi->zi_alloc_size = zcopy.alloc_size;
		zi->zi_pageable = zcopy.pageable;
		zi->zi_sleepable = zcopy.sleepable;
		zi->zi_exhaustible = zcopy.exhaustible;
#if 0
		zi->zi_collectable = zcopy.collectable;
#else
		zi->zi_collectable = FALSE;
#endif
	}

#if 0
	if (names != *namesp) {
		vm_size_t used;
		vm_map_copy_t copy;

		used = max_zones * sizeof *names;

		if (used != names_size)
			bzero((char *) (names_addr + used), names_size - used);

		kr = vm_map_copyin(ipc_kernel_map, names_addr, names_size,
				   TRUE, &copy);
		assert(kr == KERN_SUCCESS);

		*namesp = (zone_name_t *) copy;
	}
#endif
	*namesCntp = max_zones;

#if 0
	if (info != *infop) {
		vm_size_t used;
		vm_map_copy_t copy;

		used = max_zones * sizeof *info;

		if (used != info_size)
			bzero((char *) (info_addr + used), info_size - used);

		kr = vm_map_copyin(ipc_kernel_map, info_addr, info_size,
				   TRUE, &copy);
		assert(kr == KERN_SUCCESS);

		*infop = (zone_info_t *) copy;
	}
#endif
	*infoCntp = max_zones;

	return KERN_SUCCESS;
}

void all_zones_sanity_check()
{
	unsigned int	max_zones, i, j;
	zone_t		z, z2;
	vm_offset_t	f;
	kern_return_t	kr;
	struct proc	*p;

	mutex_lock(&all_zones_lock);
	max_zones = num_zones;
	z = first_zone;

	for (i = 0; i < max_zones; i++) {
		lock_zone(z);

		assert(z != ZONE_NULL);
		assert(strlen(z->zone_name) < sizeof(*z));

		for (f = z->free_elements, j = 0; f; j++) {
			f = *(vm_offset_t *)f;
		}
		assert((j + z->count) * z->elem_size <= z->cur_size);
		assert(z->elem_size >= sizeof(vm_offset_t));
		assert(z->alloc_size >= 0);

		z2 = z->next_zone;
		unlock_zone(z);
		z = z2;
	}
	mutex_unlock(&all_zones_lock);
}
