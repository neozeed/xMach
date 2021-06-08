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
 * $Log: e_mapped_time.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:28  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_mapped_time.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	June 1994
 *
 * 	gettimeofday implementation using mapped time.
 *
 * 	The initial setup is more expensive but reading the time is
 *	cheap.
 */

#include <e_defs.h>
#include <sys/time.h>

volatile struct mapped_time_value *e_mapped_time_memory = 0;
volatile mapped_timezone_t e_mapped_timezone_memory = 0;
boolean_t mapped_time_init_has_failed = FALSE;

/* 
 * Note: VM_INHERIT_NONE is used here instead of VM_INHERIT_SHARE to
 * circumvent a MK83 i386 pmap bug.  After a fork the mapping must be
 * reinitialized.
 */
void child_reinit_mapped_time()
{
	/* Delay actual mapping until it is needed. */
	e_mapped_time_memory = 0;
	e_mapped_timezone_memory = 0;
	mapped_time_init_has_failed = FALSE;
}

/* 
 * Map a page of kernel memory with a clock
 * and a page of server memory with timezone and time offset.
 */
errno_t e_init_mapped_time()
{
	kern_return_t kr;
	errno_t err;
	mach_port_t port;
	int fd;
	vm_address_t addr, time_addr;

	/* Use bsd_file_port_open when implemented */
	err = e_open("/dev/time", O_RDONLY, 0, &fd);
	if (err)
	    return err;
	kr = bsd_fd_to_file_port(process_self(), fd, &port);
	if (kr)
	    return e_mach_error_to_errno(kr);
	/* XXX failure after this should destroy the port */
	err = e_close(fd);
	if (err)
	    return err;
	addr = EMULATOR_BASE;
	kr = bsd_vm_map(process_self(), &addr, vm_page_size, 0, TRUE,
			port, 0, FALSE, VM_PROT_READ, VM_PROT_READ,
			VM_INHERIT_NONE);
	if (kr)
	    return e_mach_error_to_errno(kr);
	time_addr = addr;

	kr = mach_port_deallocate(mach_task_self(), port);

	/* Same for timezone and time offset */
	err = e_open("/dev/timezone", O_RDONLY, 0, &fd);
	if (err)
	    return err;
	kr = bsd_fd_to_file_port(process_self(), fd, &port);
	if (kr)
	    return e_mach_error_to_errno(kr);
	/* XXX failure after this should destroy the port */
	err = e_close(fd);
	if (err)
	    return err;
	addr = EMULATOR_BASE;
	kr = bsd_vm_map(process_self(), &addr, vm_page_size, 0, TRUE,
			port, 0, FALSE, VM_PROT_READ, VM_PROT_READ,
			VM_INHERIT_NONE);
	if (kr)
	    return e_mach_error_to_errno(kr);

	kr = mach_port_deallocate(mach_task_self(), port);
	if (kr)
	    return e_mach_error_to_errno(kr);

	/* XXX Check timezone version number */

	e_mapped_time_memory = (volatile struct mapped_time_value *) time_addr;
	e_mapped_timezone_memory = (volatile mapped_timezone_t) addr;

	return ESUCCESS;
}

errno_t e_mapped_gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	errno_t err;
	kern_return_t kr;

	if (e_mapped_time_memory == 0) {
		if (mapped_time_init_has_failed)
		    return e_gettimeofday(tp, tzp);
		err = e_init_mapped_time();
		/*
		 * Failed because there is a pending signal, return and
		 * process that signal.  After that we will try this again.
		 */
		if (err == ERESTART)
			return err;
		if (err) {
			e_emulator_error("e_mapped_timeofday init failed %d",
					 err);
			mapped_time_init_has_failed = TRUE;
			return e_gettimeofday(tp, tzp);
		}
	}

	/* Read the kernel time (potentially a free running counter) */
	do {
		tp->tv_sec = e_mapped_time_memory->seconds;
		tp->tv_usec = e_mapped_time_memory->microseconds;
	} while (tp->tv_sec != e_mapped_time_memory->check_seconds);

	/* Add the system clock offset to the kernel time */
	tp->tv_sec += e_mapped_timezone_memory->offset.tv_sec;
	tp->tv_usec += e_mapped_timezone_memory->offset.tv_usec;
	if (tp->tv_usec < 0) {
		tp->tv_usec += 1000000;
		tp->tv_sec--;
	} else if (tp->tv_usec > 1000000) {
		tp->tv_usec -= 1000000;
		tp->tv_sec++;
	}

	/* Get the default timezone if requested */
	if (tzp)
	    *tzp = e_mapped_timezone_memory->tz;

	return ESUCCESS;
}
