#ifndef	_notify_user_
#define	_notify_user_

/* Module notify */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>

/* SimpleRoutine mach_notify_port_deleted */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_notify_port_deleted
#if	defined(LINTLIBRARY)
    (notify, name)
	mach_port_t notify;
	mach_port_t name;
{ return mach_notify_port_deleted(notify, name); }
#else
(
	mach_port_t notify,
	mach_port_t name
);
#endif

/* SimpleRoutine mach_notify_msg_accepted */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_notify_msg_accepted
#if	defined(LINTLIBRARY)
    (notify, name)
	mach_port_t notify;
	mach_port_t name;
{ return mach_notify_msg_accepted(notify, name); }
#else
(
	mach_port_t notify,
	mach_port_t name
);
#endif

/* SimpleRoutine mach_notify_port_destroyed */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_notify_port_destroyed
#if	defined(LINTLIBRARY)
    (notify, rights, rightsPoly)
	mach_port_t notify;
	mach_port_t rights;
	mach_msg_type_name_t rightsPoly;
{ return mach_notify_port_destroyed(notify, rights, rightsPoly); }
#else
(
	mach_port_t notify,
	mach_port_t rights,
	mach_msg_type_name_t rightsPoly
);
#endif

/* SimpleRoutine mach_notify_no_senders */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_notify_no_senders
#if	defined(LINTLIBRARY)
    (notify, mscount)
	mach_port_t notify;
	mach_port_mscount_t mscount;
{ return mach_notify_no_senders(notify, mscount); }
#else
(
	mach_port_t notify,
	mach_port_mscount_t mscount
);
#endif

/* SimpleRoutine mach_notify_send_once */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_notify_send_once
#if	defined(LINTLIBRARY)
    (notify)
	mach_port_t notify;
{ return mach_notify_send_once(notify); }
#else
(
	mach_port_t notify
);
#endif

/* SimpleRoutine mach_notify_dead_name */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t mach_notify_dead_name
#if	defined(LINTLIBRARY)
    (notify, name)
	mach_port_t notify;
	mach_port_t name;
{ return mach_notify_dead_name(notify, name); }
#else
(
	mach_port_t notify,
	mach_port_t name
);
#endif

#endif	/* not defined(_notify_user_) */
