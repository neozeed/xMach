#ifndef	_netname_user_
#define	_netname_user_

/* Module netname */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <servers/netname_defs.h>

/* Routine netname_check_in */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t netname_check_in
#if	defined(LINTLIBRARY)
    (server_port, port_name, signature, port_id)
	mach_port_t server_port;
	netname_name_t port_name;
	mach_port_t signature;
	mach_port_t port_id;
{ return netname_check_in(server_port, port_name, signature, port_id); }
#else
(
	mach_port_t server_port,
	netname_name_t port_name,
	mach_port_t signature,
	mach_port_t port_id
);
#endif

/* Routine netname_look_up */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t netname_look_up
#if	defined(LINTLIBRARY)
    (server_port, host_name, port_name, port_id)
	mach_port_t server_port;
	netname_name_t host_name;
	netname_name_t port_name;
	mach_port_t *port_id;
{ return netname_look_up(server_port, host_name, port_name, port_id); }
#else
(
	mach_port_t server_port,
	netname_name_t host_name,
	netname_name_t port_name,
	mach_port_t *port_id
);
#endif

/* Routine netname_check_out */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t netname_check_out
#if	defined(LINTLIBRARY)
    (server_port, port_name, signature)
	mach_port_t server_port;
	netname_name_t port_name;
	mach_port_t signature;
{ return netname_check_out(server_port, port_name, signature); }
#else
(
	mach_port_t server_port,
	netname_name_t port_name,
	mach_port_t signature
);
#endif

/* Routine netname_version */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t netname_version
#if	defined(LINTLIBRARY)
    (server_port, version)
	mach_port_t server_port;
	netname_name_t version;
{ return netname_version(server_port, version); }
#else
(
	mach_port_t server_port,
	netname_name_t version
);
#endif

#endif	/* not defined(_netname_user_) */
