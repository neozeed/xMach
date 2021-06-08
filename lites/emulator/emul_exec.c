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
 * 15-Nov-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Use new code common with server.
 *
 * 27-Sep-94  Ian Dall (dall@hfrd.dsto.gov.au)
 *	Make obreak less susceptible to integer overflows.
 *
 * 25-Oct-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	PA RISC support from Mike Hibler.
 *
 * $Log: emul_exec.c,v $
 * Revision 1.4  2000/11/26 01:35:55  welchd
 *
 * Fixed to getdents interface
 *
 * Revision 1.3  2000/11/07 00:41:25  welchd
 *
 * Added support for executing dynamically linked Linux ELF binaries
 *
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.3  1995/09/01  23:27:54  mike
 * HP-UX compatibility support from Jeff F.
 *
 * Revision 1.2  1995/08/30  22:12:55  mike
 * Hack rpc for GDB support
 *
 * Revision 1.1.1.2  1995/03/23  01:15:30  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	emulator/emul_exec.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	March 1994
 *
 *	Exec emulator side. The actual executable is loaded here.
 */

#define KERNEL
#include <sys/errno.h>
#undef KERNEL

#include <e_defs.h>

extern int syscall_debug;
extern binary_type_t e_my_binary_type;
boolean_t load_cmu_binaries = TRUE;
vm_offset_t heap_end = 0;


errno_t emul_trap_run(int argc, char *argv[], char *envp[], integer_t *kframe,
		      char *fname, mach_port_t image_port);
errno_t emul_exec_open(const char *file, int *fd,
		       enum binary_type *binary_type,
		       union exec_data *hdr, char *cfname,
		       char *cfarg, mach_port_t image_port);
mach_error_t emul_exec_map_section(struct exec_section	*section);
errno_t emul_exec_load(int fd, enum binary_type bt,
		       union exec_data *hdr, struct exec_load_info *li,
		       vm_offset_t *heap_end, mach_port_t image_port,
		       signed int load_bias);

noreturn emul_exec_linux_start_new(
	int			*kframe,
	enum binary_type	binary_type,
	int			argc,
	char			*argv[],
	char			*envp[],
	struct exec_load_info	*interpreter_li,
	struct exec_load_info   *li,
	union exec_data         *hdr,
        unsigned int            load_addr,
	unsigned int            interp_load_addr);

errno_t emul_linux_exec_load(
	int			fd,
	enum binary_type	bt,
	mach_port_t		image_port,
        integer_t*              kframe,
	int                     argc,
	char*                   argv[],
	char*                   envp[],
	union exec_data*        hdr)
{
   errno_t err;
   char iname[256];
   unsigned int phdr;
   elf_exec* elf;
   size_t result;
   int nread;
   int interpreter_fd;
   enum binary_type interpreter_bt;
   union exec_data interpreter_hdr;
   char cfname[PATH_LENGTH], cfarg[PATH_LENGTH];
   struct exec_load_info li_data;
   struct exec_load_info interpreter_li_data;
   vm_offset_t interpreter_heap_end;
   vm_offset_t heap_end;
   unsigned int load_addr;
   unsigned int interp_load_addr;
   
   /*
    * Load the main executable
    */
   err = emul_exec_load(fd, bt, hdr, &li_data, &heap_end,
			image_port, 0);
   if (err)
     exit(-1);
   
   /*
    * Find the load address
    */
   phdr = 0;
   elf = &hdr->elf;
   while (elf->phdrs[phdr].p_type != PT_LOAD)
     {
	phdr++;
     }
//   e_emulator_error("elf->phdrs[phdr].p_vaddr %x",
//		    elf->phdrs[phdr].p_vaddr);
//   e_emulator_error("elf->phdrs[phdr].p_offset %x",
//		    elf->phdrs[phdr].p_offset);
   load_addr = elf->phdrs[phdr].p_vaddr - elf->phdrs[phdr].p_offset;
//   e_emulator_error("load_addr: 0x%x", load_addr);
   
   /*
    * Get the interpreter name
    */		     		     
//   e_emulator_error("Finding interpreter name");
   for (phdr = 0; phdr < elf->ehdr.e_phnum; phdr++) 
     {
	if (elf->phdrs[phdr].p_type == PT_INTERP)
	  {
//	     e_emulator_error("Found interpreter at %d", phdr);
//	     e_emulator_error("elf->phdrs[phdr].p_offset 0x%x",
//			      elf->phdrs[phdr].p_offset);
//	     e_emulator_error("fd %d", fd);
//	     e_emulator_error("image_port %d", image_port);
	     if (MACH_PORT_VALID(image_port)) {
		vm_address_t mapped_addr = 0;
		vm_size_t mapped_size = round_page(elf->phdrs[phdr].p_filesz);
//		e_emulator_error("elf->phdrs[phdr].p_filesz %d %d",
//				 elf->phdrs[phdr].p_filesz, mapped_size);
		err = bsd_vm_map(process_self(), &mapped_addr,
				 mapped_size, 0,
				 TRUE, image_port, 
				 elf->phdrs[phdr].p_offset, TRUE,
				 VM_PROT_READ, VM_PROT_ALL, VM_INHERIT_NONE);
		if (err != KERN_SUCCESS) {
			e_emulator_error("emul_exec_open: bsd_vm_map image:%s",
					 mach_error_string(err));
			return ENOEXEC;
		}
		bcopy((void *)mapped_addr, iname, elf->phdrs[phdr].p_filesz);
		err = vm_deallocate(mach_task_self(), mapped_addr,
				    mapped_size);
		if (err != KERN_SUCCESS) {
			e_emulator_error("emul_exec_open: vm_dealloc image:%s",
					 mach_error_string(err));
			return ENOEXEC;
		}
	     } else {
		err = e_lite_lseek(fd, 0, elf->phdrs[phdr].p_offset, 0, 
				   &nread);
		if (err != 0)
		  {
		     e_emulator_error("err %d", err);
		     return(-1);
		  }
//		e_emulator_error("elf->phdrs[phdr].p_filesz %d",
//				 elf->phdrs[phdr].p_filesz);
		err = e_read(fd, iname, elf->phdrs[phdr].p_filesz, &result);
		if (err != 0 || result != elf->phdrs[phdr].p_filesz)
		  {
		  return(-1);
		  }		
	     }	     	     
//	     e_emulator_error("iname: %s", iname);
	     break;
	  }
     }
//   strcpy(iname, "/xmach/lib/ld-linux.so.2");
   
   /*
    * FIXME: Check the interpreter
    */
   
   /*
    * Open the interpreter
    */ 
//   e_emulator_error("Opening interpreter");
   err = emul_exec_open(iname, 
			&interpreter_fd, 
			&interpreter_bt, 
			&interpreter_hdr,
			cfname, 
			cfarg, 
			MACH_PORT_NULL);
   if (err) {
      e_emulator_error("emul_exec_open failed: %s",
		       mach_error_string(unix_err(err)));
      exit(-1);
   }
   
   /*
    * Load the interpreter
    */
//   e_emulator_error("Loading interpreter\n");
   err = emul_exec_load(interpreter_fd, 
			interpreter_bt, 
			&interpreter_hdr, 
			&interpreter_li_data, 
			&interpreter_heap_end,
			MACH_PORT_NULL,
			0x40000000);
   if (err)
     exit(-1);
   
   /*
    * Find the interpreter load address
    */
   phdr = 0;
   elf = &interpreter_hdr.elf;
   while (elf->phdrs[phdr].p_type != PT_LOAD)
     {
	phdr++;
     }
   interp_load_addr = elf->phdrs[phdr].p_vaddr + 0x40000000 - 
     elf->phdrs[phdr].p_offset ;
//   e_emulator_error("interp_load_addr: 0x%x", interp_load_addr);
   interpreter_li_data.pc = interpreter_li_data.pc + 0x40000000;
   
   /*
    * Hook for mach3 GDB.
    *
    * GDB wants to gain control after the address space has been
    * initialized but before transfering control.
    */
   err = bsd_exec_done(process_self());
   
   /* And start running it */
//   e_emulator_error("About to start interpreter");
   emul_exec_linux_start_new(kframe, 
			     bt,
			     argc, 
			     argv, 
			     envp,
			     &interpreter_li_data,
			     &li_data,
			     hdr,
			     load_addr,
			     interp_load_addr);
   e_emulator_error("emul_exec_start returned (%d)", err);
   emul_panic("XXX about to exit");
   return err;
}

errno_t emul_trap_run(
	int		argc,
	char		*argv[],
	char		*envp[],
	integer_t	*kframe, /* points to argc on future user stack */
	char		*fname,
	mach_port_t	image_port)
{
	errno_t err;
	int fd;
	binary_type_t binary_type;
	struct exec_load_info li_data;
	char cfname[PATH_LENGTH], cfarg[PATH_LENGTH];
	union exec_data hdr;

	if (!MACH_PORT_VALID(image_port)) {
		if (argv[1] == 0) {
			e_emulator_error("No program, no args, nothing to do");
			exit(-1);
		}
		/* 
		 * Not started as an emulator.  Assume argv[1] is the
		 * program to run.  Shift all args by one.
		 */
		fname = argv[1];
		argc--;
		if (kframe) {
		  	/* Ok, because on the alpha ARGC occupies 64 bits
			 * on the stack.
			 *
			 * Replace argv0 with argc.
			 */
			argv[0] = (char *) ((natural_t) argc);
			kframe++;		 /* throw away old argc */
		}
		argv++;
	}
	err = emul_exec_open(fname, &fd, &binary_type, &hdr,
			     cfname, cfarg, image_port);
	if (err) {
		e_emulator_error("emul_exec_open failed: %s",
				 mach_error_string(unix_err(err)));
		exit(-1);
	}
   
	e_my_binary_type = binary_type;  
   
	/* Set expansion for @bin in namei */
	bsd_set_atexpansion(process_self(), "@bin", 5,
			    atbin_names[binary_type], 
			    strlen(atbin_names[binary_type]) + 1);

	/*
	 * Set up the emulator vectors.
	 */
	emul_setup(mach_task_self(), binary_type);
   
        if (binary_type == BT_LINUX_DYN_ELF) {
//	   e_emulator_error("Dynamic ELF binary encountered");
	   return(emul_linux_exec_load(fd, 
				       binary_type, 
				       image_port,
				       kframe,
				       argc,
				       argv,
				       envp,
				       &hdr));
        }
   
	/* Load the program */
	err = emul_exec_load(fd, binary_type, &hdr, &li_data, &heap_end,
			     image_port, 0);
	if (err)
	    exit(-1);

	/*
	 * Hook for mach3 GDB.
	 *
	 * GDB wants to gain control after the address space has been
	 * initialized but before transfering control.
	 */
	err = bsd_exec_done(process_self());

	/* And start running it */
	err = emul_exec_start(kframe, binary_type,
			      argc, argv, envp,
			      &li_data);
	e_emulator_error("emul_exec_start returned (%d)", err);
	emul_panic("XXX about to exit");
	return err;

}

errno_t emul_exec_open(
	const char	*file,
	int		*fd,		/* OUT */
	enum binary_type *binary_type,	/* OUT */
	union exec_data *hdr,		/* OUT (space allocated by caller) */
	char		*cfname,	/* OUT (space allocated by caller) */
	char		*cfarg,		/* OUT (space allocated by caller) */
	mach_port_t	image_port)
{
	errno_t err;
	size_t result;
	char *shellname;
	char *cp;

	*fd = -1;
	*binary_type = BT_BAD;

	if (MACH_PORT_VALID(image_port)) {
		vm_address_t mapped_addr = 0;
		vm_size_t mapped_size = round_page(sizeof(*hdr));
		err = bsd_vm_map(process_self(), &mapped_addr,
				 mapped_size, 0,
				 TRUE, image_port, 0, TRUE,
				 VM_PROT_READ, VM_PROT_ALL, VM_INHERIT_NONE);
		if (err != KERN_SUCCESS) {
			e_emulator_error("emul_exec_open: bsd_vm_map image:%s",
					 mach_error_string(err));
			return ENOEXEC;
		}
		bcopy((void *)mapped_addr, hdr, sizeof(*hdr));
		err = vm_deallocate(mach_task_self(), mapped_addr,
				    mapped_size);
		if (err != KERN_SUCCESS) {
			e_emulator_error("emul_exec_open: vm_dealloc image:%s",
					 mach_error_string(err));
			return ENOEXEC;
		}
	} else {
		err = e_open(file, O_RDONLY, 0, fd);
		if (err) {
			e_emulator_error("emul_exec_open open \"%s\" err=%d",
					 file, *fd);
			return err;
		}
		err = e_read(*fd, (char *) hdr, sizeof(*hdr), &result);
 		if (err || result != sizeof(*hdr)) {
			/* Handle small scripts correctly XXX */
			e_emulator_error("emul_exec_open read=%d n=%d\n",
					 err, result);
			return err;
		}
	}

	*binary_type = get_binary_type(hdr);

	if (*binary_type == BT_SCRIPT) {
		/* shell script */
		e_emulator_error("emul_exec_open: shell script");
		goto exec_fail;
	} else if (*binary_type == BT_BAD) {
		e_emulator_error("emul_exec_open: Unknown binary format: %s",
				 file);
		goto exec_fail;
	}
	if (syscall_debug > 1)
	    e_emulator_error("emul_exec_open success: \"%s\" p=%x fd=%d BT=%d",
			     file, image_port, *fd, *binary_type);
	return ESUCCESS;

exec_fail:
	if (*fd != -1) {
		e_close(*fd);
		*fd = -1;
	}
	e_emulator_error("emul_exec_open failed for file \"%s\"", file);
	return ENOEXEC;
}

errno_t emul_exec_load(
	int			fd,
	enum binary_type	bt,
	union exec_data		*hdr,
	struct exec_load_info	*li,		/* OUT (space by caller) */
	vm_offset_t		*heap_end,	/* OUT */
	mach_port_t		image_port,
	signed int              load_bias)
{
#define NSECTIONS 6		/* generous: 4 is needed for known binaries */
	mach_error_t kr;
	struct exec_section secs[NSECTIONS];
	int i;

	if (heap_end)
	    *heap_end = 0;

	kr = parse_exec_file(hdr, bt, li, secs, NSECTIONS);
	if (kr)
	    return kr;

	/* If no port was give try to get one. XXX Move out from this func */
	if (!MACH_PORT_VALID(image_port)) {
		kr = bsd_fd_to_file_port(process_self(), fd, &image_port);
		if (syscall_debug > 2)
		    e_emulator_error("emul_exec_load:open fd as port kr=x%x\n",
				     kr);
		if (kr)
		    return kr;
	}

	for (i = 0; (i < NSECTIONS) && (secs[i].how != EXEC_M_STOP); i++) {
		/* Just go ahead and fill all slots. Could be more selective */
		secs[i].file = image_port;
	        secs[i].va = secs[i].va + load_bias;
		kr = emul_exec_map_section(&secs[i]);
		if (kr) {
			e_emulator_error("emul_exec_load: map section #%d: %s\n",
					 i, mach_error_string(kr));
			break;
		}
		/* For sbrk use.  This is a kludge and assumes BSS is last. */
		if (heap_end && (*heap_end < secs[i].va + secs[i].size))
		    *heap_end = secs[i].va + secs[i].size;
	}

	/* Clear BSS beginning */
	if (li->zero_count > 0)
	    bzero((caddr_t) (li->zero_start + load_bias), li->zero_count);

	return kr;
}

mach_error_t emul_exec_map_section(
	struct exec_section	*section)
{
	vm_offset_t addr;
	mach_error_t kr;

	switch (section->how) {
	      case EXEC_M_NONE:
		/* Skip it */
		return KERN_SUCCESS;

	      case EXEC_M_MAP:
		/* 
		 * Map SECTION with vnode pager,
		 */
		addr = section->va;

		/*
		 * A long time ago, PA-RISC 1.0 had a 2k page size.
		 * Why do we care?  Turns out we still have some of
		 * these binaries hanging around.
		 *
		 * Now, section->va was masked to be 4k page aligned.
		 * As a result, we are loading the image at 0x0
		 * (rather than 0x800).  As this only affects the
		 * text segment, we allow the load to happen at 0x0,
		 * but back up the file offset to 0x0 too.
		 *
		 * I know this is a very narrow view of the problem,
		 * fortunately, all *our* ancient binaries dont mind!
		 */
		if (e_my_binary_type == BT_HPUX && section->offset == 0x800 &&
		    addr == 0 && (section->prot & VM_PROT_WRITE) == 0)
			section->offset = 0x0;

		kr = bsd_vm_map(process_self(), &addr, section->size, 0, FALSE,
				 section->file,
				 (vm_offset_t) section->offset, section->copy,
				 section->prot, section->maxprot,
				 section->inheritance);
		if (kr != KERN_SUCCESS) {
			e_emulator_error("emul_exec_map_sect: bsd_vm_map: %s\n",
					mach_error_string(kr));
			return kr;
		}
		break;

	      case EXEC_M_ZERO_ALLOCATE:
		/* Map anonymous memory right there */
		addr = section->va;
		kr = vm_map(mach_task_self(), &addr, section->size, 0, FALSE,
			    MACH_PORT_NULL,
			    (vm_offset_t) section->offset, TRUE,
			    section->prot, section->maxprot,
			    section->inheritance);
		if (kr != KERN_SUCCESS) {
			e_emulator_error("emul_exec_map_sect: anon map: %s\n",
					mach_error_string(kr));
			return kr;
		}
		break;

	      default:
		e_emulator_error("server_exec_map_section");
	}
	return KERN_SUCCESS;
}



extern vm_offset_t heap_end;

errno_t e_obreak(const char *addr, char **retval)
{
	struct vmspace *vm = &shared_base_rw->us_vmspace;
	vm_offset_t new, old;
	int rv;

	new = round_page((vm_offset_t)addr);
	if (new == 0) {
		if (retval)
		    *retval = (char *) heap_end;
		return ESUCCESS;
	}

	if ((vm_offset_t)addr > new)
	  return ENOMEM;	/* round page overflowed? */
#if 0
	old = heap_start;
	if ((integer_t)(new - old) > shared_base_ro->us_limit.pl_rlimit[RLIMIT_DATA].rlim_cur)
	    return ENOMEM;
#endif
	old = round_page(heap_end);
	if (new > old) {
		rv = vm_allocate(mach_task_self(), &old, new - old, FALSE);
		if (rv != KERN_SUCCESS) {
			e_emulator_error("obrk: grow failed: x%x -> x%x = x%x",
					 old, new, rv);
			return ENOMEM;
		}
		heap_end += (new - old);
	} else if (new < old) {
		rv = vm_deallocate(mach_task_self(), new, old - new);
		if (rv != KERN_SUCCESS) {
			e_emulator_error("obrk: shrink failed: x%x -> x%x = x%x",
					 old, new, rv);
			return ENOMEM;
		}
		heap_end -= (old - new);
	}
	if (retval)
	    *retval = addr ? (char *) addr : (char *) heap_end;
	return ESUCCESS;
}



#if 0
errno_t emul_exec_open_OLD(
	const char	*file,
	int		*fd,		/* OUT */
	enum binary_type *binary_type,	/* OUT */
	boolean_t	*indir,		/* OUT */
	union exec_header *exech,	/* OUT (space allocated by caller) */
	char		*cfname,	/* OUT (space allocated by caller) */
	char		*cfarg,		/* OUT (space allocated by caller) */
	mach_port_t	image_port)
{
	errno_t err;
	int result;
	char *shellname;
	char *cp;

	union {
		unsigned int	magic;
		unsigned short	short_magic;
		union		exec_header exec;
		char		shell[MAXINTERP]; /* #! and interpreter name */
	} exdata;

	*fd = -1;
	*indir = 0;
	*binary_type = BT_BAD;

	if (MACH_PORT_VALID(image_port)) {
		vm_address_t mapped_addr = 0x90000000; /* XXX */
		vm_size_t mapped_size = round_page(sizeof(exdata));
		err = bsd_vm_map(process_self(), &mapped_addr,
				 mapped_size, 0,
				 TRUE, image_port, 0, TRUE,
				 VM_PROT_READ, VM_PROT_ALL, VM_INHERIT_NONE);
		if (err != KERN_SUCCESS) {
			e_emulator_error("emul_exec_open: bsd_vm_map image:%s",
					 mach_error_string(err));
			return ENOEXEC;
		}
		bcopy(mapped_addr, &exdata, sizeof(exdata));
		err = vm_deallocate(mach_task_self(), mapped_addr,
				    mapped_size);
		if (err != KERN_SUCCESS) {
			e_emulator_error("emul_exec_open: vm_dealloc image:%s",
					 mach_error_string(err));
			return ENOEXEC;
		}
	} else {
		err = e_open(file, O_RDONLY, 0, fd);
		if (err) {
			e_emulator_error("emul_exec_open open \"%s\" err=%d",
					 file, *fd);
			return err;
		}
		err = e_read(*fd, (char *) &exdata, sizeof(exdata), &result);
		if (err || result != sizeof(exdata)) {
			/* Handle small scripts correctly XXX */
			e_emulator_error("emul_exec_open read=%d n=%d\n",
					 err, result);
			return err;
		}
	}

#ifdef parisc
	/*
	 * XXX PA specific
	 */
	if (!SOM_N_BADMID(exdata.exec.som) && !SOM_N_BADMAG(exdata.exec.som)) {
		if (exdata.exec.som.a_execloc != SOM_STDEXECLOC) {
			/* XXX read in actual header */
			e_emulator_error("emul_exec_open: non-contig SOM exechdr: %s",
					 file);
			goto exec_fail;
		}
		switch (exdata.exec.som.a_mid) {
		case MID_HP700:
		case MID_HP800:
			*binary_type = BT_HPBSD;
			break;
		case MID_HPUX700:
		case MID_HPUX800:
			*binary_type = BT_HPUX;
			break;
		default:
			e_emulator_error("emul_exec_open: Unknown SOM varient: %s",
					 file);
			goto exec_fail;
		}
		bcopy(&exdata.exec, exech, sizeof(exdata.exec.aout));
	} else
#endif /* parisc */
	if ((exdata.magic & ~0xfc) == 0x0b018600) {
		/* 
		 * NetBSD magic's are in inverted byte order
		 * 0xfc is mask for flags field.
		 */
		*binary_type = BT_NETBSD;
		bcopy(&exdata.exec, exech, sizeof(exdata.exec.aout));
	} else if (exdata.short_magic == ZMAGIC) {
		/* a.out */
		switch ((exdata.magic >> 16) & 0xff) /* machine id */
		{
		      case 100:
			if (exdata.exec.aout.a_entry > 0x10000000)
			    *binary_type = BT_LINUX_SHLIB;
			else
			    *binary_type = BT_LINUX;
			break;
		      case 0:
			if (load_cmu_binaries && exdata.exec.aout.a_entry) {
				*binary_type = BT_CMU_43UX;
			} else {
				*binary_type = BT_386BSD;
			}
			break;
		      default:
			e_emulator_error("emul_exec_open: Unknown a.out variant: %s",
					 file);
			goto exec_fail;
		}
		bcopy(&exdata.exec, exech, sizeof(exdata.exec.aout));
	} else if (exdata.short_magic == COFF_ISC4_MAGIC) {
		/* Interactive coff */

		*binary_type = BT_ISC4;
		bcopy(&exdata.exec, exech, sizeof(exdata.exec.coff));
	} else if (exdata.short_magic == QMAGIC) {
		/* FreeBSD or BSDI */
		*binary_type = BT_FREEBSD;
		bcopy(&exdata.exec, exech, sizeof(exdata.exec.aout));
	} else if (exdata.short_magic == OMAGIC) {
		/* 
		 * Linux OMAGIC. It's pretty certain nobody else is
		 * using this format from the sixties. In any case
		 * there isn't any way to distinguish them what I know
		 * of.
		 */
		*binary_type = BT_LINUX_O;
		bcopy(&exdata.exec, exech, sizeof(exdata.exec.aout));
	} else if (exdata.shell[0] == '#'
		   && exdata.shell[1] == '!'
		   && !*indir)
	{
		/* shell script */
		e_emulator_error("emul_exec_open: shell interpreter is a shell script");
		goto exec_fail;
	} else {
		e_emulator_error("emul_exec_open: Unknown binary format: %s",
				 file);
		goto exec_fail;
	}
	if (syscall_debug > 1)
	    e_emulator_error("emul_exec_open success: \"%s\" p=%x fd=%d BT=%d",
			     file, image_port, *fd, *binary_type);
	return ESUCCESS;

exec_fail:
	if (*fd != -1) {
		e_close(*fd);
		*fd = -1;
	}
	e_emulator_error("emul_exec_open failed for file \"%s\"", file);
	return ENOEXEC;
}

errno_t emul_exec_load_OLD(
	int			fd,
	enum binary_type	binary_type,
	union exec_header	*exech,
	vm_offset_t		*entry_point,	/* OUT */
	vm_offset_t		*heap_end,	/* OUT */
	mach_port_t		image_port)
{
	vm_offset_t text_start, data_start, bss_fragment_start;
	vm_offset_t bss_residue_start, entry;
	vm_size_t text_size, data_size, bss_fragment_size, bss_residue_size;

	off_t text_offset, data_offset;

	vm_offset_t addr;
	kern_return_t kr;
	errno_t err = ESUCCESS;

	vm_prot_t text_prot = VM_PROT_READ | VM_PROT_EXECUTE;
	vm_prot_t data_prot = VM_PROT_READ | VM_PROT_WRITE;

	switch (binary_type) {
	      case BT_386BSD:
		text_size = round_page(exech->aout.a_text);
		text_start = 0;
		text_offset = NBPG;
		break;
	      case BT_NETBSD:
		text_size = round_page(exech->aout.a_text);
		text_start = 0x1000;
		text_offset = 0;
		break;
	      case BT_CMU_43UX:
		text_size = round_page(exech->aout.a_text
				       + sizeof(struct aout_hdr));
		text_start = 0x10000;
		text_offset = 0;
		break;
	      case BT_FREEBSD:
		text_size = round_page(exech->aout.a_text);
		text_start = 0x1000;
		text_offset = 0;
		break;
	      case BT_LINUX:
		text_start = 0;
		text_offset = 1024;	/* sic. */
		break;
	      case BT_LINUX_O:
		text_start = 0;
		text_offset = 32;
		text_prot = VM_PROT_ALL;
		break;
	      case BT_LINUX_SHLIB:
		text_start = trunc_page(exech->aout.a_entry);
		text_offset = 1024;	/* sic. */
		text_prot = VM_PROT_ALL;
		break;
	      case BT_ISC4:
		text_size = round_page(exech->coff.a.tsize
				       + exech->coff.a.text_start);
		text_start = trunc_page(exech->coff.a.text_start);
		text_offset = 0;
		data_offset = trunc_page(text_offset + exech->coff.a.tsize);
		data_start = trunc_page(exech->coff.a.data_start);
		data_size = round_page(exech->coff.a.dsize
				       + exech->coff.a.data_start
				       - data_start);

		bss_fragment_start = (data_start + exech->coff.a.dsize
				       + exech->coff.a.data_start
				       - data_start);
		bss_fragment_size = (round_page(bss_fragment_start)
				     - bss_fragment_start);
		bss_residue_start = data_start + data_size;
		bss_residue_size = exech->coff.a.bsize - bss_fragment_size;

		entry = exech->coff.a.entry;
		break;
#ifdef parisc
	      case BT_HPBSD:
	      case BT_HPUX:
		text_size = round_page(exech->som.a_text);
		text_start = trunc_page(exech->som.a_tmem);
		text_offset = exech->som.a_tfile;
		data_size = round_page(exech->som.a_data);
		data_start = trunc_page(exech->som.a_dmem);
		data_offset = exech->som.a_dfile;
		bss_fragment_start = exech->som.a_dmem + exech->som.a_data;
		bss_fragment_size =
			round_page(bss_fragment_start) - bss_fragment_start;
		bss_residue_start = data_start + data_size;
		bss_residue_size = exech->som.a_bss - bss_fragment_size;
		entry = exech->som.a_entry;
		data_prot = VM_PROT_ALL;
		break;
#endif /* parisc */
	      default:
		e_emulator_error("emul_load_file: unknown binary_type x%x",
				binary_type);
		return ENOEXEC;
	}
	switch (binary_type) {
	      case BT_386BSD:
	      case BT_NETBSD:
	      case BT_CMU_43UX:
	      case BT_FREEBSD:
		entry = exech->aout.a_entry;
		data_offset = text_offset + text_size;
		data_start = text_start + text_size;
		data_size = round_page(exech->aout.a_data);
		bss_fragment_start = data_start + exech->aout.a_data;
		bss_fragment_size = data_size - exech->aout.a_data;
		bss_residue_start = data_start + data_size;
		bss_residue_size = exech->aout.a_bss - bss_fragment_size;
		break;
	      case BT_LINUX:
	      case BT_LINUX_O:
	      case BT_LINUX_SHLIB:
		text_size = round_page(exech->aout.a_text);
		entry = exech->aout.a_entry;
		data_offset = text_offset + text_size;
		data_start = text_start + text_size;
		data_size = round_page(exech->aout.a_data);
		bss_fragment_start = data_start + exech->aout.a_data;
		bss_fragment_size = data_size - exech->aout.a_data;
		bss_residue_start = data_start + data_size;
		bss_residue_size = exech->aout.a_bss - bss_fragment_size;
		break;
	}
	if (bss_residue_size < 0)
	    bss_residue_size = 0;
	else
	    bss_residue_size = round_page(bss_residue_size);

	/* 
	 * Map TEXT and DATA with vnode pager,
	 */

if (MACH_PORT_VALID(image_port)) {
	addr = text_start;
	kr = bsd_vm_map(process_self(), &addr, text_size, 0,
			 FALSE, image_port, text_offset, TRUE,
			 text_prot, VM_PROT_ALL, VM_INHERIT_COPY);
	if (kr != KERN_SUCCESS) {
		e_emulator_error("emul_exec_load: bsd_vm_map text: %s",
				 mach_error_string(kr));
		return ENOSPC;
	}

	if (data_size != 0) {
		addr = data_start;
		kr = bsd_vm_map(process_self(), &addr, data_size, 0,
				FALSE, image_port, data_offset, TRUE,
				data_prot, VM_PROT_ALL,
				VM_INHERIT_COPY);
		if (kr != KERN_SUCCESS) {
			e_emulator_error("emul_exec_load: bsd_vm_map data: %s",
					 mach_error_string(kr));
			return ENOSPC;
		}
	}
} else {
	addr = text_start;
#if 0
	/* The Mach kernel is able to handle misaligned mappings so go ahead */
	if (text_offset != round_page(text_offset) && 0) {
		int nread;
		/* Misaligned. Give up on demand paging and just read it */
		kr = vm_allocate(mach_task_self(), &addr, text_size, FALSE);
		if (kr != KERN_SUCCESS) {
			e_emulator_error("emul_exec_load: vm_allocate %s: %s",
					 "misaligned text",
					 mach_error_string(kr));
			return ENOSPC;
		}
		err = e_lseek(fd, text_offset, 0, &nread);
		emul_assert(err == 0);
		err = e_read(fd, (caddr_t) addr, text_size, &nread);
		/* It's ok for nread to be < text_size sometimes */
		emul_assert(err == 0);

		addr = data_start;
		kr = vm_allocate(mach_task_self(), &addr, data_size, FALSE);
		if (kr != KERN_SUCCESS) {
			e_emulator_error("emul_exec_load: vm_allocate %s: %s",
					 "misaligned data",
					 mach_error_string(kr));
			return ENOSPC;
		}
		err = e_lseek(fd, data_offset, 0, &nread);
		emul_assert(err == 0);
		err = e_read(fd, (caddr_t) addr, data_size, &nread);
		/* It's ok for nread to be < data_size sometimes */
		
	} else
#endif
	if (text_start + data_size == data_start) {
		err = e_mmap((caddr_t) text_start, text_size + data_size,
			     7 /* RWX */, MAP_FIXED, fd,
			     text_offset, (caddr_t *) &addr);
	} else {
		/* Coff binaries have text and data separately in memory */
		err = e_mmap((caddr_t) text_start, text_size,
			     7 /* RWX */, MAP_FIXED, fd,
			     text_offset, (caddr_t *) &addr);
		if (!err)
		    err = e_mmap((caddr_t) data_start, data_size,
				 7 /* RWX */, MAP_FIXED, fd,
				 data_offset, (caddr_t *) &addr);
	}
}
	if (err != ESUCCESS) {
		e_emulator_error("emul_exec_load: e_mmap failed: %d",err);
		return err;
	}
	/* 
	 * Allocate memory for BSS pages.
	 */
	if (bss_residue_size > 0) {
		addr = bss_residue_start;
		kr = vm_allocate(mach_task_self(), &addr, bss_residue_size,
				 FALSE);
		if (kr != KERN_SUCCESS) {
			e_emulator_error("emul_exec_load: vm_allocate bss: %s",
					 mach_error_string(kr));
			return ENOSPC;
		}
	}
	/* Clear BSS beginning */
	if (bss_fragment_size > 0)
	    bzero((caddr_t)bss_fragment_start, bss_fragment_size);

if (!MACH_PORT_VALID(image_port)) {
	/* Make text read only. */
	if (text_prot != VM_PROT_ALL) {
		kr = vm_protect(mach_task_self(), text_start, text_size,
				FALSE, text_prot);
		if (kr != KERN_SUCCESS) {
			e_emulator_error("emul_exec_load: vm_protect text: %s",
					 mach_error_string(kr));
		}
	}
	/* Make data and bss not executable */
	if (data_prot != VM_PROT_ALL) {
		kr = vm_protect(mach_task_self(),
				data_start,
				data_size + bss_residue_size,
				FALSE,
				data_prot);
		if (kr != KERN_SUCCESS) {
		    e_emulator_error("emul_exec_load: vm_protect data+bss: %s",
				     mach_error_string(kr));
		}
	}
}

	/* Set return values */
	if (entry_point)
	    *entry_point = entry;

	/* For sbrk use */
	if (heap_end)
	    *heap_end = data_start + data_size + bss_residue_size;

	return ESUCCESS;
}
#endif /* 0 */
