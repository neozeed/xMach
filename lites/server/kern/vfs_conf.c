/*
 * Copyright (c) 1989, 1993
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
 *	@(#)vfs_conf.c	8.8 (Berkeley) 3/31/94
 */

#include "ffs.h"
#include "lfs.h"
#include "mfs.h"
#include "nfs.h"
#include "fdesc.h"
#include "portal.h"
#include "nullfs.h"
#include "umapfs.h"
#include "kernfs.h"
#include "procfs.h"
#include "afs.h"
#include "cd9660.h"
#include "union.h"
#include "fifo.h"
#include "ext2fs.h"
#include "minixfs.h"
#include "msdosfs.h"
#include "devfs.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>

#include <ufs/ffs/ffs_extern.h>

/*
 * This specifies the filesystem used to mount the root.
 * This specification should be done by /etc/config.
 */
int (*mountroot)() = ffs_mountroot;

/*
 * These define the root filesystem and device.
 */
struct mount *rootfs;
struct vnode *rootvnode;

/*
 * Set up the filesystem operations for vnodes.
 * The types are defined in mount.h.
 */
#if FFS
extern	struct vfsops ufs_vfsops;
#define	UFS_VFSOPS	&ufs_vfsops
#else
#define	UFS_VFSOPS	NULL
#endif

#if LFS
extern	struct vfsops lfs_vfsops;
#define	LFS_VFSOPS	&lfs_vfsops
#else
#define	LFS_VFSOPS	NULL
#endif

#if MFS
extern	struct vfsops mfs_vfsops;
#define	MFS_VFSOPS	&mfs_vfsops
#else
#define	MFS_VFSOPS	NULL
#endif

#if NFS
extern	struct vfsops nfs_vfsops;
#define	NFS_VFSOPS	&nfs_vfsops
#else
#define	NFS_VFSOPS	NULL
#endif

#if FDESC
extern	struct vfsops fdesc_vfsops;
#define	FDESC_VFSOPS	&fdesc_vfsops
#else
#define	FDESC_VFSOPS	NULL
#endif

#if PORTAL
extern	struct vfsops portal_vfsops;
#define	PORTAL_VFSOPS	&portal_vfsops
#else
#define	PORTAL_VFSOPS	NULL
#endif

#if NULLFS
extern	struct vfsops null_vfsops;
#define NULL_VFSOPS	&null_vfsops
#else
#define NULL_VFSOPS	NULL
#endif

#if UMAPFS
extern	struct vfsops umap_vfsops;
#define UMAP_VFSOPS	&umap_vfsops
#else
#define UMAP_VFSOPS	NULL
#endif

#if KERNFS
extern	struct vfsops kernfs_vfsops;
#define KERNFS_VFSOPS	&kernfs_vfsops
#else
#define KERNFS_VFSOPS	NULL
#endif

#if PROCFS
extern	struct vfsops procfs_vfsops;
#define PROCFS_VFSOPS	&procfs_vfsops
#else
#define PROCFS_VFSOPS	NULL
#endif

#if AFS
extern	struct vfsops afs_vfsops;
#define AFS_VFSOPS	&afs_vfsops
#else
#define AFS_VFSOPS	NULL
#endif

#if CD9660
extern	struct vfsops cd9660_vfsops;
#define CD9660_VFSOPS	&cd9660_vfsops
#else
#define CD9660_VFSOPS	NULL
#endif

#if UNION
extern	struct vfsops union_vfsops;
#define	UNION_VFSOPS	&union_vfsops
#else
#define	UNION_VFSOPS	NULL
#endif

#if EXT2FS
extern	struct vfsops ext2fs_vfsops;
#define	EXT2FS_VFSOPS	&ext2fs_vfsops
#else
#define	EXT2FS_VFSOPS	NULL
#endif

#if MINIXFS
extern	struct vfsops minixfs_vfsops;
#define	MINIXFS_VFSOPS	&minixfs_vfsops
#else
#define	MINIXFS_VFSOPS	NULL
#endif

#if MSDOSFS
extern	struct vfsops msdosfs_vfsops;
#define	PC_VFSOPS	&msdosfs_vfsops
#else
#define	PC_VFSOPS	NULL
#endif

#if DEVFS
extern	struct vfsops devfs_vfsops;
#define DEVFS_VFSOPS	&devfs_vfsops
#else
#define DEVFS_VFSOPS	NULL
#endif

struct vfsops *vfssw[] = {
	NULL,			/* 0 = MOUNT_NONE */
	UFS_VFSOPS,		/* 1 = MOUNT_UFS */
	NFS_VFSOPS,		/* 2 = MOUNT_NFS */
	MFS_VFSOPS,		/* 3 = MOUNT_MFS */
	PC_VFSOPS,		/* 4 = MOUNT_PC */
	LFS_VFSOPS,		/* 5 = MOUNT_LFS */
	NULL,			/* 6 = MOUNT_LOFS */
	FDESC_VFSOPS,		/* 7 = MOUNT_FDESC */
	PORTAL_VFSOPS,		/* 8 = MOUNT_PORTAL */
	NULL_VFSOPS,		/* 9 = MOUNT_NULL */
	UMAP_VFSOPS,		/* 10 = MOUNT_UMAP */
	KERNFS_VFSOPS,		/* 11 = MOUNT_KERNFS */
	PROCFS_VFSOPS,		/* 12 = MOUNT_PROCFS */
	AFS_VFSOPS,		/* 13 = MOUNT_AFS */
	CD9660_VFSOPS,		/* 14 = MOUNT_CD9660 */
	UNION_VFSOPS,		/* 15 = MOUNT_UNION */
	EXT2FS_VFSOPS,		/* 16 = MOUNT_EXT2FS */
	MINIXFS_VFSOPS,		/* 17 = MOUNT_MINIXFS */
        DEVFS_VFSOPS,           /* 18 = MOUNT_DEVFS */
	0
};


/*
 *
 * vfs_opv_descs enumerates the list of vnode classes, each with it's own
 * vnode operation vector.  It is consulted at system boot to build operation
 * vectors.  It is NULL terminated.
 *
 */
extern struct vnodeopv_desc ffs_vnodeop_opv_desc;
extern struct vnodeopv_desc ffs_specop_opv_desc;
extern struct vnodeopv_desc ffs_fifoop_opv_desc;
extern struct vnodeopv_desc lfs_vnodeop_opv_desc;
extern struct vnodeopv_desc lfs_specop_opv_desc;
extern struct vnodeopv_desc lfs_fifoop_opv_desc;
extern struct vnodeopv_desc mfs_vnodeop_opv_desc;
extern struct vnodeopv_desc dead_vnodeop_opv_desc;
extern struct vnodeopv_desc fifo_vnodeop_opv_desc;
extern struct vnodeopv_desc spec_vnodeop_opv_desc;
extern struct vnodeopv_desc nfsv2_vnodeop_opv_desc;
extern struct vnodeopv_desc spec_nfsv2nodeop_opv_desc;
extern struct vnodeopv_desc fifo_nfsv2nodeop_opv_desc;
extern struct vnodeopv_desc fdesc_vnodeop_opv_desc;
extern struct vnodeopv_desc portal_vnodeop_opv_desc;
extern struct vnodeopv_desc null_vnodeop_opv_desc;
extern struct vnodeopv_desc umap_vnodeop_opv_desc;
extern struct vnodeopv_desc kernfs_vnodeop_opv_desc;
extern struct vnodeopv_desc procfs_vnodeop_opv_desc;
extern struct vnodeopv_desc cd9660_vnodeop_opv_desc;
extern struct vnodeopv_desc cd9660_specop_opv_desc;
extern struct vnodeopv_desc cd9660_fifoop_opv_desc;
extern struct vnodeopv_desc union_vnodeop_opv_desc;
extern struct vnodeopv_desc ext2fs_vnodeop_opv_desc;
extern struct vnodeopv_desc ext2fs_specop_opv_desc;
extern struct vnodeopv_desc ext2fs_fifoop_opv_desc;
extern struct vnodeopv_desc minixfs_vnodeop_opv_desc;
extern struct vnodeopv_desc minixfs_specop_opv_desc;
extern struct vnodeopv_desc minixfs_fifoop_opv_desc;
extern struct vnodeopv_desc msdosfs_vnodeop_opv_desc;
extern struct vnodeopv_desc devfs_vnodeop_opv_desc;
extern struct vnodeopv_desc devfs_specop_opv_desc;

struct vnodeopv_desc *vfs_opv_descs[] = {
	&ffs_vnodeop_opv_desc,
	&ffs_specop_opv_desc,
#if FIFO
	&ffs_fifoop_opv_desc,
#endif
	&dead_vnodeop_opv_desc,
#if FIFO
	&fifo_vnodeop_opv_desc,
#endif
	&spec_vnodeop_opv_desc,
#if LFS
	&lfs_vnodeop_opv_desc,
	&lfs_specop_opv_desc,
#if FIFO
	&lfs_fifoop_opv_desc,
#endif
#endif
#if MFS
	&mfs_vnodeop_opv_desc,
#endif
#if NFS
	&nfsv2_vnodeop_opv_desc,
	&spec_nfsv2nodeop_opv_desc,
#if FIFO
	&fifo_nfsv2nodeop_opv_desc,
#endif
#endif
#if FDESC
	&fdesc_vnodeop_opv_desc,
#endif
#if PORTAL
	&portal_vnodeop_opv_desc,
#endif
#if NULLFS
	&null_vnodeop_opv_desc,
#endif
#if UMAPFS
	&umap_vnodeop_opv_desc,
#endif
#if KERNFS
	&kernfs_vnodeop_opv_desc,
#endif
#if PROCFS
	&procfs_vnodeop_opv_desc,
#endif
#if CD9660
	&cd9660_vnodeop_opv_desc,
	&cd9660_specop_opv_desc,
#if FIFO
	&cd9660_fifoop_opv_desc,
#endif
#endif
#if UNION
	&union_vnodeop_opv_desc,
#endif
#if EXT2FS
	&ext2fs_vnodeop_opv_desc,
	&ext2fs_specop_opv_desc,
#if FIFO
	&ext2fs_fifoop_opv_desc,
#endif
#endif
#if MINIXFS
	&minixfs_vnodeop_opv_desc,
	&minixfs_specop_opv_desc,
#if FIFO
	&minixfs_fifoop_opv_desc,
#endif
#endif
#if MSDOSFS
	&msdosfs_vnodeop_opv_desc,
#endif
#if DEVFS
	&devfs_vnodeop_opv_desc,
        &devfs_specop_opv_desc,
#endif

	NULL
};
