#ifndef	_memory_object_default_user_
#define	_memory_object_default_user_

/* Module memory_object_default */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>

/* SimpleRoutine memory_object_create */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_create
#if	defined(LINTLIBRARY)
    (old_memory_object, new_memory_object, new_object_size, new_control_port, new_name, new_page_size)
	mach_port_t old_memory_object;
	mach_port_t new_memory_object;
	vm_size_t new_object_size;
	mach_port_t new_control_port;
	mach_port_t new_name;
	vm_size_t new_page_size;
{ return memory_object_create(old_memory_object, new_memory_object, new_object_size, new_control_port, new_name, new_page_size); }
#else
(
	mach_port_t old_memory_object,
	mach_port_t new_memory_object,
	vm_size_t new_object_size,
	mach_port_t new_control_port,
	mach_port_t new_name,
	vm_size_t new_page_size
);
#endif

/* SimpleRoutine memory_object_data_initialize */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_data_initialize
#if	defined(LINTLIBRARY)
    (memory_object, memory_control_port, offset, data, dataCnt)
	mach_port_t memory_object;
	mach_port_t memory_control_port;
	vm_offset_t offset;
	vm_offset_t data;
	mach_msg_type_number_t dataCnt;
{ return memory_object_data_initialize(memory_object, memory_control_port, offset, data, dataCnt); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control_port,
	vm_offset_t offset,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt
);
#endif

#endif	/* not defined(_memory_object_default_user_) */
