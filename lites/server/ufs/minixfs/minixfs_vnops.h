/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ffs_vnops.c	8.7 (Berkeley) 2/3/94
 */
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

#define MINIX_BLOCK_SIZE	1024

/* Global vfs data structures for minixfs. */
EXTERN int (**minixfs_vnodeop_p)();
EXTERN int (**minixfs_specop_p)();
#if FIFO
EXTERN int (**minixfs_fifoop_p)();
#endif /* FIFO */

EXTERN int minix_lblkno __P((vm_offset_t offset));
EXTERN int minix_lblktosize __P((int blk));
EXTERN int minix_blkoff __P((vm_offset_t offset));
EXTERN int minixfs_read __P((struct vop_read_args *ap));
EXTERN int minixfs_write __P((struct vop_write_args *ap));
EXTERN int minixfs_bmap __P((struct vop_bmap_args *ap));
EXTERN int minixfs_getattr __P((struct vop_getattr_args *ap));
EXTERN int minixfs_setattr __P((struct vop_setattr_args *ap));
EXTERN int minixfs_create __P((struct vop_create_args *ap));
EXTERN int minixfs_mknod __P((struct vop_mknod_args *ap));
EXTERN int minixfs_close __P((struct vop_close_args *ap));
EXTERN int minixfs_access __P((struct vop_access_args *ap));
EXTERN int minixfs_fsync __P((struct vop_fsync_args *ap));
EXTERN int minixfs_readdir __P((struct vop_readdir_args *ap));
EXTERN int minixfs_readlink __P((struct vop_readlink_args *ap));
EXTERN int minixfs_update __P((struct vop_update_args *ap));

EXTERN int minix_ino2blk __P((struct minix_super_block *fs, int ino));
EXTERN int minix_itoo __P((struct minix_super_block *fs, int ino));
EXTERN int minix_fsbtodb __P((struct minix_super_block *fs, int b));
