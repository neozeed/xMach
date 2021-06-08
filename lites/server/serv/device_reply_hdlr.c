/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1994 Johannes Helander
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND JOHANNES HELANDER ALLOW FREE USE OF THIS
 * SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND JOHANNES
 * HELANDER DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * $Log: device_reply_hdlr.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:54  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	server/serv/device_reply_hdlr.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Handler for device read and write replies.  Simulates an
 *	interrupt handler.
 */
#include <serv/server_defs.h>

#include <sys/cmu_queue.h>
#include <sys/zalloc.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/proc.h>

#include <serv/import_mach.h>
#include <serv/device.h>

mach_port_t	reply_port_set;

void	device_reply_loop(void);	/* forward */

extern void	ux_create_thread();

void add_to_reply_port_set(mach_port_t port)
{
	kern_return_t kr;
	kr = mach_port_move_member(mach_task_self(), port, reply_port_set);
	assert(kr == KERN_SUCCESS);
}


void device_reply_hdlr()
{
	register int	i;

	(void) mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET,
				  &reply_port_set);
	ux_create_thread(device_reply_loop);
}

void
device_reply_loop()
{
	kern_return_t		rc;

	union reply_msg {
	    mach_msg_header_t	hdr;
#if OSFMACH3
	    char		space[8192 + MAX_TRAILER_SIZE];
#else /* OSFMACH3 */
	    char		space[8192];
#endif /* OSFMACH3 */
	} reply_msg;
	struct proc *p;

	/*
	 * We KNOW that none of these messages have replies...
	 */

	mig_reply_header_t	rep_rep_msg;

	system_proc(&p, "DeviceReply");

	/*
	 * Wire this cthread to a kernel thread so we can
	 * up its priority
	 */
	cthread_wire();

	/*
	 * Make this thread high priority.
	 */
	set_thread_priority(mach_thread_self(), 2);

	for (;;) {
#if OSFMACH3
	    rc = mach_msg(&reply_msg.hdr,
	  MACH_RCV_MSG | MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_SEQNO),
			  0,
			  sizeof reply_msg - MAX_TRAILER_SIZE,
			  reply_port_set,
			  MACH_MSG_TIMEOUT_NONE,
			  MACH_PORT_NULL);
#else /* OSFMACH3 */
	    rc = mach_msg(&reply_msg.hdr, MACH_RCV_MSG,
			  0, sizeof reply_msg, reply_port_set,
			  MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
#endif /* OSFMACH3 */
	    if (rc == MACH_MSG_SUCCESS) {
		if (seqnos_device_reply_server(&reply_msg.hdr,
					&rep_rep_msg))
		{
		    /*
		     * None of these messages need replies
		     */
		} else {
			panic("device_reply_loop: msg not handled");
		}
	    }
	}
}

kern_return_t seqnos_device_open_reply(
	mach_port_t	reply_port,
	mach_port_seqno_t seqno,
	kern_return_t	return_code,
	mach_port_t	device_port)
{
	panic("device_open_reply");
	/* NOT USED */
	return EOPNOTSUPP;
}

kern_return_t
seqnos_device_write_reply(
	mach_port_t	reply_port,
	mach_port_seqno_t seqno,
	kern_return_t	return_code,
	int		bytes_written)
{
	struct buf *bp;

	bp = (struct buf *) port_object_receive_lookup(reply_port, seqno,
						       POT_IO_BUFFER);
	if (!bp)
	    panic("seqnos_device_write_reply: no buf");

	return bio_write_reply(bp, return_code, bytes_written);
}

kern_return_t seqnos_device_write_reply_inband(
	mach_port_t	reply_port,
	mach_port_seqno_t seqno,
	kern_return_t	return_code,
	int		bytes_written)
{
	void *object;
	port_object_type_t pot;

	object = port_object_receive_lookup_any(reply_port, seqno, &pot);
	if (!object)
	    panic("seqnos_device_write_reply_inband: no object");

	if (pot == POT_TTY)
	    return tty_write_reply((struct tty *) object,
				   return_code, bytes_written);
	else if (pot == POT_CHAR_DEV)
	    return char_select_write_reply((struct char_device *) object,
					  return_code, bytes_written);
	panic("seqnos_device_write_reply_inband: bad type");
	return ENODEV;
}

kern_return_t seqnos_device_read_reply(
	mach_port_t	reply_port,
	mach_port_seqno_t seqno,
	kern_return_t	return_code,
	io_buf_ptr_t	data,
	unsigned int	data_count)
{
	struct buf *bp;

	bp = (struct buf *) port_object_receive_lookup(reply_port, seqno,
						       POT_IO_BUFFER);
	if (!bp)
	    panic("seqnos_device_read_reply: no buf");

	return bio_read_reply(bp, return_code, data, data_count);
}

kern_return_t seqnos_device_read_reply_inband(
	mach_port_t	reply_port,
	mach_port_seqno_t seqno,
	kern_return_t	return_code,
	io_buf_ptr_t	data,
	unsigned int	data_count)
{
	void *object;
	port_object_type_t pot;

	object = port_object_receive_lookup_any(reply_port, seqno, &pot);
	if (!object)
	    panic("seqnos_device_read_reply_inband: no object");

	if (pot == POT_TTY)
	    return tty_read_reply((struct tty *) object,
				  return_code, data, data_count); 
	else if (pot == POT_CHAR_DEV)
	    return char_select_read_reply((struct char_device *) object,
					  return_code, data, data_count);
	panic("seqnos_device_read_reply_inband: bad type");
	return ENODEV;
}

#include "osfmach3.h"
#if OSFMACH3
kern_return_t seqnos_device_read_reply_overwrite(
	mach_port_t	reply_port,
	mach_port_seqno_t seqno,
	kern_return_t	return_code,
	io_buf_ptr_t	data,
	unsigned int	data_count)
{
	panic("seqnos_device_read_reply_overwrite");
}
#endif /* OSFMACH3 */
