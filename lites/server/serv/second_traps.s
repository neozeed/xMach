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
 * $Log: second_traps.s,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
# Revision 1.1.1.1  1995/03/02  21:49:48  mike
# Initial Lites release from hut.fi
#
 * Revision 2.2  93/03/12  10:55:14  rwd
 * 	Fix #define s not to have _ to avoid cpp problem.
 * 	[93/03/02  13:51:55  rwd]
 * 
 * Revision 2.1  92/04/21  17:10:47  rwd
 * BSDSS
 * 
 *
 */

 /*
 * Copyright (c) 1991
 * Open Software Foundation, Inc.
 * 
 * Permission is hereby granted to use, copy, modify and freely distribute
 * the software in this file and its documentation for any purpose without
 * fee, provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.  Further, provided that the name of Open
 * Software Foundation, Inc. ("OSF") not be used in advertising or
 * publicity pertaining to distribution of the software without prior
 * written permission from OSF.  OSF makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * 
 */

/* This file defines functions for Mach traps that exist in the standard
  * Mach kernel, but are not defined in the standard version of libmach_sa.a
  * (see the file mach/syscall_sw.h).  It is a replacement for (and
  * satisfies all the references that are defined in) the module
  * mk/src/.../user/libmach/mach_traps.cs of libmach_sa.a.  We include this
  * in the server to avoid having to modify the source to libmach, which is
  * under the mk source tree and not under our control.
 */

/* Task_by_pid is needed by the second server to initialize the
 * privileged host port and the device server port (see bsd/mach_init.c).
 * Undefining STANDALONE causes task_by_pid and several other traps to be
 * defined by mach/syscall_sw.h.  We need to change the names of the
 * trap functions so that they won't interfere with the functions of the
 * same name that implement them inside the server:
 */
#undef STANDALONE
#define NOT_MACH_TRAPS
#define UNIXOID_TRAPS 1

#define task_by_pid	  second_task_by_pid
#define pid_by_task	  second_pid_by_task
#define init_process	  second_init_process
#define map_fd		  second_map_fd
#define rfs_make_symlink  second_rfs_make_symlink
#define htg_syscall	  second_htg_syscall

#include <mach/syscall_sw.h>
