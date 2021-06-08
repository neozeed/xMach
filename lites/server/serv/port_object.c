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
 * $Log: port_object.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/08/18  18:02:49  mike
 * Add basic sanity check on sequence numbers.  If you get a bogus sequence number
 * (due to a malfunctioning RPC system :-) you can hang indefinately
 *
 * Revision 1.1.1.2  1995/03/23  01:16:57  law
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

#include <serv/server_defs.h>

/* A bogus sequence number can cause indefinately hang, give some indication */
#define SEQNO_SANITY(posn, sn) \
    if (sn - posn > 1000) { \
	warning_panic("port_object: huge seqno diff po#=x%x #=x%x", posn, sn); \
	assert(sn - posn <= 1000); \
    }

/* Maximum number of surplus urefs */
#define PORT_OBJECT_LAZY_UREFS_LIMIT 10000
/* How many to create or delete at once */
#define PORT_OBJECT_LAZY_UREFS_DELTA 2000

/* Port Object flags */
#define POF_NONE	0U

#define POF_RECEIVE	0x00000001U
	/* has receive right */

#define POF_SEND	0x00000002U
	/* Lookups are done by send right */

#define POF_SEND_ONLY	0x00000004U
	/* A send right where the receive right is external */

#define POF_GC		0x00000008U
	/* Do automatic garbage collection through notifications */

/* #define POF_TRUSTED	0x00000010U */
	/* Are send rights to be looked up provided by a trusted party? */

#define POF_CONSUME	0x00000020U
	/* Autoconsume send right on send right lookup */

#define POF_LAZY	0x00000040U
	/* Deallocate send rights lazily */

#define POF_SO		0x00000080U
	/* Send once rights are used */

#define POF_LOOKUP	0x00000100U
	/* If not set, no lookup will be done */

/* Forward decls */
static void port_object_hash_init(void);
static port_object_t port_object_hash_lookup(mach_port_t port);
static void port_object_hash_enter(port_object_t po);
static void port_object_hash_remove(mach_port_t port);
static void port_object_hash_release(mach_port_t port);
static port_object_t port_object_hash_lookup_locked(mach_port_t port);
static void port_object_drain(port_object_t po);

void null_pom_ref(void *), null_pom_deref(void *, mach_port_t);
void null_pom_lock(void *), null_pom_rm_reverse(void *, mach_port_t);

/* XXX shouldn't be here */
void file_ref(void *), file_deref(void *, mach_port_t);
void file_lock(void *), file_remove_reverse(void *, mach_port_t);
void sigport_remove_reverse(void *, mach_port_t);

/* Globals */
static zone_t port_object_zone;
static natural_t port_object_bad_allocation_count = 0;
static port_object_t port_object_bad_list = PORT_OBJECT_NULL;
static struct mutex port_object_lock
    = MUTEX_NAMED_INITIALIZER("port_object_lock");
static mach_port_t port_object_dead_port_notify_port;

const struct port_object_methods port_object_methods[POT_COUNT] = {
/*INVALID */ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		 POF_NONE},
/*PROCESS */ {proc_ref, proc_deref, proc_lock, proc_remove_reverse,
		 POF_RECEIVE | POF_GC | POF_LOOKUP},
/*FILE_HAN*/ {file_ref, file_deref, file_lock, file_remove_reverse,
		  POF_RECEIVE | POF_SEND | POF_GC | POF_CONSUME | POF_LOOKUP},
/*VN_PAGER*/ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_RECEIVE | POF_SEND | POF_LAZY |POF_LOOKUP},
/*TASK    */ {proc_ref, task_deref, proc_lock, task_remove_reverse,
		  POF_SEND_ONLY | POF_GC | POF_CONSUME | POF_LAZY |POF_LOOKUP},
/*THREAD  */ {proc_ref, proc_deref, proc_lock, thread_remove_reverse,
		  POF_SEND_ONLY | POF_GC | POF_CONSUME | POF_LAZY},
/*EXCEPTIO*/ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_RECEIVE | POF_SEND },
/*IO_BUFFE*/ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_RECEIVE | POF_SO | POF_LOOKUP},
/*DEVICE  */ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_SEND_ONLY | POF_LOOKUP},
/*PG_NAME */ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_SEND_ONLY | POF_CONSUME | POF_LAZY},
/*PG_CONTR*/ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_SEND_ONLY | POF_CONSUME | POF_LAZY},
/*SIGPORT */ {proc_ref, proc_deref, proc_lock, sigport_remove_reverse,
		  POF_SEND_ONLY | POF_GC | POF_CONSUME | POF_LAZY},
/*TTY     */ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_RECEIVE | POF_SO | POF_LOOKUP},
/*CHAR_DEV*/ {null_pom_ref, null_pom_deref, null_pom_lock, null_pom_rm_reverse,
		  POF_RECEIVE | POF_SO | POF_LOOKUP}};


boolean_t po_debug = FALSE;

void port_object_init()
{
	kern_return_t kr;

	port_object_hash_init();
	port_object_zone = zinit(sizeof(struct port_object),
				    sizeof(struct port_object) * 20000,
				    vm_page_size, TRUE, "port object");

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&port_object_dead_port_notify_port);
	assert(kr == KERN_SUCCESS);

	ux_server_add_port(port_object_dead_port_notify_port);
}

/* Null methods. Real methods are provided by object */
void null_pom_ref(void *object)
{}

void null_pom_deref(void *object, mach_port_t port)
{}

void null_pom_lock(void *object)
{}

void null_pom_rm_reverse(void *object, mach_port_t port)
{}

/* Returns type of object a port refers to */
port_object_type_t port_object_type(mach_port_t port)
{
	port_object_t po = (port_object_t) port;

	if (!MACH_PORT_VALID(port))
	    return POT_INVALID;

	return po->type;
}

/* 
 * Lookup and lock object with type check. Serialize on seqno.
 *
 * Underlying object is returned locked.
 * Internal seqno is incremented.
 */
void *port_object_receive_lookup(
	mach_port_t		port,
	mach_msg_seqno_t	seqno,
	port_object_type_t	pot)
{
	void *o;
	port_object_t po = (port_object_t) port;

	if (!MACH_PORT_VALID(port))
	    return OBJECT_NULL;

	assert(POM_FLAGS(po) & POF_RECEIVE);

	mutex_lock(&po->lock);
	while (po->seqno < seqno) {
		SEQNO_SANITY(po->seqno, seqno);
		condition_wait(&po->cond, &po->lock);
	}
	assert(po->seqno == seqno);
	po->seqno++;
	if (po->type != pot) {
		o = OBJECT_NULL;
		warning_panic("port_object_receive_lookup: type check failure");/*XXX*/
	} else {
		o = po->object;
		/* Lock underlying object */
		POM_LOCK(o, pot);
	}
	/* Wakeup threads waiting to be serialized */
	condition_broadcast(&po->cond);
	mutex_unlock(&po->lock);

	return o;
}

/* Like above but no type check. Returns po type in the third argument */
void *port_object_receive_lookup_any(
	mach_port_t		port,
	mach_msg_seqno_t	seqno,
	port_object_type_t	*pot) /* OUT */
{
	void *o;
	port_object_t po = (port_object_t) port;

	if (!MACH_PORT_VALID(port)) {
		*pot = POT_INVALID;
		return OBJECT_NULL;
	}

	assert(POM_FLAGS(po) & POF_RECEIVE);

	mutex_lock(&po->lock);
	while (po->seqno < seqno) {
		SEQNO_SANITY(po->seqno, seqno);
		condition_wait(&po->cond, &po->lock);
	}
	assert(po->seqno == seqno);
	po->seqno++;
	*pot = po->type;
	o = po->object;
	/* Lock underlying object */
	POM_LOCK(o, po->type);

	/* Wakeup threads waiting to be serialized */
	condition_broadcast(&po->cond);
	mutex_unlock(&po->lock);

	return o;
}

/* 
 * Lookup from send right received from a trusted party.
 * Perform type check.
 * Consume send right if POF_CONSUME.
 * Return underlying object locked if POF_LOOKUP.
 */

/* As above but the right came from an untrusted party and may be
 * anything. Thus use hash table.
 */
void *port_object_send_lookup(
	mach_port_t		port,
	port_object_type_t	pot)
{
	kern_return_t kr;
	void *o;
	port_object_flags_t pof;
	port_object_t po = port_object_hash_lookup(port); /* po is locked */

	if (!po)
	    return PORT_OBJECT_NULL;

	pof = POM_FLAGS(po);
	assert(pof & (POF_SEND|POF_SEND_ONLY));

	assert(po->state == POS_RECEIVER_ACTIVE
	       || po->state == POS_RECEIVER_INACTIVE
	       || po->state == POS_RECEIVER_DRAIN
	       || po->state == POS_SENDER_ALIVE);

	/* 
	 * Consume it if requested.
	 */
	if (pof & POF_CONSUME) {
		if (pof & POF_LAZY) {
			po->extra_urefs++;
			if (po->extra_urefs > PORT_OBJECT_LAZY_UREFS_LIMIT) {
				kr = mach_port_mod_refs(mach_task_self(), port,
							MACH_PORT_RIGHT_SEND,
							-PORT_OBJECT_LAZY_UREFS_DELTA);
				assert(kr == KERN_SUCCESS);
				po->extra_urefs -= PORT_OBJECT_LAZY_UREFS_DELTA;
			}
		} else {
			mach_port_deallocate(mach_task_self(), port);
		}
	}
	if (po->type != pot) {
		o = OBJECT_NULL;
		warning_panic("port_object_send_lookup: type check failure");/*XXX*/
	} else if (pof & POF_LOOKUP) {
		o = po->object;
		/* Lock underlying object */
		POM_LOCK(o, pot);
	} else {
		o = OBJECT_NULL;
	}
	if ((pof & POF_GC) && (po->state == POS_RECEIVER_DRAIN))
	    panic("port_object_send_lookup port_object_drain(po);");
	else
	    mutex_unlock(&po->lock);

	return o;
}


/* Internal: Queue bad port objects together */
void port_object_bad_object(port_object_t po)
{
	mutex_lock(&port_object_lock);
	port_object_bad_allocation_count++;
	*(port_object_t *)po = port_object_bad_list;
	port_object_bad_list = po;
	mutex_unlock(&port_object_lock);
}

/* Garbage collection: zfree all objects from the bad list */
void port_object_gc()
{
	port_object_t po, next;

	mutex_lock(&port_object_lock);
	po = port_object_bad_list;
	port_object_bad_list = PORT_OBJECT_NULL;
	mutex_unlock(&port_object_lock);

	while (po != PORT_OBJECT_NULL) {
		assert(po->type == POT_INVALID);
		next = *(port_object_t *)po;
		zfree(port_object_zone, (vm_offset_t) po);
		po = next;
	}
}

/* 
 * Internal: allocate and initialize a port object.
 */
static inline port_object_t port_object_alloc()
{
	port_object_t po;

	po = (port_object_t) zalloc(port_object_zone);
	if (!po)
	    return (port_object_t)0;
	return po;
}

static inline void port_object_clear(port_object_t po)
{
	mutex_init(&po->lock);
	mutex_set_name(&po->lock, "po->lock");
	condition_init(&po->cond);
	condition_set_name(&po->cond, "po->cond");
	po->type = POT_INVALID;
	po->state = POS_INVALID;
	po->seqno = 0;
	po->mscount = 0;
	po->socount = 0;
	po->extra_urefs = 0;
	po->object = OBJECT_NULL;
}

/* 
 * Allocate a port object for an existing send right.
 *
 * The port name will be changed.
 * A reference will be added to object.
 *
 * Object should be locked.
 */
/* 
 * XXX should check for the case where the object already exists!
 * XXX this may happen e.g. when bsd_signal_port_register is called
 * XXX twice with the same port.
 */
mach_error_t port_object_enter_send(
	mach_port_t		*port,	/* IN/OUT */
	port_object_type_t	pot,
	void			*object)
{
	port_object_t po;
	kern_return_t kr;
	mach_port_t previous;

	assert(MACH_PORT_VALID(*port));

again:
	po = port_object_alloc();
	if (!po)
	    return ENOMEM;	/* XXX */
	kr = mach_port_rename(mach_task_self(), *port, (mach_port_t) po);
	/* If name was already taken try with another */
	if (kr == KERN_NAME_EXISTS) {
		port_object_bad_object(po);
		goto again;
	}
	/* Some other error. Unlikely. */
	if (kr != KERN_SUCCESS) {
		zfree(port_object_zone, (vm_offset_t) po);
		return kr;
	}

	port_object_clear(po);

	po->type = pot;
	po->object = object;
	po->state = POS_SENDER_ALIVE;
	port_object_hash_enter(po);
	*port = (mach_port_t) po;

	/* 
	 * Get a dead name notification so we can garbage collect.
	 * The notification may be generated immediately so object
	 * should be locked if the caller needs to.
	 * But no locking is needed here as no more manipulation is done.
	 */
	kr = mach_port_request_notification(mach_task_self(),
					    *port,
					    MACH_NOTIFY_DEAD_NAME,
					    TRUE,
					    port_object_dead_port_notify_port,
					    MACH_MSG_TYPE_MAKE_SEND_ONCE,
					    &previous);
	assert(kr == KERN_SUCCESS);
	assert(previous == MACH_PORT_NULL);
	return KERN_SUCCESS;
}

/* 
 * Allocate a port object with a fresh receive right.
 *
 * A reference will be added to object.
 *
 * Object should be locked.
 */
mach_error_t port_object_allocate_receive(
	mach_port_t		*port,		/* OUT */
	port_object_type_t	pot,
	void			*object)
{
	port_object_t po;
	kern_return_t kr;

again:
	po = port_object_alloc();
	if (!po)
	    return ENOMEM;	/* XXX */
	kr = mach_port_allocate_name(mach_task_self(),
				     MACH_PORT_RIGHT_RECEIVE,
				     (mach_port_t) po);
	/* If name was already taken try with another */
	if (kr == KERN_NAME_EXISTS) {
		port_object_bad_object(po);
		goto again;
	}
	/* Some other error. Unlikely. */
	if (kr != KERN_SUCCESS) {
		zfree(port_object_zone, (vm_offset_t) po);
		return kr;
	}

	port_object_clear(po);

	po->type = pot;
	po->object = object;
	if (POM_FLAGS(po) & POF_GC) {
		po->state = POS_RECEIVER_INACTIVE;
	} else {
		/* Go directly to active state */
		po->state = POS_RECEIVER_ACTIVE;
	}
	port_object_hash_enter(po);

	*port = (mach_port_t) po;
	return KERN_SUCCESS;
}

/* 
 * Add a send right to a receive right and increment internal make
 * send count.
 *
 * Activates port object and requests a no senders notification if inactive.
 *
 * Nothing locked.
 */
mach_error_t port_object_make_send(mach_port_t port)
{
	kern_return_t kr;
	port_object_t po;
	mach_port_t previous;
	port_object_flags_t pof;

	po = port_object_hash_lookup(port); /* returns po locked */
	if (!po)
	    return KERN_INVALID_RIGHT;

	pof = POM_FLAGS(po);
	assert(pof & POF_RECEIVE);
	assert(po->state == POS_RECEIVER_ACTIVE
	       || po->state == POS_RECEIVER_INACTIVE);
	if ((pof & POF_LAZY) && po->extra_urefs && !(pof & POF_GC)) {
		po->extra_urefs--;
		mutex_unlock(&po->lock);
		return KERN_SUCCESS;
	}
	kr = mach_port_insert_right(mach_task_self(), port, port,
				    MACH_MSG_TYPE_MAKE_SEND);
	if (kr) {
		mutex_unlock(&po->lock);
		return kr;
	}
	po->mscount++;
	if (pof & POF_GC) {
		if (po->state == POS_RECEIVER_ACTIVE) {
			/* Wakeup no senders notify handler if it's waiting */
			condition_broadcast(&po->cond);
		} else {	/* POS_RECEIVER_INACTIVE */
			/* Rerequest no senders notification */
			po->state = POS_RECEIVER_ACTIVE;
			kr = mach_port_request_notification(mach_task_self(),
						port,
						MACH_NOTIFY_NO_SENDERS,
						po->mscount,
						port,
						MACH_MSG_TYPE_MAKE_SEND_ONCE,
						&previous);
			assert(kr == KERN_SUCCESS);
			assert(previous == MACH_PORT_NULL);
		}
	}
	mutex_unlock(&po->lock);
	return KERN_SUCCESS;
}

/* 
 * Add a send right to a send right.
 *
 * Nothing locked.
 */
mach_error_t port_object_copy_send(mach_port_t port)
{
	kern_return_t kr = KERN_SUCCESS;
	port_object_t po;
	mach_port_t previous;
	port_object_flags_t pof;

	po = port_object_hash_lookup(port); /* returns po locked */
	if (!po)
	    return KERN_INVALID_RIGHT;

	pof = POM_FLAGS(po);
	assert(pof & (POF_SEND_ONLY));
	assert(po->state == POS_SENDER_ALIVE);
	if (pof & POF_LAZY) {
		if (po->extra_urefs == 0) {
			kr = mach_port_mod_refs(mach_task_self(), port,
						MACH_PORT_RIGHT_SEND,
						PORT_OBJECT_LAZY_UREFS_DELTA);
			assert(kr == KERN_SUCCESS);
			if (kr == KERN_SUCCESS)
			    po->extra_urefs += PORT_OBJECT_LAZY_UREFS_DELTA;
		}
		po->extra_urefs--;
	} else {
		/* Make just one */
		kr = mach_port_mod_refs(mach_task_self(), port,
					MACH_PORT_RIGHT_SEND,
					1);
		assert(kr == KERN_SUCCESS);
	}
	mutex_unlock(&po->lock);
	return kr;
}

void port_object_delete_extra_urefs(port_object_t po, mach_port_t name)
{
	kern_return_t kr;
	int delta = -po->extra_urefs;

	po->extra_urefs = 0;

	if (delta <= 0)
	    return;

	/* Get rid of lazy evaluated consumptions */
	kr = mach_port_mod_refs(mach_task_self(),
				name,
				MACH_PORT_RIGHT_SEND,
				delta);
	assert(kr == KERN_SUCCESS || kr == KERN_INVALID_RIGHT);
	if (kr == KERN_INVALID_RIGHT)
	    kr = mach_port_mod_refs(mach_task_self(),
				    name,
				    MACH_PORT_RIGHT_DEAD_NAME,
				    delta);
	assert(kr == KERN_SUCCESS);
}

#if 0
/* XXX tmp interface.
 * Record a send one right for receive right.
 * XXX make_send_once would be better.
 */
port_object_record_send_once(mach_port_t receive)
{
	kern_return_t kr;
	port_object_t po;
	port_object_flags_t pof;

	po = port_object_hash_lookup(receive); /* returns po locked */
	if (!po)
	    return KERN_INVALID_RIGHT;

	pof = POM_FLAGS(po);
	assert(pof & POF_RECEIVE);
	assert(po->state == POS_RECEIVER_ACTIVE
	       || po->state == POS_RECEIVER_INACTIVE);

	po->socount++;
	mutex_unlock(&po->lock);
	return KERN_SUCCESS;	
}
#endif

/* 
 * Shutdown Port Object.
 * If force is FALSE only inactive port objects will be shutdown.
 *
 * The receive right is destroyed.
 * A no senders notification will be generated. It will garbage
 * collect the port object and dereference the underlying object.
 *
 * Nothing needs to be locked.
 */
void port_object_shutdown(mach_port_t port, boolean_t force)
{
	kern_return_t kr;
	port_object_t po;
	void *o;
	struct mach_port_status rstatus;
	port_object_state_t pos;
	port_object_flags_t pof;

	/* Port object is returned locked */
	po = port_object_hash_lookup_locked(port);
	if (!po)
	    return;

	pof = POM_FLAGS(po);
	pos = po->state;
	assert(pos == POS_RECEIVER_ACTIVE
	       || pos == POS_RECEIVER_INACTIVE);
	if (pos == POS_RECEIVER_ACTIVE && !force) {
		/* Never mind */
		port_object_hash_release(port);
		mutex_unlock(&po->lock);
		return;
	}
	/* 
	 * Drain sorights first before moving from set
	 * as later they can't be received.
	 *
	 * For now this means waiting for dead port notification from
	 * task before killing proc from ns notification. Later maybe more.
	 */
	while (po->socount > 0) {
		ux_server_thread_busy();
		condition_wait(&po->cond, &po->lock);
		ux_server_thread_active();
	}

	/* Kill it */

	/* Avoid more receives */
	kr = mach_port_move_member(mach_task_self(), port, MACH_PORT_NULL);
	assert(kr == KERN_SUCCESS);

	port_object_hash_remove(port);
	o = po->object;
	/* This a good time to get rid of reverse mappings */
	POM_REMOVE_REVERSE(o, po->type, port);

	/* Set new state */
	if (po->state == POS_RECEIVER_INACTIVE)	/* XXX */
	    po->state = POS_RECEIVER_DEAD;	/* XXX */
	else					/* XXX */
	    po->state = POS_RECEIVER_SHUTDOWN;

	/*
	 * Synchronize on seqno.
	 * It is ok to hold locks here as the lookup does not.
	 */
#if OSFMACH3
	{
	mach_msg_type_number_t count = MACH_PORT_RECEIVE_STATUS_COUNT;
	kr = mach_port_get_attributes(mach_task_self(),
				      port,
				      MACH_PORT_RECEIVE_STATUS,
				      (mach_port_info_t) &rstatus,
				      &count);
	if (kr != KERN_SUCCESS)
		panic("port_object_shutdown: m_p_get_attr: %s",
		      mach_error_string(kr));
	}
#else
	kr = mach_port_get_receive_status(mach_task_self(), port, &rstatus);
#endif
	assert(kr == KERN_SUCCESS);
	while (rstatus.mps_seqno > po->seqno) {
		SEQNO_SANITY(po->seqno, rstatus.mps_seqno);
		condition_wait(&po->cond, &po->lock);
	}
	/* The state may have changed. R_SHUTDOWN -> R_DEAD */

	kr = mach_port_mod_refs(mach_task_self(), port,
				MACH_PORT_RIGHT_RECEIVE, -1);
	assert(kr == KERN_SUCCESS);
	if (po->state == POS_RECEIVER_DEAD) {
		/* get rid of it */
		assert(po->socount == 0);
		POM_LOCK(o, po->type);
		POM_DEREFERENCE(o, po->type, port);
		zfree(port_object_zone, (vm_offset_t) po);
		return;
	}
	/* 
	 * Refs is now outstanding send rights (dead names actually).
	 * Some are due to send rights being sent, some are send
	 * rights being received.
	 *
	 * How do we distinguish between them?
	 * By looking at make send counts. No, doesn't help.
	 * What about looking at the message after it is sent and
	 * call a function to decrement outstanding send rigths?
	 * Too expensive. Well, so I think the solution is to look
	 * check after each consumption and with a timeout. The
	 * timeout thread will finalize if nobody else does. In
	 * most cases that will not be needed (fortunately). So
	 * I'll leave the extra ref around and check the refs only
	 * if we should deallocate now.
	 */
	mutex_unlock(&po->lock);
}

/* 
 * Garbage collect receive right port object.
 *
 * Called by no senders notification handler.
 */
mach_error_t port_object_no_senders(
	mach_port_t		port,
	mach_port_seqno_t	seqno,
	mach_port_mscount_t	mscount)
{
	kern_return_t kr;
	port_object_t po;
	void *o;
	mach_port_t previous;
	port_object_flags_t pof;

	if (!MACH_PORT_VALID(port)) {
		printf("port_object_no_senders(x%x x%x x%x INVALID)\n",
		       port, seqno, mscount);
		return EINVAL;
	}
	po = (port_object_t) port;
	pof = POM_FLAGS(po);
	assert(pof & POF_GC);

	mutex_lock(&po->lock);
	/* Wait for receivers to catch up */
	while(po->seqno < seqno) {
		SEQNO_SANITY(po->seqno, seqno);
		condition_wait(&po->cond, &po->lock);
	}
	assert(po->seqno == seqno);
	po->seqno++;
	/* Not all paths below broadcast below so do it here for now */
	condition_broadcast(&po->cond);

#if 0
	/* Now this might have been a result of object shutdown or
	 * just end of user refs. In any case we must wait for new
	 * ref creaters to get in synch. But we must distinguish to
	 * know what to do about the receive right.
	 */
	while(po->mscount < mscount) {
		assert(FALSE);	/* shouldn't get here as make send takes lock*/
		condition_wait(&po->cond, &po->lock);
	}
#endif
	if (po->state == POS_RECEIVER_SHUTDOWN) {
if (po_debug) printf("port_object_no_senders(x%x [x%x] x%x x%x Shutdown)\n",
       port, po->type, seqno, mscount);
		/* 
		 * The no senders notification is processed before
		 * shutdown completion because the sequence number is
		 * smaller. If it's not smaller, then ns was not
		 * received.
		 */
		assert(po->socount == 0);
		/* Now all refs are gone except perhaps dead send rights */
		po->state = POS_RECEIVER_DEAD;
		condition_broadcast(&po->cond);
		mutex_unlock(&po->lock);
		/* po is now gone */
		/* port_object_drain(po); */
		return KERN_SUCCESS;
	} else if (po->state == POS_RECEIVER_ACTIVE) {
		/* If no new send right have been made we should
		 * just kill the port and dereference. If new sends
		 * have been made, we should find out about it and
		 * rerequest a notification.
		 */
		if (po->mscount > mscount) {
if (po_debug) printf("port_object_no_senders(x%x [x%x] x%x x%x) Activate\n",
       port, po->type, seqno, mscount);
			/* Reinstall */
			kr = mach_port_request_notification(mach_task_self(),
						port,
						MACH_NOTIFY_NO_SENDERS,
						po->mscount,
						port,
						MACH_MSG_TYPE_MAKE_SEND_ONCE,
						&previous);
			assert(kr == KERN_SUCCESS);
			assert(previous == MACH_PORT_NULL);
		} else {
if (po_debug) printf("port_object_no_senders(x%x [x%x] x%x x%x) Deactivate\n",
       port, po->type, seqno, mscount);
			/* Kill it */
			po->state = POS_RECEIVER_INACTIVE;
			/* BUT it may be reactivated! */
			mutex_unlock(&po->lock);
			port_object_shutdown(port, FALSE);
			return KERN_SUCCESS;
		}
	} else {
		panic("port_object_no_senders: bad state %d", po->state);
	}
	return KERN_SUCCESS;
}

/* 
 * Handle dead port notification.
 *
 * Involves two port objects.
 * notify is a receive right. Decrement so count.
 * name is a send name. Garbage collect it.
 */
void port_object_dead_name(
	mach_port_t		notify,
	mach_msg_seqno_t	seqno,
	mach_port_t		name)
{
	port_object_state_t pos;
	port_object_flags_t pof;
	void *o;
	port_object_t po;
 
if (po_debug) printf("port_object_dead_name(x%x x%x x%x)\n",notify,seqno,name);
#if 0
	po = (port_object_t) notify;
	assert(POM_FLAGS(po) & POF_RECEIVE);
	mutex_lock(&po->lock);
	while (po->seqno < seqno) {
		SEQNO_SANITY(po->seqno, seqno);
		condition_wait(&po->cond, &po->lock);
	}
	assert(po->seqno == seqno);
	po->seqno++;
	assert(po->socount >= 1);
	po->socount--;
	condition_broadcast(&po->cond);

	/* Should be smart: drain etc. */
	mutex_unlock(&po->lock);
#else
	assert(notify == port_object_dead_port_notify_port);
#endif
	/* Done with receive right, now send name */
	/* 
	 * As all dead port notifications are excplicitely asked for
	 * the lookups are trusted. The notification always comes from
	 * the kernel. It can't be faked as all send once rights are
	 * known and not given to users. If this changes it is
	 * necessary to use hash table instead.
	 *
	 * I removed reference counting the notification!
	 */
	po = port_object_hash_lookup_locked(name);
	if (!po)
	    return;

	pof = POM_FLAGS(po);
	pos = po->state;

	assert(pos == POS_SENDER_ALIVE);
	assert((pof & POF_SEND_ONLY) && (pof & POF_GC)
	       && !(pof & (POF_SEND|POF_RECEIVE)));

	port_object_hash_remove(name); /* unlocks bucket */
	o = po->object;

	POM_REMOVE_REVERSE(o, po->type, name);

	/* Set new state */
	po->state = POS_SENDER_DEAD;
	POM_LOCK(o, po->type);
	POM_DEREFERENCE(o, po->type, name); /* unlocks o */

	port_object_delete_extra_urefs(po, name);

	zfree(port_object_zone, (vm_offset_t) po); /* just leave po locked */
}

/* 
 * A hash table is used not to lookup right to object (that's done
 * by the direct lookup) but to create new rights. Otherwise there
 * is the problem that when a port is destroyed and someone else
 * made a new send right we haven't been able to increment the
 * internal make send count since we didn't know whether there was
 * a race with destruction and thus we don't know when to
 * deallocate.
 *
 * Now when we want to make a new send right, we first lookup
 * through the hash table get a lock, increment the mscount and
 * release. Shutdown will lookup through the hash table and remove
 * it from there. The ns notification will then garbage collect the
 * object itself as then it is known no more external lookups can
 * be made.
 *
 * Internal lookups through hash table.
 * External lookups through casting.
 *
 * To rephrase: After shutdown it is dangerous to
 * - reshutdown -- the port may have vanished and reallocated and
 * the wrong port is destructed.
 * - make send -- again, port may be reallocated.
 * - lookup -- Internal lookups are not counted in seqno et al.
 *
 * Now I did add an extra send right around deletion so lookups are
 * ok if you keep a send right ref but how do you get that? So the
 * hash table is still needed.
 */

#define HASH_SIZE 256
/* 
 * The port names are actually pointers from zalloc.
 * The port object is 36 bytes wide and thus the 6 lowest bits
 * aren't relevant. 14 is log2(256) + 6 above.
 */
#define PORT_HASH(p) (((p >> 6) + (p >> 14)) % HASH_SIZE)

typedef struct hash_bucket {
	struct mutex lock;
	queue_head_t queue;
} *hash_bucket_t;

struct hash_bucket port_object_hash[HASH_SIZE];

/* Init hash table */
static void port_object_hash_init()
{
	int i;

	for (i = 0; i < HASH_SIZE; i++) {
		mutex_init(&port_object_hash[i].lock);
		mutex_set_name(&port_object_hash[i].lock, "port_object_hash");
		queue_init(&port_object_hash[i].queue);
	}
}

/* 
 * Lookup Port Object from hash table.
 *
 * Port Object is returned locked.
 */ 
static port_object_t port_object_hash_lookup(mach_port_t port)
{
	port_object_t po_iterator;
	port_object_t po = PORT_OBJECT_NULL;
	hash_bucket_t b;
	
	b = &port_object_hash[PORT_HASH(port)];
	mutex_lock(&b->lock);
	queue_iterate(&b->queue, po_iterator, port_object_t, chain)
	    if ((mach_port_t) po_iterator == port) {
		    po = po_iterator;
		    mutex_lock(&po->lock);
		    mutex_unlock(&b->lock);
		    return po;
	    }
	mutex_unlock(&b->lock);
	return PORT_OBJECT_NULL;
}

/* 
 * Lookup Port Object from hash table leaving bucket locked.
 *
 * Port Object is returned locked.
 *
 * Nothing is left locked if the lookup failed!
 */ 
static port_object_t port_object_hash_lookup_locked(mach_port_t port)
{
	port_object_t po_iterator;
	port_object_t po = PORT_OBJECT_NULL;
	hash_bucket_t b;
	
	b = &port_object_hash[PORT_HASH(port)];
	mutex_lock(&b->lock);
	queue_iterate(&b->queue, po_iterator, port_object_t, chain)
	    if ((mach_port_t) po_iterator == port) {
		    po = po_iterator;
		    mutex_lock(&po->lock);
		    /* Leave bucket locked */
		    return po;
	    }
	/* Unlock bucket on failure */
	mutex_unlock(&b->lock);
	return PORT_OBJECT_NULL;
}

/* 
 * Enter Port Object into hash table.
 * Duplicates are not checked for.
 *
 * Called just after Port Object creation.
 * The Port Object may not be received from or have send once rights.
 * No locks needed.
 *
 * NB: there is no risk for locking order deadlock between lookup and
 * enter as the port object may not be in the hash table if it is
 * being entered. Thus enter may never happen on a port object that is
 * in the same wait-for graph as the bucked unless more than one
 * object is being locked and manipulated by the same thread
 * simultaneously. That shouldn't be the case.
 */
static void port_object_hash_enter(port_object_t po)
{
	hash_bucket_t b;
	mach_port_t port = (mach_port_t) po;
	
	b = &port_object_hash[PORT_HASH(port)];
	mutex_lock(&b->lock);
	queue_enter(&b->queue, po, port_object_t, chain);
	mutex_unlock(&b->lock);
}

/* 
 * Remove Port Object from hash table.
 *
 * Called only from port_object_shutdown.
 *
 * Bucket and Port Object object must already be locked!
 */
static void port_object_hash_remove(mach_port_t port)
{
	port_object_t po = (port_object_t) port;
	hash_bucket_t b;

	b = &port_object_hash[PORT_HASH(port)];
	queue_remove(&b->queue, po, port_object_t, chain);
	mutex_unlock(&b->lock);
}

/* 
 * Release hash bucket.
 *
 * Called only from port_object_shutdown.
 *
 * Bucket must be locked.
 */
static void port_object_hash_release(mach_port_t port)
{
	hash_bucket_t b;

	b = &port_object_hash[PORT_HASH(port)];
	mutex_unlock(&b->lock);
}





#if 0
void port_to_object_deallocate(mach_port_t port)
{
	port_object_t po;

	if (!MACH_PORT_VALID(port))
	    return;

	po = (port_object_t) port;

	mach_port_mod_refs(mach_task_self(), port,
			   MACH_PORT_RIGHT_RECEIVE, -1);
	po->object = 0;
	zfree(port_object_zone, (vm_offset_t) po);
}
#endif
/* 
 * There is a potential race between freeing the port object and lookup.
 * The currtent master locking scheme takes care of it but here is one
 * more point to remember when getting rid of the master lock.
 * On the other hand I don't want to add a lock in lookup just to
 * circumvent this problem. Besides just adding a lock wouldn't help.
 * One possibility would be to not use no-senders notifications but
 * that doesn't work after the receive right is gone. Too bad that
 * mach_port_mod_refs or mach_port_deallocate don't return the
 * remaining ref count so we could use that. Now doing a get_refs
 * every time is too expensive and also creates a new race. Also
 * duplicating the ref count in user space doesn't seem to solve the
 * problem. And the other problem lies in receive rights where there
 * may actually be several references although the kernel keeps just
 * one around. I wish they were reference counted too so we could use
 * dead port notifications to garbage collect.
 *
 * The receive race is: How do we know that some thread didn't already
 * receive a message to a port that refers to the port object we wish
 * to deallocate. After some point sends will fail but that must be
 * synchronized with receivers.
 *
 * The same problem was present in the old BSDSS9 scheme as well, BTW.
 *
 * For send rights that come in through a message we must do checking
 * first. So this mapper can't be used for them.
 */
/*
Here is a more readable explanation of the problem:

From jvh Sun Feb 27 19:59:31 +0200 1994
From: Johannes Helander <jvh@cs.hut.fi>
To: mach3@cs.cmu.edu
Subject: How to solve race condition?
Organization: Helsinki University of Technology, CS Lab.

I just realized a race condition I don't know how to solve. I believe
it is present in both UX and BSDSS in addition to some new code I
just wrote. In the servers it will, however, not manifest itself in
practice.

It is covenient to use a port name as a pointer and cast it to the
object it represents. The problem comes in deallocating the object.
Now if I first delete the receive right I know no new messages can be
received on the port. But how do I know that some message has not
already been received? If I deallocate the object right away some
thread that had just received a message from the port but has perhaps
not yet been able to run thinks it has a completely valid port. It
will then use the port as a pointer and reference deallocated memory
or even worse, already reused memory.

Obviously I should delay the deallocation until it is known all
lookups have completed (after the lookup there can be locking and
reference counting). But how do I know how long to delay the
deallocation?

One idea was to use dead port notifications to do the actual
deallocation. But that doesn't really help as there are several
threads receiving and sequence numbers are per port instead of per
port sets and after the receive right is deleted there is no way to
find out the last sequence number. I tried requesting a dead port
notification to the port itself in the hope of getting the
notification as the last message just before shutdown to the port (A
dead name would have been ok as there don't need to be many
deallocations going on at once). But that didn't work. Instead a send
once notification was immediately generated and
mach_port_request_notification returned "(os/kern) invalid value",
which behavior, BTW, was not mentioned in the manual (OSF Jan 92 MK67
kernel interface).

The only solutions I can right now see are to wait until a thread that
started its receive after the port deletion receives a message before
any deallocation, which is awkward, or decide that using ports as
pointers directly is inherently dangerous and the technique should not
be used. But other lookup methods are inherently slower so that
doesn't seem to pleasing either.

        Johannes

From jvh Mon Feb 28 15:59:21 +0200 1994
From: Johannes Helander <jvh@cs.hut.fi>
To: Richard Draves <richdr@microsoft.com>
Cc: mach3@cs.cmu.edu
In-reply-to: Richard Draves's message of 28 Feb 1994 13:46:41 +0200
Subject: RE: How to solve race condition?
Organization: Helsinki University of Technology, CS Lab.

>   |It is covenient to use a port name as a pointer and cast it to the
>   |object it represents. The problem comes in deallocating the object.
>   |Now if I first delete the receive right I know no new messages can be
>   |received on the port. But how do I know that some message has not
>   |already been received? If I deallocate the object right away some
>   |thread that had just received a message from the port but has perhaps
>   |not yet been able to run thinks it has a completely valid port. It
>   |will then use the port as a pointer and reference deallocated memory
>   |or even worse, already reused memory.
>
>   You can use port sequence numbers to solve this problem.

The problem is that after I delete the receive right there is no way
of finding out the sequence number. If, on the other hand, I first get
the current seuence number with mach_port_get_receive_status and then
delete the port, there is still a window between the operations where
new messages may arrive (and get received).

So there must be some way to first disable the port, then check the
sequence number and after that destroy the port. It took me a while to
realize that the obvious solution is to remove the port from the set
to disable it. As I bothered the list with a stupid question I will
try to be fair and explain the solution in case it is not obvious.

1. Use the seqnos interfaces when receiving.
2. Keep track of processed sequence numbers and serialize if that is
   desirable.
3. When deallocating, first remove the port from the receive set.
4. then use mach_port_get_receive_status to get the current sequence
   number.
6. Use mach_port_mod_refs to get rid of the receive right.
7. Wait until the processed sequence numbers reach that gotten from
   mach_port_get_receive_status above.
8. Now it is safe to deallocate.

But there is one more problem. What happened to those sequence numbers
that were queued between mach_port_move_member and
mach_port_get_receive_status? They were destroyed and step 7 would
last forever. Thus we need a step 5 that receives without blocking any
messages already queued to get their sequence numbers. At this time an
error should be returned to the client.

        Johannes

From: Richard Draves <richdr@microsoft.com>
To: jvh@cs.hut.fi
Subject: RE: How to solve race condition?
Date: Mon, 28 Feb 94 09:40:07 PST

|So there must be some way to first disable the port, then check the
|sequence number and after that destroy the port. It took me a while to
|realize that the obvious solution is to remove the port from the set
|to disable it. As I bothered the list with a stupid question I will
|try to be fair and explain the solution in case it is not obvious.

Give yourself some credit, the solution is not that obvious. :-)

Rich

From: Richard Draves <richdr@microsoft.com>
To: jvh@cs.hut.fi
Cc: mach3@cs.cmu.edu
Subject: RE: How to solve race condition?
Date: Mon, 28 Feb 94 21:05:15 PST

|But there is one more problem. What happened to those sequence numbers
|that were queued between mach_port_move_member and
|mach_port_get_receive_status? They were destroyed and step 7 would
|last forever. Thus we need a step 5 that receives without blocking any
|messages already queued to get their sequence numbers. At this time an
|error should be returned to the client.

Sequence numbers get assigned when a message is dequeued - they are 
receive sequence numbers, not send sequence numbers. So I think that 
this step is not needed.

Rich


From: Paul Roy <roy@osf.org>
To: Johannes Helander <jvh@cs.hut.fi>
Cc: Richard Draves <richdr@microsoft.com>, mach3@cs.cmu.edu
Subject: Re: How to solve race condition? 
Date: Mon, 28 Feb 1994 11:04:13 -0500

>So there must be some way to first disable the port, then check the
>sequence number and after that destroy the port. It took me a while to
>realize that the obvious solution is to remove the port from the set
>to disable it. As I bothered the list with a stupid question I will
>try to be fair and explain the solution in case it is not obvious.
>
>1. Use the seqnos interfaces when receiving.
>2. Keep track of processed sequence numbers and serialize if that is
>   desirable.
>3. When deallocating, first remove the port from the receive set.
>4. then use mach_port_get_receive_status to get the current sequence
>   number.
>6. Use mach_port_mod_refs to get rid of the receive right.
>7. Wait until the processed sequence numbers reach that gotten from
>   mach_port_get_receive_status above.
>8. Now it is safe to deallocate.
>
>But there is one more problem. What happened to those sequence numbers
>that were queued between mach_port_move_member and
>mach_port_get_receive_status? They were destroyed and step 7 would
>last forever. Thus we need a step 5 that receives without blocking any
>messages already queued to get their sequence numbers. At this time an
>error should be returned to the client.

A simpler solution would be to use no-more-senders.  Mach IPC
guarantees that a no-more-senders message will be the last one queued,
and hence have the largest sequence number.  Processing of the
no-more-senders dereferences (or deallocates, depending on how you've
implemented things) your object.  By synchronizing on sequence numbers
you can guarantee that the no-more-senders message is processed last.

Depending on your application, an even simpler/cheaper solution might
be to have clients do explicit reference counting rather than relying
on Mach IPC to do it.

Paul Roy
OSF RI

From: Alessandro Forin <sandrof@microsoft.com>
Newsgroups: hut.mach3
Subject: RE: How to solve race condition?
Date: 28 Feb 1994 21:40:00 +0200
Organization: Mailing list gatewayed to local news by nntp.hut.fi
NNTP-Posting-Host: nntp.hut.fi
X-Msmail-Message-Id:  2AC26FDB
X-Msmail-Conversation-Id:  2AC26FDB
To: mach3-Request@WB1.CS.CMU.EDU
Cc: mach3@cs.cmu.edu

Logic would tell me that if you port_set_backlog()  [or whatever is the 
call that sets the length of the queue on the port] to zero you will 
block any further messages from
being queued.
But I have never tried this personally, donno if there is a bug somewhere.
But if it does work, it would work regardless of port sets.
sandro-

[I don't think this is a correct solution --jvh]
*/
