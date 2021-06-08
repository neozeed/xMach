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
 * $Log: vn_pager.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	vn_pager.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	January 1994.
 *
 *	vnode pager interface.
 */

#ifndef _VN_PAGER_H_
#define _VN_PAGER_H_

#if VNPAGER
/* this is what goes into the vnode */
typedef struct vn_pager {
	mach_port_t object_port;
	mach_port_t control_port;
	mach_port_t name_port;
	vm_size_t page_size;
	/* for change_attributes */
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
	/* for reading and writing */
	off_t size;
} *vn_pager_t;

enum vcache_state;		/* defined in vnode.h */
struct mount;
struct vnode;

#define VN_PAGER_NULL ((vn_pager_t)0)

#define DEBUG_PRINT(x) {if (xmm_debug) printf x;}
extern boolean_t xmm_debug;

struct vnode *port_to_vnode_lookup(mach_port_t port,
				   mach_msg_seqno_t seqno);
boolean_t port_to_vnode_enter(mach_port_t port, struct vnode *vn);
boolean_t port_to_vnode_remove(mach_port_t port);
mach_port_t vnode_to_port(struct vnode *vn);
void vn_pager_add_to_wait_set(mach_port_t port);
void vn_pager_delete(mach_port_t port);
void vn_pager_revoke_write (struct vnode *vn);
void vn_pager_revoke_read (struct vnode *vn);
void vn_pager_revoke_write_and_wait (struct vnode *vn);
void vn_pager_revoke_read_and_wait (struct vnode *vn);
void vn_cache_state_buf_write(struct vnode *vn);
void vn_cache_state_buf_read(struct vnode *vn);
void vn_pager_sync(struct vnode *vn, int a_wait /* MNT_WAIT or MNT_NOWAIT */);

void vnode_pager_umount(struct mount *mp);
void vnode_pager_setsize(struct vnode *vn, u_long /* XXX */ size);

#endif /* VNPAGER */
#endif /* !_VN_PAGER_H_ */

