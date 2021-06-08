#ifndef	_machid_debug_user_
#define	_machid_debug_user_

/* Module machid_debug */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach_debug/mach_debug_types.h>
#include <servers/machid_types.h>

/* Routine port_get_srights */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_port_get_srights
#if	defined(LINTLIBRARY)
    (server, auth, task, name, srights)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	mach_port_rights_t *srights;
{ return machid_port_get_srights(server, auth, task, name, srights); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	mach_port_rights_t *srights
);
#endif

/* Routine port_space_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_port_space_info
#if	defined(LINTLIBRARY)
    (server, auth, task, info, table_info, table_infoCnt, tree_info, tree_infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	ipc_info_space_t *info;
	ipc_info_name_array_t *table_info;
	mach_msg_type_number_t *table_infoCnt;
	ipc_info_tree_name_array_t *tree_info;
	mach_msg_type_number_t *tree_infoCnt;
{ return machid_port_space_info(server, auth, task, info, table_info, table_infoCnt, tree_info, tree_infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	ipc_info_space_t *info,
	ipc_info_name_array_t *table_info,
	mach_msg_type_number_t *table_infoCnt,
	ipc_info_tree_name_array_t *tree_info,
	mach_msg_type_number_t *tree_infoCnt
);
#endif

/* Routine port_dnrequest_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_port_dnrequest_info
#if	defined(LINTLIBRARY)
    (server, auth, task, name, total, used)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	unsigned *total;
	unsigned *used;
{ return machid_port_dnrequest_info(server, auth, task, name, total, used); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
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
kern_return_t machid_host_stack_usage
#if	defined(LINTLIBRARY)
    (server, auth, host, reserved, total, space, resident, maxusage, maxstack)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	vm_size_t *reserved;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return machid_host_stack_usage(server, auth, host, reserved, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
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
kern_return_t machid_processor_set_stack_usage
#if	defined(LINTLIBRARY)
    (server, auth, pset, total, space, resident, maxusage, maxstack)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t pset;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return machid_processor_set_stack_usage(server, auth, pset, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t pset,
	unsigned *total,
	vm_size_t *space,
	vm_size_t *resident,
	vm_size_t *maxusage,
	vm_offset_t *maxstack
);
#endif

/* Routine host_zone_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_host_zone_info
#if	defined(LINTLIBRARY)
    (server, auth, host, names, namesCnt, info, infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	zone_name_array_t *names;
	mach_msg_type_number_t *namesCnt;
	zone_info_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return machid_host_zone_info(server, auth, host, names, namesCnt, info, infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	zone_name_array_t *names,
	mach_msg_type_number_t *namesCnt,
	zone_info_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif

/* Routine port_kernel_object */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_port_kernel_object
#if	defined(LINTLIBRARY)
    (server, auth, task, name, object_type, object_addr)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	unsigned *object_type;
	vm_offset_t *object_addr;
{ return machid_port_kernel_object(server, auth, task, name, object_type, object_addr); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	unsigned *object_type,
	vm_offset_t *object_addr
);
#endif

/* Routine mach_kernel_object */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_mach_kernel_object
#if	defined(LINTLIBRARY)
    (server, auth, id, object_type, object_addr)
	mach_port_t server;
	mach_port_t auth;
	mach_id_t id;
	mach_type_t *object_type;
	vm_offset_t *object_addr;
{ return machid_mach_kernel_object(server, auth, id, object_type, object_addr); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mach_id_t id,
	mach_type_t *object_type,
	vm_offset_t *object_addr
);
#endif

/* Routine vm_region_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_vm_region_info
#if	defined(LINTLIBRARY)
    (server, auth, task, addr, region)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	vm_offset_t addr;
	vm_region_info_t *region;
{ return machid_vm_region_info(server, auth, task, addr, region); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	vm_offset_t addr,
	vm_region_info_t *region
);
#endif

/* Routine vm_object_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_vm_object_info
#if	defined(LINTLIBRARY)
    (server, auth, object, info)
	mach_port_t server;
	mach_port_t auth;
	mobject_name_t object;
	vm_object_info_t *info;
{ return machid_vm_object_info(server, auth, object, info); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mobject_name_t object,
	vm_object_info_t *info
);
#endif

/* Routine vm_object_pages */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_vm_object_pages
#if	defined(LINTLIBRARY)
    (server, auth, object, pages, pagesCnt)
	mach_port_t server;
	mach_port_t auth;
	mobject_name_t object;
	vm_page_info_array_t *pages;
	mach_msg_type_number_t *pagesCnt;
{ return machid_vm_object_pages(server, auth, object, pages, pagesCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mobject_name_t object,
	vm_page_info_array_t *pages,
	mach_msg_type_number_t *pagesCnt
);
#endif

#endif	/* not defined(_machid_debug_user_) */
