#ifndef	_bootstrap_user_
#define	_bootstrap_user_

/* Module bootstrap */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>

/* Routine bootstrap_privileged_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t bootstrap_privileged_ports
#if	defined(LINTLIBRARY)
    (bootstrap, priv_host, priv_device)
	mach_port_t bootstrap;
	mach_port_t *priv_host;
	mach_port_t *priv_device;
{ return bootstrap_privileged_ports(bootstrap, priv_host, priv_device); }
#else
(
	mach_port_t bootstrap,
	mach_port_t *priv_host,
	mach_port_t *priv_device
);
#endif

#endif	/* not defined(_bootstrap_user_) */
