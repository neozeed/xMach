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
 * $Log: port_object.h,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:17:04  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	port_object.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	Port to object mapping with type checking.
 */

#ifndef _PORT_OBJECT_H_
#define _PORT_OBJECT_H_

/* Port object types */
typedef enum {
	POT_INVALID,
	POT_PROCESS,	/* R + S gc. terminate process. message has R */
	POT_FILE_HANDLE,/* R + S gc, untrusted, message has S */
	POT_VNODE_PAGER,/* R + S trusted. Explicit gc */
	POT_TASK,	/* S dead port gc. lazy, terminate process, untrusted*/
	POT_THREAD,	/* S lazy, untrusted */
	POT_EXCEPTION,	/* R + S, contains task and thread */
	POT_IO_BUFFER,	/* R + SO trusted, recycle */
	POT_DEVICE,	/* S dead port gc trusted */
	POT_PAGER_NAME,	/* S trusted lazy */
	POT_PAGER_CONTROL,/* S trusted lazy */
	POT_SIGPORT,	/* S gc */
	POT_TTY,	/* R + SO */
	POT_CHAR_DEV,	/* R + SO */
	POT_COUNT
    } port_object_type_t;

/* Port object states */
typedef enum {
	POS_INVALID,
	POS_RECEIVER_ACTIVE, POS_RECEIVER_SHUTDOWN, POS_RECEIVER_INACTIVE,
	POS_RECEIVER_DRAIN, POS_RECEIVER_DEAD,
	POS_SENDER_ALIVE, POS_SENDER_DEAD,
	POS_COUNT
    } port_object_state_t;

/*
  R_ACTIVE (shutdown start)-> R_SHUTDOWN (nosenders)-> R_DEAD
  R_DEAD (shutdown end)-> R_INVALID
  R_ACTIVE (nosenders)-> R_INACTIVE -> R_DEAD
  R_INACTIVE (makesend)-> R_ACTIVE
*/

/* Flags type */
typedef u_int32_t port_object_flags_t;

/* Methods the underlying object must provide */
struct port_object_methods {
	/* Add a ref. Must be locked. */
	void (*reference)(void *object);

	/* Release ref. Consumes lock */
	void (*dereference)(void *object, mach_port_t port);

	/* Lock it */
	void (*lock)(void *object);

	/*
	 * Remove reverse mapping. I.e. no more make sends will be attempted
	 * Locks and unlocks.
	 */
	void (*rm_reverse)(void *object, mach_port_t port);

	/* Port Object flags */
	port_object_flags_t flags;
};

extern const struct port_object_methods port_object_methods[POT_COUNT];

typedef struct port_object {
	queue_chain_t chain;		/* For hash queue and bad queue */
	port_object_type_t type;	/* port_object_type above */
	struct mutex lock;
	struct condition cond;
	port_object_state_t state;	/* port_object_state above */
	mach_msg_seqno_t seqno;		/* our idea of port sequence number */
	mach_port_mscount_t mscount;	/* our idea of make send count */
	natural_t socount;		/* send once rights for this port */
	mach_port_urefs_t extra_urefs;	/* for lazy consumption */
	void *object;			/* Actual object */
} *port_object_t;

#define PORT_OBJECT_NULL ((port_object_t) 0)
#define OBJECT_NULL ((void *) 0)

#define POM_REFERENCE(o, pot) port_object_methods[pot].reference(o)
#define POM_DEREFERENCE(o, pot, port) \
    port_object_methods[pot].dereference(o, port)
#define POM_LOCK(o, pot) port_object_methods[pot].lock(o)
#define POM_REMOVE_REVERSE(o, pot, name) \
    port_object_methods[pot].rm_reverse(o, name)
#define POM_FLAGS(po)  port_object_methods[po->type].flags

/* Convenience macros */
#define task_to_proc_lookup(port) \
    ((struct proc *) port_to_object_lookup(port, POT_TASK))

/* Prototypes */
void port_object_init(void);

/* Lookup and lock object with type check. Serialize on seqno */
void *port_object_receive_lookup(mach_port_t, mach_msg_seqno_t,
				 port_object_type_t);
/* Like above but no type check. Returns po type in the third argument */
void *port_object_receive_lookup_any(mach_port_t, mach_msg_seqno_t,
				     port_object_type_t *);
/* Lookup and lock object with type check. Consume right if success */
void *port_object_send_lookup(mach_port_t, port_object_type_t);

void *port_to_object_lookup(mach_port_t, port_object_type_t);
port_object_type_t port_object_type(mach_port_t);
void port_object_gc(void);

mach_error_t port_object_enter_send(mach_port_t *, port_object_type_t, void *);
mach_error_t port_object_allocate_receive(mach_port_t *, port_object_type_t,
					  void *);
/* Add a send right to a receive right */
mach_error_t port_object_make_send(mach_port_t port);

/* Shutdown an object, ie. destroy its receive right */
void port_to_object_deallocate(mach_port_t);
void port_object_shutdown(mach_port_t, boolean_t);

/* Called on no senders notification on receive right */
mach_error_t port_object_no_senders(mach_port_t, mach_port_seqno_t,
				    mach_port_mscount_t);

/* Called on dead port notification for send right */
mach_error_t port_object_dead_port(mach_port_t name);

#endif /* !_PORT_OBJECT_H_ */
