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
 * $Log: e_exception.c,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/10/19  20:21:44  sclawson
 * get rid of the reference to EFL_USER_SET, which shouldn't have been
 * exported from Mach anyway.
 *
 * Revision 1.1.1.2  1995/03/23  01:15:40  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/i386/e_exception.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	Exception handler. Currently just does Linux system call
 *	fixup.
 */

#include <e_defs.h>
#include <i386/eflags.h>
#include <mach/exception.h>
#include <i386/trap.h>

/* XXX all the trap.h files are a mess */
#define T_GENERAL_PROTECTION    13

extern exc_server();
void port_consume(mach_port_t);
void fixup_linux_syscall_state(thread_t, struct i386_thread_state *);

noreturn e_thread_kill_self()
{
	(noreturn) thread_terminate(mach_thread_self());
}

void e_fork_a_thread(void (*func)())
{
	kern_return_t kr;
	vm_address_t stack_start;
	thread_t thread;
	struct i386_thread_state state;
	vm_offset_t stack_end;
	vm_size_t stack_size = vm_page_size * 16;

	/* Make stack. Keep it away from sbrk! */
	for (stack_start = 0xa8000000;
	     stack_start < VM_MAX_ADDRESS;
	     stack_start += stack_size)
	{
		kr = vm_allocate(mach_task_self(),
				 &stack_start,
				 stack_size,
				 FALSE);
		if (kr == KERN_SUCCESS)
		    break;
	}
	if (kr != KERN_SUCCESS)
	    emul_panic("e_fork_a_thread allocate stack");

	stack_end = stack_start + stack_size;

	/* Make initial frame */
	stack_end -= 4;
	*(int *)stack_end = (int) e_thread_kill_self;

	/* Fork off a thread */
	kr = thread_create(mach_task_self(), &thread);
	if (kr)
	    emul_panic("e_fork_a_thread thread_create");
	
	/* Fill other registers with reasonable defaults */
	emul_save_state(&state);
	state.edi = 0;
	state.esi = 0;
	state.ebp = 0;
	state.ebx = 0;
	state.edx = 0;
	state.ecx = 0;
	state.eax = 0;
	state.efl = 0;

	/* And PC and SP with real values */
	state.eip = (int) func;
	state.uesp = stack_end;

	/* Write the state into the registers */
	kr = thread_set_state(thread,
			      i386_THREAD_STATE,
			      (thread_state_t) &state,
			      i386_THREAD_STATE_COUNT);
	if (kr)
	    emul_panic("e_fork_a_thread thread_set_state");

	/* And off it goes */
	kr = thread_resume(thread);
	if (kr)
	    emul_panic("e_fork_a_thread thread_resume");
}


mach_port_t original_exception_port = MACH_PORT_NULL;
mach_port_t e_exception_port = MACH_PORT_NULL;

/* 
 * e_exception_setup is always called by the original thread.
 * This now works for Linux syscalls. Generalize later.
 */
void e_exception_setup()
{
	kern_return_t kr;

#if 0
	/* Called by child_init after fork as well */
	if (original_exception_port != MACH_PORT_NULL)
	    emul_panic("e_exception_setup called twice");
#endif

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&e_exception_port);
	if (kr)
	    emul_panic("e_exception_setup mach_port_allocate");

	/* A send right is needed for task_set_special_port */
	kr = mach_port_insert_right(mach_task_self(), e_exception_port,
				    e_exception_port, MACH_MSG_TYPE_MAKE_SEND);
	if (kr)
	    emul_panic("e_exception_setup mach_port_insert_right");

#if OSFMACH3
	{
	exception_mask_t oldmask;
	mach_msg_type_number_t count = 1;
	exception_behavior_t oldbehavior;
	thread_state_flavor_t oldflavor;

	kr = thread_get_exception_ports(mach_thread_self(),
					EXC_BAD_INSTRUCTION,
					&oldmask,
					&count,
					&original_exception_port,
					&oldbehavior,
					&oldflavor);
	}
#else
	kr = thread_get_special_port(mach_thread_self(),
				     THREAD_EXCEPTION_PORT,
				     &original_exception_port);
#endif
	if (kr)
	    emul_panic("e_exception_setup thread_get_exception_port");

	if (!MACH_PORT_VALID(original_exception_port)) {
#if OSFMACH3
		{
		exception_mask_t oldmask;
		mach_msg_type_number_t count = 1;
		exception_behavior_t oldbehavior;
		thread_state_flavor_t oldflavor;

		kr = task_get_exception_ports(mach_task_self(),
					      EXC_BAD_INSTRUCTION,
					      &oldmask,
					      &count,
					      &original_exception_port,
					      &oldbehavior,
					      &oldflavor);
		}
#else
		kr = task_get_special_port(mach_task_self(),
					   TASK_EXCEPTION_PORT,
					   &original_exception_port);
#endif
		if (kr)
		    emul_panic("e_exception_setup task_get_exception_port");
	}

#if OSFMACH3
	kr = thread_set_exception_ports(mach_thread_self(),
					EXC_BAD_INSTRUCTION,
					e_exception_port,
					EXCEPTION_DEFAULT, /* optimize! */
					0);
#else
	kr = thread_set_special_port(mach_thread_self(), THREAD_EXCEPTION_PORT,
				     e_exception_port);
#endif
	if (kr)
	    emul_panic("e_exception_setup task_set_exception_port");

	/* The send right is no longer needed */
	kr = mach_port_mod_refs(mach_task_self(), e_exception_port,
				MACH_PORT_RIGHT_SEND, -1);
	if (kr)
	    emul_panic("e_exception_setup mach_port_mod_refs");

}

void e_exception_thread()
{
	e_mach_msg_server(exc_server, 4096, e_exception_port);
	emul_panic("e_mach_msg_server returned");
}

kern_return_t catch_exception_raise(
	mach_port_t	port,
	mach_port_t	thread,
	mach_port_t	task,
	integer_t	exception,
	integer_t	code,
	integer_t	subcode)
{
	struct i386_thread_state state;
	kern_return_t kr;

	/* sanity */
	if (task != mach_task_self())
	    emul_panic("catch_exception_raise: not me!");

	/* XXX Generalize later */
	if (exception == EXC_BAD_INSTRUCTION
	    && code == T_GENERAL_PROTECTION
	    && check_for_linux_syscall(thread, &state))
	{
		fixup_linux_syscall_state(thread, &state);
#if 0
		/* MiG destroys the send rights */
		return KERN_FAILURE;
#endif
	} else {
		if (!MACH_PORT_VALID(original_exception_port))
		    emul_panic("catch_excpeption_raise: original_exception_port");
		kr = exception_raise(original_exception_port,
				     thread, task, exception, code, subcode);
		if (kr)
		    e_emulator_error("exception_raise: x%x", kr);
	}
	port_consume(thread);
	port_consume(task);
	return KERN_SUCCESS;
}

#if OSFMACH3
kern_return_t catch_exception_raise_state(
	mach_port_t		port,
	exception_type_t	exception,
	exception_type_t	code,
	exception_data_t	subcode,
	thread_state_flavor_t	*flavor,
	thread_state_t		in_state,
	thread_state_t		out_state)
{
	emul_panic("catch_exception_raise_state");
}

kern_return_t catch_exception_raise_state_identity(
	mach_port_t		port,
	mach_port_t		thread,
	mach_port_t		task,
	exception_type_t	exception,
	exception_type_t	code,
	exception_data_t	subcode,
	thread_state_flavor_t	*flavor,
	thread_state_t		in_state,
	thread_state_t		out_state)
{
	emul_panic("catch_exception_raise_state_identity");
}
#endif /* OSFMACH3 */

void port_consume(mach_port_t port)
{
	/* XXX Optimize: delete many ports at once */
	mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_SEND, -1);
}

boolean_t check_for_linux_syscall(
	thread_t thread,
	struct i386_thread_state *state)
{
	kern_return_t kr;
	mach_msg_type_number_t count = i386_THREAD_STATE_COUNT;
	unsigned short instruction; /* int 0x80 is a 16 bit instruction */

	kr = thread_get_state(thread, i386_THREAD_STATE,
			      (thread_state_t) state, &count);
	if (kr)
	    emul_panic("check_for_linux_syscall thread_get_state");

	instruction = *(unsigned short *)state->eip;
	if (instruction == 0x80cd) /* int 0x80 */
	    return TRUE;
	else
	    return FALSE;
}

extern emul_linux_trampoline();

void fixup_linux_syscall_state(
	thread_t thread,
	struct i386_thread_state *state)
{
	kern_return_t kr;
	vm_offset_t pc, sp;

	kr = thread_suspend(thread);
	if (kr)
	    emul_panic("fixup_linux_syscall thread_suspend");
	kr = thread_abort(thread);
	if (kr)
	    emul_panic("fixup_linux_syscall thread_abort");

	sp = state->uesp;
	pc = state->eip;

	/* move past int 0x80 instruction */
	pc += 2;

	/* Push old pc on stack */
	sp -= 4;
	*(int *)sp = pc;

	/* Go to trampoline */
	state->eip = (int) emul_linux_trampoline;
	state->uesp = sp;

	kr = thread_set_state(thread, i386_THREAD_STATE,
			      (thread_state_t) state, i386_THREAD_STATE_COUNT);
	if (kr)
	    emul_panic("fixup_linux_syscall thread_set_state");
	kr = thread_resume(thread);
	if (kr)
	    emul_panic("fixup_linux_syscall thread_resume");
}
