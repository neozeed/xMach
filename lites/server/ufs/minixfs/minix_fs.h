/* 
 * Minix filesystem for LITES
 * Copyright (c) 1994,95 Csizmazia Balazs
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CSIZMAZIA BALAZS ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CSIZMAZIA BALAZS DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */

#ifndef _LITES_MINIX_FS_H
#define _LITES_MINIX_FS_H

#define MINIX_NINDIR	512	/* Number of indirect pointers in an 1K MINIX BLOCK */
#define	MINIX_ROOTINO	1
#define	MAXMNTLEN	512

/* Structure of the metadata infos */

struct minix_super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned long s_max_size;
	unsigned short s_magic;
	unsigned short s_state;
/* The following items are not on the disk - they are in the core memory */
	unsigned char	fs_fmod;
	char fs_fsmnt[MAXMNTLEN]; /* file name - mounted on */
};

struct minix_inode {
	unsigned short minix_i_mode;
	unsigned short minix_i_uid;
	unsigned long minix_i_size;
	unsigned long minix_i_time;
	unsigned char minix_i_gid;
	unsigned char minix_i_nlinks;
	unsigned short minix_i_zone[9];
};

struct minix_direct {
	unsigned short inode;
	char name[14];
};

/* Minix inode fields -- accessed via inode */

#define MINIXIFY_ACCESS(x,field) (((struct minix_inode*)(&(x->i_din)))->field)
/* MINIXIFY_ACCESS's first parameter is an 'inode *' object */
#define mi_mode(x) MINIXIFY_ACCESS(x,minix_i_mode)
#define mi_uid(x) MINIXIFY_ACCESS(x,minix_i_uid)
#define mi_size(x) MINIXIFY_ACCESS(x,minix_i_size)
#define mi_time(x) MINIXIFY_ACCESS(x,minix_i_time)
#define mi_gid(x)  MINIXIFY_ACCESS(x,minix_i_gid)
#define mi_nlinks(x)  MINIXIFY_ACCESS(x,minix_i_nlinks)
#define mi_zone(x,num) MINIXIFY_ACCESS(x,minix_i_zone[num])

/* Minix inode fields -- that was all */

#define MINIX_BLOCK_SIZE 1024
#define MINIX_INODES_PER_BLOCK	((MINIX_BLOCK_SIZE)/(sizeof (struct minix_inode)))
#define MINIX_SUPER_MAGIC	0x137f
#define MINIX_SUPER_BLOCK	2
#define MINIX_BLOCK_SIZE	1024
#define FROM_MINIX_TODISKBLOCK(x) (2*(x))

#endif	/* _LITES_MINIX_FS_H */
