/* 
 * Mach Operating System
 * Copyright (c) 1994 Johannes Helander
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * JOHANNES HELANDER ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  JOHANNES HELANDER DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: e_defs.h,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/08/23  18:33:04  mike
 * temporary: extra header file for mach4 stubs
 *
 * Revision 1.1.1.2  1995/03/23  01:15:30  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/e_defs.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Common includes and declarations for the emulator.
 */

#ifndef _E_DEFS_H_
#define _E_DEFS_H_

#include <mach_init.h>
#include <mach/mig_errors.h>
#ifdef USENRPC
#include <Nbsd_1.h>
#endif
#include <bsd_1.h>
#include <sys/file.h>
#include <sys/uio.h>

#include <mach.h>
#include <mach/machine.h>
#include <sys/exec_file.h>	/* for binary_type_t etc. */

#include <sys/errno.h>
#include <sys/types.h>
#include <serv/bsd_msg.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#include <sys/ucred.h> /* to avoid warnings from vnode.h */
#include <sys/vnode.h>

#include <sys/mman.h>
#define VATTR_NULL(v) vattr_clear(v)
#define MFILE_VALID(f) TRUE

struct itimervalue;

#ifdef	MAP_UAREA
#include <sys/ushared.h>

extern int shared_enabled;
extern struct ushared_ro *shared_base_ro;
extern struct ushared_rw *shared_base_rw;
extern char *shared_readwrite;
extern int readwrite_inuse;
extern spin_lock_t readwrite_lock;
#endif	MAP_UAREA

extern mach_port_t our_bsd_server_port;
typedef void volatile noreturn;
typedef int mfile_t;

typedef vm_offset_t bnr_off_t;

extern binary_type_t e_my_binary_type;
extern int syscall_debug;
extern char *atbin_names[];

#define emul_assert(expr) \
	((void) ((expr) ? 0 : internal_assert(#expr, __FILE__, __LINE__)))

#define internal_assert(exprstr, file, line) \
    ({ \
	     (e_emulator_error("Assertion `%s' failed at %s:%u\n", \
			       exprstr, file, line), 0); \
	     emul_panic("emul_assert"); \
    })

#include <eproto.h>

/* misc_asm.s */
int emul_save_state( void *	 /* struct i386_thread_state *state */ );
noreturn emul_load_state( void * /* struct i386_thread_state *state */ );

/* e_bsd_stubs.c */
int e_emulator_error(char *fmt, ...);

/* Misc: should be somewhere else */
size_t strlen(const char *);

#endif /* !_E_DEFS_H_ */
