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
 * $Log: server_defs.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:48  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	server_defs.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	This file includes other include files generally needed
 *	everywhere in the server. This may slightly make compilation
 *	time longer but on the other hand makes it easier to add new
 *	files to the server. In any case it seems to be necessary to
 *	include most files everywhere.
 *
 *	Prototypes for internal interfaces that aren't somewhere else
 *	are also put here.
 */

#ifndef _SERVER_DEFS_H_
#define _SERVER_DEFS_H_

#include "osfmach3.h"

#include <serv/import_mach.h>
#include <sys/assert.h>
#include <sys/cmu_queue.h>
#include <sys/zalloc.h>

#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/uio.h>
#include <sys/ucred.h>
#include <sys/resourcevar.h>
#include <sys/proc.h>
#include <serv/timer.h>
#include <serv/port_object.h>
#include <sys/parallel.h>
#include <sys/vnode.h>

/* TYPES */
typedef volatile void noreturn;

/* PROTOTYPES */

/* serv_fork.c */
struct proc * newproc(struct proc *, boolean_t, boolean_t);

/* proc_to_task.c */
mach_port_t task_to_proc_enter(task_t, struct proc *);
void task_to_proc_remove(task_t);

/* ? */
void ux_server_add_port(mach_port_t);
void ux_server_remove_port(mach_port_t);
void ux_server_loop(void);

/* proc_to_task.c */
void proc_lock(struct proc *p);
void proc_ref(struct proc *p);
void proc_deref(struct proc *p, mach_port_t port);
void task_deref(struct proc *p, mach_port_t port);
void proc_remove_reverse(struct proc *p, mach_port_t port);
void task_remove_reverse(struct proc *p, mach_port_t port);
void thread_remove_reverse(struct proc *p, mach_port_t port);

/* server_exec.c */
int machid_register_process(struct proc *);
mach_error_t secure_execve(struct proc *, char *, vm_address_t, vm_size_t,
			   int, int);
mach_error_t after_exec(struct proc *, vm_address_t *, vm_size_t *, int *,
			int *, char *, mach_msg_type_number_t *, char *,
			mach_msg_type_number_t *, char *,
			mach_msg_type_number_t *, char *,
			mach_msg_type_number_t *, mach_port_t *);


void *malloc(size_t);
void free(void *);

/* vm_syscalls.c */
kern_return_t mmap_vm_map(struct proc *, vm_address_t, vm_size_t, boolean_t,
			  mach_port_t, vm_offset_t, boolean_t, vm_prot_t,
			  vm_prot_t, vm_inherit_t, vm_address_t *);

mach_error_t file_vm_map(struct proc *, vm_address_t *, vm_size_t,
			 vm_address_t, boolean_t, struct file *, vm_offset_t,
			 boolean_t, vm_prot_t, vm_prot_t, vm_inherit_t);

/* GLOBALS */
extern mach_port_t default_pager_port;
extern mach_port_t shared_memory_port;
extern mach_port_t privileged_host_port;
extern mach_port_t default_processor_set;

#if OSFMACH3
extern security_token_t security_id;
#endif

#endif /* !_SERVER_DEFS_H_ */
