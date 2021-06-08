#ifndef	_mach_i386_user_
#define	_mach_i386_user_

/* Module mach_i386 */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <device/device_types.h>
#include <device/net_status.h>
#include <mach/machine/mach_i386_types.h>

/* Routine i386_io_port_add */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t i386_io_port_add
#if	defined(LINTLIBRARY)
    (target_thread, device)
	mach_port_t target_thread;
	mach_port_t device;
{ return i386_io_port_add(target_thread, device); }
#else
(
	mach_port_t target_thread,
	mach_port_t device
);
#endif

/* Routine i386_io_port_remove */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t i386_io_port_remove
#if	defined(LINTLIBRARY)
    (target_thread, device)
	mach_port_t target_thread;
	mach_port_t device;
{ return i386_io_port_remove(target_thread, device); }
#else
(
	mach_port_t target_thread,
	mach_port_t device
);
#endif

/* Routine i386_io_port_list */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t i386_io_port_list
#if	defined(LINTLIBRARY)
    (target_thread, device_list, device_listCnt)
	mach_port_t target_thread;
	device_list_t *device_list;
	mach_msg_type_number_t *device_listCnt;
{ return i386_io_port_list(target_thread, device_list, device_listCnt); }
#else
(
	mach_port_t target_thread,
	device_list_t *device_list,
	mach_msg_type_number_t *device_listCnt
);
#endif

/* Routine i386_set_ldt */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t i386_set_ldt
#if	defined(LINTLIBRARY)
    (target_thread, first_selector, desc_list, desc_listCnt)
	mach_port_t target_thread;
	int first_selector;
	descriptor_list_t desc_list;
	mach_msg_type_number_t desc_listCnt;
{ return i386_set_ldt(target_thread, first_selector, desc_list, desc_listCnt); }
#else
(
	mach_port_t target_thread,
	int first_selector,
	descriptor_list_t desc_list,
	mach_msg_type_number_t desc_listCnt
);
#endif

/* Routine i386_get_ldt */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t i386_get_ldt
#if	defined(LINTLIBRARY)
    (target_thread, first_selector, selector_count, desc_list, desc_listCnt)
	mach_port_t target_thread;
	int first_selector;
	int selector_count;
	descriptor_list_t *desc_list;
	mach_msg_type_number_t *desc_listCnt;
{ return i386_get_ldt(target_thread, first_selector, selector_count, desc_list, desc_listCnt); }
#else
(
	mach_port_t target_thread,
	int first_selector,
	int selector_count,
	descriptor_list_t *desc_list,
	mach_msg_type_number_t *desc_listCnt
);
#endif

#endif	/* not defined(_mach_i386_user_) */
