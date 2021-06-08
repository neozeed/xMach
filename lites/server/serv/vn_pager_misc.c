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
 * $Log: vn_pager_misc.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:17:05  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	vn_pager_misc.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	January 1994.
 *
 *	vnode pager misc routines.
 */

#include "vnpager.h"

#include <serv/server_defs.h>
#include <serv/vn_pager.h>
#include <sys/synch.h>
#include <sys/mount.h>

zone_t vn_pager_object_zone;

struct vnode * port_to_vnode_lookup(mach_port_t port, mach_msg_seqno_t seqno)
{
	return (struct vnode *) port_object_receive_lookup(port, seqno, POT_VNODE_PAGER);
}

/* lookup or allocate memory object from vnode */
mach_port_t vnode_to_port (struct vnode *vn)
{
	vn_pager_t pager;
	mach_port_t port;
	kern_return_t kr;

	assert(vn->v_type == VREG);

/* XXX lock vnode */

	if (!vn || vn->v_type != VREG)
	    return MACH_PORT_NULL;
	if (vn->v_vmdata)
	    return vn->v_vmdata->object_port;
	/* allocate and initialize a new one */
	pager = (vn_pager_t) zalloc(vn_pager_object_zone);
	kr = port_object_allocate_receive(&port, POT_VNODE_PAGER, vn);
	if (!pager || kr || !MACH_PORT_VALID(port)) {
		panic("vnode_to_port");
		return MACH_PORT_NULL;
	}
	kr = port_object_make_send(port);
	if (kr)
	    panic("vnode_to_port: m_p_insert_right failed");

	pager->object_port = port;
	pager->control_port = MACH_PORT_NULL;
	pager->name_port = MACH_PORT_NULL;
	pager->page_size = 0;
	pager->may_cache = TRUE;
	pager->copy_strategy = MEMORY_OBJECT_COPY_DELAY;

	vn->v_vmdata = pager;

	vn_pager_add_to_wait_set(port);
	DEBUG_PRINT(("vnode_to_port_enter created port x%x\n", port));
	return port;
}

void vn_pager_destroy (struct vnode *vn)
{
	vn_pager_t pager;

	assert(vn->v_type == VREG);
	if (!vn->v_vmdata)
	    return;
	pager = vn->v_vmdata;
	vn->v_vmdata = VN_PAGER_NULL;
	vn->v_cache_state = VC_FREE;
	port_object_shutdown(pager->object_port, TRUE);
	/* get rid of extra send right from insert_right above */
	if (MACH_PORT_VALID(pager->object_port))
	    mach_port_deallocate(mach_task_self(), pager->object_port);

	/* Deallocate control port from memory_object_init */
	if (MACH_PORT_VALID(pager->control_port))
	    consume_control(pager->control_port);
	/* and name port as well */
	if (MACH_PORT_VALID(pager->name_port))
	    mach_port_deallocate(mach_task_self(), pager->name_port);

	/* Ref count and locking? */
	zfree(vn_pager_object_zone, (vm_offset_t) pager);
	wakeup(vn);	/* in case someone was waiting for state transition */
}

#if 0
void vn_pager_delete(mach_port_t port)
{
	struct vnode *vn;
	vn_pager_t pager;

	vn = port_object_send_lookup_(port);
	if (!vn || !vn->v_vmdata)
	    return;
	pager = vn->v_vmdata;
	vn->v_vmdata = VN_PAGER_NULL;
	port_object_shutdown(port, TRUE);
	vn->v_cache_state = VC_FREE;
	/* get rid of extra send right from insert_right above */
	mach_port_deallocate(mach_task_self(), port);

	/* Ref count and locking? */
	zfree(vn_pager_object_zone, (vm_offset_t) pager);
	wakeup(vn);	/* in case someone was waiting for state transition */
}
#endif

mach_port_t waiting_objects_set;

void vn_pager_add_to_wait_set(mach_port_t port)
{
	kern_return_t kr;
	kr = mach_port_move_member(mach_task_self(),
				   port,
				   waiting_objects_set);
	if (kr)
	    panic("vn_pager_add_to_wait_set");

}

extern boolean_t seqnos_memory_object_server();

void vn_pager_object_init_thread()
{
	struct proc *p;
	kern_return_t kr;

	cthread_wire();
	system_proc(&p, "PagerInit");

	do {
	  kr = mach_msg_server(seqnos_memory_object_server,
			     8192,
			     waiting_objects_set
#if OSFMACH3
			     ,MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_SEQNO)
#endif
			     );
	  printf("vn_pager_object_init_thread: %s", mach_error_string(kr));
	} while (kr == MACH_SEND_INTERRUPTED);
	panic("vn_pager_object_init_thread %s", mach_error_string(kr));
}

void vn_pager_init()
{
	kern_return_t kr;
	vn_pager_object_zone = zinit(sizeof(struct vn_pager),
				     sizeof(struct vn_pager) * 10000,
				     vm_page_size, TRUE,
				     "vnode pager object");
	assert(vn_pager_object_zone != (zone_t)0);
	kr = mach_port_allocate(mach_task_self(),
				MACH_PORT_RIGHT_PORT_SET,
				&waiting_objects_set);
	if (kr)
	    panic("vn_pager_init");
	ux_create_thread(vn_pager_object_init_thread);
}

/* 
 * Start writing all dirty pages associated with a file.
 * i.e. do a lock_request with should_clean set.
 */

void vn_pager_sync(struct vnode *vn, int a_wait)
{
	if (vn->v_type == VREG) {
		if (a_wait == MNT_WAIT)
		    vn_pager_revoke_write_and_wait(vn);
		else
		    vn_pager_revoke_write(vn);
	}
}

/* VC_MO_WRITE -> VC_MO_CLEANING. Master is locked. */
void vn_pager_revoke_write (struct vnode *vn)
{
	vn_pager_t pager;
	kern_return_t kr;

	assert(vn->v_type == VREG);
	pager = vn->v_vmdata;

	if (!pager
	    || !MACH_PORT_VALID(pager->object_port)
	    || !MACH_PORT_VALID(pager->control_port))
	{
		vn->v_cache_state = VC_FREE;
		wakeup(vn);	/* unnecessary */
		return;
	}

	if (vn->v_cache_state != VC_MO_WRITE)
	    return;

	vn->v_cache_state = VC_MO_CLEANING;
	kr = memory_object_lock_request(pager->control_port,
					0,
					VM_MAX_ADDRESS,
					TRUE,
					FALSE,
					VM_PROT_WRITE,
					pager->object_port
					);
	/* m_o_lock_completed changes the state to VC_READ and wakeups */
	if (kr == KERN_SUCCESS)
	    return;

	panic("vn_pager_revoke_write: m_o_lock_request returned %s\n",
	      mach_error_string(kr));
	vn->v_cache_state = VC_FREE;
	wakeup(vn);		/* unnecessary */
}

/* caller has unix_master() */
/* VC_READ -> WC_FLUSHING */
void vn_pager_revoke_read (struct vnode *vn)
{
	vn_pager_t pager;
	kern_return_t kr;

	assert(vn->v_type == VREG);
	pager = vn->v_vmdata;

	if (!pager
	    || !MACH_PORT_VALID(pager->object_port)
	    || !MACH_PORT_VALID(pager->control_port))
	{
		vn->v_cache_state = VC_FREE;
		wakeup(vn);	/* unnecessary */
		return;
	}

	assert (vn->v_cache_state == VC_READ);

	vn->v_cache_state = VC_MO_FLUSHING;
	kr = memory_object_lock_request(pager->control_port,
					0,
					VM_MAX_ADDRESS,
					TRUE,
					TRUE,
					VM_PROT_ALL,
					pager->object_port
					);
	/* m_o_lock_completed changes the state to VC_FREE and wakeups */
	if (kr == KERN_SUCCESS)
	    return;

	/* The kernel has terminated the memory object? */
	panic("vn_pager_revoke_read: m_o_lock_request(x%x): %s\n",
	      pager->control_port, mach_error_string(kr));
	vn->v_cache_state = VC_FREE;
	wakeup(vn);		/* unnecessary */
}

/* ... -> VC_FREE, VC_READ, VC_BUF_WRITE */
void vn_pager_revoke_write_and_wait (struct vnode *vn)
{
	int s;
	assert(vn->v_type == VREG);

	while (vn->v_cache_state != VC_FREE
	       && vn->v_cache_state != VC_READ
	       && vn->v_cache_state != VC_BUF_WRITE)
	{
		vn_pager_revoke_write(vn);
		if (vn->v_cache_state == VC_MO_CLEANING
		    || vn->v_cache_state == VC_MO_FLUSHING)
		{
			s = splbio();
			while (vn->v_cache_state == VC_MO_CLEANING
			       || vn->v_cache_state == VC_MO_FLUSHING)
			    tsleep(vn, 0, "revoke_read_and_wait_1", 0);
			splx(s);
		}
	}
}

/* ... -> VC_FREE */
void vn_pager_revoke_read_and_wait (struct vnode *vn)
{
	int s;

	assert(vn->v_type == VREG);
	while (vn->v_cache_state != VC_FREE) {
		vn_pager_revoke_read(vn);
		if (vn->v_cache_state == VC_MO_CLEANING
		    || vn->v_cache_state == VC_MO_FLUSHING)
		{
			s = splbio();
			while (vn->v_cache_state == VC_MO_CLEANING
			       || vn->v_cache_state == VC_MO_FLUSHING)
			    tsleep(vn, 0, "revoke_read_and_wait_1", 0);
			splx(s);
		}
	}
}

/* for buf ops.  No wakeups required here. */
/* ... -> VC_READ, VC_BUF_WRITE */
void vn_cache_state_buf_read(struct vnode *vn)
{
	assert(vn->v_type == VREG);

	while (TRUE) {
		switch (vn->v_cache_state) {
		      case VC_READ:
		      case VC_BUF_WRITE:
			return;
		      case VC_FREE:
			vn->v_cache_state = VC_READ;
			return;
		      default:
			vn_pager_revoke_write_and_wait(vn);
		}
	}
}

/* ... -> VC_BUF_WRITE */
void vn_cache_state_buf_write(struct vnode *vn)
{
	assert(vn->v_type == VREG);

	while (TRUE) {
		switch (vn->v_cache_state) {
		      case VC_BUF_WRITE:
			return;
		      case VC_FREE:
			vn->v_cache_state = VC_BUF_WRITE;
			return;
		      default:
			vn_pager_revoke_read_and_wait(vn);
		}
	}
}

/* 
 * File was unlinked or similar.
 * Make object disappear after it has been unmapped.
 */
void vnode_pager_uncache(struct vnode *vn)
{
	vn_pager_t pager;
	kern_return_t kr;
	boolean_t was_cachable;
#if OSFMACH3
	struct memory_object_behave_info behavior;
	struct memory_object_perf_info perf;
#endif

	if(vn->v_type != VREG)
	    return;

	pager = vn->v_vmdata;
	if (!pager)
	    return;

	was_cachable = pager->may_cache;
	pager->may_cache = FALSE;

	if (!was_cachable
	    || !MACH_PORT_VALID(pager->object_port)
	    || !MACH_PORT_VALID(pager->control_port))
	    return;

#if OSFMACH3
	behavior.copy_strategy = pager->copy_strategy;
	behavior.temporary = FALSE;
	behavior.invalidate = FALSE;
	behavior.write_completions = FALSE;
	kr = memory_object_change_attributes(pager->control_port,
					     MEMORY_OBJECT_BEHAVIOR_INFO,
					     (memory_object_info_t) &behavior,
					     MEMORY_OBJECT_BEHAVE_INFO_COUNT,
					     MACH_PORT_NULL);

	perf.cluster_size = PAGE_SIZE;
	perf.may_cache = pager->may_cache;
	kr = memory_object_change_attributes(pager->control_port,
					     MEMORY_OBJECT_PERFORMANCE_INFO,
					     (memory_object_info_t) &perf,
					     MEMORY_OBJECT_PERF_INFO_COUNT,
					     MACH_PORT_NULL);
#else
	kr = memory_object_change_attributes(pager->control_port,
					     pager->may_cache,
					     pager->copy_strategy,
					     MACH_PORT_NULL);
#endif /* OSFMACH3 */
	assert(kr == KERN_SUCCESS);
}


/* Make memory objects associated with file system non-persistent */
void vnode_pager_umount(struct mount *mp)
{
	struct vnode *vn;

	for (vn = mp->mnt_vnodelist.lh_first;
	     vn;
	     vn = vn->v_mntvnodes.le_next)
	{
		if (vn->v_mount != mp)
		    panic("vnode_pager_umount");
		if (vn->v_type == VREG)
		    vnode_pager_uncache(vn);
	}
}

/* Dummy functions */
void vnode_pager_setsize(struct vnode *vn, u_long size)
{
	vn_pager_t pager;

	pager = vn->v_vmdata;
	if (!pager || vn->v_type != VREG)
	    return;

	pager->size = size;
}

void vnode_pager_init(){}
int vnode_pager_setup(){ printf("vnode_pager_setup called"); return 0; }
void vnode_pager_release(){}
