/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log: netisr.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:47  mike
 * Initial Lites release from hut.fi
 *
 *
 */

#include "iso.h"
#include "ns.h"
#include "inet.h"
#include "imp.h"
#include "ccitt.h"
#include "ether_as_syscall.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <net/netisr.h>

int		netisr = 0;
struct mutex	netisr_mutex = MUTEX_NAMED_INITIALIZER("netisr_mutex");

/*
 * Must be called at splnet.
 */
void dosoftnet()
{
    proc_invocation_t pk = get_proc_invocation();
    struct proc *p = pk->k_p;
    int original_master = pk->k_master_lock;

    mutex_lock(&netisr_mutex);

#if	NIMP > 0
	if (netisr & (1<<NETISR_IMP)) {
	    netisr &= ~(1<<NETISR_IMP);
	    mutex_unlock(&netisr_mutex);
	    impintr();
	    mutex_lock(&netisr_mutex);
	}
#endif	NIMP > 0
#if	INET
	if (netisr & (1<<NETISR_IP)) {
	    netisr &= ~(1<<NETISR_IP);
#if !ETHER_AS_SYSCALL
	    mutex_unlock(&netisr_mutex);
	    ipintr();
	    mutex_lock(&netisr_mutex);
#endif /* !ETHER_AS_SYSCALL */
	}
	if (netisr & (1<<NETISR_ARP)) {
	    netisr &= ~(1<<NETISR_ARP);
#if !ETHER_AS_SYSCALL
	    mutex_unlock(&netisr_mutex);
	    arpintr();
	    mutex_lock(&netisr_mutex);
#endif /* !ETHER_AS_SYSCALL */
	}
#endif	/* INET */
#if	ISO
	if (netisr & (1<<NETISR_ISO)) {
	    netisr &= ~(1<<NETISR_ISO);
	    mutex_unlock(&netisr_mutex);
	    isointr();
	    mutex_lock(&netisr_mutex);
	}
#endif	ISO
#if	CCITT
	if (netisr & (1<<NETISR_CCITT)) {
	    netisr &= ~(1<<NETISR_CCITT);
	    mutex_unlock(&netisr_mutex);
	    ccittintr();
	    mutex_lock(&netisr_mutex);
	}
#endif	CCITT
#if	NS
	if (netisr & (1<<NETISR_NS)) {
	    netisr &= ~(1<<NETISR_NS);
	    mutex_unlock(&netisr_mutex);
	    nsintr();
	    mutex_lock(&netisr_mutex);
	}
#endif	NS
	if (netisr & (1<<NETISR_RAW)) {
	    netisr &= ~(1<<NETISR_RAW);
	    mutex_unlock(&netisr_mutex);
	}
	else {
	    mutex_unlock(&netisr_mutex);
	}

	if (original_master != pk->k_master_lock)
		panic("dosoftnet");
}
