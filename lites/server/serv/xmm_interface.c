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
 * $Log: xmm_interface.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:49  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	xmm_interface.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	January 1994.
 *
 * External memory manager interfaces for a Lites vnode pager.
 * memory_object_init is usually run on the wired VNodePager thread.
 * All other requests are run on normal server threads. Since it is
 * unknown which process caused a page fault and in any case objects
 * may be mapped to several processes, the requests are executed using
 * process zero's (the server itself) credentials. Access control is
 * handled by the mapping entity.
 *
 * In order to do access control object ports are not be handed
 * outside the server but instead just representatives.
 * Representatives are mapped by bsd_vm_map and the server holds
 * representatives' receive rights. The server then does access
 * control and does vm_map on the real object.
 * This is now done by port file handles (see file.h etc).
 *
 * Since there is no other real locking on vnodes etc. it is necessary
 * to grab the unix master lock.
 */

#include "vnpager.h"
#include <serv/server_defs.h>
#include <serv/vn_pager.h>
#include <sys/synch.h>

#define XMM_START_SERVER()				\
	struct proc *p = &proc0;			\
	proc_invocation_t pk;				\
	server_thread_register_locked(p);		\
	pk = get_proc_invocation();			\
	pk->k_reply_msg = 0;				\
	{

#define XMM_END_SERVER()				\
	}						\
	server_thread_deregister(p);			\
	unix_release();					\
	return KERN_SUCCESS;

boolean_t xmm_debug = FALSE;

/* XXX Use port_objects and lazy evaluate deletion */
void consume_control(mach_port_t port)
{
	kern_return_t kr;

	kr = mach_port_deallocate(mach_task_self(), port);
	assert(kr == KERN_SUCCESS);
}

#if OSFMACH3
kern_return_t seqnos_memory_object_notify (
	mach_port_t memory_object, /* representative */
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	host_name_port_t host_name,
	vm_size_t memory_object_page_size)
{
	struct vnode *vn;
	vn_pager_t pager;
	kern_return_t kr;
	memory_object_attr_info_data_t attributes;

	XMM_START_SERVER();
	DEBUG_PRINT(("seqnos_memory_object_notify(x%x) start\n", memory_object));
	vn = port_to_vnode_lookup(memory_object, seqno);
	if (!vn)
	    panic("seqnos_memory_object_init no vn");
	unix_master();		/* XXX */
	pager = vn->v_vmdata;
	if (!pager)
	    panic("seqnos_memory_object_init no pager");
	vref(vn);		/* the mapping is one reference */

	assert(pager->control_port == MACH_PORT_NULL);
	pager->control_port = memory_control;
	pager->name_port = MACH_PORT_NULL;
	pager->page_size = memory_object_page_size;

	/* move port from waiting set to ux server set */
	ux_server_add_port(memory_object);

	attributes.object_ready  = TRUE;
	attributes.copy_strategy = pager->copy_strategy;
	attributes.may_cache     = pager->may_cache;

	kr = memory_object_establish(host_name,
				     memory_object,
#if OSF_LEDGERS
				     MACH_PORT_NULL,
#endif /* OSF_LEDGERS */
				     memory_control,
				     memory_object,
				     VM_PROT_ALL,
				     security_id,
				     MEMORY_OBJECT_ATTRIBUTE_INFO,
				     (memory_object_info_t) &attributes,
				     MEMORY_OBJECT_ATTR_INFO_COUNT);
	assert(kr == KERN_SUCCESS);
	DEBUG_PRINT(("seqnos_memory_object_notify(x%x) end\n", memory_object));
	XMM_END_SERVER();
}

#else /* OSFMACH3 */

kern_return_t seqnos_memory_object_init (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	mach_port_t memory_object_name,
	vm_size_t memory_object_page_size)
{
	struct vnode *vn;
	vn_pager_t pager;
	kern_return_t kr;

	XMM_START_SERVER();
	DEBUG_PRINT(("seqnos_memory_object_init(x%x) start\n", memory_object));
	vn = port_to_vnode_lookup(memory_object, seqno);
	if (!vn)
	    panic("seqnos_memory_object_init no vn");
	unix_master();		/* XXX */
	pager = vn->v_vmdata;
	if (!pager)
	    panic("seqnos_memory_object_init no pager");
	vref(vn);		/* the mapping is one reference */

	assert(pager->control_port == MACH_PORT_NULL);
	pager->control_port = memory_control;
	pager->name_port = memory_object_name;
	pager->page_size = memory_object_page_size;

	/* move port from waiting set to ux server set */
	ux_server_add_port(memory_object);

	kr = memory_object_ready(memory_control,
				 pager->may_cache,
				 pager->copy_strategy);
	assert(kr == KERN_SUCCESS);
	DEBUG_PRINT(("seqnos_memory_object_init(x%x) end\n", memory_object));
	XMM_END_SERVER();
}

#endif /* OSFMACH3 */

kern_return_t seqnos_memory_object_terminate (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	mach_port_t memory_object_name)
{
	struct vnode *vn;
	kern_return_t kr;
	vn_pager_t pager;

	XMM_START_SERVER();

	DEBUG_PRINT(("seqnos_memory_object_terminate(x%x)\n", memory_object));

	vn = port_to_vnode_lookup(memory_object, seqno);
	if (!vn)
	    panic("seqnos_memory_object_terminate no vn");
	unix_master();		/* XXX */

	/* consume send rights from memory_object_init */
	consume_control(memory_control);
	kr = mach_port_deallocate(mach_task_self(), memory_object_name);
	assert(kr == KERN_SUCCESS);

	/* get rid of the receive rights this upcall returns */
	kr = mach_port_mod_refs(mach_task_self(), memory_control,
				 MACH_PORT_RIGHT_RECEIVE, -1);
	assert(kr == KERN_SUCCESS);
	kr = mach_port_mod_refs(mach_task_self(), memory_object_name,
				 MACH_PORT_RIGHT_RECEIVE, -1);
	assert(kr == KERN_SUCCESS);

	pager = vn->v_vmdata;
	if (!pager)
	    panic("seqnos_memory_object_terminate no pager");
	pager->control_port = MACH_PORT_NULL;
	pager->name_port = MACH_PORT_NULL;
	vn->v_cache_state = VC_FREE;
	wakeup(vn);
	vn_pager_add_to_wait_set(memory_object);
	/* 
	 * XXX Figure out exactly which of vrele/vhold/vget etc should
	 * XXX be used and how. When is a vnode really terminated?
	 */
	vrele(vn);
	XMM_END_SERVER();
}

kern_return_t seqnos_memory_object_copy (
	mach_port_t old_memory_object,
	mach_port_seqno_t seqno,
	mach_port_t old_memory_control,
	vm_offset_t offset,
	vm_size_t length,
	mach_port_t new_memory_object)
{
	panic("seqnos_memory_object_copy called");
	return ENOSYS;
}

kern_return_t seqnos_memory_object_data_request (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	vm_prot_t desired_access)
{
	struct vnode *vn;
	vn_pager_t pager;
	kern_return_t kr;
	int error;
	pointer_t data;
	struct ucred *cred;
	int resid;

	XMM_START_SERVER();
	DEBUG_PRINT(("seqnos_memory_object_data_request(x%x o=x%x l=x%x a=x%x) start\n",
		     memory_object, offset, length, desired_access));
	vn = port_to_vnode_lookup(memory_object, seqno);
	unix_master();		/* XXX */
	pager = vn->v_vmdata;

	if (!vn || !pager) {
		panic("seqnos_memory_object_data_request");
		return KERN_FAILURE;
	}
	assert(MACH_PORT_VALID(pager->control_port));
	assert(pager->control_port == memory_control);
	assert(pager->page_size == length);

	kr = vm_allocate(mach_task_self(), &data, length, TRUE);
	assert(kr == KERN_SUCCESS);

	cred = p->p_ucred;

	/* vn_rdwr locks the vnode */
	error = vn_pageinout(UIO_READ, vn, (caddr_t) data, length,
			     (off_t) offset, UIO_SYSSPACE, IO_UNIT,
			     cred, &resid, p);

	if (error == 0) {

		DEBUG_PRINT(("supplying x%x (%x %x %x %x %x %x %x %x) re=%x\n",
			     memory_object,
			     memory_control,
			     offset,
			     data,
			     length,
			     TRUE,
			     VM_PROT_WRITE,
			     FALSE,
			     memory_object,
			     resid));
		/* XXX could grant write access right away in most cases */
		/* XXX Check v_cache_state for lock value! */
		kr = memory_object_data_supply(memory_control,
					       offset,
					       data,
					       length,
					       TRUE,
					       VM_PROT_WRITE,
					       FALSE,
					       memory_object);
		assert(kr == KERN_SUCCESS);
	} else if (TRUE) {
		kr = vm_deallocate(mach_task_self(), data, length);
		assert(kr == KERN_SUCCESS);
		/* 
		 * XXX Some mmap flags might require zero filled
		 * XXX memory. Put a flag in pager struct and check
		 * XXX the flag (and error) here and decide which
		 * XXX reply to send (m_o_d_unavailable or m_o_d_error).
		 */
		printf("seqnos_memory_object_data_request: vn_pageinout: x%x",
		       error);
		printf("m_o_data_error x%x (%x %x %x %x \"%s\")\n",
		       memory_object, memory_control, offset, length,
		       error, mach_error_string(error));
		kr = memory_object_data_error(memory_control,
					      offset,
					      length,
					      error);
		assert(kr == KERN_SUCCESS);
	} else {
		kr = vm_deallocate(mach_task_self(), data, length);
		assert(kr == KERN_SUCCESS);

		printf("m_o_data_unavailable x%x (%x %x %x) vn_pageinou=x%x\n",
		       memory_object, memory_control, offset, length, error);
		kr = memory_object_data_unavailable(memory_control,
						    offset,
						    length);
		assert(kr == KERN_SUCCESS);
	}
	consume_control(memory_control);
	DEBUG_PRINT(("memory_data_request(x%x) end\n", memory_object));
	XMM_END_SERVER();
}

kern_return_t seqnos_memory_object_data_unlock (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	vm_prot_t desired_access)
{
	struct vnode *vn;
	vn_pager_t pager;
	kern_return_t kr;
	vm_prot_t lock_value = VM_PROT_NO_CHANGE;
	int s;

	XMM_START_SERVER();
	DEBUG_PRINT(("m_o_data_unlock(x%x o=x%x l=x%x, a=x%x) start\n",
		     memory_object, offset, length, desired_access));
	vn = port_to_vnode_lookup(memory_object, seqno);
	unix_master();		/* XXX */
	pager = vn->v_vmdata;

	if (!vn || !pager)
	    panic("seqnos_memory_object_data_unlock");
	assert(MACH_PORT_VALID(pager->control_port));
	assert(pager->control_port == memory_control);
	assert(pager->page_size == length);

	/* wait for ongoing transitions to complete */
	if (vn->v_cache_state == VC_MO_CLEANING
	    || vn->v_cache_state == VC_MO_FLUSHING)
	{
		s = splbio();		/* tsleep requires spl */
		while (vn->v_cache_state == VC_MO_CLEANING
		       || vn->v_cache_state == VC_MO_FLUSHING)
		{
			/* spl might be required? */
			tsleep(vn, 0, "m_o_data_unlock", 0);
		}
		splx(s);
	}

	/* User attempts to write memory */
	if (desired_access & VM_PROT_WRITE) {
		switch (vn->v_cache_state) {
		      case VC_FREE:
		      case VC_READ:
			lock_value = VM_PROT_NONE;
			vn->v_cache_state = VC_MO_WRITE;
			/* 
			 * wakeup is not needed as waiting happens
			 * only for VC_MO_FLUSHING and VC_MO_CLEANING
			 * and this transition does not affect that.
			 */
			break;

		      case VC_MO_WRITE:
			lock_value = VM_PROT_NONE;
			break;

		      /* Kernel always requests read access first */
		      case VC_BUF_WRITE:
			lock_value = VM_PROT_NONE;
			vn->v_cache_state = VC_MO_WRITE;
			printf ("m_o_data_unlock (require write) x%x",
				vn->v_cache_state);
			break;

		      default:
			panic ("m_o_data_unlock x%x", vn->v_cache_state);
		}
	} else {
		/* User reads or executes */
		switch (vn->v_cache_state) {
		      case VC_FREE:
		      case VC_BUF_WRITE:
			lock_value = VM_PROT_WRITE;
			vn->v_cache_state = VC_READ;
			break;

		      case VC_READ:
		      case VC_MO_WRITE:
			lock_value = VM_PROT_WRITE;
			break;

		      default:
			panic ("m_o_data_unlock (require read) x%x",
			       vn->v_cache_state);
		}
	}
	
	/* XXX should do some sanity checking first. */
	/* Access check was done in bsd_vm_map */
	kr = memory_object_lock_request(memory_control, offset, length,
					FALSE, FALSE, lock_value,
					MACH_PORT_NULL);
	assert(kr == KERN_SUCCESS);
	consume_control(memory_control);
	DEBUG_PRINT(("m_o_data_unlock(x%x) end\n", memory_object));
	XMM_END_SERVER();
}

kern_return_t seqnos_memory_object_data_write (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t data,
	vm_size_t length)
{
	struct vnode *vn;
	vn_pager_t pager;
	kern_return_t kr;
	int error;
	struct ucred *cred;
	int resid;
	vm_size_t wlen = length;

	XMM_START_SERVER();
	DEBUG_PRINT(("seqnos_memory_object_data_write(x%x o=x%x l=x%x) start\n",
		     memory_object, offset, length));
	vn = port_to_vnode_lookup(memory_object, seqno);
	unix_master();		/* XXX */
	pager = vn->v_vmdata;

	if (!vn || !pager)
	    panic("seqnos_memory_object_data_write");
	assert(MACH_PORT_VALID(pager->control_port));
	assert(pager->control_port == memory_control);
	assert(pager->page_size == length);

	assert (vn->v_cache_state == VC_MO_WRITE
		|| vn->v_cache_state == VC_MO_CLEANING);

	cred = p->p_ucred;

	/* 
	 * Writing does not do boundary checking but instead enlarges
	 * the file. So we do the boundary checking here.
	 */
	if (offset > pager->size) {
		printf("m_o_data_write: pageout x%x + x%x outside file end x%lx dropped.\n", offset, length, pager->size);
	} else {
		wlen = pager->size - offset;
		if (offset + length > pager->size) {
			printf("m_o_data_write: truncating pageout x%x + x%x -> x%x (size=xl%x)\n", offset, length, wlen, pager->size);
		}

		error = vn_pageinout(UIO_WRITE, vn, (caddr_t) data, wlen,
				     offset, UIO_SYSSPACE, IO_UNIT, cred,
				     &resid, p);

		if (error) {
			printf("memory_object_data_write: vn_pageinout failed: %s (x%x)");
			warning_panic("pageout dropped");
		} 
	}
	kr = vm_deallocate(mach_task_self(), data, length);
	assert(kr == KERN_SUCCESS);
	consume_control(memory_control);
	DEBUG_PRINT(("memory_object_data_write(x%x) end\n", memory_object));
	XMM_END_SERVER();
}

kern_return_t seqnos_memory_object_lock_completed (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length)
{
	struct vnode *vn;
	vn_pager_t pager;

	XMM_START_SERVER();
	DEBUG_PRINT(("seqnos_memory_object_lock_completed(x%x o=x%x l=x%x) start\n", memory_object, offset, length));
	vn = port_to_vnode_lookup(memory_object, seqno);
	unix_master();		/* XXX */
	pager = vn->v_vmdata;

	if (!vn || !pager)
	    panic("seqnos_memory_object_lock_completed");
	assert(MACH_PORT_VALID(pager->control_port));
	assert(pager->control_port == memory_control);

	switch (vn->v_cache_state) {
	case VC_MO_CLEANING:
	  vn->v_cache_state = VC_READ;
	  break;
	case VC_MO_FLUSHING:
	  vn->v_cache_state = VC_FREE;
	  break;
	default:
	  panic ("memory_object_lock_completed: v_cache_state x%x",
		 vn->v_cache_state);
	}
	wakeup (vn);

	consume_control(memory_control);
	DEBUG_PRINT(("memory_object_lock_completed(x%x) end\n",
		     memory_object));
	XMM_END_SERVER();
}

kern_return_t seqnos_memory_object_supply_completed (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	kern_return_t result,
	vm_offset_t error_offset)
{
	struct vnode *vn;

	/* XXX There is no reason this should be called */
	vn = port_to_vnode_lookup(memory_object, seqno);
	assert(result == KERN_SUCCESS);
	consume_control(memory_control);
	return KERN_SUCCESS;
}

kern_return_t seqnos_memory_object_data_return (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt,
	boolean_t dirty,
	boolean_t kernel_copy)
{
	panic("seqnos_memory_object_data_return unimplemented");
	consume_control(memory_control);
	return ENOSYS;
}

kern_return_t seqnos_memory_object_change_completed (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	boolean_t may_cache,
	memory_object_copy_strategy_t copy_strategy)
{
	struct vnode *vn;

	XMM_START_SERVER();
	vn = port_to_vnode_lookup(memory_object, seqno);
	unix_master();		/* XXX */
	printf("seqnos_memory_object_change_completed(%x %x %x) vn=x%x\n",
	       memory_object, may_cache, copy_strategy, vn);
	XMM_END_SERVER();
}

#if OSFMACH3
kern_return_t seqnos_memory_object_rejected (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	kern_return_t reason)
{
	panic("seqnos_memory_object_rejected unimplemented");
	consume_control(memory_control);
	return ENOSYS;
}

kern_return_t seqnos_memory_object_synchronize (
	mach_port_t memory_object,
	mach_port_seqno_t seqno,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	vm_sync_t sync_flags)
{
	panic("seqnos_memory_object_synchronize unimplemented");
	consume_control(memory_control);
	return ENOSYS;
}
#endif /* OSFMACH3 */
