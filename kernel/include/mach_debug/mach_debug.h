#ifndef	_mach_debug_user_
#define	_mach_debug_user_

/* Module mach_debug */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach_debug/mach_debug_types.h>

/* Routine host_zone_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t host_zone_info
#if	defined(LINTLIBRARY)
    (host, names, namesCnt, info, infoCnt)
	mach_port_t host;
	zone_name_array_t *names;
	mach_msg_type_number_t *namesCnt;
	zone_info_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return host_zone_info(host, names, namesCnt, info, infoCnt); }
#else
(
	mach_port_t host,
	zone_name_array_t *names,
	mach_msg_type_number_t *namesCnt,
	zone_info_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif

/* Routine mach_port_get_srights */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_get_srights
#if	defined(LINTLIBRARY)
    (task, name, srights)
	mach_port_t task;
	mach_port_t name;
	mach_port_rights_t *srights;
{ return mach_port_get_srights(task, name, srights); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_rights_t *srights
);
#endif

/* Routine host_ipc_hash_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t host_ipc_hash_info
#if	defined(LINTLIBRARY)
    (host, info, infoCnt)
	mach_port_t host;
	hash_info_bucket_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return host_ipc_hash_info(host, info, infoCnt); }
#else
(
	mach_port_t host,
	hash_info_bucket_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif

/* Routine host_ipc_marequest_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t host_ipc_marequest_info
#if	defined(LINTLIBRARY)
    (host, max_requests, info, infoCnt)
	mach_port_t host;
	unsigned *max_requests;
	hash_info_bucket_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return host_ipc_marequest_info(host, max_requests, info, infoCnt); }
#else
(
	mach_port_t host,
	unsigned *max_requests,
	hash_info_bucket_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif

/* Routine mach_port_space_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_space_info
#if	defined(LINTLIBRARY)
    (task, info, table_info, table_infoCnt, tree_info, tree_infoCnt)
	mach_port_t task;
	ipc_info_space_t *info;
	ipc_info_name_array_t *table_info;
	mach_msg_type_number_t *table_infoCnt;
	ipc_info_tree_name_array_t *tree_info;
	mach_msg_type_number_t *tree_infoCnt;
{ return mach_port_space_info(task, info, table_info, table_infoCnt, tree_info, tree_infoCnt); }
#else
(
	mach_port_t task,
	ipc_info_space_t *info,
	ipc_info_name_array_t *table_info,
	mach_msg_type_number_t *table_infoCnt,
	ipc_info_tree_name_array_t *tree_info,
	mach_msg_type_number_t *tree_infoCnt
);
#endif

/* Routine mach_port_dnrequest_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_dnrequest_info
#if	defined(LINTLIBRARY)
    (task, name, total, used)
	mach_port_t task;
	mach_port_t name;
	unsigned *total;
	unsigned *used;
{ return mach_port_dnrequest_info(task, name, total, used); }
#else
(
	mach_port_t task,
	mach_port_t name,
	unsigned *total,
	unsigned *used
);
#endif

/* Routine host_stack_usage */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t host_stack_usage
#if	defined(LINTLIBRARY)
    (host, reserved, total, space, resident, maxusage, maxstack)
	mach_port_t host;
	vm_size_t *reserved;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return host_stack_usage(host, reserved, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t host,
	vm_size_t *reserved,
	unsigned *total,
	vm_size_t *space,
	vm_size_t *resident,
	vm_size_t *maxusage,
	vm_offset_t *maxstack
);
#endif

/* Routine processor_set_stack_usage */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t processor_set_stack_usage
#if	defined(LINTLIBRARY)
    (pset, total, space, resident, maxusage, maxstack)
	mach_port_t pset;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return processor_set_stack_usage(pset, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t pset,
	unsigned *total,
	vm_size_t *space,
	vm_size_t *resident,
	vm_size_t *maxusage,
	vm_offset_t *maxstack
);
#endif

/* Routine host_virtual_physical_table_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t host_virtual_physical_table_info
#if	defined(LINTLIBRARY)
    (host, info, infoCnt)
	mach_port_t host;
	hash_info_bucket_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return host_virtual_physical_table_info(host, info, infoCnt); }
#else
(
	mach_port_t host,
	hash_info_bucket_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif

/* Routine host_load_symbol_table */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t host_load_symbol_table
#if	defined(LINTLIBRARY)
    (host, task, name, symtab, symtabCnt)
	mach_port_t host;
	mach_port_t task;
	symtab_name_t name;
	vm_offset_t symtab;
	mach_msg_type_number_t symtabCnt;
{ return host_load_symbol_table(host, task, name, symtab, symtabCnt); }
#else
(
	mach_port_t host,
	mach_port_t task,
	symtab_name_t name,
	vm_offset_t symtab,
	mach_msg_type_number_t symtabCnt
);
#endif

/* Routine mach_port_kernel_object */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_kernel_object
#if	defined(LINTLIBRARY)
    (task, name, object_type, object_addr)
	mach_port_t task;
	mach_port_t name;
	unsigned *object_type;
	vm_offset_t *object_addr;
{ return mach_port_kernel_object(task, name, object_type, object_addr); }
#else
(
	mach_port_t task,
	mach_port_t name,
	unsigned *object_type,
	vm_offset_t *object_addr
);
#endif

/* Routine mach_vm_region_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_vm_region_info
#if	defined(LINTLIBRARY)
    (task, address, region, object)
	mach_port_t task;
	vm_address_t address;
	vm_region_info_t *region;
	mach_port_t *object;
{ return mach_vm_region_info(task, address, region, object); }
#else
(
	mach_port_t task,
	vm_address_t address,
	vm_region_info_t *region,
	mach_port_t *object
);
#endif

/* Routine mach_vm_object_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_vm_object_info
#if	defined(LINTLIBRARY)
    (object, info, shadow, copy)
	mach_port_t object;
	vm_object_info_t *info;
	mach_port_t *shadow;
	mach_port_t *copy;
{ return mach_vm_object_info(object, info, shadow, copy); }
#else
(
	mach_port_t object,
	vm_object_info_t *info,
	mach_port_t *shadow,
	mach_port_t *copy
);
#endif

/* Routine mach_vm_object_pages */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_vm_object_pages
#if	defined(LINTLIBRARY)
    (object, pages, pagesCnt)
	mach_port_t object;
	vm_page_info_array_t *pages;
	mach_msg_type_number_t *pagesCnt;
{ return mach_vm_object_pages(object, pages, pagesCnt); }
#else
(
	mach_port_t object,
	vm_page_info_array_t *pages,
	mach_msg_type_number_t *pagesCnt
);
#endif

#endif	/* not defined(_mach_debug_user_) */
