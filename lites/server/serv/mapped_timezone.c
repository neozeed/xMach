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
 * $Log: mapped_timezone.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	serv/mapped_timezone.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	June 1994
 *
 *	Time offset and timezone management and exporting.
 */

#include <serv/server_defs.h>
#include <sys/time.h>

#ifndef TIMEZONE
#define TIMEZONE -120
#endif
#ifndef DST
#define DST DST_EET
#endif

mach_port_t mapped_timezone_port;
mapped_timezone_t mapped_timezone_memory;

void init_mapped_timezone()
{
	vm_address_t addr;
	kern_return_t kr;

	kr = default_pager_object_create(default_pager_port,
					 &mapped_timezone_port,
					 vm_page_size);

	if (kr != KERN_SUCCESS)
	    panic("init_mapped_timezone: object create");

	kr = vm_map(mach_task_self(),
		    &addr, vm_page_size,
		    0, TRUE, mapped_timezone_port, 0, FALSE,
		    VM_PROT_READ|VM_PROT_WRITE, VM_PROT_ALL,
		    VM_INHERIT_NONE);
	if (kr != KERN_SUCCESS)
	    panic("init_mapped_timezone: vm_map");
	mapped_timezone_memory = (mapped_timezone_t) addr;

	mapped_timezone_memory->version = 0;
	mapped_timezone_memory->tz.tz_minuteswest = TIMEZONE;
	mapped_timezone_memory->tz.tz_dsttime = DST;
	mapped_timezone_memory->offset.tv_sec = 0;
	mapped_timezone_memory->offset.tv_usec = 0;
}

unsigned int maptz_user_count = 0;

int maptz_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	/* Nothing real to be done here */
	maptz_user_count++;
	return KERN_SUCCESS;
}

int maptz_close(dev_t dev, int flag, int mode, struct proc *p)
{
	/* Nothing real to be done here */
	maptz_user_count--;
	return KERN_SUCCESS;
}

kern_return_t maptz_map(
	mach_port_t	device,
	vm_prot_t	prot,
	vm_offset_t	offset,
	vm_size_t	size,
	mach_port_t	*pager,
	int		unmap)
{
	if (!MACH_PORT_VALID(device))
	    return EINVAL;
	*pager = mapped_timezone_port;
	return KERN_SUCCESS;
}

mach_port_t maptz_port(dev_t dev)
{
	return mapped_timezone_port;
}
