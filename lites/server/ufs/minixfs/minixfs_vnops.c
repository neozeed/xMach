/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 * minixfs_vnops.c		(Csizmazia Balazs) 94,95 v0.1
 * from
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

#include "fifo.h"
#include "diagnostic.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dirent.h>

#include <vm/vm.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <ufs/ufs/lockf.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/minixfs/minix_fs.h>

#define EXTERN
#include <ufs/minixfs/minixfs_vnops.h>
#undef EXTERN
#define EXTERN extern
#include <ufs/minixfs/minixfs_lookup.h>
#include <ufs/minixfs/minixfs_vfsops.h>
#undef EXTERN


#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

/* Global vfs data structures for ufs. */
int (**minixfs_vnodeop_p)();
struct vnodeopv_entry_desc minixfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, minixfs_lookup },		/* lookup */
	{ &vop_create_desc, minixfs_create },		/* create */
	{ &vop_mknod_desc, minixfs_mknod },		/* mknod */
	{ &vop_open_desc, ufs_open },			/* open */
	{ &vop_close_desc, minixfs_close },		/* close */
	{ &vop_access_desc, minixfs_access },		/* access */
	{ &vop_getattr_desc, minixfs_getattr },		/* getattr */
	{ &vop_setattr_desc, minixfs_setattr },		/* setattr */
	{ &vop_read_desc, minixfs_read },		/* read */
	{ &vop_write_desc, minixfs_write },		/* write */
	{ &vop_ioctl_desc, ufs_ioctl },			/* ioctl */
	{ &vop_select_desc, ufs_select },		/* select */
	{ &vop_mmap_desc, ufs_mmap },			/* mmap */
	{ &vop_fsync_desc, minixfs_fsync },		/* fsync */
	{ &vop_seek_desc, ufs_seek },			/* seek */
	{ &vop_remove_desc, ufs_remove },		/* remove */
	{ &vop_link_desc, ufs_link },			/* link */
	{ &vop_rename_desc, ufs_rename },		/* rename */
	{ &vop_mkdir_desc, ufs_mkdir },			/* mkdir */
	{ &vop_rmdir_desc, ufs_rmdir },			/* rmdir */
	{ &vop_symlink_desc, ufs_symlink },		/* symlink */
	{ &vop_readdir_desc, minixfs_readdir },		/* readdir */
	{ &vop_readlink_desc, minixfs_readlink },	/* readlink */
	{ &vop_abortop_desc, ufs_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ufs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, minixfs_bmap },		/* bmap */
	{ &vop_strategy_desc, ufs_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, ufs_pathconf },		/* pathconf */
	{ &vop_advlock_desc, ufs_advlock },		/* advlock */
	{ &vop_blkatoff_desc, minixfs_blkatoff },	/* blkatoff */
	{ &vop_valloc_desc, ffs_valloc },		/* valloc */
	{ &vop_reallocblks_desc, ffs_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, ffs_truncate },	/* truncate */
	{ &vop_update_desc, minixfs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc minixfs_vnodeop_opv_desc =
	{ &minixfs_vnodeop_p, minixfs_vnodeop_entries };

int (**minixfs_specop_p)();
struct vnodeopv_entry_desc minixfs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, ufsspec_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, minixfs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsspec_read },		/* read */
	{ &vop_write_desc, ufsspec_write },		/* write */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_select_desc, spec_select },		/* select */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, spec_seek },			/* seek */
	{ &vop_remove_desc, spec_remove },		/* remove */
	{ &vop_link_desc, spec_link },			/* link */
	{ &vop_rename_desc, spec_rename },		/* rename */
	{ &vop_mkdir_desc, spec_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, spec_rmdir },		/* rmdir */
	{ &vop_symlink_desc, spec_symlink },		/* symlink */
	{ &vop_readdir_desc, spec_readdir },		/* readdir */
	{ &vop_readlink_desc, spec_readlink },		/* readlink */
	{ &vop_abortop_desc, spec_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ufs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_blkatoff_desc, spec_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, spec_valloc },		/* valloc */
	{ &vop_reallocblks_desc, spec_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, spec_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc minixfs_specop_opv_desc =
	{ &minixfs_specop_p, minixfs_specop_entries };

#if FIFO
int (**minixfs_fifoop_p)();
struct vnodeopv_entry_desc minixfs_fifoop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fifo_lookup },		/* lookup */
	{ &vop_create_desc, fifo_create },		/* create */
	{ &vop_mknod_desc, fifo_mknod },		/* mknod */
	{ &vop_open_desc, fifo_open },			/* open */
	{ &vop_close_desc, ufsfifo_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, minixfs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsfifo_read },		/* read */
	{ &vop_write_desc, ufsfifo_write },		/* write */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_select_desc, fifo_select },		/* select */
	{ &vop_mmap_desc, fifo_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, fifo_seek },			/* seek */
	{ &vop_remove_desc, fifo_remove },		/* remove */
	{ &vop_link_desc, fifo_link },			/* link */
	{ &vop_rename_desc, fifo_rename },		/* rename */
	{ &vop_mkdir_desc, fifo_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, fifo_rmdir },		/* rmdir */
	{ &vop_symlink_desc, fifo_symlink },		/* symlink */
	{ &vop_readdir_desc, fifo_readdir },		/* readdir */
	{ &vop_readlink_desc, fifo_readlink },		/* readlink */
	{ &vop_abortop_desc, fifo_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ufs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, fifo_bmap },			/* bmap */
	{ &vop_strategy_desc, fifo_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, fifo_pathconf },		/* pathconf */
	{ &vop_advlock_desc, fifo_advlock },		/* advlock */
	{ &vop_blkatoff_desc, fifo_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, fifo_valloc },		/* valloc */
	{ &vop_reallocblks_desc, fifo_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, fifo_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc minixfs_fifoop_opv_desc =
	{ &minixfs_fifoop_p, minixfs_fifoop_entries };
#endif /* FIFO */

#ifdef DEBUG
/*
 * Enabling cluster read/write operations.
 */
#include <sys/sysctl.h>
int doclusterread = 1;
struct ctldebug debug11 = { "doclusterread", &doclusterread };
int doclusterwrite = 1;
struct ctldebug debug12 = { "doclusterwrite", &doclusterwrite };
#else
/* XXX for ufs_readwrite */
#define doclusterread 1
#define doclusterwrite 1
#endif

/* #include <ufs/ufs/ufs_readwrite.c> */

int minix_lblkno (vm_offset_t offset)
{
	return offset / MINIX_BLOCK_SIZE;
}

int minix_lblktosize (int blk)
{
	return blk * MINIX_BLOCK_SIZE;
}

int minix_blkoff (vm_offset_t offset)
{
	return offset % MINIX_BLOCK_SIZE;
}

int minix_ino2blk (struct minix_super_block *fs, int ino)
{
        int blk;

	blk=0 /* it's Mach */+2 /* boot+superblock */ + fs->s_imap_blocks +
		fs->s_zmap_blocks + (ino-1)/MINIX_INODES_PER_BLOCK;
        return blk;
}

int minix_itoo (struct minix_super_block *fs, int ino)
{
	return (ino - 1) % MINIX_INODES_PER_BLOCK;
}


int minix_fsbtodb (struct minix_super_block *fs, int b)
{
        return (b * MINIX_BLOCK_SIZE) / 512 /* DEV_BSIZE */;
}

/*
 * Vnode op for reading.
 */
/* ARGSUSED */
minixfs_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct vnode *vp;
	register struct inode *ip;
	register struct uio *uio;
	struct buf *bp;
	daddr_t lbn, nextlbn;
	off_t bytesinfile;
	long size, xfersize, blkoffset;
	int error;
	u_short mode;
	long	filesize; /* debugging */

	vp = ap->a_vp;
	ip = VTOI(vp);
	mode = ip->i_mode;
	uio = ap->a_uio;

#ifdef KELL
	printf("read inode size:%d , nr:%d , vp:%d\n",
		ip->i_number,(((struct minix_inode *)(&ip->i_din))->minix_i_size),vp);
#endif


#if DIAGNOSTIC
	if (uio->uio_rw != UIO_READ)
		panic("minix_fs_read: mode");

/*
	if (vp->v_type == VLNK) {
		if ((int)ip->i_size < vp->v_mount->mnt_maxsymlinklen)
			panic("%s: short symlink", READ_S);
	} else if (vp->v_type != VREG && vp->v_type != VDIR)
		panic("%s: type %d", READ_S, vp->v_type);
*/
#endif
/*
	fs = ip->I_FS;
	if ((u_quad_t)uio->uio_offset > fs->fs_maxfilesize)
		return (EFBIG);
*/

	filesize=(((struct minix_inode *)(&ip->i_din))->minix_i_size);
	for (error = 0, bp = NULL; uio->uio_resid > 0; bp = NULL) {
		if ((bytesinfile = filesize - uio->uio_offset) <= 0)
			break;
		lbn = minix_lblkno(uio->uio_offset);
		nextlbn = lbn + 1;
		size = MINIX_BLOCK_SIZE /* BLKSIZE(fs, ip, lbn); */;
		blkoffset = minix_blkoff(uio->uio_offset);
		xfersize = MINIX_BLOCK_SIZE - blkoffset;
		if (uio->uio_resid < xfersize)
			xfersize = uio->uio_resid;
		if (bytesinfile < xfersize)
			xfersize = bytesinfile;

		if (minix_lblktosize(nextlbn) > filesize)
			error = bread(vp, lbn, size, NOCRED, &bp);
		else if (0 /*doclusterread*/) {
			printf("cluster_read: size:%d filesize:%d\n",size,filesize);
			error = cluster_read(vp,
			    filesize, lbn, size, NOCRED, &bp);
			}
		else if (lbn - 1 == vp->v_lastr) {
			int nextsize = MINIX_BLOCK_SIZE /* BLKSIZE(fs, ip, nextlbn);*/;
			error = breadn(vp, lbn,
			    size, &nextlbn, &nextsize, 1, NOCRED, &bp);
		} else
			error = bread(vp, lbn, size, NOCRED, &bp);

		if (error)
			break;
		vp->v_lastr = lbn;

		/*
		 * We should only get non-zero b_resid when an I/O error
		 * has occurred, which should cause us to break above.
		 * However, if the short read did not cause an error,
		 * then we want to ensure that we do not uiomove bad
		 * or uninitialized data.
		 */
		size -= bp->b_resid;
		if (size < xfersize) {
			if (size == 0)
				break;
			xfersize = size;
		}
		if (error =
		    uiomove((char *)bp->b_data + blkoffset, (int)xfersize, uio))
			break;

/*
		if (S_ISREG(mode) && (xfersize + blkoffset == MINIX_BLOCK_SIZE ||
		    uio->uio_offset == filesize))
			bp->b_flags |= B_AGE;
*/
		brelse(bp);
	}
	if (bp != NULL)
		brelse(bp);
/*	ip->i_flag |= IN_ACCESS; */
	return (error);

	return EOPNOTSUPP;
}

/*
 * Vnode op for writing.
 */
minixfs_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	return EOPNOTSUPP;
}

/*
 * Synch an open file.
 */
/* ARGSUSED */
int
minixfs_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_waitfor;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct buf *bp;
	struct timeval tv;
	struct buf *nbp;
	int s;

	/*
	 * Flush all dirty buffers associated with a vnode.
	 */
loop:
	s = splbio();
	for (bp = vp->v_dirtyblkhd.lh_first; bp; bp = nbp) {
		nbp = bp->b_vnbufs.le_next;
		if ((bp->b_flags & B_BUSY))
			continue;
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("minixfs_fsync: not dirty");
		bremfree(bp);
		bp->b_flags |= B_BUSY;
		splx(s);
		/*
		 * Wait for I/O associated with indirect blocks to complete,
		 * since there is no way to quickly wait for them below.
		 */
		if (bp->b_vp == vp || ap->a_waitfor == MNT_NOWAIT)
			(void) bawrite(bp);
		else
			(void) bwrite(bp);
		goto loop;
	}
	if (ap->a_waitfor == MNT_WAIT) {
		while (vp->v_numoutput) {
			vp->v_flag |= VBWAIT;
			sleep((caddr_t)&vp->v_numoutput, PRIBIO + 1);
		}
#if DIAGNOSTIC
		if (vp->v_dirtyblkhd.lh_first) {
			vprint("minixfs_fsync: dirty", vp);
			goto loop;
		}
#endif
	}
	splx(s);
	get_time(&tv);
	return (VOP_UPDATE(ap->a_vp, &tv, &tv, ap->a_waitfor == MNT_WAIT));
}

minixfs_bmap(
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap)
{

	struct inode *ip = VTOI(ap->a_vp);
	daddr_t lblkno = ap->a_bn;
	struct	minix_inode	*mip;
#define bloffset ap->a_bn
	daddr_t		retval;
	unsigned short	*ushort_ptr;
	struct buf	*bp;
	int	error;

	/*
	 * Check for underlying vnode requests and ensure that logical
	 * to physical mapping is requested.
	 */
	if (ap->a_vpp != NULL)
		*ap->a_vpp = ip->i_devvp;
	if (ap->a_bnp == NULL)
		return (0);

	/*
	 * Compute the requested block number
	 */
#ifdef KELL
printf("minixfs_bmap called!\n");
#endif

	mip=(struct minix_inode *) &(ip->i_din);
	/* bloffset=(offset/BLOCK_SIZE); it was initialised */
	if (bloffset < 7) {
		retval = FROM_MINIX_TODISKBLOCK(mip->minix_i_zone[bloffset]);
		if (retval == 0) retval=(-1);
		*ap->a_bnp = retval;
		return 0;
	}
	bloffset-=7;
	if (bloffset < MINIX_NINDIR) {
		if (mip->minix_i_zone[7] == 0) {
			*ap->a_bnp = (-1);
			return 0;
		} else {
			error=bread(ip->i_devvp,FROM_MINIX_TODISKBLOCK(mip->minix_i_zone[7]),MINIX_BLOCK_SIZE,NOCRED,&bp);
			if (error) {
				printf("XXX: csb bmap: bread failed!\n");
				brelse(bp);
				return error;
			}
			ushort_ptr=(unsigned short *)(bp->b_data);
			retval = FROM_MINIX_TODISKBLOCK(ushort_ptr[bloffset]);
			if (retval == 0) retval=(-1);
			*ap->a_bnp = retval;
			brelse(bp);
			return 0;
		}
	}
	bloffset-=MINIX_NINDIR;
	if (bloffset < MINIX_NINDIR * MINIX_NINDIR) {
		if (mip->minix_i_zone[8] == 0) {
			*ap->a_bnp = (-1);
			return 0; /* net yet allocated */
		} else {
			error=bread(ip->i_devvp,FROM_MINIX_TODISKBLOCK(mip->minix_i_zone[8]),MINIX_BLOCK_SIZE,NOCRED,&bp);
			if (error) {
				printf("XXX: csb bmap: bread failed!\n");
				brelse(bp);
				return error;
			}
			ushort_ptr=(unsigned short *)(bp->b_data);
			retval = FROM_MINIX_TODISKBLOCK(ushort_ptr[bloffset/MINIX_NINDIR]);
			if (retval == 0) retval=(-1);
			*ap->a_bnp = retval;
			brelse(bp);
			if (retval == (-1)) {
				*ap->a_bnp = (-1);
				return 0;
			} else {
				error=bread(ip->i_devvp,retval,MINIX_BLOCK_SIZE,NOCRED,&bp);
				if (error) {
					printf("XXX: csb bmap: bread failed!\n");
					brelse(bp);
					return error;
				}
				ushort_ptr=(unsigned short *)(bp->b_data);
				retval = FROM_MINIX_TODISKBLOCK(ushort_ptr[bloffset%MINIX_NINDIR]);
				if (retval == 0) retval=(-1);
				*ap->a_bnp = retval;
				brelse(bp);
				return 0;
			}
		}
	}
	printf("minix_block_map: (died ...) Block-index of a file is too big\n");
	*ap->a_bnp = 0;
	while (1)  ; /* XXX it's ugly - we need a panic()? */
}


/* ARGSUSED */
int
minixfs_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
/* XXX It isnt done  - I must work on it! */
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	register struct vattr *vap = ap->a_vap;
	struct timeval time;
	unsigned short urdev;
	get_time(&time);

#ifdef KELL
printf("minixfs_getattr called!\n");
#endif
	ITIMES(ip, &time, &time);
	/*
	 * Copy from inode table
	 */
	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	vap->va_mode = mi_mode(ip) & ~IFMT;
	vap->va_nlink = mi_nlinks(ip);
	vap->va_uid = mi_uid(ip);
	vap->va_gid = mi_gid(ip);
	urdev = (((struct minix_inode*)(&ip->i_din))->minix_i_zone[0]);
	vap->va_rdev = urdev; /* (urdev/256)*65536+urdev%256; */
	vap->va_size = (mi_size(ip));
	vap->va_atime.ts_sec = mi_time(ip);
	vap->va_atime.ts_nsec = 0;
	vap->va_mtime.ts_sec = mi_time(ip);
	vap->va_mtime.ts_nsec = 0;
	vap->va_ctime.ts_sec = mi_time(ip);
	vap->va_ctime.ts_nsec = 0;
	vap->va_flags = ip->i_flags;
	vap->va_gen = ip->i_gen;
	/* this doesn't belong here */
	if (vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_bytes = (-1); /* XXX??? dbtob(ip->i_blocks); */
	vap->va_type = vp->v_type; /* XXX ??? csb */
	vap->va_filerev = ip->i_modrev; /* XXX ???? csb */
	return (0);
}

int
minixfs_setattr(ap)
	struct vop_setattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	printf("minixfs_setattr called!\n");
	return EOPNOTSUPP;
}

/*
 * Create a regular file
 */
int
minixfs_create(ap)
	struct vop_create_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	printf("minixfs_create called!\n");
	return EOPNOTSUPP;
}

/*
 * Mknod vnode call
 */
/* ARGSUSED */
int
minixfs_mknod(ap)
	struct vop_mknod_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	printf("minixfs_mknod called!\n");
	return EOPNOTSUPP;
}

/*
 * Close called.
 *
 * (XXX Update the times on the inode.)
 */
/* ARGSUSED */
int
minixfs_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);

#ifdef KELL
printf("minixfs_close called!\n");
#endif
	if (vp->v_usecount > 1 && !(ip->i_flag & IN_LOCKED)) {
		struct timeval time;
		get_time(&time);

		ITIMES(ip, &time, &time);
	}
	return (0);
}

int
minixfs_access(ap)
	struct vop_access_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	register struct ucred *cred = ap->a_cred;
	mode_t mask, mode = ap->a_mode;
	register gid_t *gp;
	int i, error;

#ifdef KELL
printf("minixfs_access called!\n");
#endif

#if DIAGNOSTIC
	if (!VOP_ISLOCKED(vp)) {
		vprint("minixfs_access: not locked", vp);
		panic("minixfs_access: not locked");
	}
#endif
#if QUOTA
	if (mode & VWRITE)
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			if (error = getinoquota(ip))
				return (error);
			break;
		}
#endif

	/* If immutable bit set, nobody gets to write it. */
	if ((mode & VWRITE) && (ip->i_flags & IMMUTABLE))
		return (EPERM);

	/* Otherwise, user id 0 always gets access. */
	if (cred->cr_uid == 0)
		return (0);

	mask = 0;

	/* Otherwise, check the owner. */
	if (cred->cr_uid == ip->i_uid) {
		if (mode & VEXEC)
			mask |= S_IXUSR;
		if (mode & VREAD)
			mask |= S_IRUSR;
		if (mode & VWRITE)
			mask |= S_IWUSR;
		return ((ip->i_mode & mask) == mask ? 0 : EACCES);
	}

	/* Otherwise, check the groups. */
	for (i = 0, gp = cred->cr_groups; i < cred->cr_ngroups; i++, gp++)
		if (ip->i_gid == *gp) {
			if (mode & VEXEC)
				mask |= S_IXGRP;
			if (mode & VREAD)
				mask |= S_IRGRP;
			if (mode & VWRITE)
				mask |= S_IWGRP;
			return ((ip->i_mode & mask) == mask ? 0 : EACCES);
		}

	/* Otherwise, check everyone else. */
	if (mode & VEXEC)
		mask |= S_IXOTH;
	if (mode & VREAD)
		mask |= S_IROTH;
	if (mode & VWRITE)
		mask |= S_IWOTH;
	return ((ip->i_mode & mask) == mask ? 0 : EACCES);
}

/*
 * Vnode op for reading directories.
 * 
 * The routine below assumes that the on-disk format of a directory
 * is the same as that defined by <sys/dirent.h>. If the on-disk
 * format changes, then it will be necessary to do a conversion
 * from the on-disk format that read returns to the format defined
 * by <sys/dirent.h>.
 *
 * The above message isn't true anymore.
 */
int
minixfs_readdir(ap)
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct uio *uio = ap->a_uio;
	int count, lost, error;
	struct minix_direct midirect;

/* Sure, that we have only one iovec  -- see it in vfs_syscall.c/getdirents ... */
/*
 * e_lite_getdirent d_reclen-nel noveli a seek-positiont, ezert ezzel vigyazni
 * kell.
 * Talan egy (x/26)*16 megfelel majd.
 */

#define SIXTEEN 16 /* :-) */
#define TWENTYTWO 26

	count = uio->uio_resid; /* legyenek boldogok akik akarnak ... */
	count &= ~(SIXTEEN - 1); /* Let it SIXTEEN */

count=64; /* Buffer must be at least 32 byte wide (its size) */


/*
printf("count=%d  offset=%d\n",count,(uio->uio_offset/26)*16);
*/

	lost = uio->uio_resid - count;
	uio->uio_resid = count;
	uio->uio_iov->iov_len = count;
	if (count < SIXTEEN || (((uio->uio_offset/TWENTYTWO)*SIXTEEN) & (SIXTEEN - 1)))
	{
		return (EINVAL);
	}

	{
		struct	uio	auio;
		struct	iovec	aiov;
		caddr_t dirbuf;
		struct dirent dent;

		auio = *uio;
		auio.uio_offset=(auio.uio_offset/TWENTYTWO)*SIXTEEN;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_READ;
		aiov.iov_len = count;
		MALLOC(dirbuf, caddr_t, count, M_TEMP, M_WAITOK); /* dirbuf=(caddr_t)malloc(count); */
		aiov.iov_base = dirbuf;
		error = VOP_READ(ap->a_vp, &auio, 0, ap->a_cred);
		dent.d_fileno=((struct minix_direct *)dirbuf)->inode;
		dent.d_reclen=26;
		if ((count - auio.uio_resid) == 0) dent.d_reclen=0;
		dent.d_namlen=14; /* XXX 14+1 \0 a vegen! kell! */

		dent.d_type=0; /* DT_UNKNOWN XXX what should it be? it seems to be a good choice */
		memcpy(dent.d_name,((struct minix_direct *)dirbuf)->name,14);
		dent.d_name[14]='\0';
		dent.d_namlen=strlen(dent.d_name);
		error = uiomove(&dent, 26, uio);
		FREE(dirbuf, M_TEMP);
	}

	uio->uio_resid += lost;
	return (error);
}

/*
 * Return target name of a symbolic link
 */
int
minixfs_readlink(ap)
	struct vop_readlink_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap;
{
	printf("minixfs_readlink called!\n");
	return EOPNOTSUPP;
}

/*
 * Update the access, modified, and inode change times as specified by the
 * IACCESS, IUPDATE, and ICHANGE flags respectively. The IMODIFIED flag is
 * used to specify that the inode needs to be updated but that the times have
 * already been set. The access and modified times are taken from the second
 * and third parameters; the inode change time is always taken from the current
 * time. If waitfor is set, then wait for the disk write of the inode to
 * complete.
 */
int
minixfs_update(ap)
	struct vop_update_args /* {
		struct vnode *a_vp;
		struct timeval *a_access;
		struct timeval *a_modify;
		int a_waitfor;
	} */ *ap;
{
	register struct fs *fs;
	struct buf *bp;
	struct inode *ip;
	int error;
	struct timeval time;

	ip = VTOI(ap->a_vp);
	if (ap->a_vp->v_mount->mnt_flag & MNT_RDONLY) {
		ip->i_flag &=
		    ~(IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE);
		return (0);
	}
	if ((ip->i_flag &
	    (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) == 0)
		return (0);
	if (ip->i_flag & IN_ACCESS)
		ip->i_atime.ts_sec = ap->a_access->tv_sec;
	if (ip->i_flag & IN_UPDATE) {
		ip->i_mtime.ts_sec = ap->a_modify->tv_sec;
		ip->i_modrev++;
	}
	if (ip->i_flag & IN_CHANGE) {
		get_time(&time);
		ip->i_ctime.ts_sec = time.tv_sec;
	}
	ip->i_flag &= ~(IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE);
	fs = ip->i_fs;
	/*
	 * Ensure that uid and gid are correct. This is a temporary
	 * fix until fsck has been changed to do the update.
	 */
	if (fs->fs_inodefmt < FS_44INODEFMT) {		/* XXX */
		ip->i_din.di_ouid = ip->i_uid;		/* XXX */
		ip->i_din.di_ogid = ip->i_gid;		/* XXX */
	}						/* XXX */
	if (error = bread(ip->i_devvp,
	    fsbtodb(fs, ino_to_fsba(fs, ip->i_number)),
		(int)fs->fs_bsize, NOCRED, &bp)) {
		brelse(bp);
		return (error);
	}
	*((struct dinode *)bp->b_data +
	    ino_to_fsbo(fs, ip->i_number)) = ip->i_din;
	if (ap->a_waitfor)
		return (bwrite(bp));
	else {
		bdwrite(bp);
		return (0);
	}
}
