#ifndef	_default_pager_user_
#define	_default_pager_user_

/* Module default_pager */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/default_pager_types.h>

/* Routine default_pager_object_create */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t default_pager_object_create
#if	defined(LINTLIBRARY)
    (default_pager, memory_object, object_size)
	mach_port_t default_pager;
	memory_object_t *memory_object;
	vm_size_t object_size;
{ return default_pager_object_create(default_pager, memory_object, object_size); }
#else
(
	mach_port_t default_pager,
	memory_object_t *memory_object,
	vm_size_t object_size
);
#endif

/* Routine default_pager_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t default_pager_info
#if	defined(LINTLIBRARY)
    (default_pager, info)
	mach_port_t default_pager;
	default_pager_info_t *info;
{ return default_pager_info(default_pager, info); }
#else
(
	mach_port_t default_pager,
	default_pager_info_t *info
);
#endif

/* Routine default_pager_objects */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t default_pager_objects
#if	defined(LINTLIBRARY)
    (default_pager, objects, objectsCnt, ports, portsCnt)
	mach_port_t default_pager;
	default_pager_object_array_t *objects;
	mach_msg_type_number_t *objectsCnt;
	mach_port_array_t *ports;
	mach_msg_type_number_t *portsCnt;
{ return default_pager_objects(default_pager, objects, objectsCnt, ports, portsCnt); }
#else
(
	mach_port_t default_pager,
	default_pager_object_array_t *objects,
	mach_msg_type_number_t *objectsCnt,
	mach_port_array_t *ports,
	mach_msg_type_number_t *portsCnt
);
#endif

/* Routine default_pager_object_pages */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t default_pager_object_pages
#if	defined(LINTLIBRARY)
    (default_pager, memory_object, pages, pagesCnt)
	mach_port_t default_pager;
	mach_port_t memory_object;
	default_pager_page_array_t *pages;
	mach_msg_type_number_t *pagesCnt;
{ return default_pager_object_pages(default_pager, memory_object, pages, pagesCnt); }
#else
(
	mach_port_t default_pager,
	mach_port_t memory_object,
	default_pager_page_array_t *pages,
	mach_msg_type_number_t *pagesCnt
);
#endif

/* Routine default_pager_paging_file */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t default_pager_paging_file
#if	defined(LINTLIBRARY)
    (default_pager, master_device_port, filename, add)
	mach_port_t default_pager;
	mach_port_t master_device_port;
	default_pager_filename_t filename;
	boolean_t add;
{ return default_pager_paging_file(default_pager, master_device_port, filename, add); }
#else
(
	mach_port_t default_pager,
	mach_port_t master_device_port,
	default_pager_filename_t filename,
	boolean_t add
);
#endif

/* Routine default_pager_register_fileserver */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t default_pager_register_fileserver
#if	defined(LINTLIBRARY)
    (default_pager, fileserver_port)
	mach_port_t default_pager;
	mach_port_t fileserver_port;
{ return default_pager_register_fileserver(default_pager, fileserver_port); }
#else
(
	mach_port_t default_pager,
	mach_port_t fileserver_port
);
#endif

#endif	/* not defined(_default_pager_user_) */
