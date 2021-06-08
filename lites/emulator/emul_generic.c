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
 * $Log: emul_generic.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:28  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	emulator/emul_generic.c
 *	Author:	Randall W. Dean
 *	Date:	1992
 *
 *	Generic system call RPC.
 */

#include <e_defs.h>
#include <mach.h>
#include <mach/message.h>
#include <mach/msg_type.h>

/*
 * Generic emulated system call.
 */
struct bsd_request bsd_req_template = {
    {
	MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE),
					/* msgh_bits */
	0,				/* msgh_size */
	MACH_PORT_NULL,			/* msgh_remote_port */
	MACH_PORT_NULL,			/* msgh_local_port */
	0,				/* msgh_seqno (msgh_reserved in OSF) */
	BSD_REQ_MSG_ID			/* msgh_id */
    },
#if UNTYPED_IPC
    { 0, },
#else
    {
	MACH_MSG_TYPE_INTEGER_T,	/* msgt_name */
	sizeof(integer_t)*8,		/* msgt_size */
	12,				/* msgt_number */
	TRUE,				/* msgt_inline */
	FALSE,				/* msgt_longform */
	FALSE,				/* msgt_deallocate */
	0				/* msgt_unused */
    },
#endif /* UNTYPED_IPC */
    0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 },
};

#if UNTYPED_IPC

kern_return_t
emul_generic(
	mach_port_t	serv_port,
	int		syscode,
	integer_t	* argp,
	integer_t	* rvalp)
{
  	kern_return_t	error;
	mach_port_t	reply_port;
	union bsd_msg	bsd_msg;

	bsd_msg.req = bsd_req_template;
	bsd_msg.req.NDR = NDR_record; /* no constant initializer in headers */

	reply_port = mig_get_reply_port();
	bsd_msg.req.hdr.msgh_remote_port = serv_port;
	bsd_msg.req.hdr.msgh_local_port  = reply_port;

	bsd_msg.req.syscode	= (integer_t)syscode;
	bsd_msg.req.rval2	= rvalp[1];
	bsd_msg.req.arg[0]	= argp[0];
	bsd_msg.req.arg[1]	= argp[1];
	bsd_msg.req.arg[2]	= argp[2];
	bsd_msg.req.arg[3]	= argp[3];
	bsd_msg.req.arg[4]	= argp[4];
	bsd_msg.req.arg[5]	= argp[5];
	bsd_msg.req.arg[6]	= argp[6];
	bsd_msg.req.arg[7]	= argp[7];
	bsd_msg.req.arg[8]	= argp[8];
	bsd_msg.req.arg[9]	= argp[9];

	error = mach_msg(&bsd_msg.req.hdr, MACH_SEND_MSG|MACH_RCV_MSG,
			 sizeof bsd_msg.req, sizeof bsd_msg, reply_port,
			 MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

	if (error == MACH_SEND_INVALID_REPLY || error == MACH_RCV_INVALID_NAME)
	    mig_dealloc_reply_port(reply_port);
	else
	    mig_put_reply_port(reply_port);

	if (error != MACH_MSG_SUCCESS) {
	    return (error);
	}

	error = bsd_msg.rep.retcode;

	if (error == 0) {

	    char			*start;
	    char			*end;
	    vm_size_t			size;
	    struct data_desc 		*dd;

	    vm_address_t	user_addr, msg_addr;

	    /*
	     * Pass return values back to caller.
	     */
	    rvalp[0] = bsd_msg.rep.rval[0];
	    rvalp[1] = bsd_msg.rep.rval[1];

	    start = bsd_msg.rep.data;
	    /*
	     * Scan reply message for data to copy
	     */
	    end   = (char *)&bsd_msg + bsd_msg.rep_simple.hdr.msgh_size;
	    while (end > start) {

		dd = (struct data_desc *) start;

		user_addr = dd->addr;
		start = (char *) (dd + 1);
		size = dd->size;

		bcopy(start, (char *)user_addr, size);
		start += size;
		/* data is rounded to int-size */
		start = (char *) ( ((vm_offset_t)start + sizeof(int) - 1)
				  & ~(sizeof(int) - 1) );
	    }
	}

	return error;
}

#else /* UNTYPED_IPC */

kern_return_t
emul_generic(
	mach_port_t	serv_port,
	int		syscode,
	integer_t	* argp,
	integer_t	* rvalp)
{
  	kern_return_t	error;
	mach_port_t	reply_port;
	union bsd_msg	bsd_msg;

	bsd_msg.req = bsd_req_template;

	reply_port = mig_get_reply_port();
	bsd_msg.req.hdr.msgh_remote_port = serv_port;
	bsd_msg.req.hdr.msgh_local_port  = reply_port;

	bsd_msg.req.syscode	= (integer_t)syscode;
	bsd_msg.req.rval2	= rvalp[1];
	bsd_msg.req.arg[0]	= argp[0];
	bsd_msg.req.arg[1]	= argp[1];
	bsd_msg.req.arg[2]	= argp[2];
	bsd_msg.req.arg[3]	= argp[3];
	bsd_msg.req.arg[4]	= argp[4];
	bsd_msg.req.arg[5]	= argp[5];
	bsd_msg.req.arg[6]	= argp[6];
	bsd_msg.req.arg[7]	= argp[7];
	bsd_msg.req.arg[8]	= argp[8];
	bsd_msg.req.arg[9]	= argp[9];

	error = mach_msg(&bsd_msg.req.hdr, MACH_SEND_MSG|MACH_RCV_MSG,
			 sizeof bsd_msg.req, sizeof bsd_msg, reply_port,
			 MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

	if (error == MACH_SEND_INVALID_REPLY || error == MACH_RCV_INVALID_NAME)
	    mig_dealloc_reply_port(reply_port);
	else
	    mig_put_reply_port(reply_port);

	if (error != MACH_MSG_SUCCESS) {
	    return (error);
	}

	error = bsd_msg.rep.retcode;

	if (error == 0) {

	    char			*start;
	    char			*end;
	    mach_msg_type_long_t	*tp;
	    vm_size_t			size;
	    struct data_desc 		*dd;

	    vm_address_t	user_addr, msg_addr;

	    /*
	     * Pass return values back to caller.
	     */
	    rvalp[0] = bsd_msg.rep.rval[0];
	    rvalp[1] = bsd_msg.rep.rval[1];

	    /*
	     * Scan reply message for data to copy
	     */
	    start = (char *)&bsd_msg + sizeof(struct bsd_reply);
	    end   = (char *)&bsd_msg + bsd_msg.rep.hdr.msgh_size;
	    while (end > start) {

		if (end - start < sizeof(mach_msg_type_t) + sizeof(vm_address_t) + sizeof(mach_msg_type_t)) {
			e_emulator_error("emul_generic: reply message size messup: skipping end - start = x%x\n", end - start);
			break;
		}

#ifdef alpha
		/*
		 * Descriptor for address
		 * Do some rounding to make it work on the Alpha.
		 */
		start = (char *) ((vm_offset_t)start
				  & ~(sizeof(vm_offset_t)-1));
		/* 
		 * On the i386, on the other hand, the user might
		 * leave the stack unaligned.  This might result in
		 * &bsd_msg being unaligned and thus the rounding
		 * breaking things.  I don't know how to make the
		 * compiler align the stack (if nothing else, for
		 * performance).  Should this be ifdef alpha or ifndef
		 * i386 I don't know.  Enable this code fragment for
		 * machines that need it.
		 */
#endif

		dd = (struct data_desc *) start;

		/*
		 * Address
		 */
		user_addr = dd->addr;

		/*
		 * Data - size is in bytes
		 */
		tp = &dd->data_type;
		start = (char *) tp;
		if (tp->msgtl_header.msgt_longform) {
		    size = tp->msgtl_number;
		    start += sizeof(mach_msg_type_long_t);
		} else {
		    size = tp->msgtl_header.msgt_number;
		    start += sizeof(mach_msg_type_t);
		}

		if (tp->msgtl_header.msgt_inline) {
		    bcopy(start, (char *)user_addr, size);
		    start += size;
		    /* data is rounded to int-size */
		    start = (char *) ( ((vm_offset_t)start + sizeof(int) - 1)
				      & ~(sizeof(int) - 1) );
		} else {
		    msg_addr = *(vm_address_t *)start;
		    start += sizeof(vm_address_t);
		    bcopy((char *)msg_addr, (char *)user_addr, size);
		    (void) vm_deallocate(mach_task_self(), msg_addr, size);
		}
	    }
	}

	return (error);
}

#endif /* UNTYPED_IPC */
