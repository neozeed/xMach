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
 * 25-May-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	LITES.
 *
 * 20-Jan-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Adapted to BSDSS.
 *	Changed "" includes to <> includes.
 *	New physmem and host_basic_info files.
 *	Fixed loadavg to use host_info() and to print Mach factors on
 *	a second line.
 * $Log: devfs_vnops.c,v $
 * Revision 1.1  2000/10/27 01:58:46  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/06/25  04:16:48  law
 * Install fix for "update race in 4.4 filesystme code"
 *
 * Revision 1.1.1.1  1995/03/02  21:49:49  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	server/miscfs/kernfs/kernfs_vnops.c
 *	Origin:	Adapted to Lites from 4.4 BSD LITE.
 *
 *	Kernel parameter filesystem (/kern)
 */
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
 *	@(#)kernfs_vnops.c	8.6 (Berkeley) 2/10/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vmmeter.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/devfs/devfs.h>
#include <serv/import_mach.h>
#include <mach/processor_info.h>

int (**devfs_specop_p)();

#define KSTRING	256		/* Largest I/O available via this filesystem */
#define	UIO_MX 32

#define	READ_MODE	(S_IRUSR|S_IRGRP|S_IROTH)
#define	WRITE_MODE	(S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH)
#define DIR_MODE	(S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)

#define MAX_NODES              (4096)

struct devfs_node {
        char* name;
        int is_block;
        dev_t dev;
} devfs_nodes[MAX_NODES];
unsigned int nr_devfs_nodes = 0;

void devfs_register(char* name, int is_block, dev_t dev)
{
   devfs_nodes[nr_devfs_nodes].name = name;
   devfs_nodes[nr_devfs_nodes].is_block = is_block;
   devfs_nodes[nr_devfs_nodes].dev = dev;
   nr_devfs_nodes++;
}

void devfs_unregister(char* name)
{
   unsigned int i;
   
   for (i = 0; i < nr_devfs_nodes; i++)
     {
	if (strcmp(name, devfs_nodes[i].name) == 0)
	  {
	     if (i < (nr_devfs_nodes - 1))
	       {
		  devfs_nodes[i] = devfs_nodes[nr_devfs_nodes - 1];
	       }
	     nr_devfs_nodes--;
	     return;
	  }
     }
}

/*
 * Create a vnode for a character device.
 */
int
devfs_bdevvp(dev, vpp)
	dev_t dev;
	struct vnode **vpp;
{
	register struct vnode *vp;
	struct vnode *nvp;
	int error;

	if (dev == NODEV)
		return (0);
	error = getnewvnode(VT_NON, (struct mount *)0, devfs_specop_p, &nvp);
	if (error) {
		*vpp = 0;
		return (error);
	}
	vp = nvp;
	insmntque(vp, 0);

	vp->v_type = VBLK;
	if (nvp = checkalias(vp, dev, (struct mount *)0)) {
		vput(vp);
		vp = nvp;
	}
	*vpp = vp;
	return (0);
}

/*
 * Create a vnode for a character device.
 */
int
devfs_cdevvp(dev, vpp)
	dev_t dev;
	struct vnode **vpp;
{
	register struct vnode *vp;
	struct vnode *nvp;
	int error;

	if (dev == NODEV)
		return (0);
	error = getnewvnode(VT_NON, (struct mount *)0, devfs_specop_p, &nvp);
	if (error) {
		*vpp = 0;
		return (error);
	}
	vp = nvp;
	insmntque(vp, 0);

	vp->v_type = VCHR;
	if (nvp = checkalias(vp, dev, (struct mount *)0)) {
		vput(vp);
		vp = nvp;
	}
	*vpp = vp;
	return (0);
}

/*
 * vp is the current namei directory
 * ndp is the name to locate in that directory...
 */
mach_error_t devfs_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap;
{
   struct vnode **vpp = ap->a_vpp;
   struct vnode *dvp = ap->a_dvp;
   struct componentname *cnp = ap->a_cnp;
   struct vnode *fvp;
   struct vnode* nvp;
   int error, i;
   char *pname;

//   printf("devfs_lookup(%x)\n", ap);
//   printf("devfs_lookup(dp = %x, vpp = %x, cnp = %x)\n", dvp, vpp, ap->a_cnp);

   pname = cnp->cn_nameptr;
   
   if (cnp->cn_namelen == 1 && *pname == '.') {
      *vpp = dvp;
      VREF(dvp);
      /*VOP_LOCK(dvp);*/
      return (0);
   }
   
   
   for (i = 0; i < nr_devfs_nodes; i++)
     {
	if (strcmp(devfs_nodes[i].name, pname) == 0)
	  {
	     if (devfs_nodes[i].is_block)
	       {
		  error = devfs_bdevvp(devfs_nodes[i].dev, &nvp);
	       }
	     else
	       {
		  error = devfs_cdevvp(devfs_nodes[i].dev, &nvp);
	       }
	     if (error)
	       {
		  printf("devfs_bdevvp/devfs_cdevvp failed\n");
		  *vpp = NULL;
		  return(error);
	       }	     
	     *vpp = nvp;
	     VREF(nvp);
	     VOP_LOCK(nvp);
	     return(0);
	  }
     }
   
   *vpp = NULL;
   return(ENOENT);
}

mach_error_t devfs_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	/*
	 * Can always open the root (modulo perms)
	 */
	if (vp->v_flag & VROOT)
		return (0);

	return (0);
}

static mach_error_t
devfs_access(ap)
	struct vop_access_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct ucred *cred = ap->a_cred;
	mode_t mode = ap->a_mode;

	return (0);
}


mach_error_t devfs_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{   
   struct vnode *vp = ap->a_vp;
   struct vattr *vap = ap->a_vap;
   int error = 0;
   unsigned int i;
   
   bzero((caddr_t) vap, sizeof(*vap));
   vattr_null(vap);
   vap->va_uid = 0;
   vap->va_gid = 0;
   vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
   /* vap->va_qsize = 0; */
   vap->va_blocksize = DEV_BSIZE;
   microtime(&vap->va_atime);
   vap->va_mtime = vap->va_atime;
   vap->va_ctime = vap->va_ctime;
   vap->va_gen = 0;
   vap->va_flags = 0;
   vap->va_rdev = vp->v_rdev;
   /* vap->va_qbytes = 0; */
   vap->va_bytes = 0;
   
   if (vp->v_flag & VROOT) 
     {      
	vap->va_type = VDIR;
	vap->va_mode = DIR_MODE;
	vap->va_nlink = 2;
	vap->va_fileid = 2;
	vap->va_size = DEV_BSIZE;
     } 
   else 
     {
	for (i = 0; i < nr_devfs_nodes; i++)
	  {
	     if (devfs_nodes[i].dev == vp->v_rdev)
	       {
		  break;
	       }
	  }
	if (i == nr_devfs_nodes)
	  {
	     return(EOPNOTSUPP);
	  }
	
	if (devfs_nodes[i].is_block)
	  {
	     vap->va_type = VBLK;
	  }
	else
	  {
	     vap->va_type = VCHR;
	  }
	vap->va_mode = WRITE_MODE; /* XXX */
	vap->va_nlink = 1;
	vap->va_fileid = 3 + i;
	vap->va_size = 0;
     }
   vp->v_type = vap->va_type;
   return (error);
}

mach_error_t devfs_setattr(ap)
	struct vop_setattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	/*
	 * Silently ignore attribute changes.
	 * This allows for open with truncate to have no
	 * effect until some data is written.  I want to
	 * do it this way because all writes are atomic.
	 */
	return (0);
}

static mach_error_t
devfs_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
   return (EOPNOTSUPP);
}

static mach_error_t
devfs_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
   return (0);
}


mach_error_t devfs_readdir(ap)
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap;
{
	struct uio *uio = ap->a_uio;
	int i;
	int error;
        
	i = uio->uio_offset / UIO_MX;
	error = 0;
	while (uio->uio_resid > 0 && i < nr_devfs_nodes) {
		struct dirent d;
		struct dirent *dp = &d;
		struct devfs_node* dn = &devfs_nodes[i];

		bzero((caddr_t) dp, UIO_MX);

		dp->d_namlen = strlen(dn->name);
	        memcpy(dp->d_name, dn->name, dp->d_namlen+1);


		/*
		 * Fill in the remaining fields
		 */
		dp->d_reclen = UIO_MX;
		dp->d_fileno = i + 3;
		dp->d_type = DT_UNKNOWN;	/* XXX */
		/*
		 * And ship to userland
		 */
		error = uiomove((caddr_t) dp, UIO_MX, uio);
		if (error)
			break;
		i++;
	}

	uio->uio_offset = i * UIO_MX;

	return (error);
}

mach_error_t devfs_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	/*
	 * Clear out the v_type field to avoid
	 * nasty things happening in vgone().
	 */
	vp->v_type = VNON;

	printf("kernfs_inactive(%x)\n", vp);

	return (0);
}

mach_error_t devfs_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	printf("devfs_reclaim(%x)\n", vp);

	if (vp->v_data) {
		FREE(vp->v_data, M_TEMP);
		vp->v_data = 0;
	}
	return (0);
}

/*
 * Return POSIX pathconf information applicable to special devices.
 */
mach_error_t devfs_pathconf(ap)
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		int *a_retval;
	} */ *ap;
{

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_MAX_CANON:
		*ap->a_retval = MAX_CANON;
		return (0);
	case _PC_MAX_INPUT:
		*ap->a_retval = MAX_INPUT;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_VDISABLE:
		*ap->a_retval = _POSIX_VDISABLE;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Print out the contents of a /dev/fd vnode.
 */
/* ARGSUSED */
mach_error_t devfs_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	printf("tag VT_DEVFS, devfs vnode\n");
	return (0);
}

/*void*/
mach_error_t devfs_vfree(ap)
	struct vop_vfree_args /* {
		struct vnode *a_pvp;
		ino_t a_ino;
		int a_mode;
	} */ *ap;
{

	return (0);
}

/*
 * /dev/fd vnode unsupported operation
 */
mach_error_t devfs_enotsupp()
{

	return (EOPNOTSUPP);
}

/*
 * /dev/fd "should never get here" operation
 */
mach_error_t devfs_badop()
{

	panic("devfs: bad op");
	/* NOTREACHED */
	return EOPNOTSUPP;
}

/*
 * devfs vnode null operation
 */
mach_error_t devfs_nullop()
{

	return (0);
}

#define devfs_create ((int (*) __P((struct  vop_create_args *)))devfs_enotsupp)
#define devfs_mknod ((int (*) __P((struct  vop_mknod_args *)))devfs_enotsupp)
#define devfs_close ((int (*) __P((struct  vop_close_args *)))nullop)
#define devfs_ioctl ((int (*) __P((struct  vop_ioctl_args *)))devfs_enotsupp)
#define devfs_select ((int (*) __P((struct  vop_select_args *)))devfs_enotsupp)
#define devfs_mmap ((int (*) __P((struct  vop_mmap_args *)))devfs_enotsupp)
#define devfs_fsync ((int (*) __P((struct  vop_fsync_args *)))nullop)
#define devfs_seek ((int (*) __P((struct  vop_seek_args *)))nullop)
#define devfs_remove ((int (*) __P((struct  vop_remove_args *)))devfs_enotsupp)
#define devfs_link ((int (*) __P((struct  vop_link_args *)))devfs_enotsupp)
#define devfs_rename ((int (*) __P((struct  vop_rename_args *)))devfs_enotsupp)
#define devfs_mkdir ((int (*) __P((struct  vop_mkdir_args *)))devfs_enotsupp)
#define devfs_rmdir ((int (*) __P((struct  vop_rmdir_args *)))devfs_enotsupp)
#define devfs_symlink ((int (*) __P((struct vop_symlink_args *)))devfs_enotsupp)
#define devfs_readlink \
	((int (*) __P((struct  vop_readlink_args *)))devfs_enotsupp)
#define devfs_abortop ((int (*) __P((struct  vop_abortop_args *)))nullop)
#define devfs_lock ((int (*) __P((struct  vop_lock_args *)))nullop)
#define devfs_unlock ((int (*) __P((struct  vop_unlock_args *)))nullop)
#define devfs_bmap ((int (*) __P((struct  vop_bmap_args *)))devfs_badop)
#define devfs_strategy ((int (*) __P((struct  vop_strategy_args *)))devfs_badop)
#define devfs_islocked ((int (*) __P((struct  vop_islocked_args *)))nullop)
#define devfs_advlock ((int (*) __P((struct vop_advlock_args *)))devfs_enotsupp)
#define devfs_blkatoff \
	((int (*) __P((struct  vop_blkatoff_args *)))devfs_enotsupp)
#define devfs_valloc ((int(*) __P(( \
		struct vnode *pvp, \
		int mode, \
		struct ucred *cred, \
		struct vnode **vpp))) devfs_enotsupp)
#define devfs_truncate \
	((int (*) __P((struct  vop_truncate_args *)))devfs_enotsupp)
#define devfs_update ((int (*) __P((struct  vop_update_args *)))devfs_enotsupp)
#define devfs_bwrite ((int (*) __P((struct  vop_bwrite_args *)))devfs_enotsupp)

int (**devfs_vnodeop_p)();
struct vnodeopv_entry_desc devfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, devfs_lookup },	/* lookup */
	{ &vop_create_desc, devfs_create },	/* create */
	{ &vop_mknod_desc, devfs_mknod },	/* mknod */
	{ &vop_open_desc, devfs_open },	/* open */
	{ &vop_close_desc, devfs_close },	/* close */
	{ &vop_access_desc, devfs_access },	/* access */
	{ &vop_getattr_desc, devfs_getattr },	/* getattr */
	{ &vop_setattr_desc, devfs_setattr },	/* setattr */
	{ &vop_read_desc, devfs_read },	/* read */
	{ &vop_write_desc, devfs_write },	/* write */
	{ &vop_ioctl_desc, devfs_ioctl },	/* ioctl */
	{ &vop_select_desc, devfs_select },	/* select */
	{ &vop_mmap_desc, devfs_mmap },	/* mmap */
	{ &vop_fsync_desc, devfs_fsync },	/* fsync */
	{ &vop_seek_desc, devfs_seek },	/* seek */
	{ &vop_remove_desc, devfs_remove },	/* remove */
	{ &vop_link_desc, devfs_link },	/* link */
	{ &vop_rename_desc, devfs_rename },	/* rename */
	{ &vop_mkdir_desc, devfs_mkdir },	/* mkdir */
	{ &vop_rmdir_desc, devfs_rmdir },	/* rmdir */
	{ &vop_symlink_desc, devfs_symlink },	/* symlink */
	{ &vop_readdir_desc, devfs_readdir },	/* readdir */
	{ &vop_readlink_desc, devfs_readlink },/* readlink */
	{ &vop_abortop_desc, devfs_abortop },	/* abortop */
	{ &vop_inactive_desc, devfs_inactive },/* inactive */
	{ &vop_reclaim_desc, devfs_reclaim },	/* reclaim */
	{ &vop_lock_desc, devfs_lock },	/* lock */
	{ &vop_unlock_desc, devfs_unlock },	/* unlock */
	{ &vop_bmap_desc, devfs_bmap },	/* bmap */
	{ &vop_strategy_desc, devfs_strategy },/* strategy */
	{ &vop_print_desc, devfs_print },	/* print */
	{ &vop_islocked_desc, devfs_islocked },/* islocked */
	{ &vop_pathconf_desc, devfs_pathconf },/* pathconf */
	{ &vop_advlock_desc, devfs_advlock },	/* advlock */
	{ &vop_blkatoff_desc, devfs_blkatoff },/* blkatoff */
	{ &vop_valloc_desc, devfs_valloc },	/* valloc */
	{ &vop_vfree_desc, devfs_vfree },	/* vfree */
	{ &vop_truncate_desc, devfs_truncate },/* truncate */
	{ &vop_update_desc, devfs_update },	/* update */
	{ &vop_bwrite_desc, devfs_bwrite },	/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc devfs_vnodeop_opv_desc =
	{ &devfs_vnodeop_p, devfs_vnodeop_entries };


int (**devfs_specop_p)();
struct vnodeopv_entry_desc devfs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, spec_close },		/* close */
	{ &vop_access_desc, devfs_access },		/* access */
	{ &vop_getattr_desc, devfs_getattr },		/* getattr */
	{ &vop_setattr_desc, devfs_setattr },		/* setattr */
	{ &vop_read_desc, spec_read },		/* read */
	{ &vop_write_desc, spec_write },		/* write */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_select_desc, spec_select },		/* select */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, spec_fsync },		/* fsync */
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
	{ &vop_inactive_desc, spec_inactive },		/* inactive */
	{ &vop_reclaim_desc, spec_reclaim },		/* reclaim */
	{ &vop_lock_desc, spec_lock },			/* lock */
	{ &vop_unlock_desc, spec_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, spec_print },			/* print */
	{ &vop_islocked_desc, spec_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_blkatoff_desc, spec_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, spec_valloc },		/* valloc */
	{ &vop_reallocblks_desc, spec_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, spec_vfree },		/* vfree */
	{ &vop_truncate_desc, spec_truncate },		/* truncate */
	{ &vop_update_desc, spec_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc devfs_specop_opv_desc =
	{ &devfs_specop_p, devfs_specop_entries };
