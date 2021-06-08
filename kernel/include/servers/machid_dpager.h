#ifndef	_machid_dpager_user_
#define	_machid_dpager_user_

/* Module machid_dpager */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/default_pager_types.h>
#include <servers/machid_types.h>

/* Routine default_pager_info */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_default_pager_info
#if	defined(LINTLIBRARY)
    (server, auth, default_pager, info)
	mach_port_t server;
	mach_port_t auth;
	mdefault_pager_t default_pager;
	default_pager_info_t *info;
{ return machid_default_pager_info(server, auth, default_pager, info); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mdefault_pager_t default_pager,
	default_pager_info_t *info
);
#endif

/* Routine default_pager_objects */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_default_pager_objects
#if	defined(LINTLIBRARY)
    (server, auth, default_pager, objects, objectsCnt)
	mach_port_t server;
	mach_port_t auth;
	mdefault_pager_t default_pager;
	default_pager_object_array_t *objects;
	mach_msg_type_number_t *objectsCnt;
{ return machid_default_pager_objects(server, auth, default_pager, objects, objectsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mdefault_pager_t default_pager,
	default_pager_object_array_t *objects,
	mach_msg_type_number_t *objectsCnt
);
#endif

/* Routine default_pager_object_pages */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t machid_default_pager_object_pages
#if	defined(LINTLIBRARY)
    (server, auth, default_pager, object, pages, pagesCnt)
	mach_port_t server;
	mach_port_t auth;
	mdefault_pager_t default_pager;
	mobject_name_t object;
	default_pager_page_array_t *pages;
	mach_msg_type_number_t *pagesCnt;
{ return machid_default_pager_object_pages(server, auth, default_pager, object, pages, pagesCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mdefault_pager_t default_pager,
	mobject_name_t object,
	default_pager_page_array_t *pages,
	mach_msg_type_number_t *pagesCnt
);
#endif

#endif	/* not defined(_machid_dpager_user_) */
