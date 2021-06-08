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
 * $Log: e_machinedep.c,v $
 * Revision 1.2  2000/10/27 01:55:28  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:38  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/i386/e_machinedep.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Machine dependent stuff for the i386.
 */

#include <e_defs.h>
#include <syscall_table.h>
#include <i386/eflags.h>

binary_type_t e_my_binary_type;

char *atbin_names[] =
{
	ATSYS_NAMES("i386_")
};

errno_t
e_fork_call(boolean_t isvfork, pid_t *pid, boolean_t *ischild)
{
	struct i386_thread_state	state;
	
	int rv[2];
	errno_t error;

	volatile int x;

	/*
	 * Set up registers for child.  It resumes on its own stack.
	 */

	/* Prevent caching of caller saved registers. They are destroyed */
	asm volatile("nop"
		     : /* no inputs */
		     : /* no outputs */
		     : "eax", "edx", "ecx", "cc");
	x = emul_save_state(&state);
	asm volatile("nop"
		     : /* no inputs */
		     : /* no outputs */
		     : "eax", "edx", "ecx", "cc");
	if (x != 0) {
		*ischild = TRUE;
		*pid = x;
		child_init();
		if (syscall_debug > 1)
		    e_emulator_error("CHILD parent=%d", x);
		return 0;
	}

	state.eax = 1;
	
	/* FP regs!!!! */

	/*
	 * Create the child.
	 */
	error = (isvfork?bsd_vfork:bsd_fork)
	    (our_bsd_server_port,
	     (int *)&state,
	     i386_THREAD_STATE_COUNT,
	     rv);

	if (error == 0)
	    *pid = rv[0];

	*ischild = FALSE;
	return error;

}

/* Structure xorresponds to Linux 0.99.15h */
struct linux_sigcontext {
        unsigned short gs, __gsh;
        unsigned short fs, __fsh;
        unsigned short es, __esh;
        unsigned short ds, __dsh;
        unsigned long edi;
        unsigned long esi;
        unsigned long ebp;
        unsigned long esp;
        unsigned long ebx;
        unsigned long edx;
        unsigned long ecx;
        unsigned long eax;
        unsigned long trapno;
        unsigned long err;
        unsigned long eip;
        unsigned short cs, __csh;
        unsigned long eflags;
        unsigned long esp_at_signal;
        unsigned short ss, __ssh;
        unsigned long i387;
        unsigned long oldmask;
        unsigned long cr2;
};

struct	cmu_43ux_sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore */
	int	sc_gs;
	int	sc_fs;
	int	sc_es;
	int	sc_ds;
	int	sc_edi;
	int	sc_esi;
	int	sc_ebp;
	int	sc_esp;
	int	sc_ebx;
	int	sc_edx;
	int	sc_ecx;
	int	sc_eax;
	int	sc_trapno;	/* err & trapno keep the context */
	int	sc_err;		/* switching code happy */
	int	sc_eip;
	int	sc_cs;
	int	sc_efl;
	int	sc_uesp;
	int	sc_ss;
    };

struct mach386_sigframe {
	int	sf_signum;
	int	sf_code;
	struct cmu_43ux_sigcontext *sf_scp;
	int	sf_handler;
	struct cmu_43ux_sigcontext sf_sc;
    };

struct sigframe {
	int	sf_signum;
	int	sf_code;
	struct	sigcontext *sf_scp;
	sig_t	sf_handler;
	int	sf_eax;	
	int	sf_edx;	
	int	sf_ecx;	
	struct	sigcontext sf_sc;
} ;

boolean_t linux_one_shots = FALSE;

/* Take a signal */
boolean_t take_a_signal(struct i386_thread_state *state)
{
	int		old_onstack, sig, code, new_sp;
	sigset_t	old_mask;
	int		error;
	void (*handler)();
	/*
	 * Get the signal to take from the server.  It also
	 * switches the signal mask and the stack, so we must -- XXX Not stack
	 * be off the old user stack before calling it.
	 */
	error = bsd_take_signal(our_bsd_server_port,
				&old_mask,
				&old_onstack,
				&sig,
				&code,
				(int *) &handler,
				&new_sp);

	/*
	 * If there really were no signals to take, return.
	 */
	if (error || sig == 0)
	    return FALSE;

	if (syscall_debug > 1)
	    e_emulator_error("SIG%d x%x", sig, handler);

	/*
	 * Put the signal context and signal frame on the signal stack.
	 */
	if (new_sp != 0) {
		e_emulator_error("take_signals: stack switch needed");
	    /*
	     * Build signal frame and context on user's stack.
	     */
	}
	/*
	 * Build the signal context to be used by sigreturn.
	 */

	switch(e_my_binary_type) {
	      case BT_CMU_43UX:
		{
			struct cmu_43ux_sigcontext sc;

			/*
			 * Build the signal context to be used by sigreturn.
			 */
			sc.sc_onstack = old_onstack;
			sc.sc_mask = old_mask;

			sc.sc_edx = state->edx;
			sc.sc_ecx = state->ecx;
			sc.sc_eax = state->eax;
			sc.sc_ebx = state->ebx;
			sc.sc_edi = state->edi;
			sc.sc_esi = state->esi;
			sc.sc_eip = state->eip;
			sc.sc_efl = state->efl;
			sc.sc_uesp = state->uesp;
			sc.sc_ebp = state->ebp;

			/* Then just call it */
			(*handler)(sig, code, &sc);

			state->edx = sc.sc_edx;
			state->ecx = sc.sc_ecx;
			state->eax = sc.sc_eax;
			state->ebx = sc.sc_ebx;
			state->edi = sc.sc_edi;
			state->esi = sc.sc_esi;
			state->eip = sc.sc_eip;
			state->efl = sc.sc_efl;
			state->uesp = sc.sc_uesp;
			state->ebp = sc.sc_ebp;

			break;
		}
	      case BT_LINUX:
	      case BT_LINUX_ELF:
		{
			struct linux_sigcontext sc;
			/* Take care of one shot signals. XXX Breaks bash. */
			if (linux_one_shots)
			    e_linux_one_shot_signal_check_and_eliminate(sig);

			/*
			 * Build the signal context to be used by sigreturn.
			 */
			sc.oldmask = old_mask;

			sc.edx = state->edx;
			sc.ecx = state->ecx;
			sc.eax = state->eax;
			sc.ebx = state->ebx;
			sc.edi = state->edi;
			sc.esi = state->esi;
			sc.eip = state->eip;
			sc.eflags = state->efl;
			sc.esp_at_signal = state->uesp;
			sc.ebp = state->ebp;

			sig = signal_number_bsd_to_linux(sig);

			/* Then just call it */
			(*handler)(sig, code, &sc);

			state->edx = sc.edx;
			state->ecx = sc.ecx;
			state->eax = sc.eax;
			state->ebx = sc.ebx;
			state->edi = sc.edi;
			state->esi = sc.esi;
			state->eip = sc.eip;
			state->efl = sc.eflags;
			state->uesp = sc.esp_at_signal;
			state->ebp = sc.ebp;
			break;
		}
	      default:
		{
			struct sigcontext sc;

			sc.sc_onstack = old_onstack;
			sc.sc_mask = old_mask;

			sc.sc_pc = state->eip;
			sc.sc_ps = state->efl;
			sc.sc_sp = state->uesp;
			sc.sc_fp = state->ebp;

			/* Then just call it */
			(*handler)(sig, code, &sc);

			state->eip = sc.sc_pc;
			state->efl = sc.sc_ps;
			state->uesp = sc.sc_sp;
			state->ebp = sc.sc_fp;

			state->eax = 1;
		}
	}

	if (syscall_debug > 2)
	    e_emulator_error("return_SIG%d", sig);

	/* Re-enable signal */
	error = bsd_sigreturn(our_bsd_server_port,
			      old_onstack & 01,
			      old_mask);
	return TRUE;
}

/* 
 * Take some signals. Called after system calls if needed.
 *
 * XXX This signal code does not care about sigstacks and similar
 * XXX messy stuff. However, it works in most simple cases.
 */
void take_signals_and_do_sigreturn()
{
	struct i386_thread_state state;
	volatile int x;

	asm volatile("nop"
		     : /* no inputs */
		     : /* no outputs */
		     : "eax", "edx", "ecx", "cc");
	x = emul_save_state(&state);
	asm volatile("nop"
		     : /* no inputs */
		     : /* no outputs */
		     : "eax", "edx", "ecx", "cc");
	/* 
	 * Check for longjmp or sigreturn out of handler.
	 */
	if (x != 0)
	    return;

	while (take_a_signal(&state))
	    ;

	/* Now return where requested or from this function if unchanged */
	if ((state.eax == 0)
	    && (state.eip > EMULATOR_BASE)
	    && (state.eip < EMULATOR_END))
	{
		state.eax = 1;	/* for setjmp test above (avoid loop) */
	}
	emul_load_state(&state);
	/*NOTREACHED*/
}

/* This is needed for gdb (and longjmp in general) to work */
errno_t e_cmu_43ux_sigreturn(struct cmu_43ux_sigcontext *sc)
{
	struct i386_thread_state state;

	emul_save_state(&state);

	state.edi = sc->sc_edi;
	state.esi = sc->sc_esi;
	state.ebp = sc->sc_ebp;
	state.ebx = sc->sc_ebx;
	state.edx = sc->sc_edx;
	state.ecx = sc->sc_ecx;
	state.eax = sc->sc_eax;
	state.eip = sc->sc_eip;
	state.efl = sc->sc_efl;
	state.uesp = sc->sc_uesp;

	if ((state.eax == 0)
	    && (state.eip > EMULATOR_BASE)
	    && (state.eip < EMULATOR_END))
	{
		state.eax = 1;	/* for setjmp test above (avoid loop) */
	}

	if (syscall_debug > 2)
	    e_emulator_error("cmu_43ux_SIGRETURN");

	/* Re-enable signal */
	bsd_sigreturn(our_bsd_server_port,
		      sc->sc_onstack & 01,
		      sc->sc_mask);
	emul_load_state(&state);
	/*NOTREACHED*/
	(noreturn) emul_panic("e_cmu_43ux_sigreturn: _longjmp returned!");
	return 0;		/* avoid bogus warning, never returns */
}

errno_t e_bsd_sigreturn(struct sigcontext *sc)
{
	struct i386_thread_state state;

	emul_save_state(&state);

	state.eip = sc->sc_pc;
	state.efl = sc->sc_ps;
	state.uesp = sc->sc_sp;
	state.ebp = sc->sc_fp;
	state.eax = 1;

	if (syscall_debug > 2)
	    e_emulator_error("bsd_SIGRETURN");

	/* Re-enable signal */
	bsd_sigreturn(our_bsd_server_port,
		      sc->sc_onstack & 01,
		      sc->sc_mask);
	emul_load_state(&state);
	/*NOTREACHED*/
	(noreturn) emul_panic("e_bsd_sigreturn: _longjmp returned!");
	return 0;		/* avoid bogus warning, never returns */
}

errno_t e_linux_sigreturn(struct linux_sigcontext *sc)
{
	struct i386_thread_state state;

	emul_save_state(&state);

	state.edx = sc->edx;
	state.ecx = sc->ecx;
	state.eax = sc->eax;
	state.ebx = sc->ebx;
	state.edi = sc->edi;
	state.esi = sc->esi;
	state.eip = sc->eip;
	state.efl = sc->eflags;
	state.uesp = sc->esp_at_signal;
	state.ebp = sc->ebp;

	if ((state.eax == 0)
	    && (state.eip > EMULATOR_BASE)
	    && (state.eip < EMULATOR_END))
	{
		state.eax = 1;	/* for setjmp test above (avoid loop) */
	}

	if (syscall_debug > 2)
	    e_emulator_error("linux_SIGRETURN");

	/* Re-enable signal */
	bsd_sigreturn(our_bsd_server_port,
		      0, sc->oldmask);
	emul_load_state(&state);
	/*NOTREACHED*/
	(noreturn) emul_panic("e_linux_sigreturn: _longjmp returned!");
	return 0;		/* avoid bogus warning, never returns */
}

#ifdef OSF				/* XXX */
vprintf() {}
vsprintf() {}
sprintf(char *s) {strcpy(s, "SPRINTFXXX"); }

printf()				/* XXX */
{					/* XXX */
	emul_panic("printf called");	/* XXX */
}					/* XXX */
#endif					/* XXX */
