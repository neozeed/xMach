/*
 * Copyright (c) 1989, 1991, 1993, 1994
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
 * minixfs_vfsops.c		(Csizmazia Balazs) 94,95 v0.1
 * from
 *	@(#)ffs_vfsops.c	8.8 (Berkeley) 4/18/94
 * (Actually, I started my development with  ext2fs_vfsops.c  ,which
 * was a slightly modified version of ffs_vfsops.c, made by Johannes Helander
 * distributed in an "unofficial" release of Lites.)
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

/* extern int (**minixfs_vnodeop_p)(); */

#include "quota.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/stat.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <ufs/minixfs/minix_fs.h>

#define EXTERN extern
#include <ufs/minixfs/minixfs_vnops.h>
#include <ufs/minixfs/minixfs_lookup.h>
#undef EXTERN
#define EXTERN
#include <ufs/minixfs/minixfs_vfsops.h>
#undef EXTERN

#define	ROOTNAME "/dev/hd0b"

extern u_long	nextgennumber;

minixfs_mountroot()
{
	extern struct vnode *rootvp;
	register struct minix_super_block *fs;
	register struct mount *mp;
	struct proc *p = get_proc();	/* XXX */
	struct ufsmount *ump;
	u_int size;
	int error;
	
	/*
	 * Get vnodes for swapdev and rootdev.
	 */
	if (bdevvp(swapdev, &swapdev_vp) || bdevvp(rootdev, &rootvp))
		panic("minixs_mountroot: can't setup bdevvp's");

	mp = bsd_malloc((u_long)sizeof(struct mount), M_MOUNT, M_WAITOK);
	bzero((char *)mp, (u_long)sizeof(struct mount));
	mp->mnt_op = &minixfs_vfsops;
	mp->mnt_flag = MNT_RDONLY;
	if (error = minixfs_mountfs(rootvp, mp, p)) {
		bsd_free(mp, M_MOUNT);
		return (error);
	}


	if (error = vfs_lock(mp)) {
		(void)minixfs_unmount(mp, 0, p); /* XXX MINIX */
		bsd_free(mp, M_MOUNT);
		return (error);
	}
	TAILQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mp->mnt_flag |= MNT_ROOTFS;
	mp->mnt_vnodecovered = NULLVP;
	ump = VFSTOUFS(mp);
	fs 
            =
        ump->um_minixfs;
	bzero(fs->fs_fsmnt, sizeof(fs->fs_fsmnt));
	fs->fs_fsmnt[0] = '/';
	bcopy((caddr_t)fs->fs_fsmnt, (caddr_t)mp->mnt_stat.f_mntonname,
	    MNAMELEN);
	(void) copystr(ROOTNAME, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void)minixfs_statfs(mp, &mp->mnt_stat, p);
	vfs_unlock(mp);
	/* inittodr(fs->fs_time); XXX: we need this too */
	return (0);
}


/*
 * Return buffer with the contents of block "offset" from the beginning of
 * directory "ip".  If "res" is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
int
minixfs_blkatoff(ap)
	struct vop_blkatoff_args /* {
		struct vnode *a_vp;
		off_t a_offset;
		char **a_res;
		struct buf **a_bpp;
	} */ *ap;
{
	struct inode *ip;
	register struct minix_super_block *fs;
	struct buf *bp;
	daddr_t lbn;
	int bsize, error;

	ip = VTOI(ap->a_vp);
	fs = ip->i_minixfs; /* we don't need it really - MINIX worx with fixed sized fs-ses */
	lbn = (ap->a_offset)/MINIX_BLOCK_SIZE;
	bsize = MINIX_BLOCK_SIZE;

	*ap->a_bpp = NULL;
	if (error = bread(ap->a_vp, lbn, bsize, NOCRED, &bp)) {
		brelse(bp);
		return (error);
	}

	if (ap->a_res)
		*ap->a_res = (char *)bp->b_data + minix_blkoff(ap->a_offset);
	*ap->a_bpp = bp;
	return (0);
}

minixfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	return (0);
}

minixfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
	return (EOPNOTSUPP);
}

minixfs_sync(mp, waitfor)
	struct mount *mp;
	int waitfor;
{
	return (0);
}


/*
 * Return the root of a filesystem.
 */
int
minixfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *nvp;
	int error;

	if (error = VFS_VGET(mp, (ino_t)MINIX_ROOTINO, &nvp))
		return (error);
	*vpp = nvp;
	return (0);
}

/* extern int (**minixfs_specop_p)(); */

/*
 * Minixfs flat namespace lookup.
 * Currently unsupported.
 */
minixfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	register struct minix_super_block *fs;
	register struct inode *ip;
	struct ufsmount *ump;
	struct buf *bp;
	struct vnode *vp;
	dev_t dev;
	int i, type, error;

	ump = VFSTOUFS(mp);
	dev = ump->um_dev;
	if ((*vpp = ufs_ihashget(dev, ino)) != NULL)
		return (0);

	/* Allocate a new vnode/inode. */
	if (error = getnewvnode(VT_UFS, mp, minixfs_vnodeop_p, &vp)) {
		*vpp = NULL;
		return (error);
	}
	type = ump->um_devvp->v_tag == VT_MFS ? M_MFSNODE : M_FFSNODE; /* XXX csb */
	MALLOC(ip, struct inode *, sizeof(struct inode), type, M_WAITOK);
	insmntque(vp, mp);
/*
 * Dont forget, that the MINIX fs's inode size is less than
 * the inode size of the FFS/LFS!
 */
	bzero((caddr_t)ip, sizeof(struct inode));
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_fs = fs = ump->um_fs;
	ip->i_dev = dev;
	ip->i_number = ino;
#if QUOTA
	for (i = 0; i < MAXQUOTAS; i++)
		ip->i_dquot[i] = NODQUOT;
#endif
	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ufs_ihashins(ip);

	/* Read in the disk contents for the inode, copy into the inode. */
	if (error = bread(ump->um_devvp, minix_fsbtodb(fs, minix_ino2blk(fs, ino)),
	    MINIX_BLOCK_SIZE, NOCRED, &bp)) {
		/*
		 * The inode does not contain anything useful, so it would
		 * be misleading to leave it on its hash chain. With mode
		 * still zero, it will be unlinked and returned to the free
		 * list by vput().
		 */
		vput(vp);
		brelse(bp);
		*vpp = NULL;
		return (error);
	}
	bcopy((struct minix_inode *)bp->b_data + minix_itoo(fs, ino), &ip->i_din, sizeof(struct minix_inode));
	brelse(bp);

	/*
	 * Initialize the vnode from the inode, check for aliases.
	 * Note that the underlying vnode may have changed.
	 */
	if (error = minixfs_vinit(mp, minixfs_specop_p, FFS_FIFOOPS, &vp)) { /* XXX valoban FFS? rather MINIXFS! */
		vput(vp);
		*vpp = NULL;
		return (error);
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);
	/*
	 * Set up a generation number for this inode if it does not
	 * already have one. This should only happen on old filesystems.
	 */
/*
????????????????????????????????????????????????????????
what to do with generation numbers? MINIXfs hasnt'got it! XXX
????????????????????????????????????????????????????????
	if (ip->i_gen == 0) {
		struct timeval time;

		get_time(&time);
		if (++nextgennumber < (u_long)time.tv_sec)
			nextgennumber = time.tv_sec;
		ip->i_gen = nextgennumber;
		if ((vp->v_mount->mnt_flag & MNT_RDONLY) == 0)
			ip->i_flag |= IN_MODIFIED;
	}
*/
	/*
	 * Ensure that uid and gid are correct. This is a temporary
	 * fix until fsck has been changed to do the update.
	 */
/* XXX: ip->i_uid, ip->i_gid aktualizalasa kell? */

	*vpp = vp;
	return (0);
}


minixfs_fhtovp(mp, fhp, setgen, vpp)
	struct mount *mp;
	struct fid *fhp;
	int setgen;
	struct vnode **vpp;
{
printf("minixfs_fhtovp(mp, fhp, setgen, vpp) not supported!\n");
	return (EOPNOTSUPP);
}

minixfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
printf("minixfs_vptofh(mp, fhp, setgen, vpp) not supported!\n");
	return (EOPNOTSUPP);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int 
minixfs_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct ufs_args args;
	struct ufsmount *ump;
	register struct fs *fs;
	u_int size;
	int error, flags;

	if (error = copyin(data, (caddr_t)&args, sizeof (struct ufs_args)))
		return (error);
	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if ((mp->mnt_flag & MNT_UPDATE) && 0 /* XXX in this version - csb */) {
/* it is not the case by my tests, so I do noything with it (now)... */
		ump = VFSTOUFS(mp);
		fs = ump->um_fs;
		error = 0;
		if (fs->fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			if (vfs_busy(mp))
				return (EBUSY);
			error = ffs_flushfiles(mp, flags, p);
			vfs_unbusy(mp);
		}
		if (!error && (mp->mnt_flag & MNT_RELOAD))
			error = ffs_reload(mp, ndp->ni_cnd.cn_cred, p);
		if (error)
			return (error);
		if (fs->fs_ronly && (mp->mnt_flag & MNT_WANTRDWR))
			fs->fs_ronly = 0;
		if (args.fspec == 0) {
			/*
			 * Process export requests.
			 */
			return (vfs_export(mp, &ump->um_export, &args.export));
		}
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.fspec, p);
	if (error = namei(ndp))
		return (error);
	devvp = ndp->ni_vp;

	if (devvp->v_type != VBLK) {
		vrele(devvp);
		return (ENOTBLK);
	}
	if (major(devvp->v_rdev) >= nblkdev) {
		vrele(devvp);
		return (ENXIO);
	}
	if ((mp->mnt_flag & MNT_UPDATE) == 0)
		error = minixfs_mountfs(devvp, mp, p);
	else {
		if (devvp != ump->um_devvp)
			error = EINVAL;	/* needs translation */
		else
			vrele(devvp);
	}
	if (error) {
		vrele(devvp);
		return (error);
	}
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	(void) copyinstr(path, fs->fs_fsmnt, sizeof(fs->fs_fsmnt) - 1, &size);
	bzero(fs->fs_fsmnt + size, sizeof(fs->fs_fsmnt) - size);
	bcopy((caddr_t)fs->fs_fsmnt, (caddr_t)mp->mnt_stat.f_mntonname,
	    MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, 
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void)minixfs_statfs(mp, &mp->mnt_stat, p);
	return (0);
}

/*
 * Common code for mount and mountroot
 */
int
minixfs_mountfs(devvp, mp, p)
	register struct vnode *devvp;
	struct mount *mp;
	struct proc *p;
{
	register struct ufsmount *ump;
	struct buf *bp;
	register struct fs *fs;
	dev_t dev = devvp->v_rdev;
	struct partinfo dpart;
	caddr_t base, space;
	int havepart = 0, blks;
	int error, i, size;
	int ronly;
	extern struct vnode *rootvp;
	struct minix_super_block *es;
	unsigned long sb_block = MINIX_SUPER_BLOCK;

	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
	if (error = vfs_mountedon(devvp))
		return (error);
	if (vcount(devvp) > 1 && devvp != rootvp)
		return (EBUSY);
	if (error = vinvalbuf(devvp, V_SAVE, p->p_ucred, p, 0, 0))
		return (error);

	ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
	if (error = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED, p))
		return (error);
	if (VOP_IOCTL(devvp, DIOCGPART, (caddr_t)&dpart, FREAD, NOCRED, p) != 0)
		size = DEV_BSIZE;
	else {
		havepart = 1;
		size = dpart.disklab->d_secsize;
	}

	bp = NULL;
	ump = NULL;

{

	/* finally minix specific */
	if (error = bread(devvp, sb_block, MINIX_BLOCK_SIZE, NOCRED, &bp))
		goto out;
	es = (struct minix_super_block *)bp->b_data;
	if (es->s_magic != MINIX_SUPER_MAGIC) {
		error = EINVAL;		/* XXX needs translation */
		goto out;
	}
	ump = bsd_malloc(sizeof *ump, M_UFSMNT, M_WAITOK);
	bzero((caddr_t)ump, sizeof *ump);
	ump->um_minixfs = bsd_malloc((u_long)sizeof(*es), M_UFSMNT,
	    M_WAITOK);

	/* XXX convert es to fs */
}
	bcopy(bp->b_data, ump->um_fs, (u_int)(sizeof(*es /* minix_super_block */)));
	brelse(bp);
	bp = NULL;
	es = ump->um_fs; /*  we copied it with bcopy */
	if (ronly == 0)
		es->fs_fmod = 1;
	mp->mnt_data = (qaddr_t)ump;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	mp->mnt_stat.f_fsid.val[1] = MOUNT_MINIXFS;
	mp->mnt_maxsymlinklen = 0; /* XXX we haven't got symlinks! */
	mp->mnt_flag |= MNT_LOCAL;
	ump->um_mountp = mp;
	ump->um_dev = dev;
	ump->um_devvp = devvp;
	ump->um_nindir = MINIX_NINDIR; /* number of indirect pointers in an 1K fs block */
	ump->um_bptrtodb = 1; /* log2 of ( MINIX_BLOCK_SIZE / DEV_BSIZE ) */
	ump->um_seqinc = 0; /* XXX: ?????????????????? What is it ???????? */
	for (i = 0; i < MAXQUOTAS; i++)
		ump->um_quotas[i] = NULLVP;
	devvp->v_specflags |= SI_MOUNTEDON;
	return (0);
out:
	if (bp)
		brelse(bp);
	(void)VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, NOCRED, p);
	if (ump) {
		bsd_free(ump->um_fs, M_UFSMNT);
		bsd_free(ump, M_UFSMNT);
		mp->mnt_data = (qaddr_t)0;
	}
	return (error);
}

/*
 * unmount system call
 */
int
minixfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	register struct ufsmount *ump;
	register struct fs *fs;
	int error, flags, ronly;

	flags = 0;
	if (mntflags & MNT_FORCE) {
		if (mp->mnt_flag & MNT_ROOTFS)
			return (EINVAL);
		flags |= FORCECLOSE;
	}
	if (error = ffs_flushfiles(mp, flags, p))
		return (error);
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	ronly = fs->fs_ronly;
	if (!ronly) {
		fs->fs_clean = 1;
		minixfs_sbupdate(ump, MNT_WAIT); 
	}
	ump->um_devvp->v_specflags &= ~SI_MOUNTEDON;
	error = VOP_CLOSE(ump->um_devvp, ronly ? FREAD : FREAD|FWRITE,
		NOCRED, p);
	vrele(ump->um_devvp);
	bsd_free(fs->fs_csp[0], M_UFSMNT);
	bsd_free(fs, M_UFSMNT);
	bsd_free(ump, M_UFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Get file system statistics.
 */
int
minixfs_statfs(mp, sbp, p)
	struct mount *mp;
	register struct statfs *sbp;
	struct proc *p;
{
	register struct ufsmount *ump;
	register struct minix_super_block *fs;

	ump = VFSTOUFS(mp);
	fs = ump->um_minixfs;
	if (fs->s_magic != MINIX_SUPER_MAGIC)
		panic("minixfs_statfs");
	sbp->f_type = MOUNT_UFS;
	sbp->f_bsize = MINIX_BLOCK_SIZE;
	sbp->f_iosize = MINIX_BLOCK_SIZE;
	sbp->f_blocks = fs->s_nzones - fs->s_firstdatazone;
	sbp->f_bfree = 0; /* now it's a good choice XXX */
	sbp->f_bavail = 0; /* free blks for superuser */
	sbp->f_files =  fs->s_ninodes - MINIX_ROOTINO; /* NEED MINIX */
	sbp->f_ffree = 0; /* free file inodes in fs */
	if (sbp != &mp->mnt_stat) {
		bcopy((caddr_t)mp->mnt_stat.f_mntonname,
			(caddr_t)&sbp->f_mntonname[0], MNAMELEN);
		bcopy((caddr_t)mp->mnt_stat.f_mntfromname,
			(caddr_t)&sbp->f_mntfromname[0], MNAMELEN);
	}
	strncpy(sbp->f_fstypename, mp->mnt_op->vfs_name, MFSNAMELEN);	
	return (0);
}

struct vfsops minixfs_vfsops = {
	"minixfs",
	minixfs_mount,
	minixfs_start,
	minixfs_unmount,
	minixfs_root,
	minixfs_quotactl,
	minixfs_statfs,
	minixfs_sync,
	minixfs_vget,
	minixfs_fhtovp,
	ffs_vptofh,
	ffs_init,
};

/*
 * Initialize the vnode associated with a new inode, handle aliased
 * vnodes.
 */
int
minixfs_vinit(mntp, specops, fifoops, vpp)
	struct mount *mntp;
	int (**specops)();
	int (**fifoops)();
	struct vnode **vpp;
{
	struct inode *ip;
	struct vnode *vp, *nvp;
	int	localmode;

	vp = *vpp;
	ip = VTOI(vp);
#define S_IFMT 0170000
	localmode =mi_mode(ip);
	switch(vp->v_type = IFTOVT(localmode)) {
	case VCHR:
	case VBLK:
		vp->v_op = specops;
		if (nvp = checkalias(vp, mi_zone(ip,0), mntp)) {
			/*
			 * Discard unneeded vnode, but save its inode.
			 */
			ufs_ihashrem(ip);
			VOP_UNLOCK(vp);
			nvp->v_data = vp->v_data;
			vp->v_data = NULL;
			vp->v_op = spec_vnodeop_p;
			vrele(vp);
			vgone(vp);
			/*
			 * Reinitialize aliased inode.
			 */
			vp = nvp;
			ip->i_vnode = vp;
			ufs_ihashins(ip);
		}
		break;
	case VFIFO:
#if FIFO
		vp->v_op = fifoops;
		break;
#else
		return (EOPNOTSUPP);
#endif
	}
	if (ip->i_number == (ino_t)MINIX_ROOTINO)
                vp->v_flag |= VROOT;
	/*
	 * Initialize modrev times
	 */
	{
		struct timeval mono_time;
		get_time(&mono_time);

		/*SETHIGH(ip->i_modrev, mono_time.tv_sec);
		SETLOW(ip->i_modrev, mono_time.tv_usec * 4294); XXX What is it?*/
	}
	*vpp = vp;
	return (0);
}

/*
 * Write a superblock and associated information back to disk.
 */
int
minixfs_sbupdate(mp, waitfor)
	struct ufsmount *mp;
	int waitfor;
{
	register struct fs *fs = mp->um_minixfs;
	register struct buf *bp;
	int blks;
	caddr_t space;
	int i, size, error = 0;

	bp = getblk(mp->um_devvp, MINIX_SUPER_BLOCK, MINIX_BLOCK_SIZE, 0, 0);
	bcopy((caddr_t)fs, bp->b_data, sizeof(struct minix_super_block));
	if (waitfor == MNT_WAIT)
		error = bwrite(bp);
	else
		bawrite(bp);
/* now returns here ... */
	return (error);
}
