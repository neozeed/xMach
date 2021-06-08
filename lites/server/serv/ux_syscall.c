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
 * $Log: ux_syscall.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:17:01  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File: serv/ux_syscall.c
 *	Authors:
 *	Randall Dean, Carnegie Mellon University, 1992.
 *	Johannes Helander, Helsinki University of Technology, 1994.
 */

#include "syscalltrace.h"

#include <serv/import_mach.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/parallel.h>
#include <sys/proc.h>

#include <serv/bsd_msg.h>
#include <serv/syscalltrace.h>
#include <serv/server_defs.h>
#include <serv/syscall_subr.h>

extern struct sysent	sysent[];
int			nsysent;

#if	SYSCALLTRACE
int		syscalltrace;		/* no processes */
extern char	*syscallnames[];
#endif

/*
 * Generic UX server message handler.
 * Generic system call.
 */
boolean_t
ux_generic_server(InHeadP, OutHeadP)
	mach_msg_header_t *InHeadP, *OutHeadP;
{
	register struct bsd_request	*req = (struct bsd_request *)InHeadP;
	register struct bsd_reply	*rep = (struct bsd_reply *)OutHeadP;
	register struct sysent	*callp;
	register int		syscode = req->syscode;
	struct proc *p;
	proc_invocation_t pk = get_proc_invocation();
	struct proc *proc;
	mach_msg_seqno_t seqno;
	mach_msg_header_t *hdr;
#if	SYSCALLTRACE
	int			pid;
#endif	SYSCALLTRACE
	int error, retval[2];

	/*
	 * Fix the reply message.
	 */
#if UNTYPED_IPC
	mach_msg_format_0_trailer_t *trailer;
#else
	static mach_msg_type_t bsd_rep_int_type = {
	    /* msgt_name */		MACH_MSG_TYPE_INTEGER_T,
	    /* msgt_size */		sizeof(integer_t)*8,
	    /* msgt_number */		(sizeof(integer_t)*2+sizeof(int)*2)
						/ sizeof(integer_t),
	    /* msgt_inline */		TRUE,
	    /* msgt_longform */		FALSE,
	    /* msgt_deallocate */	FALSE,
	    /* msgt_unused */		0
	};
#endif
	hdr = &rep->hdr;

	/*
	 * Set up standard reply.
	 */
	hdr->msgh_bits =
		MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(InHeadP->msgh_bits), 0);
	hdr->msgh_remote_port = InHeadP->msgh_remote_port;
	hdr->msgh_local_port = MACH_PORT_NULL;
	hdr->msgh_id = InHeadP->msgh_id + 100;

	if (InHeadP->msgh_id != BSD_REQ_MSG_ID) {
#if UNTYPED_IPC
	    mig_reply_error_t *OutP = (mig_reply_error_t *)OutHeadP;
	    OutP->RetCode = MIG_BAD_ID;
	    OutP->NDR 	  = NDR_record;
	    OutP->Head.msgh_size = sizeof *OutP;
#else
	    static mach_msg_type_t RetCodeType = {
	        /* msgt_name */			MACH_MSG_TYPE_INTEGER_32,
	     	/* msgt_size */			32,
		/* msgt_number */		1,
	    	/* msgt_inline */		TRUE,
	    	/* msgt_longform */		FALSE,
	    	/* msgt_deallocate */		FALSE,
		/* msgt_unused */		0
	    };
	    mig_reply_header_t *OutP = (mig_reply_header_t *)OutHeadP;
	    OutP->RetCodeType = RetCodeType;
	    OutP->RetCode = MIG_BAD_ID;
	    OutP->Head.msgh_size = sizeof *OutP;
#endif
	    return (FALSE);
	}

#if UNTYPED_IPC
	trailer = (mach_msg_format_0_trailer_t *)((vm_offset_t)InHeadP +
		round_msg(InHeadP->msgh_size));
	seqno = trailer->msgh_seqno;
	hdr->msgh_size = sizeof(struct bsd_reply_simple);
	hdr->msgh_reserved = 0;
#else
	rep->int_type = bsd_rep_int_type;
	seqno = req->hdr.msgh_seqno;
	hdr->msgh_size = sizeof(struct bsd_reply);
#endif

	/* Get process -- locks p */
	p = proc_receive_lookup(req->hdr.msgh_local_port, seqno);
	assert(p);		/* XXX remove. debug only */
	if (!p)
	    return FALSE;

	/* attach process state to invocation state */
	pk->k_p = p;

	/*
	 * Set up server thread to handle process -- unlocks p
	 */
	if ((rep->retcode = start_server_op(p, req->syscode))
		!= KERN_SUCCESS)
	{
	    return (TRUE);
	}

	/*
	 * Save the reply msg and initialize current_output.
	 * The user_copy/user_reply_msg code uses them for copyout.
	 */
	pk->k_reply_msg = hdr;
	pk->k_current_size = 0;
	pk->k_ool_count = 0;

	retval[0] = 0;
	retval[1] = req->rval2;

	/*
	 * Find the system call table descriptor for this call.
	 */
	if (syscode < 0) {
	    callp = &sysent[0];
	}
	else {
	    if (syscode >= nsysent)
		callp = &sysent[0];
	    else
		callp = &sysent[syscode];
	}

	unix_master();

	/*
	 * Catch any signals.  If no other error and not restartable,
	 * return EINTR.
	 */
#if	SYSCALLTRACE
	    pid = p->p_pid;
#if 0
	    if (syscalltrace &&
		    (syscalltrace == pid || syscalltrace < 0)) {

		register int	j;
		char		*cp;

		if (syscode >= nsysent ||
			syscode < 0)
		    printf("[%d]%d", pid, syscode);
		else
		    printf("[%d]%s", pid, syscallnames[syscode]);

		cp = "(";
		for (j = 0; j < callp->sy_narg; j++) {
		    printf("%s%x", cp, req->arg[j]);
		    cp = ", ";
		}
		if (j)
		    cp = ")";
		else
		    cp = "";
		printf("%s\n", cp);
	    }
#endif 0
#endif	SYSCALLTRACE
	    /*
	     * OSF1_SERVER: Do the system call with arguments on the stack
	     */
	    if (callp->sy_call == NULL)
		panic("ux_generic_server: NULL sy_call");
	    error = (*callp->sy_call)(
			p,
			req->arg,
			retval);

	/* 
	 * NOTE: master lock must be held from exit to now in order
	 * to avoid someone reclaiming p.
	 * RENOTE: procs are now reference counted so no problem.
	 */
	unix_release();

	if (pk->k_master_lock > 0) {
	    panic("Master still held", pk->k_master_lock);
	    pk->k_master_lock = 0;
	}

	if (pk->k_master_lock < 0) {
	    panic("Master not held", pk->k_master_lock);
	    pk->k_master_lock = 0;
	}

	/* if exiting, deregister and short circut end_server_op */
	if (p->p_task == MACH_PORT_NULL) { 
	    proc = NULL;
	    server_thread_deregister(p);
	} else {
	    proc = p;
	}

	/*
	 * Wrap up any trailing data in the reply message.
	 */
	if (proc) finish_reply_msg();

	rep->retcode = end_server_op(proc, error, &rep->interrupt);
	rep->rval[0] = retval[0];
	rep->rval[1] = retval[1];

#if SYSCALLTRACE
	if (syscalltrace &&
		(syscalltrace == pid || syscalltrace < 0)) {
	    printf(" (%x,%x)", rep->rval[0], rep->rval[1]);
#if	0 
	    if (syscode >= nsysent ||
		    syscode < 0)
		printf("    [%d]%d", pid, syscode);
	    else
		printf("    [%d]%s", pid, syscallnames[syscode]);
	    printf(" returns %d", rep->retcode);
	    printf(" (%x,%x)", rep->rval[0], rep->rval[1]);
	    printf("%s\n",
		   (rep->interrupt) ? " Interrupt" : "");
#endif 0
	}
#endif	SYSCALLTRACE

	/* XXX OSF XXX post process hdr !!! XXX */
	return (TRUE);
}

#if 0
/* 
 *  rpsleep - perform a resource pause sleep
 *
 *  rsleep = function to perform resource specific sleep
 *  arg1   = first function parameter
 *  arg2   = second function parameter
 *  mesg1  = first component of user pause message
 *  mesg2  = second component of user pause message
 *
 *  Display the appropriate pause message on the user's controlling terminal.
 *  Save the current non-local goto information and establish a new return
 *  environment to transfer here.  Invoke the supplied function to sleep
 *  (possibly interruptably) until the resource becomes available.  When the
 *  sleep finishes (either normally or abnormally via a non-local goto caused
 *  by a signal), restore the old return environment and display a resume
 *  message on the terminal.  The notify flag bit is set when the pause message
 *  is first printed.  If it is cleared on return from the function, the
 *  continue message is printed here.  If not, this bit will remain set for the
 *  duration of the polling process and the rpcont() routine will be called
 *  directly from the poller when the resource pause condition is no longer
 *  pending.
 *
 *  Return: true if the resource has now become available, or false if the wait
 *  was interrupted by a signal.
 */

boolean_t
rpsleep(rsleep, arg1, arg2, mesg1, mesg2)
int (*rsleep)();
int arg1;
int arg2;
char *mesg1;
char *mesg2;
{
    label_t lsave;
    boolean_t ret = TRUE;

    if ((u.u_rpswhich&URPW_NOTIFY) == 0)
    {
        u.u_rpswhich |= URPW_NOTIFY;
	uprintf("[%s: %s%s, pausing ...]\r\n", u.u_comm, mesg1, mesg2);
    }

    bcopy((caddr_t)&u.u_qsave, (caddr_t)&lsave, sizeof(lsave));
    if (setjmp(&u.u_qsave) == 0)
	(*rsleep)(arg1, arg2);
    else
	ret = FALSE;
    bcopy((caddr_t)&lsave, (caddr_t)&u.u_qsave, sizeof(lsave));

    if ((u.u_rpswhich&URPW_NOTIFY) == 0)
	rpcont();
    return(ret);
}


/* 
 *  rpcont - continue from resource pause sleep
 *
 *  Clear the notify flag and print the continuation message on the controlling
 *  terminal.  When this routine is called, the resource pause condition is no
 *  longer pending and we can afford to clear all bits since only the notify
 *  bit should be set to begin with.
 */

rpcont()
{
    u.u_rpswhich = 0;
    uprintf("[%s: ... continuing]\r\n", u.u_comm);
}


#endif 0

/* 
 * A bad request was received.
 * Take care of sequence numbers anyway.
 * Destroy request message.
 */
boolean_t
bad_request_server(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{
	mach_msg_seqno_t seqno;
#if UNTYPED_IPC
	mig_reply_error_t *out = (mig_reply_error_t *)OutHeadP;

	mach_msg_format_0_trailer_t *trailer
	  = (mach_msg_format_0_trailer_t *)((vm_offset_t)InHeadP +
		round_msg(InHeadP->msgh_size));

	seqno = trailer->msgh_seqno;

	out->NDR = NDR_record;
#else
	struct out {
		mach_msg_header_t hdr;
		mig_reply_header_t reply;
	} *out = (struct out *) OutHeadP;

	seqno = InHeadP->msgh_seqno;
#endif

	warning_panic("bad_request_server");

	/* Increment sequence number */
	port_object_receive_lookup(InHeadP->msgh_local_port,
				   seqno,
				   POT_INVALID);

	/* Make sure the user gets some error */
#if UNTYPED_IPC
	if (out->RetCode == KERN_SUCCESS)
	    out->RetCode = ENOSYS;
#else
	if (out->reply.RetCode == KERN_SUCCESS)
	    out->reply.RetCode = ENOSYS;
#endif
}
