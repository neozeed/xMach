#ifndef	_dp_helper_user_
#define	_dp_helper_user_

/* Module dp_helper */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>

/* SimpleRoutine dp_helper_paging_space */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t dp_helper_paging_space
#if	defined(LINTLIBRARY)
    (dp_helper, space_shortage, approx_amount)
	mach_port_t dp_helper;
	boolean_t space_shortage;
	vm_size_t approx_amount;
{ return dp_helper_paging_space(dp_helper, space_shortage, approx_amount); }
#else
(
	mach_port_t dp_helper,
	boolean_t space_shortage,
	vm_size_t approx_amount
);
#endif

#endif	/* not defined(_dp_helper_user_) */
