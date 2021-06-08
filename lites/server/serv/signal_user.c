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
 * $Log: signal_user.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:59  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	serv/signal_user.,c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *	Origin:	Edited from MiG generated code.
 */
/* 
 * I coouldn't get MiG to generate the correct code (MACH_SEND_TIMEOUT)
 * so I fixed it by hand. jvh 5/94
 */
#include "osfmach3.h"
#include <serv/server_defs.h>

#define msgh_request_port	msgh_remote_port
#define msgh_reply_port		msgh_local_port

#if OSFMACH3

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/rpc.h>
#include <mach/port.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <serv/bsd_types.h>
#include <mach_debug/mach_debug_types.h>




/* SimpleRoutine signal_notify */
kern_return_t signal_notify(
	mach_port_t sigport,
	mach_port_t thread)
{
    {
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t thread;
		/* end of the kernel processed data */
	} Request;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	union {
		Request In;
	} Mess;

	register Request *InP = &Mess.In;



	InP->msgh_body.msgh_descriptor_count = 1;
	InP->thread.name = thread;
	InP->thread.disposition = 17;
	InP->thread.type = MACH_MSG_PORT_DESCRIPTOR;

	InP->Head.msgh_bits = MACH_MSGH_BITS_COMPLEX|
		MACH_MSGH_BITS(19, 0);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = sigport;
	InP->Head.msgh_reply_port = MACH_PORT_NULL;
	InP->Head.msgh_id = 102000;

	return mach_msg_overwrite(&InP->Head, MACH_SEND_MSG|MACH_SEND_TIMEOUT, sizeof(Request), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL, (mach_msg_header_t *) 0, 0);
    }
}

#else /* OSFMACH3 */

/* SimpleRoutine signal_notify */
kern_return_t signal_notify(
	mach_port_t sigport,
	mach_port_t thread)
{
	typedef struct {
		mach_msg_header_t Head;
		mach_msg_type_t threadType;
		mach_port_t thread;
	} Request;

	union {
		Request In;
	} Mess;

	register Request *InP = &Mess.In;


	static mach_msg_type_t threadType = {
		/* msgt_name = */		MACH_MSG_TYPE_MOVE_SEND,
		/* msgt_size = */		32,
		/* msgt_number = */		1,
		/* msgt_inline = */		TRUE,
		/* msgt_longform = */		FALSE,
		/* msgt_deallocate = */		FALSE,
		/* msgt_unused = */		0
	};

	InP->threadType = threadType;

	InP->thread = thread;

	InP->Head.msgh_bits = MACH_MSGH_BITS_COMPLEX|
		MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = sigport;
	InP->Head.msgh_reply_port = MACH_PORT_NULL;
	InP->Head.msgh_seqno = 0;
	InP->Head.msgh_id = 102000;

	return mach_msg(&InP->Head, MACH_SEND_MSG|MACH_SEND_TIMEOUT, 32, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
}

#endif /* OSFMACH3 */
