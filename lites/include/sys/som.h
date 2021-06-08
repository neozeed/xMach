/* 
 * Copyright (c) 1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Computer
 * Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
/*
 * HISTORY
 * $Log: som.h,v $
 * Revision 1.2  2000/10/27 01:55:29  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:34  mike
 * Initial Lites release from hut.fi
 *
 */

#ifndef	_SYS_SOM_H_
#define	_SYS_SOM_H_

/*
 * Simplified SOM header structure for use by kern_exec code.
 *
 * Code should check ``somexecloc'' and make sure it is
 * sizeof(struct skel_som_filehdr), otherwise it needs to
 * seek to that location and read the som_auxhdr.
 *
 * Actually that's not true.  According to the draft SOM ABI,
 * the exec header must immediately follow the file header.
 */

struct _som_filehdr {
	short		mid;		/* machine ID, see below */
	short		magic;		/* executable type, see below */
	long		pad0[6];	/* do not care */
	unsigned int	somexecloc;	/* location of aux. header */
	long		pad1[24];	/* do not care */
};

/* mid values */
#define	MID_HP700	700	/* hp700 BSD binary */
#define	MID_HP800	800	/* hp800 BSD binary */
#define	MID_HPUX700	0x210	/* hp700 HP-UX binary */
#define	MID_HPUX800	0x20B	/* hp800 HP-UX binary */

/* magic values */
#define	SOM_OMAGIC	0407	/* old impure format */
#define	SOM_NMAGIC	0410	/* read-only text */
#define	SOM_ZMAGIC	0413	/* demand load format */

/* standard location of auxiliary header */
#define	SOM_STDEXECLOC	sizeof(struct _som_filehdr)

struct _som_auxhdr {
	long		auxid[2];	/* aux header id */
	long		tsize;		/* size of text in bytes */
	long		tmem;		/* start of text in memory */
	long		tfile;		/* start of text in file */
	long		dsize;		/* size of idata in bytes */
	long		dmem;		/* start of idata in memory */
	long		dfile;		/* start of idata in file */
	long		bsize;		/* size of bss in bytes */
	long		entry;		/* entry point (offset) */
	long		flags;		/* flags passed from linker */
	long		bfill;		/* bss fill value (ignored, always 0) */
};

/* flags values */
#define	SOM_NODEREFZERO	1

struct som_exechdr {
	struct _som_filehdr	fhdr;	/* file header */
	struct _som_auxhdr	ahdr;	/* auxiliary header */
};

/*
 * XXX for BSD compatibility
 */
struct bsd_exechdr {
	struct {
		short	mid;
		short	magic;
	} fhdr;				/* semi-filehdr */
	struct	_som_auxhdr	ahdr;	/* auxiliary header */
};

/* handy macros */
#define	SOM_N_BADMID(x) \
	((x).fhdr.mid != MID_HP700 && \
	 (x).fhdr.mid != MID_HP800 && \
	 (x).fhdr.mid != MID_HPUX700 && \
	 (x).fhdr.mid != MID_HPUX800)


#define	SOM_N_BADMAG(x) \
	((x).fhdr.magic != SOM_OMAGIC && \
	 (x).fhdr.magic != SOM_NMAGIC && \
	 (x).fhdr.magic != SOM_ZMAGIC)

#define	SOM_N_TXTOFF(x) \
	((x).fhdr.tfile)

#endif	/* _SYS_SOM_H_ */
