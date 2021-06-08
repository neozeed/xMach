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
 * $Log: exec_file.h,v $
 * Revision 1.3  2000/11/07 00:41:29  welchd
 *
 * Added support for executing dynamically linked Linux ELF binaries
 *
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:35  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	exec_file.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	November 1994
 *
 *	Definitions for interpreting executable headers.
 */

#include <sys/param.h>
#include <sys/aout.h>
#include <sys/coff.h>
#include <sys/som.h>
#include <sys/elf.h>
/* mach/machine.h needs to be included by the user of this file */

typedef enum binary_type {
	BT_BAD, BT_LITES_Z, BT_LITES_Q, BT_LITES_SOM, BT_LITES_LITTLE_ECOFF,
	BT_LITES_ELF,
	BT_386BSD, BT_NETBSD, BT_FREEBSD, BT_CMU_43UX,
	BT_SCRIPT, BT_ISC4, BT_LINUX, BT_LINUX_SHLIB, BT_LINUX_O,
	BT_MIPSEL, BT_MIPSEB, BT_HPBSD, BT_HPUX, BT_HPREREAD,
	BT_ELF, BT_ALPHAOSF1, BT_LINUX_ELF, BT_LINUX_DYN_ELF
    } binary_type_t;

/* Names for the above (for @bin expansion) */
#define ATSYS_NAMES(m) \
    m ## "bad", m ## "lites", m ## "lites", m ## "lites", m ## "lites", \
    m ## "lites", \
    m ## "bnr", m ## "netbsd", m ## "freebsd", m ## "ux", \
    "script", m ## "isc4", m ## "linux", m ## "linux", m ## "linux", \
    m ## "ultrix", m ## "riscos", m ## "hpbsd", m ## "hpux", m ## "hpkludge", \
    m ## "hpelf", m ## "osf1", m ## "linux", m ## "linux (dynamic)"

union exec_data {
	unsigned short	short_magic;
	unsigned	magic;
	struct aout_hdr	aout;
	struct exechdr	coff;
	struct som_exechdr	som;
	elf_exec	elf;
	char		shell[MAXINTERP];/* #! and interpreter name */
};

/* Internal representation */
enum exec_map_method {
	EXEC_M_NONE,		/* Ignore */
	EXEC_M_MAP,		/* Use vm_map or equivalent */
	EXEC_M_MAP_PARTIAL,	/* Only part of page comes from file */
	EXEC_M_ZERO_ALLOCATE,	/* Allocate zeroed anonymous memory */
	EXEC_M_ZERO,		/* Zero the memory */
	EXEC_M_LOAD_FILE,	/* Load another file */
	EXEC_M_STOP		/* Stop processing this file */
    };

struct exec_section {
	enum exec_map_method	how;		/* How to map */
	mach_port_t		file;		/* file to map */
	vm_prot_t		prot;		/* Leave this access */
	vm_prot_t		maxprot;	/* Maximum access */
	vm_inherit_t		inheritance;	/* What happens in fork */
	boolean_t		copy;		/* Changes visible if FALSE */
	vm_address_t		va;		/* Where to map in memory */
	vm_size_t		size;		/* How much in memory */
	size_t			amount;		/* How much from file */
	off_t			offset;		/* Where in file */
};

struct exec_load_info {
	natural_t	pc;		/* Program counter (entry point) */
	natural_t	gp;		/* Global pointer (if applicable) */
	natural_t	sp;		/* Stack pointer (if relevant) */
	natural_t	fp;		/* Frame pointer (if relevant) */

	/* These are used for BSS zeroing as an optimization */
	vm_address_t	zero_start;	/* Zero memory from here */
	vm_size_t	zero_count;	/* This many bytes */
};

/* Prototypes for exec_file.c exported functions */

binary_type_t get_binary_type(union exec_data *hdr);
void guess_binary_type_from_header(union exec_data *hdr, binary_type_t *bt,
				   cpu_type_t *ct);

