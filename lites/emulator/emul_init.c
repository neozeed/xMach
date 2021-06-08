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
 * 02-Jan-95  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Make child_init() call a new function, e_signal_child_setup().
 *
 * 18-Nov-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Make sp aligned to an word boundary after args are loaded.
 *
 * $Log: emul_init.c,v $
 * Revision 1.3  2000/11/26 01:35:55  welchd
 *
 * Fixed to getdents interface
 *
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
 *	File:	emulator/emul_init.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Initialization for the Lites emulator.
 */

/*
 * Setup emulator entry points.
 */
#include <e_defs.h>
#include <mach.h>
#include <mach/message.h>
#include <bsd_types_gen.h>

//int syscall_debug = 100;
int syscall_debug = 0;

void e_signal_child_setup(void);


/*
 * Initialize emulator.  Our bootstrap port is the BSD request port.
 */
mach_port_t	our_bsd_server_port;
mach_port_t process_self()
{
	return our_bsd_server_port;
}

void emul_init()
{
	task_get_bootstrap_port(mach_task_self(), &our_bsd_server_port);
	emul_mapped_init();
}

extern errno_t (*emul_run_function)();

int emulator_main(natural_t sp)
{
	char **envp, **argv;
	integer_t *kframe;
	int argc;
	char *XXXargv[] = {"ZAP", "OLDEMUL", "FOO", 0};
	kern_return_t kr;
	char fname[PATH_LENGTH];
	mach_port_t image_port;

	/*
	 * Initialize the emulator.
	 */
	emul_init();
   
	kr = emul_setup_argv(sp, &argv, &argc, &envp, &kframe, fname,
			     &image_port);
	if (kr) {		/* XXX */
		/* XXX probably called from old emulator */
		argv = XXXargv;
		argc = 3;
	}

	return emul_trap_run(argc, argv, envp, kframe, fname,
				    image_port);
}

extern boolean_t with_kframe;

mach_error_t emul_setup_argv(
	natural_t	old_sp,
	char	 	***argv,	/* OUT */
	int 		*argc,		/* OUT */
	char	 	***envp,	/* OUT */
	integer_t 	**kframep,	/* OUT */
	char		*fname,		/* OUT (space allocated by caller) */
	mach_port_t	*image_port)	/* OUT */
{
	int i;
	vm_address_t arg_addr;
	vm_size_t arg_size, space_needed;
	int arg_count, env_count;
	mach_msg_type_number_t emul_name_count, fname_count, cfname_count;
	mach_msg_type_number_t cfarg_count;
	char emul_name[PATH_LENGTH], cfname[PATH_LENGTH];
	char cfarg[PATH_LENGTH];
	kern_return_t kr;
	char *ap;
	char **av;
	boolean_t with_interpreter;
	boolean_t with_cfarg;

	emul_name[0] = cfname[0] = cfarg[0] = fname[0] = '\0';

	emul_name_count = PATH_LENGTH;
	fname_count = PATH_LENGTH;
	cfname_count = PATH_LENGTH;
	cfarg_count = PATH_LENGTH; /* not a path really but arguments */
	kr = bsd_after_exec(our_bsd_server_port,
			    &arg_addr, &arg_size, &arg_count, &env_count,
			    image_port,
			    emul_name, &emul_name_count,
			    fname, &fname_count,
			    cfname, &cfname_count,
			    cfarg, &cfarg_count);
	if (kr == LITES_EALREADY) {
		*envp = (char **) 0;
		*argv = (char **) 0;
		*argc = 0;
		return kr;
	}
	if (kr != KERN_SUCCESS) {
		e_emulator_error("bsd_after_exec failed kr = x%x", kr);
		task_terminate(mach_task_self());
	}
	with_interpreter = cfname_count > 0 && cfname[0] != '\0';
	with_cfarg = cfarg_count > 0 && cfarg[0] != '\0' && with_interpreter;
	/* zero terminate all strings */
	if (emul_name_count < 1)
	    emul_name[0] = '\0';
	else
	    emul_name[emul_name_count - 1] = '\0';
	if (fname_count < 1)
	    fname[0] = '\0';
	else
	    fname[fname_count - 1] = '\0';
	if (cfname_count < 1)
	    cfname[0] = '\0';
	else
	    cfname[cfname_count - 1] = '\0';
	if (cfarg_count < 1)
	    cfarg[0] = '\0';
	else
	    cfarg[cfarg_count - 1] = '\0';

	/* 
	 * This is what we'll have
	 * argc, [cfnamep cfargp] argv0p argv1p ... argvNp 0 env0p env1p ... envNp 0
	 * [cfname 0 cfarg 0 fname 0 ]
	 * argv0 0 argv1 0 ... argvN 0 env0 0 env1 0 ... envN 0
	 * space trampoline kernel
	 *
	 * The first line is char pointers, the second and third chars.
	 * The first line always immediately precedes the second.
	 * The third line normally immediately precedes the third but
	 * that is not necessary.
	 * 
	 * cfnamep, cfargp, cfname, cfarg, and fname are there only if the
	 * program is a shell script.
	 */
	space_needed = (env_count + arg_count + 2) * sizeof(char *);
	if (with_interpreter) {
		space_needed += strlen(cfname) + 1;
		space_needed += strlen(fname) + 1;
		space_needed += strlen(cfarg) + 1;
		space_needed += sizeof(char *); /* for cfname */
		if (with_cfarg)
		    space_needed += sizeof(char *);
	}
	if (with_kframe)
	    space_needed += sizeof(natural_t);

	/* Round up to word boundary to keep stack aligned */
	space_needed = ((space_needed + sizeof(natural_t) - 1)
	  		& ~(sizeof(natural_t) - 1));
#if 0
	/* The server now makes the stack big enough */
	if ((space_needed > vm_page_size) && 0) {
		/* 
		 * deallocate old junk stack and allocate new space
		 * there. For now, just allocate some space.
		 */
		vm_allocate(mach_task_self(),
			    (vm_address_t *) &av,
			    round_page(space_needed),
			    TRUE);
	} else
#endif
#if STACK_GROWS_UP
	av = (char **) (arg_addr + arg_size);
#else
	av = (char **) (arg_addr - space_needed);
#endif

	*kframep = (integer_t *) av;
	if (with_kframe) {
		/* For compatibility with traditional kframe */
		av++;
	}

	*argv = av;
	if (with_interpreter)
	    av++;		/* room for cfname and cfarg */
	if (with_cfarg)
	    av++;
	ap = (char *) arg_addr;
	for (i = 0; i < arg_count; i++) {
		*av++ = ap;
		/* find next null */
		while (*ap++)
		    ;
	}
	*argc = i;
	*av++ = (char *) 0;
	*envp = av;
	for (i = 0; i < env_count; i++) {
		*av++ = ap;
#if 1
		if (!memcmp(ap, "EMULATOR_DEBUG=",
			     sizeof("EMULATOR_DEBUG=") - 1))
		    syscall_debug = *(ap + sizeof("EMULATOR_DEBUG=")-1) - '0';
#endif
		while (*ap++)
		    ;
	}
	*av++ = (char *) 0;
	if (with_interpreter) {
		/* copy cfname, cfarg, and fname after the vectors */
		ap = (char *) av;
		(*argv)[0] = ap;
		strcpy(ap, cfname);
		ap += strlen(cfname) + 1;
		*argc += 1;

		if (with_cfarg) {
			(*argv)[1] = ap;
			strcpy(ap, cfarg);
			ap += strlen(cfarg) + 1;
			*argc += 1;
		}

		/* 
		 * Use fname only if it's not empty. Otherwise use the
		 * original name.
		 *
		 * Makes me wonder if this is of any use in any case
		 * or if it should be done always. The choice is to
		 * either believe what the caller put in argv[0] or as
		 * the first arg to exec. There is a clear redundancy
		 * there.
		 */
		if (strlen(fname) > 0) {
			if (with_cfarg)
			    (*argv)[2] = ap;
			else
			    (*argv)[1] = ap;
			strcpy(ap, fname);
			ap += strlen(fname) + 1;
			/* Argc does not change as this was an overwrite */
		}
		/* 
		 * fname is no longer needed here but is used by
		 * emul_exec to determine what program should be
		 * executed. So make it cfname instead.
		 */
		strcpy(fname, cfname);
	}
	if (with_kframe)
	    **kframep = (integer_t) *argc;
	return KERN_SUCCESS;
}

void (*emul_child_init_func)() = 0;

/*
 * Initialize emulator on child branch of fork.
 */
void child_init()
{
	/* XXX ras support */
	mach_init();		/* reset mach_task_self_ ! */
	emul_init();
	mig_init(0);		/* arg is not used */
	child_reinit_mapped_time();
	e_signal_child_setup();

	if (emul_child_init_func)
	    (*emul_child_init_func)();
}

/*
 * Fail in an interesting, easy-to-debug way.
 */
void
emul_panic(msg)
	char *msg;
{
	e_emulator_error("emul_panic: %s. Suspending self\n", msg);
	task_suspend(mach_task_self());
}
