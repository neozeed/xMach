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
 * $Log: ux_server_loop.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.7  1995/08/21  22:10:55  mike
 * nuke old leaky register code
 *
 * Revision 1.3  1995/03/23  01:44:06  law
 * Update to 950323 snapshot + utah changes
 *
 *
 */
/* 
 *	File: 	serv/ux_server_loop.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Server thread utilities and main loop.
 */

#include <serv/server_defs.h>
#include <serv/bsd_msg.h>
#include <bsd_types_gen.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/proc.h>

#if OSFMACH3

/* These are missing from cthreads */
mach_error_t cthread_mach_msg(
	mach_msg_header_t *msg,
	mach_msg_option_t option,
	mach_msg_size_t size,
	mach_msg_size_t receive_limit,
	mach_port_t receive_name,
	mach_msg_timeout_t timeout,
	mach_port_t notify,
	int ux_server_receive_min,
	int ux_server_receive_max)
{
	return mach_msg(msg, option, size, receive_limit, receive_name,
			timeout, notify);
}

void cthread_msg_busy(mach_port_t port, int receive_min, int receive_max)
{
}

void cthread_msg_active(mach_port_t port, int receive_min, int receive_max)
{
}

#endif /* OSFMACH3 */



mach_port_t ux_server_port_set = MACH_PORT_NULL;

/*
 * Number of server threads available to handle user messages.
 */
struct mutex	ux_server_thread_count_lock = MUTEX_INITIALIZER;
int		ux_server_thread_count = 0;
int		ux_server_thread_min = 4;
int		ux_server_receive_min = 2;
int		ux_server_receive_max = 6;
int		ux_server_thread_max = 80;
int		ux_server_stack_size = 4096*16;
#ifdef USEACTIVATIONS
/* Activation stacks must come from wired cthreads */
int		ux_server_max_kernel_threads = 0;
#else
/* This must be at least 5 (wired) + 1 (running) + receive_max */
int		ux_server_max_kernel_threads = 13;
#endif

void ux_create_single_server_thread(); /* forward */

void
ux_server_init()
{
	mach_port_t first_port;
	kern_return_t kr;

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET,
				&ux_server_port_set);

	cthread_set_kernel_limit(ux_server_max_kernel_threads);

}

void
ux_server_add_port(port)
	mach_port_t	port;
{
	(void) mach_port_move_member(mach_task_self(),
				     port, ux_server_port_set);
}

void
ux_server_remove_port(port)
	mach_port_t	port;
{
	(void) mach_port_move_member(mach_task_self(), port, MACH_PORT_NULL);
}

void *ux_thread_bootstrap (cthread_fn_t real_routine)
{
	proc_invocation_t pk;
	void *ret;
	struct proc_invocation pkdata;

	pk = &pkdata;
	if (pk == NULL)
	    panic("ux_thread_bootstrap");
	queue_init(&pk->k_servers_chain);
	pk->k_wchan = NULL;
	pk->k_wmesg = NULL;
	/* reset k_p later if system proc or something else */
	pk->k_p = 0;
	pk->k_master_lock = 0;
	pk->k_reply_msg = NULL;
	pk->k_current_size = 0;
	pk->k_ipl = 0;
	pk->k_timedout = FALSE;
	pk->k_flag = 0;
	pk->cthread = cthread_self(); /* to make debugging easier */
	mutex_init(&pk->lock);
	pk->event = 0;
	pk->condition = 0;

	cthread_set_data(cthread_self(), pk);
#ifdef USEACTIVATIONS
	cthread_wire();
#endif

	ret = ((*real_routine)(0));

	if (pk->k_wchan || pk->k_p != 0 || pk->k_master_lock
	    || pk->k_ipl || pk->k_reply_msg)
	{
		panic("ux_thread_bootstrap");
	}
	return ret;
}

/*
 * Create a thread
 */
void
ux_create_thread(cthread_fn_t routine)
{
	cthread_detach(cthread_fork(ux_thread_bootstrap, routine));
}

void	ux_server_loop(void);	/* forward */

void
ux_create_server_thread()
{
	ux_create_thread(ux_server_loop);
}

void
ux_server_thread_busy()
{
	cthread_msg_busy(ux_server_port_set,
			 ux_server_receive_min,
			 ux_server_receive_max);
	mutex_lock(&ux_server_thread_count_lock);
	if (--ux_server_thread_count < ux_server_thread_min) {
	    mutex_unlock(&ux_server_thread_count_lock);
	    ux_create_server_thread();
	}
	else {
	    mutex_unlock(&ux_server_thread_count_lock);
	}
}

void
ux_server_thread_active()
{
	cthread_msg_active(ux_server_port_set,
			 ux_server_receive_min,
			 ux_server_receive_max);
	mutex_lock(&ux_server_thread_count_lock);
	++ux_server_thread_count;
	mutex_unlock(&ux_server_thread_count_lock);
}

#define	ux_server_thread_check() \
	(ux_server_thread_count > ux_server_thread_max)

#define UX_MAX_MSGSIZE	(SMALL_ARRAY_LIMIT+1024)

/*
 * Main loop of server.
 */
void ux_server_loop()
{
	register kern_return_t	ret;
	proc_invocation_t pk = get_proc_invocation();

	union request_msg {
	    mach_msg_header_t	hdr;
#if OSFMACH3
	    mig_reply_error_t	death_pill;
#else
	    mig_reply_header_t	death_pill;
#endif
	    char		space[UX_MAX_MSGSIZE];
	    union bsd_msg	room;
	} msg_buffer_1, msg_buffer_2;

	mach_msg_header_t *request_ptr;
#if OSFMACH3
	mig_reply_error_t *reply_ptr;
#else
	mig_reply_header_t *reply_ptr;
#endif
	mach_msg_header_t *tmp;

	char	name[64];

	sprintf(name, "ST 0x%x", cthread_self());
	cthread_set_name(cthread_self(), name);

	pk->k_p = NULL;		/* fix device reply instead XXX */
	ux_server_thread_active();

	request_ptr = &msg_buffer_1.hdr;
	reply_ptr = &msg_buffer_2.death_pill;

	do {
#if OSFMACH3
	    ret = cthread_mach_msg(request_ptr,
	MACH_RCV_MSG | MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_SEQNO),
				   0,
				   sizeof msg_buffer_1 - MAX_TRAILER_SIZE,
				   ux_server_port_set,
				   MACH_MSG_TIMEOUT_NONE,
				   MACH_PORT_NULL,
				   ux_server_receive_min,
				   ux_server_receive_max);
#else /* OSFMACH3 */
	    ret = cthread_mach_msg(request_ptr, MACH_RCV_MSG,
				   0, sizeof msg_buffer_1, ux_server_port_set,
				   MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
				   ux_server_receive_min,
				   ux_server_receive_max);
#endif /* OSFMACH3 */
	    if (ret != MACH_MSG_SUCCESS)
		panic("ux_server_loop: receive", ret);
	    while (ret == MACH_MSG_SUCCESS) {
		if (!seqnos_memory_object_server(request_ptr,&reply_ptr->Head))
		if (!bsd_1_server(request_ptr, &reply_ptr->Head))
		if (!ux_generic_server(request_ptr, &reply_ptr->Head))
		if (!exc_server(request_ptr, &reply_ptr->Head))
		if (!seqnos_notify_server(request_ptr, &reply_ptr->Head))
		    bad_request_server(request_ptr, &reply_ptr->Head);

		/* Don't lose a sequence number if a type check failed */
		if (reply_ptr->RetCode == MIG_BAD_ARGUMENTS) {
			printf("MiG type check failed: req id %d port=x%x\n",
			       request_ptr->msgh_id,
			       request_ptr->msgh_local_port);
			bad_request_server(request_ptr, &reply_ptr->Head);
		}

		if (reply_ptr->Head.msgh_remote_port == MACH_PORT_NULL) {
		    /* no reply port, just get another request */
		    break;
		}

		if (reply_ptr->RetCode == MIG_NO_REPLY) {
		    /* deallocate reply port right */
		    (void) mach_port_deallocate(mach_task_self(),
					reply_ptr->Head.msgh_remote_port);
		    break;
		}

#if OSFMACH3
		ret = cthread_mach_msg(&reply_ptr->Head,
  MACH_SEND_MSG|MACH_RCV_MSG|MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_SEQNO),
				       reply_ptr->Head.msgh_size,
				       sizeof msg_buffer_2 - MAX_TRAILER_SIZE,
				       ux_server_port_set,
				       MACH_MSG_TIMEOUT_NONE,
				       MACH_PORT_NULL,
				       ux_server_receive_min,
				       ux_server_receive_max);
#else /* OSFMACH3 */
		ret = cthread_mach_msg(&reply_ptr->Head,
				       MACH_SEND_MSG|MACH_RCV_MSG,
				       reply_ptr->Head.msgh_size,
				       sizeof msg_buffer_2,
				       ux_server_port_set,
				       MACH_MSG_TIMEOUT_NONE,
				       MACH_PORT_NULL,
				       ux_server_receive_min,
				       ux_server_receive_max);
#endif /* OSFMACH3 */
		if (ret != MACH_MSG_SUCCESS) {
		    if (ret == MACH_SEND_INVALID_DEST) {
			/* deallocate reply port right */
			/* XXX should destroy entire reply msg */
			(void) mach_port_deallocate(mach_task_self(),
					reply_ptr->Head.msgh_remote_port);
		    } else {
			    panic("ux_server_loop: rpc x%x", ret);
		    }
		}

		tmp = request_ptr;
		request_ptr = (mach_msg_header_t *) reply_ptr;
#if OSFMACH3
		reply_ptr = (mig_reply_error_t *) tmp;
#else
		reply_ptr = (mig_reply_header_t *) tmp;
#endif
	    }

	} while (!ux_server_thread_check());

	ux_server_thread_busy();

        printf("Server loop done: %s\n",name);

	return;	/* exit */
}

#ifdef USEACTIVATIONS
/*
 * Activation code
 * XXX there are some PA specifics buried here.
 */

#define MAX_ACTIVATIONS	40	/* max server activations to create */
#define NUM_ACTIVATIONS	8	/* # of activations to create per request */

int num_activations = 0;
vm_offset_t act_stacks[NUM_ACTIVATIONS];
struct mutex act_stacks_lock = MUTEX_INITIALIZER;
struct mutex more_act_lock = MUTEX_INITIALIZER;
extern boolean_t use_activations;

void migr_rpc_exit(mig_reply_header_t *, boolean_t);

#include <mach/rpc.h>

/*
 * Simplified version of ux_server_loop for standard migrating threads path.
 *
 * XXX we should pass in a pointer to a "large enough" reply buffer rather
 * than allocate it locally.  That way we can just return from this function
 * to the machine-dependent migr_rpc_entry and let it do the machine-dependent
 * return.
 */
void
ux_act_entry(request_ptr)
	mach_msg_header_t *request_ptr;
{
	union request_msg {
	    mig_reply_header_t	reply;
	    char		space[UX_MAX_MSGSIZE];
	} reply_buf;
	mig_reply_header_t *reply_ptr = &reply_buf.reply;
	boolean_t doreply = TRUE;

#if 0
	static int count = 100;
	if (--count > 0)
		printf("%d: u_a_e(%x): id=%d, lp=0x%x, rp=0x%x, bits=0x%x, seq=0x%x\n",
		       count, request_ptr,
		       request_ptr->msgh_id,
		       request_ptr->msgh_local_port,
		       request_ptr->msgh_remote_port,
		       request_ptr->msgh_bits,
		       request_ptr->msgh_seqno);
#endif
	if (!seqnos_memory_object_server(request_ptr, &reply_ptr->Head))
	if (!bsd_1_server(request_ptr, &reply_ptr->Head))
	if (!ux_generic_server(request_ptr, &reply_ptr->Head))
	if (!exc_server(request_ptr, &reply_ptr->Head))
	if (!seqnos_notify_server(request_ptr, &reply_ptr->Head)) {
	    printf("Bad MiG request: req id %d port=x%x\n",
		   request_ptr->msgh_id,
		   request_ptr->msgh_local_port);
	    bad_request_server(request_ptr, &reply_ptr->Head);
	}

	/* Don't lose a sequence number if a type check failed */
	if (reply_ptr->RetCode == MIG_BAD_ARGUMENTS) {
		printf("MiG type check failed: req id %d port=x%x\n",
		       request_ptr->msgh_id,
		       request_ptr->msgh_local_port);
		bad_request_server(request_ptr, &reply_ptr->Head);
	}
#if 1
	/*
	 * XXX what should we do in these cases?
	 */
	if (reply_ptr->Head.msgh_remote_port == MACH_PORT_NULL) {
		/* no reply port, just get another request */
		printf("ux_act_entry: id=%d(%d): no reply port\n",
		       request_ptr->msgh_id,
		       request_ptr->msgh_id == 100000 ?
		       ((int *)request_ptr)[8] : 0);
		doreply = FALSE;
	} else if (reply_ptr->RetCode == MIG_NO_REPLY) {
		/* no reply desired */
		doreply = FALSE;		
	}
#endif
	migr_rpc_exit(reply_ptr, doreply);
	/* NOTREACHED */
}

void
ux_more_acts()
{
	int ocount = num_activations;

	mutex_lock(&more_act_lock);
	if (ocount == num_activations && num_activations < MAX_ACTIVATIONS) {
		printf("crankin out some more activations...\n");
		ux_act_create(NUM_ACTIVATIONS);
		printf("...total now %d\n", num_activations);
	}
	mutex_unlock(&more_act_lock);
}

ux_act_init()
{
	ux_act_create(NUM_ACTIVATIONS);
}

ux_act_create(num)
	int num;
{
	int dp, i;
	mach_port_t act;
	kern_return_t kr;
	rpc_info_t rpcinfo;
	extern void migr_rpc_entry();

	/*
	 * Get stacks for activations.
	 */
	if (num > NUM_ACTIVATIONS)
		num = NUM_ACTIVATIONS;
	act_get_stacks(num);

	/*
	 * Register the activations.
	 */
	for (i = 0; i < num; i++) {
		kr = mach_port_create_act(mach_task_self(), ux_server_port_set,
					  act_stacks[i] + UX_MAX_MSGSIZE,
					  act_stacks[i], UX_MAX_MSGSIZE, &act);
		if (kr != KERN_SUCCESS) {
			printf("ux_act_init: m_p_c_a(%x, %x, %x, %x) -> %x\n",
			       mach_task_self(), ux_server_port_set,
			       act_stacks[i], &act, kr);
			break;
		}
		num_activations++;
	}
	if (i == 0) {
		printf("ux_act_init: WARNING no activations!\n");
		return;
	}
	/*
	 * Register our entry point.
	 */
	rpcinfo.entry_pc = (vm_offset_t) migr_rpc_entry;
	rpcinfo.entry_dp = (vm_offset_t) get_base_pointer();
	kr = mach_port_set_rpcinfo(mach_task_self(), ux_server_port_set,
				   &rpcinfo, PARISC_RPC_INFO_COUNT);
	if (kr != KERN_SUCCESS)
		printf("ux_act_init: m_p_s_r(%x, %x, %x [%x/%x], %x) -> %x\n",
		       mach_task_self(), ux_server_port_set,
		       &rpcinfo, rpcinfo.entry_pc, rpcinfo.entry_dp,
		       PARISC_RPC_INFO_COUNT, kr);
}

/* XXX mike hacks */

int act_serverix;

/*
 * Make us some cthread looking stacks for use by activations.
 * We do this by acutally creating some number of cthreads, stealing
 * their stacks, and deep-sixing the threads themselves.
 */
act_get_stacks(num)
	int num;
{
	int i;
	any_t act_dead_thread();

printf("act_get_stacks: firing off %d threads, ux_max=%d\n",
num, ux_server_max_kernel_threads);

	act_serverix = 0;

#if 0
	ux_server_max_kernel_threads += num;
	cthread_set_kernel_limit(ux_server_max_kernel_threads);
#endif

	for (i = 0; i < num; i++)
		ux_create_thread(act_dead_thread);
	do {
		cthread_yield();
		mutex_lock(&act_stacks_lock);
		i = act_serverix;
		mutex_unlock(&act_stacks_lock);
	} while (i != num);
}

any_t
act_dead_thread(arg)
	any_t arg;
{
	kern_return_t rv;
	vm_offset_t mysp;
	int myix;

#if 0
	cthread_wire();
#endif

	/*
	 * Record our SP and note that we are done
	 */
	mysp = ((vm_offset_t)&mysp+128 + 7) & ~7; /* XXX PA */
	mutex_lock(&act_stacks_lock);
	myix = act_serverix++;
	act_stacks[myix] = mysp;
	mutex_unlock(&act_stacks_lock);

	/*
	 * Kill the thread
	 */
	rv = thread_terminate(mach_thread_self());
	/*
	 * Just in case...
	 * block the thread.
	 */
	printf("thread %x: didn't die (%x), sleeping...\n",
	       mach_thread_self(), rv);
	do {
		mach_msg_header_t hdr;

		/*
		 * XXX maybe just spin these puppies at depressed
		 * priority to save kernel activations.
		 */
		rv = mach_msg(&hdr, MACH_RCV_MSG, 0, sizeof hdr,
			      mach_reply_port(), MACH_MSG_TIMEOUT_NONE,
			      MACH_PORT_NULL);
		printf("dead_thread %d: mach_msg returned %x\n", myix, rv);
	} while (rv == KERN_SUCCESS);
	/*
	 * Just in case...
	 * Eat Dirt.
	 */
	*(int *)0x6969 = rv;
}
/* XXX mike hacks */
#endif
