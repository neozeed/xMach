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
 * $Log: bsd_msg.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:17:03  law
 * lites-950323 from jvh.
 *
 * Revision 2.1  92/04/21  17:10:46  rwd
 * BSDSS
 * 
 *
 */

/*
 * Request and reply message for generic BSD kernel call.
 */

#include <mach/boolean.h>
#include <mach/message.h>

#if UNTYPED_IPC

#define BSD_MSG_DATA_SIZE       (8192*2+1024)

struct	bsd_request {
    mach_msg_header_t	hdr;
    NDR_record_t NDR;
    integer_t		rval2;
    integer_t		syscode;	/* Really only int32 */
    integer_t		arg[10];
    mach_msg_format_0_trailer_t trailer;
};

struct bsd_reply {
    mach_msg_header_t	hdr;
    NDR_record_t	NDR;
    integer_t		rval[2];
    kern_return_t	retcode;	/* unnecessary */
    boolean_t		interrupt;	/* unnecessary */
    char		data[BSD_MSG_DATA_SIZE];
};

struct bsd_reply_body {
    mach_msg_header_t	hdr;
    mach_msg_body_t	body;	/* count of ool descriptors */
    /* more follows */
};
    
struct bsd_reply_simple {
    mach_msg_header_t	hdr;
    NDR_record_t	NDR;
    integer_t		rval[2];
    kern_return_t	retcode;	/* unnecessary */
    boolean_t		interrupt;	/* unnecessary */
    /* more follows */
};
    
struct data_desc {
	vm_offset_t addr;	/* user address */
	vm_size_t size;		/* size of data */
};

union bsd_msg {
    struct bsd_request	req;
    struct bsd_reply	rep;
    mach_msg_base_t	rep_base;
    struct bsd_reply_simple rep_simple;
};
#else /* UNTYPED_IPC */

struct	bsd_request {
    mach_msg_header_t	hdr;
    mach_msg_type_t	int_type;	/* integer_t[12] */
    integer_t		rval2;
    integer_t		syscode;	/* Really only int32 */
    integer_t		arg[10];
};

struct	bsd_reply {
    mach_msg_header_t	hdr;
    mach_msg_type_t	int_type;	/* integer_t[2]+int[2] */
    integer_t		rval[2];
    kern_return_t	retcode;
    boolean_t		interrupt;
};

struct data_desc {
	mach_msg_type_t addr_type;
	vm_offset_t addr;
	mach_msg_type_long_t data_type;
	vm_offset_t data;
};

union bsd_msg {
    struct bsd_request	req;
    struct bsd_reply	rep;
    char		msg[8192];
};
#endif /* UNTYPED_IPC */

#define	BSD_REQ_MSG_ID		100000
