#ifndef	_mach_port_user_
#define	_mach_port_user_

/* Module mach_port */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>

/* Routine mach_port_names */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_names
#if	defined(LINTLIBRARY)
    (task, names, namesCnt, types, typesCnt)
	mach_port_t task;
	mach_port_array_t *names;
	mach_msg_type_number_t *namesCnt;
	mach_port_type_array_t *types;
	mach_msg_type_number_t *typesCnt;
{ return mach_port_names(task, names, namesCnt, types, typesCnt); }
#else
(
	mach_port_t task,
	mach_port_array_t *names,
	mach_msg_type_number_t *namesCnt,
	mach_port_type_array_t *types,
	mach_msg_type_number_t *typesCnt
);
#endif

/* Routine mach_port_type */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_type
#if	defined(LINTLIBRARY)
    (task, name, ptype)
	mach_port_t task;
	mach_port_t name;
	mach_port_type_t *ptype;
{ return mach_port_type(task, name, ptype); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_type_t *ptype
);
#endif

/* Routine mach_port_rename */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_rename
#if	defined(LINTLIBRARY)
    (task, old_name, new_name)
	mach_port_t task;
	mach_port_t old_name;
	mach_port_t new_name;
{ return mach_port_rename(task, old_name, new_name); }
#else
(
	mach_port_t task,
	mach_port_t old_name,
	mach_port_t new_name
);
#endif

/* Routine mig_mach_port_allocate_name */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mig_mach_port_allocate_name
#if	defined(LINTLIBRARY)
    (task, right, name)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_t name;
{ return mig_mach_port_allocate_name(task, right, name); }
#else
(
	mach_port_t task,
	mach_port_right_t right,
	mach_port_t name
);
#endif

/* Routine mig_mach_port_allocate */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mig_mach_port_allocate
#if	defined(LINTLIBRARY)
    (task, right, name)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_t *name;
{ return mig_mach_port_allocate(task, right, name); }
#else
(
	mach_port_t task,
	mach_port_right_t right,
	mach_port_t *name
);
#endif

/* Routine mach_port_destroy */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_destroy
#if	defined(LINTLIBRARY)
    (task, name)
	mach_port_t task;
	mach_port_t name;
{ return mach_port_destroy(task, name); }
#else
(
	mach_port_t task,
	mach_port_t name
);
#endif

/* Routine mig_mach_port_deallocate */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mig_mach_port_deallocate
#if	defined(LINTLIBRARY)
    (task, name)
	mach_port_t task;
	mach_port_t name;
{ return mig_mach_port_deallocate(task, name); }
#else
(
	mach_port_t task,
	mach_port_t name
);
#endif

/* Routine mach_port_get_refs */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_get_refs
#if	defined(LINTLIBRARY)
    (task, name, right, refs)
	mach_port_t task;
	mach_port_t name;
	mach_port_right_t right;
	mach_port_urefs_t *refs;
{ return mach_port_get_refs(task, name, right, refs); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_right_t right,
	mach_port_urefs_t *refs
);
#endif

/* Routine mach_port_mod_refs */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_mod_refs
#if	defined(LINTLIBRARY)
    (task, name, right, delta)
	mach_port_t task;
	mach_port_t name;
	mach_port_right_t right;
	mach_port_delta_t delta;
{ return mach_port_mod_refs(task, name, right, delta); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_right_t right,
	mach_port_delta_t delta
);
#endif

/* Routine old_mach_port_get_receive_status */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t old_mach_port_get_receive_status
#if	defined(LINTLIBRARY)
    (task, name, status)
	mach_port_t task;
	mach_port_t name;
	old_mach_port_status_t *status;
{ return old_mach_port_get_receive_status(task, name, status); }
#else
(
	mach_port_t task,
	mach_port_t name,
	old_mach_port_status_t *status
);
#endif

/* Routine mach_port_set_qlimit */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_set_qlimit
#if	defined(LINTLIBRARY)
    (task, name, qlimit)
	mach_port_t task;
	mach_port_t name;
	mach_port_msgcount_t qlimit;
{ return mach_port_set_qlimit(task, name, qlimit); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_msgcount_t qlimit
);
#endif

/* Routine mach_port_set_mscount */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_set_mscount
#if	defined(LINTLIBRARY)
    (task, name, mscount)
	mach_port_t task;
	mach_port_t name;
	mach_port_mscount_t mscount;
{ return mach_port_set_mscount(task, name, mscount); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_mscount_t mscount
);
#endif

/* Routine mach_port_get_set_status */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_get_set_status
#if	defined(LINTLIBRARY)
    (task, name, members, membersCnt)
	mach_port_t task;
	mach_port_t name;
	mach_port_array_t *members;
	mach_msg_type_number_t *membersCnt;
{ return mach_port_get_set_status(task, name, members, membersCnt); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_array_t *members,
	mach_msg_type_number_t *membersCnt
);
#endif

/* Routine mach_port_move_member */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_move_member
#if	defined(LINTLIBRARY)
    (task, member, after)
	mach_port_t task;
	mach_port_t member;
	mach_port_t after;
{ return mach_port_move_member(task, member, after); }
#else
(
	mach_port_t task,
	mach_port_t member,
	mach_port_t after
);
#endif

/* Routine mach_port_request_notification */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_request_notification
#if	defined(LINTLIBRARY)
    (task, name, id, sync, notify, notifyPoly, previous)
	mach_port_t task;
	mach_port_t name;
	mach_msg_id_t id;
	mach_port_mscount_t sync;
	mach_port_t notify;
	mach_msg_type_name_t notifyPoly;
	mach_port_t *previous;
{ return mach_port_request_notification(task, name, id, sync, notify, notifyPoly, previous); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_msg_id_t id,
	mach_port_mscount_t sync,
	mach_port_t notify,
	mach_msg_type_name_t notifyPoly,
	mach_port_t *previous
);
#endif

/* Routine mig_mach_port_insert_right */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mig_mach_port_insert_right
#if	defined(LINTLIBRARY)
    (task, name, poly, polyPoly)
	mach_port_t task;
	mach_port_t name;
	mach_port_t poly;
	mach_msg_type_name_t polyPoly;
{ return mig_mach_port_insert_right(task, name, poly, polyPoly); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_t poly,
	mach_msg_type_name_t polyPoly
);
#endif

/* Routine mach_port_extract_right */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_extract_right
#if	defined(LINTLIBRARY)
    (task, name, msgt_name, poly, polyPoly)
	mach_port_t task;
	mach_port_t name;
	mach_msg_type_name_t msgt_name;
	mach_port_t *poly;
	mach_msg_type_name_t *polyPoly;
{ return mach_port_extract_right(task, name, msgt_name, poly, polyPoly); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_msg_type_name_t msgt_name,
	mach_port_t *poly,
	mach_msg_type_name_t *polyPoly
);
#endif

/* Routine mach_port_get_receive_status */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_get_receive_status
#if	defined(LINTLIBRARY)
    (task, name, status)
	mach_port_t task;
	mach_port_t name;
	mach_port_status_t *status;
{ return mach_port_get_receive_status(task, name, status); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_status_t *status
);
#endif

/* Routine mach_port_set_seqno */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_port_set_seqno
#if	defined(LINTLIBRARY)
    (task, name, seqno)
	mach_port_t task;
	mach_port_t name;
	mach_port_seqno_t seqno;
{ return mach_port_set_seqno(task, name, seqno); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_seqno_t seqno
);
#endif

#endif	/* not defined(_mach_port_user_) */
