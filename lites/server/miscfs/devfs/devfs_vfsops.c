
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 */

/*
 * Device filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/devfs/devfs.h>

#include <sys/devfs.h>

errno_t mount_devfs(void)
{
   register struct vnode *vp;
   register struct mount *mp;
   int error, flag = 0;
   struct nameidata nd;
   struct proc* p = get_proc(); 
   char path[] = "/dev";
   unsigned int type = MOUNT_DEVFS;
   unsigned int flags = 0;
   void* data;
   
   /*
    * Must be super user
    */
   if (error = suser(p->p_ucred, &p->p_acflag))
     {
	return (error);
     }
   
   /*
    * Get vnode to be covered
    */
   NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, path, p);
   if (error = namei(&nd))
     {
	return (error);
     }
   vp = nd.ni_vp;
   if (error = vinvalbuf(vp, V_SAVE, p->p_ucred, p, 0, 0))
     {
	return (error);
     }
   if (vp->v_type != VDIR) 
     {
	vput(vp);
	return (ENOTDIR);
     }
   if (type > MOUNT_MAXTYPE || vfssw[type] == NULL) 
     {
	vput(vp);
	return (ENODEV);
     }
   
   /*
    * Allocate and initialize the file system.
    */
   mp = (struct mount *)malloc((u_long)sizeof(struct mount),
			       M_MOUNT, M_WAITOK);
   bzero((char *)mp, (u_long)sizeof(struct mount));
   mp->mnt_op = vfssw[type];
   if (error = vfs_lock(mp)) 
     {
	free((caddr_t)mp, M_MOUNT);
	vput(vp);      
	return (error);
     }
   
   if (vp->v_mountedhere != NULL) 
     {
	vfs_unlock(mp);
	free((caddr_t)mp, M_MOUNT);
	vput(vp);
	return (EBUSY);
     }
   
   vp->v_mountedhere = mp;
   mp->mnt_vnodecovered = vp;

   /*
    * Set the mount level flags.
    */
   if (flags & MNT_RDONLY)
     mp->mnt_flag |= MNT_RDONLY;
   else if (mp->mnt_flag & MNT_RDONLY)
     mp->mnt_flag |= MNT_WANTRDWR;
   mp->mnt_flag &=~ (MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
		     MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC);
   mp->mnt_flag |= flags & (MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
			    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC);
   /*
    * Mount the filesystem.
    */
   error = VFS_MOUNT(mp, path, data, &nd, p);

   /*
    * Put the new filesystem on the mount list after root.
    */
   cache_purge(vp);
   if (!error) 
     {
	TAILQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	VOP_UNLOCK(vp);
	vfs_unlock(mp);
	error = VFS_START(mp, 0, p);
     } 
   else 
     {
	mp->mnt_vnodecovered->v_mountedhere = (struct mount *)0;
	vfs_unlock(mp);
	free((caddr_t)mp, M_MOUNT);
	vput(vp);
     }
   return (error);
}


mach_error_t devfs_init()
{
   devfs_register("console", 0, makedev(0, 0));
   devfs_register("tty", 0, makedev(1, 0));
   devfs_register("kmem", 0, makedev(2, 1));
   devfs_register("null", 0, makedev(2, 2));
   devfs_register("zero", 0, makedev(2, 12));
   devfs_register("iopl", 0, makedev(22, 0));
   devfs_register("kbd", 0, makedev(23, 0));
   devfs_register("time", 0, makedev(25, 0));
   devfs_register("timezone", 0, makedev(26, 0));
   
   return KERN_SUCCESS;
}

/*
 * Mount the Kernel params filesystem
 */
mach_error_t devfs_mount(struct mount* mp, 
			 char* path, 
			 caddr_t data, 
			 struct nameidata* ndp, 
			 struct proc* p)
{
	int error = 0;
	u_int size;
	struct devfs_mount *fmp;
	struct vnode *rvp;

	/*
	 * Update is a no-op
	 */
	if (mp->mnt_flag & MNT_UPDATE)
		return (EOPNOTSUPP);

	error = getnewvnode(VT_DEVFS, mp, devfs_vnodeop_p, &rvp);	/* XXX */
	if (error)
		return (error);

	MALLOC(fmp, struct devfs_mount *, sizeof(struct devfs_mount),
				M_UFSMNT, M_WAITOK);	/* XXX */
        insmntque(rvp, mp);
	rvp->v_type = VDIR;
	rvp->v_flag |= VROOT;

	fmp->dfs_root = rvp;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_data = (qaddr_t) fmp;
	getnewfsid(mp, MOUNT_DEVFS);

	(void) copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN - 1, &size);
	bzero(mp->mnt_stat.f_mntonname + size, MNAMELEN - size);
	bzero(mp->mnt_stat.f_mntfromname, MNAMELEN);
	bcopy("devfs", mp->mnt_stat.f_mntfromname, sizeof("devfs"));

	return (0);
}

mach_error_t devfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	return (0);
}

mach_error_t devfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	int error;
	int flags = 0;
	extern int doforce;
	struct vnode *rootvp = VFSTODEVFS(mp)->dfs_root;


	printf("devfs_unmount(mp = %x)\n", mp);


	if (mntflags & MNT_FORCE) {
		/* devfs can never be rootfs so don't check for it */
		if (!doforce)
			return (EINVAL);
		flags |= FORCECLOSE;
	}

	/*
	 * Clear out buffer cache.  I don't think we
	 * ever get anything cached at this level at the
	 * moment, but who knows...
	 */
	if (rootvp->v_usecount > 1)
		return (EBUSY);

	printf("devfs_unmount: calling vflush\n");

	if (error = vflush(mp, rootvp, flags))
		return (error);


	vprint("devfs root", rootvp);

	/*
	 * Release reference on underlying root vnode
	 */
	vrele(rootvp);
	/*
	 * And blow it away for future re-use
	 */
	vgone(rootvp);
	/*
	 * Finally, throw away the devfs_mount structure
	 */
	bsd_free(mp->mnt_data, M_UFSMNT);	/* XXX */
	mp->mnt_data = 0;
	return 0;
}

mach_error_t devfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *vp;


	/*
	 * Return locked reference to root.
	 */
	vp = VFSTODEVFS(mp)->dfs_root;
	VREF(vp);
	VOP_LOCK(vp);
	*vpp = vp;
	return (0);
}

mach_error_t devfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
	return (EOPNOTSUPP);
}

mach_error_t devfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{

	printf("devfs_statfs(mp = %x)\n", mp);


	sbp->f_type = MOUNT_DEVFS;
	sbp->f_flags = 0;
	sbp->f_bsize = DEV_BSIZE;
	sbp->f_iosize = DEV_BSIZE;
	sbp->f_blocks = 2;		/* 1K to keep df happy */
	sbp->f_bfree = 0;
	sbp->f_bavail = 0;
	sbp->f_files = 0;
	sbp->f_ffree = 0;
	if (sbp != &mp->mnt_stat) {
		bcopy(&mp->mnt_stat.f_fsid, &sbp->f_fsid, sizeof(sbp->f_fsid));
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}
        strncpy(sbp->f_fstypename, mp->mnt_op->vfs_name, MFSNAMELEN);
	return (0);
}

mach_error_t devfs_sync(mp, waitfor)
	struct mount *mp;
	int waitfor;
{
	return (0);
}

/*
 * devfs flat namespace lookup.
 * Currently unsupported.
 */
mach_error_t devfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{

	return (EOPNOTSUPP);
}


mach_error_t devfs_fhtovp(mp, fhp, setgen, vpp)
	struct mount *mp;
	struct fid *fhp;
	int setgen;
	struct vnode **vpp;
{
	return (EOPNOTSUPP);
}

mach_error_t devfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	return (EOPNOTSUPP);
}

struct vfsops devfs_vfsops = {
	"devfs",
	devfs_mount,
	devfs_start,
	devfs_unmount,
	devfs_root,
	devfs_quotactl,
	devfs_statfs,
	devfs_sync,
	devfs_vget,
	devfs_fhtovp,
	devfs_vptofh,
	devfs_init,
};
