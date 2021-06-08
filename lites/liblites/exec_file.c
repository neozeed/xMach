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
 * 13-Dec-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Elf support from Jeffrey Law.
 *
 * $Log: exec_file.c,v $
 * Revision 1.3  2000/11/07 00:41:29  welchd
 *
 * Added support for executing dynamically linked Linux ELF binaries
 *
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/09/01  06:55:35  gback
 * added QMAGIC support for Linux
 *
 * Revision 1.1.1.2  1995/03/23  01:16:20  law
 * lites-950323 from jvh.
 *
 *
 */
/* 
 *	File:	exec_file.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	November 1994
 *
 *	Interpret executable headers.
 */

#undef KERNEL	/* This code is the same in server and user space */
#include <mach.h>
#include <mach/machine.h>	/* for CPU_TYPE_* */
#include <mach/error.h>

#include <sys/types.h>
#include <sys/exec_file.h>
#include <sys/errno.h>

#define CPU_TYPE_UNKNOWN ((cpu_type_t) 0)

/* These should come from somewhere else but I define it here for now XXX */
#if defined(i386)
#define my_cpu_type CPU_TYPE_I386
#elif defined(mips)
#define my_cpu_type CPU_TYPE_MIPS
#elif defined(parisc)
#define my_cpu_type CPU_TYPE_HPPA
#elif defined(ns532)
#define my_cpu_type CPU_TYPE_NS32532
#elif defined (alpha)
#define my_cpu_type CPU_TYPE_ALPHA
#else
-- fix this and send me the patch ... --
#endif

binary_type_t get_binary_type(
       union exec_data *hdr)
{
	binary_type_t bt = BT_BAD;
	cpu_type_t ct = CPU_TYPE_UNKNOWN;
	guess_binary_type_from_header(hdr, &bt, &ct);

	/* Refuse to load binaries known to be for another architecture */
	if (ct != CPU_TYPE_UNKNOWN && ct != my_cpu_type)
	    return BT_BAD;
	return bt;
}

/* 
 * Figure out from the file header what kind of a binary we are
 * looking at.  Guess the cpu type also.
 * Add more heuristics to this function.
 *
 * The defaults are set by the caller: BT_BAD and CPU_TYPE_UNKNOWN.
 */
void guess_binary_type_from_header(
	union exec_data *hdr,
	binary_type_t	*bt,	/* OUT */
	cpu_type_t	*ct)	/* OUT */
{
	int tmp;

	/********* First check for LITES binary *************/
	if ((hdr->magic == 0x03200107 || hdr->magic == 0x020b0107)
	    && (unsigned)hdr->som.ahdr.entry >= 0x10000000) {
		*bt = BT_LITES_SOM;
		*ct = CPU_TYPE_HPPA;
		return;
	}

	if ((hdr->magic == 0x7f454c46)
	    && (unsigned) hdr->elf.ehdr.e_entry > 0x10000000) {
		*bt = BT_LITES_ELF;
		*ct = CPU_TYPE_HPPA;
		return;
	}

        /* dwelch: Check for Linux ELF binaries as well */
	if (hdr->magic == 0x464c457f) /* 'ELF\0177' little endian */
	     {		
		if ((unsigned) hdr->elf.ehdr.e_entry > 0x10000000)
		  {
		     *bt = BT_LITES_ELF;
		     *ct = CPU_TYPE_I386; /* rather check the machine field */
		     return;
		  }
		else
		  {
		     unsigned int phdr;
		     elf_exec* elf;
		     		     
		     elf = &hdr->elf;
		     for (phdr = 0; phdr < elf->ehdr.e_phnum; phdr++) 
		       {
			  if (elf->phdrs[phdr].p_type == PT_INTERP)
			    {
			       *bt = BT_LINUX_DYN_ELF;
			       *ct = CPU_TYPE_I386;
			       return;
			    }
		       }		     		     
		     *bt = BT_LINUX_ELF;
		     *ct = CPU_TYPE_I386;
		     return;
		  }
	     }

	/* Linked by FreeBSD or NetBSD linker or Linux with -qmagic */
	if ((hdr->short_magic == QMAGIC || (hdr->magic>>16) == 0x0b01)
	    && (hdr->aout.a_entry >= 0x90000000)) {
	    /* this used to be   a_entry >= 0x10000000. 
	     * The reason for this temporary hack is that linux 
	     * ld.so in QMAGIC form has an entry of 0x62f00020
	     * but we really don't want it to be recognized 
	     * as a BT_LITES_Q	
	     */
		*bt = BT_LITES_Q;
		/* XXX take care of cpu type as well. Later. */
		return;
	}

	/* UX linker */
	if (hdr->short_magic == ZMAGIC && hdr->aout.a_entry >= 0x10000000) {
		int mid = (hdr->magic>>16) & 0xff;
		if (mid == 0 || mid == 0x45) {
			*bt = BT_LITES_Z;
			return;
		}
	}

	/* Mips little endian. The test could be more intelligent here */
	if (hdr->short_magic == COFF_LITTLE_MIPSMAGIC
	    && hdr->coff.a.text_start  >= 0x0fc00000
	    && hdr->coff.a.text_start  < 0x10000000) {
		*bt = BT_LITES_LITTLE_ECOFF;
		*ct = CPU_TYPE_MIPS;
		return;
	}

#ifdef alpha
	/* Alpha little endian. Difficult to compile on 32 bit hosts */
	if (hdr->short_magic == COFF_LITTLE_ALPHAMAGIC
	    && hdr->coff.a.text_start  >= 0x11c000000UL
	    && hdr->coff.a.text_start  <  0x120000000UL) {
		*bt = BT_LITES_LITTLE_ECOFF;
		*ct = CPU_TYPE_ALPHA;
		return;
	}
#endif

	/********* Not a LITES binary *************/

	switch (hdr->short_magic) {
	      case ZMAGIC:
	      case QMAGIC:
		switch ((hdr->magic >> 16) & 0xff) /* machine id */
		{
		      case 100:
			if (hdr->aout.a_entry > 0x10000000) {
				*bt = BT_LINUX_SHLIB;
			} else {
				*bt = BT_LINUX;
			}
			*ct = CPU_TYPE_I386;
			return;
		      case 0:
			if (hdr->aout.a_entry) {
				*bt = BT_CMU_43UX;
			} else {
				*bt = BT_386BSD;
			}
			return;
		      case 0x45: 	/* pc532 with gnu ld 2.3 */
			if (hdr->aout.a_entry) {
				*bt = BT_CMU_43UX;
				*ct = CPU_TYPE_NS32532;
				return;
			}
		}
		/* FreeBSD or BSDI */
		*bt = hdr->short_magic==QMAGIC ? BT_FREEBSD : *bt;
		return;

	      case COFF_LITTLE_MIPSMAGIC:
		/* Mips little endian */
		*bt = BT_MIPSEL;
		*ct = CPU_TYPE_MIPS;
		return;

	      case COFF_BIG_MIPSMAGIC:
		/* Mips big endian */
		*bt = BT_MIPSEB;
		*ct = CPU_TYPE_MIPS;
		return;

	      case COFF_LITTLE_ALPHAMAGIC:
		/* Alpha little endian */
		*bt = BT_ALPHAOSF1;
		*ct = CPU_TYPE_ALPHA;
		return;

	      case COFF_ISC4_MAGIC:
		/* Interactive coff */
		*bt = BT_ISC4;
		*ct = CPU_TYPE_I386;
		return;

	      case OMAGIC:
		/* 
		 * Linux OMAGIC. It's pretty certain nobody else is
		 * using this format from the sixties. In any case
		 * there isn't any way to distinguish them what I know
		 * of.
		 */
		*bt = BT_LINUX_O;
		return;
	}

	if (!SOM_N_BADMID(hdr->som) && !SOM_N_BADMAG(hdr->som)) {
		*ct = CPU_TYPE_HPPA;
		if (hdr->som.fhdr.somexecloc != SOM_STDEXECLOC) {
			/* 
			 * non-contig SOM exechdr.
			 * Forget it but give other code a chance...
			 */
			*bt = BT_HPREREAD;
			return;
		}
		switch (hdr->som.fhdr.mid) {
		case MID_HP700:
		case MID_HP800:
			*bt = BT_HPBSD;
			return;
		case MID_HPUX700:
		case MID_HPUX800:
			*bt = BT_HPUX;
			return;
		}
		return;
	}
           
	if (hdr->elf.ehdr.e_ident[0] == ELFMAG0
	    && hdr->elf.ehdr.e_ident[1] == ELFMAG1
	    && hdr->elf.ehdr.e_ident[2] == ELFMAG2
	    && hdr->elf.ehdr.e_ident[3] == ELFMAG3) {
		*bt = BT_ELF;
		/* XXX */
		*ct = CPU_TYPE_UNKNOWN;
		return;
	}
       
	if (hdr->shell[0] == '#' && hdr->shell[1] == '!') {
		*bt = BT_SCRIPT;
		return;
	}

#if BYTE_ORDER == LITTLE_ENDIAN
	/* 
	 * NetBSD tried to make the magic number byte order
	 * independent but got it the wrong way (the magic is not in
	 * the first two bytes of a file as its supposed to be).
	 */
	tmp = ntohl(hdr->magic);

	/* 6 bits of flags, 10 bits of mid, and 16 bits magic */
	if ((tmp & 0xffff) == ZMAGIC) {
		*bt = BT_NETBSD;
		switch ((tmp >> 16) && 0x3ff) {
		      case 134:		/* 0b018600 */
			*ct = CPU_TYPE_I386;
			return;
		      case 137:		/* 0b018900 */
			*ct = CPU_TYPE_NS32532;
			return;
		}
	}
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
}

/* Exec file interpretation dispatcher */
mach_error_t parse_exec_file(
	union exec_data		*hdr,
	binary_type_t		bt,
	struct exec_load_info	*li,	/* OUT (space allocated by caller) */
	struct exec_section	secs[],	/* OUT array (space alloc by caller) */
	int			nsecs)	/* size of array (max num of secs) */
{
	switch (bt) {
	      case BT_LITES_Z:
	      case BT_LITES_Q:
	      case BT_386BSD:
	      case BT_NETBSD:
	      case BT_FREEBSD:
	      case BT_CMU_43UX:
	      case BT_LINUX:
	      case BT_LINUX_SHLIB:
	      case BT_LINUX_O:
		return aout_parse_exec_file(&hdr->aout, bt, li, secs, nsecs);
		break;
	      case BT_LITES_LITTLE_ECOFF:
	      case BT_ISC4:
	      case BT_MIPSEL:
	      case BT_MIPSEB:
	      case BT_ALPHAOSF1:
		return ecoff_parse_exec_file(&hdr->aout, bt, li, secs, nsecs);
		break;
	      case BT_LITES_SOM:
	      case BT_HPBSD:
	      case BT_HPUX:
	      case BT_HPREREAD:
		return som_parse_exec_file(&hdr->aout, bt, li, secs, nsecs);
		break;
	      case BT_LITES_ELF:
	      case BT_ELF:
	      case BT_LINUX_ELF:
              case BT_LINUX_DYN_ELF:
		return elf_parse_exec_file(&hdr->elf, bt, li, secs, nsecs);
		break;
	      default:
		return LITES_ENOEXEC;
	}
}

/* Initialize a section to reasonable defaults */
void exec_section_clear(struct exec_section *sec)
{
	bzero((void *) sec, sizeof(*sec));

	sec->how = EXEC_M_MAP;
	sec->prot = VM_PROT_ALL;
	sec->maxprot = VM_PROT_ALL;
	sec->inheritance = VM_INHERIT_COPY;
	sec->copy = TRUE;

	/* secs[*]->file is set by caller of parse_exec_file */
}

void exec_load_info_clear(struct exec_load_info *li)
{
	bzero((void *) li, sizeof(*li));	
}

/* 
 * a.out is a poor binary format and also its interpretation varies
 * Use entry address to decide where to load. Truncate to 16M boundary.
 * Add 0x1000 in order to cope with standard QMAGIC loaded at 0x1000.
 */
#define TEXT_START_TRUNC(A) ((~((vm_address_t)0xffffff) & A) + vm_page_size)

mach_error_t aout_parse_exec_file(
	struct aout_hdr		*hdr,
	binary_type_t		bt,
	struct exec_load_info	*li,	/* OUT (space allocated by caller) */
	struct exec_section	secs[],	/* OUT array (space alloc by caller) */
	int			nsecs)	/* size of array (max num of secs) */
{
	struct exec_section *text, *data, *bss;

	vm_offset_t bss_fragment_start;
	vm_size_t bss_fragment_size;

	if (nsecs < 4)
	    return LITES_EFBIG;	/* shouldn't happen */

	text = &secs[0];
	data = &secs[1];
	bss  = &secs[2];
	secs[3].how = EXEC_M_STOP;

	exec_section_clear(text);
	exec_section_clear(data);
	exec_section_clear(bss);
	bss->how  = EXEC_M_ZERO_ALLOCATE;

	exec_load_info_clear(li);

	text->prot = VM_PROT_READ | VM_PROT_EXECUTE;
	data->prot = VM_PROT_READ | VM_PROT_WRITE;
	bss->prot = VM_PROT_READ | VM_PROT_WRITE;

	text->size = round_page(hdr->a_text);
	text->offset = 0;

	switch (bt) {
	      case BT_LITES_Z:
		text->size = round_page(hdr->a_text + sizeof(struct aout_hdr));
		text->va = TEXT_START_TRUNC(hdr->a_entry);
		break;
	      case BT_LITES_Q:
		text->va = TEXT_START_TRUNC(hdr->a_entry);
		break;
	      case BT_386BSD:
		text->offset = NBPG;
		break;
	      case BT_NETBSD:
		text->va = 0x1000;
		break;
	      case BT_CMU_43UX:
		text->size = round_page(hdr->a_text + sizeof(struct aout_hdr));
		text->va = 0x10000;
		break;
	      case BT_FREEBSD:
		text->va = 0x1000;
		break;
	      case BT_LINUX:
		text->va = trunc_page(hdr->a_entry);
		text->offset = ((union exec_data *)hdr)->short_magic == QMAGIC 
				? 0 : 1024; /* ZMAGIC */
		break;
	      case BT_LINUX_O:
		text->offset = 32;
		text->prot = VM_PROT_ALL;
		break;
	      case BT_LINUX_SHLIB:
		text->va = trunc_page(hdr->a_entry);
		text->offset = ((union exec_data *)hdr)->short_magic == QMAGIC 
				? 0 : 1024; /* ZMAGIC */
		text->prot = VM_PROT_ALL;	/* or emacs breaks... */
		break;
	      default:
		/* default values */
	}
	text->amount = text->size;

	data->offset = text->offset + text->size;
	data->va = text->va + text->size;
	data->size = round_page(hdr->a_data);
	data->amount = hdr->a_data;

	bss_fragment_start = data->va + hdr->a_data;
	bss_fragment_size = data->size - hdr->a_data;
	if (hdr->a_bss <= bss_fragment_size)
	    bss->how = EXEC_M_STOP; /* forget this and the rest */
	else
	    bss->size = round_page(hdr->a_bss - bss_fragment_size);

	bss->va = data->va + data->size;

	/* 
	 * Fill in load info.
	 * Put BSS zeroing here as there is only one range to zero.
	 */
	li->pc = hdr->a_entry;
	li->zero_start = bss_fragment_start;
	li->zero_count = bss_fragment_size;

	return KERN_SUCCESS;
}

mach_error_t ecoff_parse_exec_file(
	struct exechdr		*hdr,
	binary_type_t		bt,
	struct exec_load_info	*li,	/* OUT (space allocated by caller) */
	struct exec_section	secs[],	/* OUT array (space alloc by caller) */
	int			nsecs)	/* size of array (max num of secs) */
{
	struct exec_section *text, *data, *bss;

	vm_offset_t bss_fragment_start;
	vm_size_t bss_fragment_size, bss_residue_size;

	if (nsecs < 4)
	    return LITES_EFBIG;	/* shouldn't happen */

	text = &secs[0];
	data = &secs[1];
	bss  = &secs[2];
	secs[3].how = EXEC_M_STOP;

	exec_section_clear(text);
	exec_section_clear(data);
	exec_section_clear(bss);
	bss->how  = EXEC_M_ZERO_ALLOCATE;

	exec_load_info_clear(li);

	text->prot = VM_PROT_READ | VM_PROT_EXECUTE;
	data->prot = VM_PROT_READ | VM_PROT_WRITE;
	bss->prot = VM_PROT_READ | VM_PROT_WRITE;

	text->size = round_page(hdr->a.tsize);
	text->va = trunc_page(hdr->a.text_start);
	text->offset = 0;
	text->amount = text->size;

	data->va = trunc_page(hdr->a.data_start);
	data->size = round_page(hdr->a.dsize + hdr->a.data_start - data->va);
	data->amount = hdr->a.dsize;
	data->offset = trunc_page(text->offset + hdr->a.tsize);

	bss_fragment_start = hdr->a.data_start + hdr->a.dsize;
	bss_fragment_size = (round_page(bss_fragment_start)
			     - bss_fragment_start);
	bss->va = data->va + data->size;

	if (hdr->a.bsize <= bss_fragment_size)
	    bss->how = EXEC_M_STOP; /* forget this and the rest */
	else
	    bss->size = round_page(hdr->a.bsize - bss_fragment_size);

	/* 
	 * Fill in load info.
	 * Put BSS zeroing here as there is only one range to zero.
	 */
	li->pc = hdr->a.entry;
	li->gp = hdr->a.gp_value;
	li->zero_start = bss_fragment_start;
	li->zero_count = bss_fragment_size;

	return KERN_SUCCESS;
}

mach_error_t som_parse_exec_file(
	struct som_exechdr	*hdr,
	binary_type_t		bt,
	struct exec_load_info	*li,	/* OUT (space allocated by caller) */
	struct exec_section	secs[],	/* OUT array (space alloc by caller) */
	int			nsecs)	/* size of array (max num of secs) */
{
	struct exec_section *text, *data, *bss;

	vm_offset_t bss_fragment_start;
	vm_size_t bss_fragment_size;

	if (nsecs < 4)
	    return LITES_EFBIG;	/* shouldn't happen */

	text = &secs[0];
	data = &secs[1];
	bss  = &secs[2];
	secs[3].how = EXEC_M_STOP;

	exec_section_clear(text);
	exec_section_clear(data);
	exec_section_clear(bss);
	bss->how  = EXEC_M_ZERO_ALLOCATE;

	exec_load_info_clear(li);

	text->prot = VM_PROT_READ | VM_PROT_EXECUTE;
	data->prot = VM_PROT_ALL;
	bss->prot = VM_PROT_ALL;

	text->size = round_page(hdr->ahdr.tsize);
	text->va = trunc_page(hdr->ahdr.tmem);
	text->amount = text->size;
	text->offset = hdr->ahdr.tfile;

	data->size = round_page(hdr->ahdr.dsize);
	data->va = trunc_page(hdr->ahdr.dmem);
	data->offset = hdr->ahdr.dfile;
	data->amount = hdr->ahdr.dsize;

	bss_fragment_start = hdr->ahdr.dmem + hdr->ahdr.dsize;
	bss_fragment_size =
	    round_page(bss_fragment_start) - bss_fragment_start;

	bss->size = hdr->ahdr.bsize - bss_fragment_size;
	bss->va = data->va + data->size;
	if (hdr->ahdr.bsize <= bss_fragment_size)
	    bss->how = EXEC_M_STOP; /* forget this and the rest */
	else
	    bss->size = round_page(hdr->ahdr.bsize - bss_fragment_size);

	li->pc = hdr->ahdr.entry;
	li->zero_start = bss_fragment_start;
	li->zero_count = bss_fragment_size;

	return KERN_SUCCESS;
}


mach_error_t elf_parse_exec_file(
	elf_exec		*elf,
	binary_type_t		bt,
	struct exec_load_info	*li,	/* OUT (space allocated by caller) */
	struct exec_section	secs[],	/* OUT array (space alloc by caller) */
	int			nsecs)	/* size of array (max num of secs) */
{
	unsigned int phdr;

	if (nsecs < elf->ehdr.e_phnum)
		return LITES_EFBIG;

	exec_load_info_clear(li);
        
	/* Now iterate over them filling in info as needed.  */
	for (phdr = 0; phdr < elf->ehdr.e_phnum; phdr++) {
		vm_size_t off;

		/* Skip zero sized segments and those which are not part
		   of the executable image.  */
		if (elf->phdrs[phdr].p_type != PT_LOAD
		    || elf->phdrs[phdr].p_memsz == 0)
			continue;

		/* Initialize the exec section map for this section.  */
		exec_section_clear(&secs[phdr]);
		secs[phdr].va = trunc_page(elf->phdrs[phdr].p_vaddr);
		off = elf->phdrs[phdr].p_vaddr - secs[phdr].va;
		secs[phdr].amount = elf->phdrs[phdr].p_filesz + off;
		secs[phdr].offset = elf->phdrs[phdr].p_offset - off;
		secs[phdr].size = round_page(elf->phdrs[phdr].p_memsz + off);
		if (elf->phdrs[phdr].p_flags & PF_X)
			secs[phdr].prot |= VM_PROT_EXECUTE;
		if (elf->phdrs[phdr].p_flags & PF_W)
			secs[phdr].prot |= VM_PROT_WRITE;
		if (elf->phdrs[phdr].p_flags & PF_R)
			secs[phdr].prot |= VM_PROT_READ;
		if (elf->phdrs[phdr].p_filesz == 0)
			secs[phdr].how = EXEC_M_ZERO_ALLOCATE;
		else
			secs[phdr].how = EXEC_M_MAP;
		/* XXX Current "bss" scheme assumes there's only one
		   "bss" style segment.  This assumption isn't necessarily
		   valid for SOM or ELF.

		   For ELF, make sure to zero everything up to a page boundary.
		   This is necessary as ELF executables don't really have a
		   separate bss segment who's size we round up to make
		   vm_allocate/vm_map and eventually e_obreak happy.  */
		if ((secs[phdr].prot & VM_PROT_WRITE) != 0
		    && secs[phdr].size > elf->phdrs[phdr].p_filesz) {
			li->zero_start = elf->phdrs[phdr].p_vaddr
					   + elf->phdrs[phdr].p_filesz;
			li->zero_count = secs[phdr].size - secs[phdr].amount;
		}

		secs[phdr+1].how = EXEC_M_STOP;
	}

	li->pc = elf->ehdr.e_entry;

	return KERN_SUCCESS;
}
