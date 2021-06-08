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
 * 4-Feb-95  Johannes Helander (jvh) at Helsinki University of Technology
 *	Picked up 64 bit fixes from Alessandro Forin (2/93):
 *
 * 	Rewritten most of the alignment logic for 64bit archies.
 * 	It is sad to think that *without* a proper structure
 * 	definition for the data_desc thing the code would have
 * 	been right already.  Quintuple sigh.
 * 	Well, at least it is readable (except for the macros I
 * 	had to add :-((.
 *
 * $Log: user_reply_msg.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:17:00  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	serv/user_reply_msg.c
 *	Author:	Randall W. Dean
 *	Date:	1992
 *
 *	Routines to extend generic reply message to handle data
 *	intended for user.
 */

#include <map_uarea.h>

#include <serv/import_mach.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/proc.h>

#include <serv/bsd_msg.h>

#define	BSD_REP_DATA_SIZE	(sizeof(struct data_desc))

#define	BSD_REP_DATA_MAX	(2*16) /* XXX - BSD allows at most 16 iovs */


#if UNTYPED_IPC

/*
 * NOTE: These macros do not support 64 bit machines.
 *       The messages do not have trailers.
 *	 There will never be any out-of-line data here.
 */

/* Get the next descriptor at the end of the current msg */
#define	get_desc(_d_,_m_)				  		  \
MACRO_BEGIN								  \
	  _d_ = (struct data_desc *) ((char *) (_m_) 			  \
				+ (_m_)->msgh_size); 			  \
MACRO_END

#define	incr_msg_size(_m_,_n_,_d_)					  \
MACRO_BEGIN								  \
	register int _nn_ = _n_;					  \
	/* Make sure next allocated data desc is aligned. */		  \
	(_nn_) = ((_nn_) + sizeof(natural_t) - 1) &		  	  \
		 ~ (sizeof(natural_t) - 1);			  	  \
									  \
	(_m_)->msgh_size += (sizeof *(_d_) + (_nn_));		 	  \
MACRO_END

#define DESCRIPTOR_DATA_ADDR(_d_) ((vm_offset_t)((char *)(_d_)		  \
				    +BSD_REP_DATA_SIZE))

/* The limit on inline data we can allow, lest we run out of space. */

/* XXX Does not count possible ool descriptors inside data area */

#define	BSD_REP_SIZE_MAX	(BSD_MSG_DATA_SIZE - \
				 BSD_REP_DATA_MAX * BSD_REP_DATA_SIZE)

/*
 *	extend_reply_msg reserves space in the reply message for
 *	"size" bytes of data, eventually to be copied to "user_addr"
 *	in the client.
 *
 *	Not all of the reserved space must be used immediately.
 *	extend_current_output should be used to mark space as used.
 *
 *	finish_reply_msg should be used to finalize the msg,
 *	discarding unused memory and setting the msg's size correctly.
 */

void
finish_data_desc(
	mach_msg_header_t *msg,
	struct data_desc *dd)
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	vm_size_t number = dd->size;

	if (number > 0)
	  incr_msg_size(msg,number,dd);

	pk->k_current_size = 0;
}

vm_offset_t
extend_reply_msg(
	vm_offset_t	user_addr,
	vm_size_t	size,
	vm_size_t	total_size)
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	mach_msg_header_t *msg = pk->k_reply_msg;
	struct data_desc *dd;
	vm_offset_t addr;

	if (size == 0)
		return 0;

	/*
	 *	Is there a preexisting data descriptor that we can use?
	 */

	if (pk->k_current_size != 0) {
		vm_size_t used;

		get_desc(dd,msg);

		used = dd->size;

		if ((user_addr == dd->addr + used) &&
		    (used + size <= pk->k_current_size)) {
			/* yes, we can use this data descriptor */

			addr = DESCRIPTOR_DATA_ADDR (dd);

			return addr + used;
		}

		finish_data_desc(msg, dd);
	}

	/*
	 *	We have to allocate a new data descriptor.
	 *	If possible, we try to make it inline.
	 */

	if (msg->msgh_size + BSD_REP_DATA_SIZE > BSD_MSG_DATA_SIZE)
		panic("extend_reply_msg");

	get_desc (dd,msg);

	dd->addr = user_addr;
	dd->size = 0;

	/*
	 *	We don't let the amount of inline data in the message
	 *	exceed BSD_REP_SIZE_MAX.  This ensures that there
	 *	will be enough space in the message for data descriptors.
	 *	We use msg->msgh_size as a conservative estimate
	 *	of the current amount of inline data.
	 */

	if (msg->msgh_size + size > BSD_REP_SIZE_MAX) {
	  	panic ("extend_reply_msg: 0x%x bytes require OOL memory",
		       size, size);
	} else {
		addr = DESCRIPTOR_DATA_ADDR (dd);
		pk->k_current_size = size;
	}

	return addr;
}

void
extend_current_output(size)
	vm_size_t	size;
{
	proc_invocation_t pk = get_proc_invocation();

	if (size != 0) {
		register mach_msg_header_t *msg = pk->k_reply_msg;
		register struct data_desc *dd;

		get_desc(dd,msg);

		dd->size += size;
	}
}

void
finish_reply_msg()
{
	proc_invocation_t pk = get_proc_invocation();

	/*
	 *	If we were working on a data descriptor,
	 *	it must be finalized.
	 */

	if (pk->k_current_size != 0) {
		mach_msg_header_t *msg = pk->k_reply_msg;
		struct data_desc *dd;

		get_desc(dd,msg);

		finish_data_desc(msg, dd);
	}
}

int
moveout(data, user_addr, count)
	vm_offset_t data;
	vm_offset_t user_addr;
	vm_size_t count;
{
	copyout (data, user_addr, count);

	(void) vm_deallocate(mach_task_self(), data, count);

	return 0;
}

#else /* UNTYPED_IPC */

mach_msg_type_long_t	data_type_ooline = {
	{
	    0,		/* name - unused */
	    0,		/* size - unused */
	    0,		/* number - unused */
	    FALSE,	/* not inline */
	    TRUE,	/* longform */
	    TRUE,	/* deallocate */
	    0		/* unused */
	},
	MACH_MSG_TYPE_CHAR,	/* name */
	8,			/* size */
	0			/* number - fill in */
};

mach_msg_type_long_t	data_type_inline = {
	{
	    0,		/* name - unused */
	    0,		/* size - unused */
	    0,		/* number - unused */
	    TRUE,	/* inline */
	    TRUE,	/* longform */
	    FALSE,	/* NOT deallocate */
	    0		/* unused */
	},
	MACH_MSG_TYPE_CHAR,	/* name */
	8,			/* size */
	0			/* number - fill in */
};

mach_msg_type_t		addr_type = {
	MACH_MSG_TYPE_INTEGER_T,
	sizeof(integer_t)*8,
	1,
	TRUE,		/* inline */
	FALSE,
	FALSE,
	0
};

/*
 *	Chicaneries for 64bit machines.  There is no reason
 *	for a data_desc.addr_type to be on a longword boundary,
 *	but the compiler wants that because of the pointer
 *	that follows it (it _always_ pads _after_ the type).
 *	[ macros cuz they reduce to a single stmt if 32bit ]
 */
#define	get_desc(d,m)					  		  \
MACRO_BEGIN								  \
	if (sizeof(vm_offset_t) > sizeof(mach_msg_type_t)) {		  \
		vm_offset_t _ptr_ = (vm_offset_t) (m) + (m)->msgh_size;	  \
		if (_ptr_ & sizeof(mach_msg_type_t))			  \
			_ptr_ -= sizeof(mach_msg_type_t);		  \
		d = (struct data_desc *) _ptr_;				  \
	} else {							  \
		d = (struct data_desc *) ((char *) (m) + (m)->msgh_size); \
	}								  \
MACRO_END

#define	set_desc_and_addr(_d_,_m_,_a_)					  \
MACRO_BEGIN								  \
	if (sizeof(vm_offset_t) > sizeof(mach_msg_type_t)) {		  \
		vm_offset_t	_vptr_;					  \
									  \
		_vptr_ = (vm_offset_t) (_m_) + (_m_)->msgh_size;	  \
		get_desc(_d_,_m_);					  \
		if ((vm_offset_t)(_d_) != _vptr_) {			  \
			*(mach_msg_type_t*)_vptr_ = _a_;		  \
		} else {						  \
			(_d_)->addr_type = _a_;				  \
		}							  \
	} else {							  \
		(_d_) = (struct data_desc *)				  \
			((char *) (_m_) + (_m_)->msgh_size);		  \
		(_d_)->addr_type = addr_type;				  \
	}								  \
MACRO_END

#define	incr_msg_size(_m_,_n_,_d_)					  \
MACRO_BEGIN								  \
	register int _nn_ = _n_;					  \
	/* Make sure next typetype is aligned.	*/			  \
	/* Long-aligned would be too much	*/			  \
	(_nn_) = ((_nn_) + sizeof(mach_msg_type_t) - 1) &		  \
		 ~ (sizeof(mach_msg_type_t) - 1);			  \
									  \
	if ((sizeof(vm_offset_t) > sizeof(mach_msg_type_t)) &&		  \
	    ((_m_)->msgh_size & sizeof(mach_msg_type_t)))		  \
		(_m_)->msgh_size += /* no padding! */			  \
			sizeof(mach_msg_type_t) +			  \
			sizeof (_d_)->addr + sizeof (_d_)->data_type +	  \
			(_nn_);						  \
	else								  \
		(_m_)->msgh_size += (_nn_) +				  \
			(sizeof *(_d_) - sizeof (_d_)->data);		  \
MACRO_END

/* The limit on inline data we can allow, lest we run out of space. */

#define	BSD_REP_SIZE_MAX	(8192 - \
				 sizeof(struct bsd_reply) - \
				 BSD_REP_DATA_MAX * BSD_REP_DATA_SIZE)

/*
 *	extend_reply_msg reserves space in the reply message for
 *	"size" bytes of data, eventually to be copied to "user_addr"
 *	in the client.
 *
 *	Not all of the reserved space must be used immediately.
 *	extend_current_output should be used to mark space as used.
 *
 *	finish_reply_msg should be used to finalize the msg,
 *	discarding unused memory and setting the msg's size correctly.
 */

void
finish_data_desc(
	mach_msg_header_t *msg,
	struct data_desc *dd)
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	vm_size_t number = dd->data_type.msgtl_number;

	/*
	 *	If the descriptor is out-of-line,
	 *	free any unused memory.  In any case,
	 *	increment the msg size if the descriptor
	 *	actually contains data.
	 */

	if (dd->data_type.msgtl_header.msgt_inline) {
		if (number > 0)
			incr_msg_size(msg,number,dd);
	} else {
		vm_size_t used = round_page(number);

		if (used < pk->k_current_size)
			(void) vm_deallocate(mach_task_self(),
					     dd->data + used,
					     pk->k_current_size - used);

		if (number > 0) {
			incr_msg_size(msg,sizeof(vm_offset_t),dd);

			/* the reply message is no longer simple */
			msg->msgh_bits |= MACH_MSGH_BITS_COMPLEX;
		}
	}

	pk->k_current_size = 0;
}

vm_offset_t
extend_reply_msg(
	vm_offset_t	user_addr,
	vm_size_t	size,
	vm_size_t	total_size)
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	mach_msg_header_t *msg = pk->k_reply_msg;
	struct data_desc *dd;
	vm_offset_t addr;

	if (size == 0)
		return 0;

	/*
	 *	Is there a preexisting data descriptor that we can use?
	 */

	if (pk->k_current_size != 0) {
		vm_size_t used;

		get_desc(dd,msg);

		used = dd->data_type.msgtl_number;

		if ((user_addr == dd->addr + used) &&
		    (used + size <= pk->k_current_size)) {
			/* yes, we can use this data descriptor */

			if (dd->data_type.msgtl_header.msgt_inline)
				addr = (vm_offset_t) &dd->data;
			else
				addr = dd->data;

			return addr + used;
		}

		finish_data_desc(msg, dd);
	}

	/*
	 *	We have to allocate a new data descriptor.
	 *	If possible, we try to make it inline.
	 */

	if (msg->msgh_size + sizeof(struct data_desc) > 8192)
		panic("extend_reply_msg");

	set_desc_and_addr(dd,msg,addr_type);

	dd->addr = user_addr;

	/*
	 *	We don't let the amount of inline data in the message
	 *	exceed BSD_REP_SIZE_MAX.  This ensures that there
	 *	will be enough space in the message for data descriptors.
	 *	We use msg->msgh_size as a conservative estimate
	 *	of the current amount of inline data.
	 */

	if (msg->msgh_size + size > BSD_REP_SIZE_MAX) {
		/* must use out-of-line memory */

		size = round_page(size);
		(void) vm_allocate(mach_task_self(), &addr, total_size, TRUE);
		dd->data_type = data_type_ooline;
		dd->data = addr;
		pk->k_current_size = total_size;
	} else {
		dd->data_type = data_type_inline;
		addr = (vm_offset_t) &dd->data;
		pk->k_current_size = size;
	}

	return addr;
}

void
extend_current_output(size)
	vm_size_t	size;
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;

	if (size != 0) {
		register mach_msg_header_t *msg = pk->k_reply_msg;
		register struct data_desc *dd;

		get_desc(dd,msg);

		dd->data_type.msgtl_number += size;
	}
}

void
finish_reply_msg()
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;

	/*
	 *	If we were working on a data descriptor,
	 *	it must be finalized.
	 */

	if (pk->k_current_size != 0) {
		register mach_msg_header_t *msg = pk->k_reply_msg;
		register struct data_desc *dd;

		get_desc(dd,msg);

		finish_data_desc(msg, dd);
	}
}

int
moveout(data, user_addr, count)
	vm_offset_t data;
	vm_offset_t user_addr;
	vm_size_t count;
{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;
	register mach_msg_header_t *msg = pk->k_reply_msg;
	register struct data_desc *dd;

#if	MAP_UAREA
	if (msg == 0) {
		bcopy(data, user_addr, count);
		(void) vm_deallocate(mach_task_self(), data, count);
		return 0;
	}
#endif	MAP_UAREA

	finish_reply_msg();

	/*
	 *	We have to add a new out-of-line data descriptor.
	 *	This is like a combined extend_reply_msg,
	 *	extend_current_output, finish_reply_msg operation.
	 */

	if (msg->msgh_size + sizeof *dd > 8192)
		panic("moveout");

	set_desc_and_addr(dd,msg,addr_type);

	dd->addr = user_addr;
	dd->data_type = data_type_ooline;
	dd->data_type.msgtl_number = count;
	dd->data = data;

	incr_msg_size(msg,sizeof(vm_offset_t),dd);

	/* the reply message is no longer simple */
	msg->msgh_bits |= MACH_MSGH_BITS_COMPLEX;

	return 0;
}
#endif /* UNTYPED_IPC */
