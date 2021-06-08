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
 * $Log: server_exec.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:58  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	server_exec.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	March 1994
 *
 *	Exec server side. The server loads and starts the emulator.
 *	The emulator has the responsibility of mapping the actual
 *	executable.
 */

#include "map_uarea.h"
#include "machid_register.h"
#include "second_server.h"
#include "vnpager.h"

#include <serv/server_defs.h>
#include <sys/exec_file.h>

#include <machine/vmparam.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/assert.h>
#include <bsd_types_gen.h>
#include <sys/namei.h>
#include <sys/acct.h>
#include <sys/filedesc.h>
#include <sys/wait.h>

/* Local prototypes */
static mach_error_t server_exec_open(struct proc *, char *,
				     struct vnode **, binary_type_t *,
				     boolean_t *, boolean_t *,
				     union exec_data *,
				     char *, char *, char *, uid_t *, gid_t *,
				     mach_port_t *, boolean_t *);
static mach_error_t server_exec_load(struct proc *, struct vnode *,
				     binary_type_t, union exec_data *,
				     struct exec_load_info *);
static mach_error_t server_exec_write_trampoline(struct proc *);
static mach_error_t server_exec_set_suid(struct proc *, uid_t, gid_t);
static mach_error_t server_exec(struct proc *, char *, char *, char *,
				char *, struct exec_load_info *,
				mach_port_t *);
void fdcloseexec(struct proc *p);


struct after_exec_state {
	vm_address_t	arg_addr;
	vm_size_t	arg_size;
	int		arg_count;
	int		env_count;
	char		emul_name[PATH_LENGTH];	/* emulator name */
	char		fname[PATH_LENGTH];	/* actual file name */
	char		cfname[PATH_LENGTH];	/* #! interpreter name */
	char		cfarg[PATH_LENGTH];	/* #! interpreter arg */
	mach_port_t	image_port;		/* File handle for emulated */
};

zone_t after_exec_zone = ZONE_NULL;

/* Initialization. Called at boot time. */
void exec_system_init()
{
	after_exec_zone = zinit(sizeof(struct after_exec_state), 1024 * 1024,
				10 * sizeof(struct after_exec_state), TRUE,
				"after exec state");
}

mach_error_t s_execve(
	struct proc	*p,
	char		*fname,
	vm_address_t	old_arg_addr,
	vm_size_t	arg_size,
	int		arg_count,
	int		env_count)
{
	char		emul_name[PATH_LENGTH];
	char		cfname[PATH_LENGTH];
	char		cfarg[PATH_LENGTH];
	mach_port_t	image_port = MACH_PORT_NULL;

	mach_error_t kr;
	pointer_t data;
	mach_msg_type_number_t count;
	mach_port_t old_task;
	vm_offset_t sp;
	struct after_exec_state *state;
	vm_address_t new_arg_addr, stack_start;
	vm_size_t stack_size;
	vm_size_t argv_space;	/* space to leave for building argv. */
	struct exec_load_info li_data;

	arg_size = round_page(arg_size);

	old_task = p->p_task;
	emul_name[0] = cfname[0] = cfarg[0] = '\0';

	/* 
	 * get a copy already here as server_exec will destruct or
	 * clear the task.
	 */
	kr = vm_read(old_task, old_arg_addr, arg_size, &data, &count);
	assert(kr == KERN_SUCCESS); /* XXX refine */

	kr = server_exec(p, fname, emul_name, cfname, cfarg,
			 &li_data, &image_port);

	if (kr != KERN_SUCCESS) {
		if (MACH_PORT_VALID(image_port))
		    mach_port_deallocate(mach_task_self(), image_port);

		vm_deallocate(mach_task_self(), data, count);
		return kr;
	}

	/* 
	 * Still to do after server_exec:
	 * Stack, Arguments, thread state, zap old task, save state,resume new.
	 * LATER: Exception port (and perhaps name server).
	 */

	/* The stack will be put right below the arguments */
	/* Leave some space for the emulator to build argv and envv into */
#define MIN_STACK_SIZE (4096*4096)
	argv_space = round_page((arg_count + env_count + 80) * sizeof(char *));
	stack_size = p->p_vmspace->vm_ssize * vm_page_size;
	if (stack_size < vm_page_size + arg_size + argv_space + MIN_STACK_SIZE)
	{
		vm_size_t new_size;
		/* enlarge stack immediately if it is minimal */
		new_size = (vm_page_size + arg_size + argv_space
			    + MIN_STACK_SIZE);
		p->p_vmspace->vm_ssize = new_size / NBPG;
		stack_size = argv_space + MIN_STACK_SIZE;
	} else {
		stack_size -= vm_page_size;	/* for trampoline */
		stack_size -= arg_size;
	}
#if STACK_GROWS_UP
	new_arg_addr = USRSTACK + vm_page_size;
	stack_start = new_arg_addr + arg_size;
	sp = stack_start + argv_space;
#else
	new_arg_addr = VM_MAX_ADDRESS - vm_page_size;	/* trampoline */
	new_arg_addr -= arg_size;			/* beginning of args */
	new_arg_addr = trunc_page(new_arg_addr);
	stack_start = new_arg_addr - stack_size;
	/* 
	 * Assume that stack grows down and predecrement (otherwise
	 * the first arg will be overwritten).
	 */
	sp = new_arg_addr - argv_space;
#endif
	/* 
	 * Get a copy of the arguments and put it into the new task.
	 * vm_map would be much nicer. Have a memory object and just
	 * remap it. (the read is done above).
	 * XXX Should add some sanity checks here.
	 */
	kr = vm_allocate(p->p_task, &new_arg_addr, count, FALSE);
	assert(kr == KERN_SUCCESS);
	kr = vm_write(p->p_task, new_arg_addr, data, count);
	assert(kr == KERN_SUCCESS);
	kr = vm_deallocate(mach_task_self(), data, count);
	assert(kr == KERN_SUCCESS);
	/* Allocate stack */
	kr = vm_allocate(p->p_task, &stack_start, stack_size, FALSE);
	assert(kr == KERN_SUCCESS);


	/* Set thread state */
	li_data.sp = sp;
	set_emulator_state(p, &li_data);

	/* ok to resume from now on... (requires locking with after_exec) */

	/* Save some state around for after_exec */
	state = (struct after_exec_state *) zalloc(after_exec_zone);
	assert(state != NULL);
	state->arg_addr = new_arg_addr;
	state->arg_size = arg_size;
	state->arg_count = arg_count;
	state->env_count = env_count;
	mig_strncpy(state->fname, fname, PATH_LENGTH-1);
	state->fname[PATH_LENGTH-1] = '\0';
	mig_strncpy(state->emul_name, emul_name, PATH_LENGTH-1);
	state->fname[PATH_LENGTH-1] = '\0';
	mig_strncpy(state->cfname, cfname, PATH_LENGTH-1);
	state->cfname[PATH_LENGTH-1] = '\0';
	mig_strncpy(state->cfarg, cfarg, PATH_LENGTH-1);
	state->cfarg[PATH_LENGTH-1] = '\0';

	state->image_port = image_port;

	assert(p->p_after_exec_state == NULL);
	p->p_after_exec_state = state;

	/* Make it run */
	kr = thread_resume(p->p_thread);
	assert(kr == KERN_SUCCESS); /* XXX refine */

	return KERN_SUCCESS;
}

mach_error_t after_exec(
	struct proc		*p,
	vm_address_t		*arg_addr,	/* OUT */
	vm_size_t		*arg_size,	/* OUT */
	int			*arg_count,	/* OUT */
	int			*env_count,	/* OUT */
	char			*emul_name,	/* OUT */
	mach_msg_type_number_t	*emul_name_count, /* OUT */
	char			*fname,		/* OUT */
	mach_msg_type_number_t	*fname_count,	/* OUT */
	char			*cfname,	/* OUT */
	mach_msg_type_number_t	*cfname_count,	/* OUT */
	char			*cfarg,		/* OUT */
	mach_msg_type_number_t	*cfarg_count,	/* OUT */
	mach_port_t		*image_port)	/* OUT */
{
	struct after_exec_state *state;
	size_t length;

	state = p->p_after_exec_state;
	if (state == NULL)
	    return EALREADY;	/* This op can be done once only. So there! */

	p->p_after_exec_state = NULL;

	*arg_addr = state->arg_addr;
	*arg_size = state->arg_size;
	*arg_count = state->arg_count;
	*env_count = state->env_count;

	length = strlen(state->emul_name) + 1;
	if (*emul_name_count < length)
	    length = *emul_name_count;
	mig_strncpy(emul_name, state->emul_name, length);

	length = strlen(state->fname) + 1;
	if (*fname_count < length)
	    length = *fname_count;
	mig_strncpy(fname, state->fname, length);

	length = strlen(state->cfname) + 1;
	if (*cfname_count < length)
	    length = *cfname_count;
	mig_strncpy(cfname, state->cfname, length);

	length = strlen(state->cfarg) + 1;
	if (*cfarg_count < length)
	    length = *cfarg_count;
	mig_strncpy(cfarg, state->cfarg, length);

	*image_port = state->image_port;

	/* Deallocate state */
	zfree(after_exec_zone, (vm_offset_t) state);

	return KERN_SUCCESS;
}

/* 
 * Cleanup task for exec. Empty its address space and leave just on
 * thread around.
 */
mach_error_t server_exec_cleanup_task(struct proc *p)
{
	thread_array_t thread_list;
	mach_msg_type_number_t thread_count;
	kern_return_t kr;
	int i;
	
	/* Zap all the threads but the first */
	kr = task_threads(p->p_task, &thread_list, &thread_count);
	assert(kr == KERN_SUCCESS);

#if 1
	for (i = 0; i < thread_count; i++)
	    if (thread_list[i] == p->p_thread)
		break;
	assert(i < thread_count);
	for (i = 0; i < thread_count; i++)
	    if (thread_list[i] == p->p_thread) {
		    kr = thread_suspend(p->p_thread);
		    assert(kr == KERN_SUCCESS);
		    kr = thread_abort(p->p_thread);
		    assert(kr == KERN_SUCCESS);
	    } else if (MACH_PORT_VALID(thread_list[i])) {
		    thread_terminate(thread_list[i]);
		    mach_port_deallocate(mach_task_self(), thread_list[i]);
	    }
#else
	/* 
	 * Use the first thread as the new one. Bring it to a known
	 * state.
	 */
	kr = thread_suspend(thread_list[0]);
	assert(kr == KERN_SUCCESS); /* XXX dealloc list, call s_e_c_new_task */
	kr = thread_abort(thread_list[0]);
	assert(kr == KERN_SUCCESS); /* XXX dealloc list, call s_e_c_new_task */
	/* Don't leak refs */
	if (p->p_thread != thread_list[0])
	    mach_port_deallocate(mach_task_self(), p->p_thread);
	/* This is the new primary thread */
	p->p_thread = thread_list[0];
	/* Zap the rest */
	for (i = 1; i < thread_count; i++)
	    if (MACH_PORT_VALID(thread_list[i])) {
		    thread_terminate(thread_list[i]);
		    mach_port_deallocate(mach_task_self(),
					 thread_list[i]);
	    }
#endif
	/* 
	 * Delete uref that came from task_threads.
	 * XXX Should be a port_object function call.
	 * XXX Convert right to po->extra_uref and lazy evaluate
	 */
	kr = mach_port_deallocate(mach_task_self(), p->p_thread);
	assert(kr == KERN_SUCCESS);
	vm_deallocate(mach_task_self(), (vm_offset_t)thread_list,
		      (vm_size_t)thread_count * sizeof(thread_t));
	/* Clear address space */
	kr = vm_deallocate(p->p_task, VM_MIN_ADDRESS,
			   VM_MAX_ADDRESS - VM_MIN_ADDRESS);
	assert(kr == KERN_SUCCESS);
#if MAP_UAREA
	/* Remap shared window */
	kr = mapin_user(p);
	assert(kr == KERN_SUCCESS);
#endif
	return KERN_SUCCESS;
}

/* 
 * Allocate a new empty task. Leave the task itself
 * around but zap all it's threads and remove all Unix
 * resurces from it. The task port will be all that is
 * left in the server space.
 * The new task gets all unix resources, one thread
 * and an empty address space except the mapped uarea
 * that is moved from the old task.
 */
mach_error_t server_exec_create_new_task(struct proc *p)
{
	/* Create the new task */
	mach_port_t new_task, new_thread, old_task;
	mach_port_t previous, new_proc_port;
	thread_array_t thread_list;
	mach_msg_type_number_t thread_count;
	kern_return_t kr;
	int i;

	/* would it be better to use some other parent? */
	kr = task_create(p->p_task,
#if OSF_LEDGERS
			 0, 0,
#endif
			 FALSE,
			 &new_task);
	if (kr != KERN_SUCCESS) {
		printf("server_exec_create_new: task_create failure %x\n", kr);
		return kr;
	}
	kr = thread_create(new_task, &new_thread);
	if (kr != KERN_SUCCESS) {
		task_terminate(new_task);
		mach_port_deallocate(mach_task_self(), new_task);
		printf("server_exec_create_n: thread_create failure %x\n", kr);
		return kr;
	}

	/* Now castrate the old one */
	old_task = p->p_task;

	p->p_thread = new_thread; /* used by task_to_proc_enter */
	new_proc_port = task_to_proc_enter(new_task, p);
	new_task = p->p_task;	/* task_to_proc_enter changed the name */
	kr = task_set_bootstrap_port(new_task, new_proc_port);
	if (kr != KERN_SUCCESS)
	    panic("server_exec_create_new_task: set boostrap x%x\n", kr);

	/* XXX task_set_bootstrap_port port should be polymorphic! */
	/* XXX emulate MACH_MSG_TYPE_MOVE_SEND */
	kr = mach_port_deallocate(mach_task_self(), new_proc_port);
	assert(kr == KERN_SUCCESS);

	/* The old task is now useless. */
	/* As a side effect the procs ref count goes down by two */
	kr = task_terminate(old_task);
	assert(kr == KERN_SUCCESS);
#if 0
	kr = mach_port_deallocate(mach_task_self(), old_task);
	assert(kr == KERN_SUCCESS);
	/* po notification deallocates the port right */

	p->p_task = new_task;
	p->p_thread = new_thread;
#endif

#if MAP_UAREA
	/* Remap shared window */
	kr = mapin_user(p);
#endif
	return KERN_SUCCESS;
}

int copy_args(char **, int *, vm_offset_t *, vm_size_t *, unsigned int	*);

/* 
 * Duplicate work of the emulator. For first program loading.
 *
 * Create an argv transfer structure in the old task. After this a
 * secure_execve system call is simulated. p is the "program calling
 * exec".
 */
mach_error_t kludge_out_args(
	struct proc	*p,		/* OUT */
	vm_address_t	*arg_addr,	/* OUT */
	vm_size_t	*arg_size,	/* OUT */
	int		*arg_count,	/* OUT */
	int		*env_count,	/* OUT */
	char		**argp,
	char		**envp)
{
	unsigned int	char_count = 0;
	vm_address_t my_copy;
	kern_return_t kr;

	*arg_size = vm_page_size;
	kr = vm_allocate(mach_task_self(), &my_copy, *arg_size, TRUE);
	assert(kr == KERN_SUCCESS);

	if (argp) {
	    if (copy_args(argp, arg_count,
			&my_copy, arg_size, &char_count) != 0)
		return E2BIG;
	} else {
	    arg_count = 0;
	}

	if (envp) {
	    if (copy_args(envp, env_count,
			&my_copy, arg_size, &char_count) != 0)
		return E2BIG;
	} else {
	    env_count = 0;
	}

	kr = vm_allocate(p->p_task, arg_addr, *arg_size, FALSE); /* XXX TRUE */
	assert(kr == KERN_SUCCESS);
	kr = vm_write(p->p_task, *arg_addr, my_copy, *arg_size);
	assert(kr == KERN_SUCCESS);
	kr = vm_deallocate(mach_task_self(), my_copy, *arg_size);
	assert(kr == KERN_SUCCESS);
	return KERN_SUCCESS;
}

/* Taken from CMU's emulator */
/*
 * Copy zero-terminated string and return its length,
 * including the trailing zero.  If longer than max_len,
 * return -1.
 */
static int
xcopystr(from, to, max_len)
	register char	*from;
	register char	*to;
	register int	max_len;
{
	register int	count;

	count = 0;
	while (count < max_len) {
	    count++;
	    if ((*to++ = *from++) == 0) {
		return (count);
	    }
	}
	return (-1);
}

/* Taken from CMU's emulator */
int
copy_args(
	char		**argp,
	int		*arg_count,	/* OUT */
	vm_offset_t	*arg_addr,	/* IN/OUT */
	vm_size_t	*arg_size,	/* IN/OUT */
	unsigned int	*char_count)	/* IN/OUT */
{
	register char		*ap;
	register int		len;
	register unsigned int	cc = *char_count;
	register char		*cp = (char *)*arg_addr + cc;
	register int		na = 0;

	while ((ap = *argp++) != 0) {
	    na++;
	    while ((len = xcopystr(ap, cp, *arg_size - cc)) < 0) {
		/*
		 * Allocate more
		 */
		vm_offset_t	new_arg_addr;

		if (vm_allocate(mach_task_self(),
				&new_arg_addr,
				(*arg_size) * 2,
				TRUE) != KERN_SUCCESS)
		    return E2BIG;
		(void) vm_copy(mach_task_self(),
				*arg_addr,
				*arg_size,
				new_arg_addr);
		(void) vm_deallocate(mach_task_self(),
				*arg_addr,
				*arg_size);
		*arg_addr = new_arg_addr;
		*arg_size *= 2;

		cp = (char *)*arg_addr + cc;
	    }
	    cc += len;
	    cp += len;
	}
	*arg_count = na;
	*char_count = cc;
	return (0);
}


extern char emulator_path[];
extern char emulator_old_path[];

boolean_t load_with_emulator = TRUE;
char *emulator_names[] = {emulator_path,
			  emulator_old_path,
			  "/sbin/emulator", 0};

/* 
 * Open a file. Returns vnode locked iff success
 */
mach_error_t server_exec_open_header(
	struct proc	*p,
	char		*fname,
	struct ucred	*cred,
	struct vnode	**vnp,		/* OUT */
	union exec_data	*hdr,		/* OUT (space allocated by caller) */
	int		*hdr_size,	/* OUT */
	char		*canonical_name,/* OUT (space allocated by caller) */
	int		namelen)
{
	struct nameidata nd;
	mach_error_t err;
	struct vnode *vn;

	if (canonical_name) {
		NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | SAVENAME, UIO_SYSSPACE,
		       fname, p);
	} else {
		NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, fname, p);
	}

	err = namei(&nd);
	if (err)
	    return err;

	vn = nd.ni_vp;

	/* is it a regular file? */
	if (vn->v_type != VREG) {
		err = EACCES;
		goto out;
	}

	/* is it executable? */
	err = VOP_ACCESS(vn, VEXEC, cred, p);
	if (err)
	    goto out;

	hdr->shell[0] = '\0';	/* for zero length files */
	err = vn_rdwr(UIO_READ, vn, (caddr_t)hdr, sizeof (*hdr),
		     (off_t)0, UIO_SYSSPACE, (IO_UNIT|IO_NODELOCKED), 
		     cred, hdr_size, p);
	if (err)
	    goto out;

out:
	if (canonical_name)
	    mig_strncpy(canonical_name, nd.ni_cnd.cn_pnbuf, namelen);

	if (nd.ni_cnd.cn_flags & HASBUF) {
		FREE(nd.ni_cnd.cn_pnbuf, M_NAMEI);
		nd.ni_cnd.cn_flags &= ~HASBUF;
	} else if (canonical_name)
	    panic("server_exec_open_header: SAVENAME but HASBUF not set");

	if (err)
	    vput(vn);
	else
	    *vnp = vn;
	return err;
}

/* Exec sanity check on file */
mach_error_t server_exec_native_sanity(
	struct vnode	*vp,
	union exec_data	*hdr,
	struct ucred	*cred,
	struct proc	*p,
	binary_type_t	*binary_type)	/* OUT */
{
	struct vattr vattr;
	mach_error_t err;
	binary_type_t type;

	err = VOP_GETATTR(vp, &vattr, cred, p);
	if (err)
	    return err;

	type = get_binary_type(hdr);

	if (type != BT_LITES_Z && type != BT_LITES_Q
	    && type != BT_LITES_SOM && type != BT_LITES_LITTLE_ECOFF
	    && type != BT_LITES_ELF)
	    return ENOEXEC;
	/* 
	 * sanity check (courtesy of rwd)
	 * "ain't not such thing as a sanity clause" -groucho
	 */
	if ((type == BT_LITES_Q || type == BT_LITES_Z)
	    && (hdr->aout.a_text > MAXTSIZ
		|| hdr->aout.a_text > vattr.va_size
		|| hdr->aout.a_data == 0
		|| hdr->aout.a_data > DFLDSIZ
		|| hdr->aout.a_data > vattr.va_size
		|| (hdr->aout.a_data + hdr->aout.a_text
		    > vattr.va_size)
		|| hdr->aout.a_bss > MAXDSIZ
		|| (hdr->aout.a_text + hdr->aout.a_data
		    + hdr->aout.a_bss > MAXTSIZ + MAXDSIZ)))
	{
		return ENOEXEC;
	}
	*binary_type = type;
	return KERN_SUCCESS;
}

/* Returns vnode locked iff success */
mach_error_t server_exec_open(
	struct proc	*p,
	char		*fname,
	struct vnode	**vnp,		/* OUT */
	binary_type_t	*binary_type,	/* OUT */
	boolean_t	*indir,		/* OUT */
	boolean_t	*emulated,	/* OUT */
	union exec_data	*hdr,		/* OUT (space allocated by caller) */
	char		*emul_name,	/* OUT (space allocated by caller) */
	char		*cfname,	/* OUT (space allocated by caller) */
	char		*cfarg,		/* OUT (space allocated by caller) */
	uid_t		*uidp,		/* OUT */
	gid_t		*gidp,		/* OUT */
	mach_port_t	*image_port,	/* OUT */
	boolean_t	*dangerous)	/* OUT */
{
	mach_error_t err;
	int result, i;
	char *shellname;
	char *cp;
	struct vnode *vp;
	struct vattr vattr;
	struct pcred *pcred = p->p_cred;
	struct ucred *cred;
	int hdr_size;
	binary_type_t type;

	*emulated = FALSE;
	*indir = FALSE;
	*binary_type = BT_BAD;

	cred = pcred->pc_ucred;
	*uidp = pcred->p_ruid;		/* get original uid/gid XXX? */
	*gidp = pcred->p_rgid;

	err = server_exec_open_header(p, fname, cred, &vp, hdr, &hdr_size,
				      0, 0);
	if (err)
	    return err;
	
	err = VOP_GETATTR(vp, &vattr, cred, p);
	if (err) {
		vput(vp);
		return err;
	}
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {	/* no exec on fs */
		vput(vp);
		return EACCES;
	}
	if ((vp->v_mount->mnt_flag & MNT_NOSUID) == 0) {
		if (vattr.va_mode & VSUID) {	/* check for SUID */
			*uidp = vattr.va_uid;
			*dangerous = TRUE;
		}
		if (vattr.va_mode & VSGID) {	/* check for SGID */
			*gidp = vattr.va_gid;
			*dangerous = TRUE;
		}
	}
	/* Check for execute only case */
	if (VOP_ACCESS(vp, VREAD, cred, p) != KERN_SUCCESS)
	    *dangerous = TRUE;

	if (*dangerous && (p->p_flag & P_TRACED)) {
		vput(vp);
		return EACCES;
	}

	/* Shell scripts */
	type = get_binary_type(hdr);
	if (type == BT_SCRIPT) {
		int i, j;
		*indir = TRUE;
		err = ENOEXEC;
		shellname = 0;
		for (i = 2; i <= MAXINTERP; i++) {
			if (hdr->shell[i] == '\n') {
				hdr->shell[i] = '\0';
				err = KERN_SUCCESS;
				break;
			}
			if (hdr->shell[i] == '\t')
			    hdr->shell[i] = ' ';
		}
		if (err) {
			vput(vp);
			return err;
		}
		for (i = 2; hdr->shell[i] == ' '; i++)
		    ;
		shellname = &hdr->shell[i];
		for (; hdr->shell[i] != '\0' && hdr->shell[i] != ' '; i++)
		    ;
		cfarg[0] = '\0';
		if (hdr->shell[i] != '\0') {
			hdr->shell[i] = '\0';
			for (i++; hdr->shell[i] == ' '; i++)
			    ;
			for (j = 0;
			     hdr->shell[i] != ' '
			     && hdr->shell[i] != '\0' && i <= MAXINTERP;
			     i++, j++)
			{
				cfarg[j] = hdr->shell[i];
			}
			cfarg[j] = '\0';
		}
		vput(vp);
		err = server_exec_open_header(p, shellname, cred, &vp,
					      hdr, &hdr_size, cfname,
					      MAXCOMLEN);
		if (err)
		    return err;

		 *uidp = pcred->p_ruid;
		 *gidp = pcred->p_rgid;

		type = get_binary_type(hdr);
	}
	/* Non-native binaries */
	if (type != BT_LITES_Z && type != BT_LITES_Q
	    && type != BT_LITES_SOM && type != BT_LITES_LITTLE_ECOFF
	    && type != BT_LITES_ELF)
	{
		extern struct fileops vnops;
		kern_return_t kr;
		struct file	*file_handle = (struct file *) 0;

		*emulated = TRUE;

		/* 
		 * The current vnode is the one the emulator will need.
		 * Wrap a file handle around it and return it for after_exec.
		 *
		 * XXX undo falloc_port on failure after this!
		 */
		kr = falloc_port(p, &file_handle, image_port);
		assert(kr == KERN_SUCCESS);
		file_handle->f_flag = FREAD; /* read only */
		file_handle->f_type = DTYPE_VNODE;
		file_handle->f_ops = &vnops;
		file_handle->f_data = (caddr_t)vp;
		assert(vp->v_usecount >= 1);

		/* Keep ref but release lock */
		VOP_UNLOCK(vp);
		assert(vp->v_usecount >= 1);

		for (i = 0; emulator_names[i]; i++) {
			err = server_exec_open_header(p, emulator_names[i],
						      cred, &vp,
						      hdr, &hdr_size,
						      emul_name, MAXCOMLEN);
			if (err != KERN_SUCCESS) {
				printf("%s: emulator \"%s\" not found: %d\n",
				       "server_exec_open",
				       emulator_names[i], err);
				continue;
			}
			err = server_exec_native_sanity(vp, hdr, cred, p,
							&type);
			if (err) {
				printf("%s: emulator \"%s\" faulty: %d\n",
				       "server_exec_open",
				       emulator_names[i], err);
				vput(vp);
				vp = NULL;
				continue;
			}
			break;
		}
		if (err) {
			printf("server_exec: Unable to exec any emulator\n");
			return err;
		}
	} else {
		err = server_exec_native_sanity(vp, hdr, cred, p, &type);
		if (err) {
			vput(vp);
			return err;
		}
	}

	/* Emulator or other native */

	*binary_type = type;
	*vnp = vp;
	return KERN_SUCCESS;
}

static mach_error_t server_exec_load(
	struct proc		*p,
	struct vnode		*vp,
	binary_type_t		bt,
	union exec_data		*hdr,
	struct exec_load_info	*li)	/* OUT (space allocated by caller) */
{
#define NSECTIONS 6		/* generous: 4 is needed for known binaries */
	mach_error_t kr;
	struct exec_section secs[NSECTIONS];
	int i;
	mach_port_t port;

	port = vnode_to_port(vp);
	if (!MACH_PORT_VALID(port)) {
		printf("server_exec_load pid=%d: vnode_to_port failed\n",
		       p->p_pid);
		return EINVAL;
	}

	kr = parse_exec_file(hdr, bt, li, secs, NSECTIONS);
	if (kr)
	    return kr;

	/* keep a reference as mmap_vm_map drops master_lock */
	vref(vp);
	for (i = 0; (i < NSECTIONS) && (secs[i].how != EXEC_M_STOP); i++) {
		/* Just go ahead and fill all slots. Could be more selective */
		secs[i].file = port;
		kr = server_exec_map_section(p, &secs[i]);
		if (kr) {
			printf("server_exec_load: map section #%d: %s\n",
			       i, mach_error_string(kr));
			break;
		}
	}
	vrele(vp);

	/* These are no longer used for anything */
	p->p_vmspace->vm_tsize = secs[0].size / NBPG;	/* XXX */
	p->p_vmspace->vm_dsize = (secs[1].size		/* XXX */
				  + secs[2].size) / NBPG; /* XXX */

	return kr;
}

mach_error_t server_exec_map_section(
	struct proc		*p,
	struct exec_section	*section)
{
	kern_return_t kr;
	vm_address_t addr;

	switch (section->how) {
	      case EXEC_M_NONE:
		/* Skip it */
		return KERN_SUCCESS;

	      case EXEC_M_MAP:
		/* 
		 * Map SECTION with vnode pager,
		 */
		kr = mmap_vm_map(p, section->va, section->size, FALSE,
				 section->file,
				 (vm_offset_t ) section->offset, section->copy,
				 section->prot, section->maxprot,
				 section->inheritance, NULL);
		if (kr != KERN_SUCCESS) {
			printf("server_exec_map_section: mmap_vm_map: %s\n",
			       mach_error_string(kr));
			return kr;
		}
		break;

	      case EXEC_M_ZERO_ALLOCATE:
		/* Map anonymous memory right there */
		kr = mmap_vm_map(p, section->va, section->size, FALSE,
				 MACH_PORT_NULL,
				 (vm_offset_t) section->offset, TRUE,
				 section->prot, section->maxprot,
				 section->inheritance, NULL);
		if (kr != KERN_SUCCESS) {
			printf("server_exec_map_section: anon map: %s\n",
			       mach_error_string(kr));
			return kr;
		}
		break;

	      default:
		panic("server_exec_map_section");
	}
	return KERN_SUCCESS;
}

static mach_error_t server_exec(
	struct proc		*p,
	char			*fname,
	char 			*emul_name,	/* OUT */
	char 			*cfname,	/* OUT */
	char 			*cfarg,		/* OUT */
	struct exec_load_info	*li,		/* OUT */
	mach_port_t		*image_port)	/* OUT */
{
	mach_error_t kr;
	struct vnode *vp;
	binary_type_t binary_type;
	boolean_t indir, emulated;
	union exec_data hdr;
	uid_t uid;
	gid_t gid;
	mach_port_t old_task = p->p_task;
	boolean_t dangerous = FALSE; /* suid or execute only */

	kr = server_exec_open(p, fname, &vp, &binary_type, &indir, &emulated,
			      &hdr, emul_name, cfname, cfarg, &uid, &gid,
			      image_port, &dangerous);
	if (kr)
	    return kr;

#if 0
	/* START Mystical (and probably useless) stuff from BSDSS9 */
	if (exech.a_text != 0 && (vp->v_flag & VTEXT) == 0 &&
	    vp->v_usecount != 1) {
		register struct file *fp;

		for (fp = filehead; fp; fp = fp->f_filef) {
			if (fp->f_type == DTYPE_VNODE &&
			    fp->f_count > 0 &&
			    (struct vnode *)fp->f_data == vp &&
			    (fp->f_flag & FWRITE)) {
				vput(vp);
				return ETXTBSY;
			}
		}
	}
	/* END Mystical BSDSS9 stuff */
#endif 0
	/* Wake up parent waiting for state transition */
	if ((p->p_flag & P_PPWAIT)) {
		p->p_flag &= ~P_PPWAIT;
		wakeup((caddr_t)p->p_pptr);
	}

	p->p_flag |= P_EXEC;

	if (dangerous)
	    kr = server_exec_create_new_task(p);
	else
	    kr = server_exec_cleanup_task(p);
	if (kr) {
		vput(vp);
		return kr;
	}
	kr = server_exec_load(p, vp, binary_type, &hdr, li);
	vput(vp);
	if (kr)
	    goto kill_it;

	kr = server_exec_write_trampoline(p);
	if (kr)
	    goto kill_it;

	fdcloseexec(p);
	execsigs(p);

	/* Cancel itimer */
	untimeout(realitexpire, (void *) p);

	if (dangerous) {
		kr = server_exec_set_suid(p, uid, gid);
		if (kr)
		    goto kill_it;
	}

	p->p_acflag &= ~AFORK;
	{
		/* Save the rightmost MAXCOMLEN chars */
		char *name;
		int len;

		if (indir)
		    name = cfname;
		else
		    name = fname;
		len = strlen(name);
		if (len > MAXCOMLEN)
		    name += len - MAXCOMLEN;
		/* 
		 * (mig_strncpy does what strncpy should:
		 * null terminates the string
		 */
		mig_strncpy(p->p_comm, name, MAXCOMLEN);
	}
#if MACHID_REGISTER
	machid_register_process(p);
#endif /* MACHID_REGISTER */

	return KERN_SUCCESS;

kill_it:
	/* The task is now beyond repair. Get rid of it */
	proc_zap(p, W_EXITCODE(-2, SIGKILL));
	task_terminate(old_task);
	return kr;
}

#if MACHID_REGISTER
#if SECOND_SERVER
extern int second_server;
int pid_registration_type = 0; /* XXX */
static inline int MAP_PID_FOR_MIDSRVR(int realpid)
{
    if(pid_registration_type > 0 && pid_registration_type < 10) 
	return((pid_registration_type * 1000000) + realpid);
    else if(pid_registration_type < 0 && pid_registration_type > -10)
	return(((1 + pid_registration_type) * 1000000) - realpid);
    else
	return realpid;
}

/* After one registration failure we never try it again.  tri */
static boolean_t machid_register_failed = FALSE;

int machid_register_process(struct proc *p)
{
    extern mach_port_t mid_server;

    if(second_server && (!machid_register_failed)) {
	if (MACH_PORT_VALID(mid_server)) {
	    kern_return_t kr;
	    mach_id_t mid;

	    kr = machid_mach_register(mid_server, p->p_task,
				      p->p_task, MACH_TYPE_TASK,
				      &mid);
	    if (kr) {
		printf("machid_mach_register: "
		       "process machid registration failed: %s\n",
		       mach_error_string(kr));
		machid_register_failed = TRUE;
	    } else {
		char *s = p->p_comm;
		char *id = p->p_comm;
		while(s && *s) {
		    if(*s == '/' && *(s + 1))
			id = s + 1;
		    s++;
		}
		kr = machid_task_set_unix_info(mid_server, p->p_task, mid,
					       MAP_PID_FOR_MIDSRVR(p->p_pid),
					       id, MAXCOMLEN);
		if (kr) {
		    printf("process machid registration failed: %s\n",
			   mach_error_string(kr));
		    machid_register_failed = 1;
		}
	    }
	}
    }
    return(!machid_register_failed);
}

#else
/*
** Stubs will do if there is no second server code at all.
*/
static inline int MAP_PID_FOR_MIDSRVR(int realpid)
{ 
    return 0; 
}
int machid_register_process(struct proc *p)
{
    return 0;
}

#endif /* SECOND_SERVER */
#endif /* MACHID_REGISTER */

/* from BSDSS. */
void
fdcloseexec(struct proc *p)
{
	register struct filedesc *fdp = p->p_fd;
	register struct file **fpp;
	register char *of;
	register int i;

	fpp = fdp->fd_ofiles;
	of = fdp->fd_ofileflags;
	for (i = fdp->fd_lastfile; i-- >= 0; fpp++,of++)
		if (*fpp && (*of & UF_EXCLOSE)) {
			(void) closef(*fpp, p);
			*fpp = NULL;
			*of = 0;
		}

	while (fdp->fd_lastfile > 0 && 
	       fdp->fd_ofiles[fdp->fd_lastfile] == NULL)
	    fdp->fd_lastfile--;
}


#if 0
/* server_exec_load loads only emulators or other native programs */

static mach_error_t OLD_server_exec_load(
	struct proc	*p,
	struct vnode	*vp,
	binary_type_t	binary_type,
	union exec_data	*hdr,
	struct exec_load_info *li)
{
	vm_offset_t text_start, data_start, bss_fragment_start;
	vm_offset_t bss_residue_start, data_dirty_start;
	vm_size_t text_size, data_size, bss_fragment_size;
	vm_size_t bss_residue_size, data_clean_size, data_dirty_size;
	struct ucred *cred = p->p_ucred;

	off_t text_offset, data_dirty_offset;

	vm_offset_t addr;
	kern_return_t kr;
	mach_error_t err;
	mach_port_t port;
	int resid;

	struct aout_hdr *exech = &hdr->aout;

	exec_load_info_clear(li);
	li->pc = exech->a_entry;

	port = vnode_to_port(vp);
	if (!MACH_PORT_VALID(port)) {
		printf("server_exec_load pid=%d: vnode_to_port failed\n",
		       p->p_pid);
		return EINVAL;
	}

/* 
 * a.out is a poor binary format and also its interpretation varies
 * Use entry address to decide where to load. Truncate to 16M boundary.
 * Add 0x1000 in order to cope with standard QMAGIC loaded at 0x1000.
 */
#define TEXT_START_TRUNC(A) ((~((vm_address_t)0xffffff) & A) + vm_page_size)

	switch (binary_type) {
	      case BT_LITES_Z:
		text_size = round_page(exech->a_text +sizeof(struct aout_hdr));
		text_start = TEXT_START_TRUNC(exech->a_entry);
		text_offset = 0;
		break;
	      case BT_LITES_Q:
		text_size = round_page(exech->a_text);
		text_start = TEXT_START_TRUNC(exech->a_entry);
		text_offset = 0;
		break;
#ifdef parisc
	      case BT_LITES_SOM:
		text_size = round_page(exech->som.a_text);
		text_start = trunc_page(exech->som.a_tmem);
		text_offset = exech->som.a_tfile;
		break;
#endif
	      case BT_LITES_MIPSEL:
		/* XXX Later */
		break;
	      default:
		printf("server_exec_load: unknown binary_type x%x",
		       binary_type);
		return ENOEXEC;
	}
	data_start = text_start + text_size;
	data_size = round_page(exech->a_data);
	bss_fragment_start = data_start + exech->a_data;
	bss_fragment_size = data_size - exech->a_data;
	bss_residue_start = data_start + data_size;
	bss_residue_size = exech->a_bss - bss_fragment_size;
	if (bss_residue_size < 0)
	    bss_residue_size = 0;
	else
	    bss_residue_size = round_page(bss_residue_size);

	data_clean_size = trunc_page(exech->a_data);
	data_dirty_start = trunc_page(data_start + exech->a_data);
	data_dirty_size = NBPG - bss_fragment_size;
	data_dirty_offset = text_offset + text_size + data_clean_size;
	/* 
	 * Map TEXT and DATA with vnode pager,
	 */

	addr = text_start;
	/* keep a reference as mmap_vm_map drops master_lock */
	vref(vp);
	kr = mmap_vm_map(p, text_start, text_size + data_clean_size, FALSE,
			 port, text_offset, TRUE, VM_PROT_ALL, VM_PROT_ALL,
			 VM_INHERIT_COPY, NULL);
	vrele(vp);
	if (kr != KERN_SUCCESS) {
		printf("server_exec_load: mmap_vm_map failed: %s\n",
		       mach_error_string(kr));
		return kr;
	}
	/* 
	 * Allocate memory for BSS pages.
	 * Unfortunately the last page of DATA may be on the
	 * same page as BSS. BSS must be zeroed but the pager
	 * is unaware of it. So we must read the last piece of
	 * DATA *now*.
	 */
	addr = data_dirty_start;
	kr = vm_allocate(p->p_task, &addr,
			 round_page(data_dirty_size) + bss_residue_size,
			 FALSE);
	if (kr != KERN_SUCCESS) {
		printf("server_exec_load: vm_allocate bss: %s",
		       mach_error_string(kr));
		return kr;
	}
	/* get some zeroed space */
	if ((kr = vm_allocate(mach_task_self(),
			      &addr, 
			      round_page(data_dirty_size),
			      TRUE)) != KERN_SUCCESS) {
		printf("server_exec_load: vm_allocate failure %x\n", kr);
		return kr;
	}
	/* fill some of it with data from the file */
	kr = vn_rdwr(UIO_READ, vp,
		     (caddr_t)addr,
		     data_dirty_size,
		     data_dirty_offset,
		     UIO_SYSSPACE, (IO_UNIT|IO_NODELOCKED),
		     cred, &resid, p);
	/* write it into the client */
	if (kr == 0)
	    kr = vm_write(p->p_task, data_dirty_start,
			  addr, round_page(data_dirty_size));
	/* and get rid of our copy */
	(void) vm_deallocate(mach_task_self(),
			     addr,
			     round_page(data_dirty_size));
	if (kr) {
		printf("server_exec_load: dirty data page read: %s\n",
		       mach_error_string(kr));
		return kr;
	}

	/* Make text read only */
	kr = vm_protect(p->p_task, text_start, text_size,
			FALSE, VM_PROT_READ|VM_PROT_EXECUTE);
	if (kr != KERN_SUCCESS) {
		printf("server_exec_load: vm_protect text: %s",
		       mach_error_string(kr));
		return kr;
	}
	/* Make data and bss RW */
	kr = vm_protect(p->p_task,
			data_start,
			data_size + bss_residue_size,
			FALSE,
			VM_PROT_READ|VM_PROT_WRITE);
	if (kr != KERN_SUCCESS) {
		printf("server_exec_load: vm_protect data+bss: %s",
		       mach_error_string(kr));
		return kr;
	}

	/* These are no longer used for anything */
	p->p_vmspace->vm_tsize = text_size / NBPG;
	p->p_vmspace->vm_dsize = (data_size + bss_residue_size) / NBPG;

	return KERN_SUCCESS;
}
#endif 0

extern char *trampoline_page;

static mach_error_t server_exec_write_trampoline(struct proc *p)
{
#if 0 /* no longer used */
	vm_size_t size;
	vm_address_t addr;
	kern_return_t kr;

	size = NBPG;
	addr = trunc_page((vm_offset_t)USRSTACK) - size;

	kr = vm_allocate(p->p_task, &addr, size, FALSE);
	if (kr != KERN_SUCCESS) {
		printf("server_exec_make_stack: %s\n", mach_error_string(kr));
		return kr;
	}

	kr = vm_write(p->p_task, trunc_page(USRSTACK - TRAMPOLINE_MAX_SIZE),
		      (vm_offset_t) trampoline_page, NBPG);
	if (kr != KERN_SUCCESS)
	    panic("vm_write trampoline",kr);
#else
	return KERN_SUCCESS;
#endif
}

static mach_error_t server_exec_set_suid(
	struct proc	*p,
	uid_t		uid,
	gid_t		gid)
{
	struct ucred *cred = p->p_ucred;

	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((p->p_flag & P_TRACED)==0) {
		if (uid != cred->cr_uid || gid != cred->cr_gid)
			p->p_ucred = cred = crcopy(cred);
		cred->cr_uid = uid;
		cred->cr_gid = gid;
	} else
		psignal(p, SIGTRAP);
	p->p_cred->p_svuid = cred->cr_uid;
	p->p_cred->p_svgid = cred->cr_gid;
/*	u.u_prof.pr_scale = 0;*/
	return KERN_SUCCESS;
}
