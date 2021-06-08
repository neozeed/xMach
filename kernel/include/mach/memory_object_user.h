#ifndef	_memory_object_user_
#define	_memory_object_user_

/* Module memory_object */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>

/* SimpleRoutine memory_object_init */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_init
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, memory_object_name, memory_object_page_size)
	mach_port_t memory_object;
	mach_port_t memory_control;
	mach_port_t memory_object_name;
	vm_size_t memory_object_page_size;
{ return memory_object_init(memory_object, memory_control, memory_object_name, memory_object_page_size); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	mach_port_t memory_object_name,
	vm_size_t memory_object_page_size
);
#endif

/* SimpleRoutine memory_object_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_terminate
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, memory_object_name)
	mach_port_t memory_object;
	mach_port_t memory_control;
	mach_port_t memory_object_name;
{ return memory_object_terminate(memory_object, memory_control, memory_object_name); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	mach_port_t memory_object_name
);
#endif

/* SimpleRoutine memory_object_copy */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_copy
#if	defined(LINTLIBRARY)
    (old_memory_object, old_memory_control, offset, length, new_memory_object)
	mach_port_t old_memory_object;
	mach_port_t old_memory_control;
	vm_offset_t offset;
	vm_size_t length;
	mach_port_t new_memory_object;
{ return memory_object_copy(old_memory_object, old_memory_control, offset, length, new_memory_object); }
#else
(
	mach_port_t old_memory_object,
	mach_port_t old_memory_control,
	vm_offset_t offset,
	vm_size_t length,
	mach_port_t new_memory_object
);
#endif

/* SimpleRoutine memory_object_data_request */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_data_request
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length, desired_access)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{ return memory_object_data_request(memory_object, memory_control, offset, length, desired_access); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	vm_prot_t desired_access
);
#endif

/* SimpleRoutine memory_object_data_unlock */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_data_unlock
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length, desired_access)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{ return memory_object_data_unlock(memory_object, memory_control, offset, length, desired_access); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	vm_prot_t desired_access
);
#endif

/* SimpleRoutine memory_object_data_write */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_data_write
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, data, dataCnt)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_offset_t data;
	mach_msg_type_number_t dataCnt;
{ return memory_object_data_write(memory_object, memory_control, offset, data, dataCnt); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt
);
#endif

/* SimpleRoutine memory_object_lock_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_lock_completed
#if	defined(LINTLIBRARY)
    (memory_object, memory_objectPoly, memory_control, offset, length)
	mach_port_t memory_object;
	mach_msg_type_name_t memory_objectPoly;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
{ return memory_object_lock_completed(memory_object, memory_objectPoly, memory_control, offset, length); }
#else
(
	mach_port_t memory_object,
	mach_msg_type_name_t memory_objectPoly,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length
);
#endif

/* SimpleRoutine memory_object_supply_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_supply_completed
#if	defined(LINTLIBRARY)
    (memory_object, memory_objectPoly, memory_control, offset, length, result, error_offset)
	mach_port_t memory_object;
	mach_msg_type_name_t memory_objectPoly;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{ return memory_object_supply_completed(memory_object, memory_objectPoly, memory_control, offset, length, result, error_offset); }
#else
(
	mach_port_t memory_object,
	mach_msg_type_name_t memory_objectPoly,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	kern_return_t result,
	vm_offset_t error_offset
);
#endif

/* SimpleRoutine memory_object_data_return */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_data_return
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, data, dataCnt, dirty, kernel_copy)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_offset_t data;
	mach_msg_type_number_t dataCnt;
	boolean_t dirty;
	boolean_t kernel_copy;
{ return memory_object_data_return(memory_object, memory_control, offset, data, dataCnt, dirty, kernel_copy); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt,
	boolean_t dirty,
	boolean_t kernel_copy
);
#endif

/* SimpleRoutine memory_object_change_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t memory_object_change_completed
#if	defined(LINTLIBRARY)
    (memory_object, memory_objectPoly, may_cache, copy_strategy)
	mach_port_t memory_object;
	mach_msg_type_name_t memory_objectPoly;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{ return memory_object_change_completed(memory_object, memory_objectPoly, may_cache, copy_strategy); }
#else
(
	mach_port_t memory_object,
	mach_msg_type_name_t memory_objectPoly,
	boolean_t may_cache,
	memory_object_copy_strategy_t copy_strategy
);
#endif

#endif	/* not defined(_memory_object_user_) */
