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
 * $Log: e_stat.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.5  1995/11/30  03:44:06  mike
 * HP-UX compat changes from Jeff (with a couple of minor additions by mike)
 *
 * Revision 1.4  1995/09/02  05:38:30  mike
 * XXX parisc ifdefs to avoid compilation problems on i386
 *
 * Revision 1.3  1995/09/01  23:27:53  mike
 * HP-UX compatibility support from Jeff F.
 *
 * Revision 1.2  1995/08/15  06:48:34  sclawson
 * modifications from lites-1.1-950808
 *
 * Revision 1.1.1.1  1995/03/02  21:49:28  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_stat.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Various stat conversions.
 */

#include <e_defs.h>

/* VATTR TO STAT CONVERSIONS */

/* Structure printed by gdb */
struct bnr_stat
{
	/* Use basic types in order to avoid size changes from headers */
	short		bnr_st_dev;
	unsigned int	bnr_st_ino;
	unsigned short	bnr_st_mode;
	short		bnr_st_nlink;
	unsigned short	bnr_st_uid;
	unsigned short	bnr_st_gid;
	short		bnr_st_rdev;
	int		bnr_st_size;
	int		bnr_st_atime;
	int		bnr_st_spare1;
	int		bnr_st_mtime;
	int		bnr_st_spare2;
	int		bnr_st_ctime;
	int		bnr_st_spare3;
	int		bnr_st_blksize;
	int		bnr_st_blocks;
	unsigned int	bnr_st_flags;
	unsigned int	bnr_st_gen;
};

/* Structure taken from Lite stat.h */
struct lite_stat {
	dev_t	st_dev;
	ino_t	st_ino;
	mode_t	st_mode;
	nlink_t	st_nlink;
	uid_t	st_uid;
	gid_t	st_gid;
	dev_t	st_rdev;
	struct	timespec st_atimespec;
	struct	timespec st_mtimespec;
	struct	timespec st_ctimespec;
	off_t	st_size;
	quad_t	st_blocks;
	unsigned long	st_blksize;
	unsigned long	st_flags;
	unsigned long	st_gen;
	long	st_lspare;
	quad_t	st_qspare[2];
};

/* structure printed by gdb. Corresponds to Linux 1.1 */
struct linux_stat {
	short unsigned int st_dev;
	short unsigned int __pad1;
	long unsigned int st_ino;
	short unsigned int st_mode;
	short unsigned int st_nlink;
	short unsigned int st_uid;
	short unsigned int st_gid;
	short unsigned int st_rdev;
	short unsigned int __pad2;
	long int st_size;
	long unsigned int st_blksize;
	long unsigned int st_blocks;
	long int linux_st_atime;
	long unsigned int __unused1;
	long int linux_st_mtime;
	long unsigned int __unused2;
	long int linux_st_ctime;
	long unsigned int __unused3;
	long unsigned int __unused4;
	long unsigned int __unused5;
};

struct  sysv_stat {
        dev_t   st_dev;
        ino_t   st_ino;
        mode_t  st_mode;
        nlink_t st_nlink;
        uid_t   st_uid;
        gid_t   st_gid;
        dev_t   st_rdev;
        off_t   st_size;
        time_t  sysv_st_atime;
        time_t  sysv_st_mtime;
        time_t  sysv_st_ctime;
};

/* OSF1 V3.0 alpha stat struct as printed by gdb */
struct osf1_stat {
	int osf1_st_dev;
	unsigned int osf1_st_ino;
	unsigned int osf1_st_mode;
	unsigned short osf1_st_nlink;
	unsigned int osf1_st_uid;
	unsigned int osf1_st_gid;
	int osf1_st_rdev;
	long osf1_st_size;
	int osf1_st_atime;
	int osf1_st_spare1;
	int osf1_st_mtime;
	int osf1_st_spare2;
	int osf1_st_ctime;
	int osf1_st_spare3;
	unsigned int osf1_st_blksize;
	int osf1_st_blocks;
	unsigned int osf1_st_flags;
	unsigned int osf1_st_gen;
};

/* HP-UX stat structure is in here (where it should be...) */
#include <e_hpux.h>

struct linux_ipc_perm
{
	int  key;
	ushort uid;   /* owner euid and egid */
	ushort gid;
	ushort cuid;  /* creator euid and egid */
	ushort cgid;
	ushort mode;
	ushort seq;   /* sequence number */
};

struct linux_shmid_ds {
	struct	linux_ipc_perm shm_perm;
	int	shm_segsz;
	long	shm_atime;		/* last attach time */
	long	shm_dtime;		/* last detach time */
	long	shm_ctime;		/* last change time */
	unsigned short	shm_cpid;	/* pid of creator */
	unsigned short	shm_lpid;	/* pid of last operator */
	short	shm_nattch;		/* no. of current attaches */
	/* not exported */
	unsigned short	 shm_npages;
	void	*shm_pages;
	void *attaches;
};

/* Convert vnode type to directory flag */
static unsigned int e_Vtype_to_Stype(enum vtype Vtype)
{
	switch (Vtype) {
	case VREG:
		return S_IFREG;
	case VDIR:
		return S_IFDIR;
	case VBLK:
		return S_IFBLK;
	case VCHR:
		return S_IFCHR;
	case VLNK:
		return S_IFLNK;
	case VSOCK:
		return S_IFSOCK;
	case VFIFO:
		return S_IFIFO;
	default:
		return 0;
	};
}

void e_vattr_to_bnr_stat(
	struct vattr		*va,
	struct bnr_stat		*sb)
{
	sb->bnr_st_dev = va->va_fsid;
	sb->bnr_st_ino = va->va_fileid;
	sb->bnr_st_mode = va->va_mode | e_Vtype_to_Stype(va->va_type);
	sb->bnr_st_nlink = va->va_nlink;
	sb->bnr_st_uid = va->va_uid;
	sb->bnr_st_gid = va->va_gid;
	sb->bnr_st_rdev = va->va_rdev;
	sb->bnr_st_size = va->va_size;
	sb->bnr_st_atime = va->va_atime.ts_sec;
	sb->bnr_st_mtime = va->va_mtime.ts_sec;
	sb->bnr_st_ctime = va->va_ctime.ts_sec;
	sb->bnr_st_blksize = va->va_blocksize;
	sb->bnr_st_flags = va->va_flags;
	sb->bnr_st_gen = va->va_gen;
	sb->bnr_st_blocks = va->va_bytes / S_BLKSIZE;

	sb->bnr_st_spare1 = 0;
	sb->bnr_st_spare2 = 0;
	sb->bnr_st_spare3 = 0;
}

void e_vattr_to_lite_stat(
	struct vattr		*va,
	struct lite_stat	*sb)
{
	sb->st_dev = va->va_fsid;
	sb->st_ino = va->va_fileid;
	sb->st_mode = va->va_mode | e_Vtype_to_Stype(va->va_type);
	sb->st_nlink = va->va_nlink;
	sb->st_uid = va->va_uid;
	sb->st_gid = va->va_gid;
	sb->st_rdev = va->va_rdev;
	sb->st_size = va->va_size;
	sb->st_atimespec = va->va_atime;
	sb->st_mtimespec= va->va_mtime;
	sb->st_ctimespec = va->va_ctime;
	sb->st_blksize = va->va_blocksize;
	sb->st_flags = va->va_flags;
	sb->st_gen = va->va_gen;
	sb->st_blocks = va->va_bytes / S_BLKSIZE;
}

void e_vattr_to_linux_stat(
	struct vattr		*va,
	struct linux_stat	*sb)
{
	sb->st_dev = va->va_fsid;
	sb->st_ino = va->va_fileid;
	sb->st_mode = va->va_mode | e_Vtype_to_Stype(va->va_type);
	sb->st_nlink = va->va_nlink;
	sb->st_uid = va->va_uid;
	sb->st_gid = va->va_gid;
	sb->st_rdev = va->va_rdev;
	sb->st_size = va->va_size;
	sb->st_blksize = va->va_blocksize;
	sb->st_blocks = va->va_bytes / S_BLKSIZE;
	sb->linux_st_atime = va->va_atime.ts_sec;
	sb->linux_st_mtime = va->va_mtime.ts_sec;
	sb->linux_st_ctime = va->va_ctime.ts_sec;
}

void e_vattr_to_sysv_stat(
	struct vattr		*va,
	struct sysv_stat	*sb)
{
	sb->st_dev = va->va_fsid;
	sb->st_ino = va->va_fileid;
	sb->st_mode = va->va_mode | e_Vtype_to_Stype(va->va_type);
	sb->st_nlink = va->va_nlink;
	sb->st_uid = va->va_uid;
	sb->st_gid = va->va_gid;
	sb->st_rdev = va->va_rdev;
	sb->st_size = va->va_size;
	sb->sysv_st_atime = va->va_atime.ts_sec;
	sb->sysv_st_mtime = va->va_mtime.ts_sec;
	sb->sysv_st_ctime = va->va_ctime.ts_sec;
}

void e_vattr_to_osf1_stat(
	struct vattr		*va,
	struct osf1_stat	*sb)
{
	sb->osf1_st_dev = va->va_fsid;
	sb->osf1_st_ino = va->va_fileid;
	sb->osf1_st_mode = va->va_mode | e_Vtype_to_Stype(va->va_type);
	sb->osf1_st_nlink = va->va_nlink;
	sb->osf1_st_uid = va->va_uid;
	sb->osf1_st_gid = va->va_gid;
	sb->osf1_st_rdev = va->va_rdev;
	sb->osf1_st_size = va->va_size;
	sb->osf1_st_atime = va->va_atime.ts_sec;
	sb->osf1_st_mtime = va->va_mtime.ts_sec;
	sb->osf1_st_ctime = va->va_ctime.ts_sec;
	sb->osf1_st_blksize = va->va_blocksize;
	sb->osf1_st_flags = va->va_flags;
	sb->osf1_st_gen = va->va_gen;

	sb->osf1_st_spare1 = 0;
	sb->osf1_st_spare2 = 0;
	sb->osf1_st_spare3 = 0;
}

void e_vattr_to_hpux_stat(struct vattr *va, hpux_stat_t *hpsb)
{
	bzero(hpsb, sizeof(*hpsb));
	hpsb->hst_dev = va->va_fsid;
	hpsb->hst_ino = va->va_fileid;
	hpsb->hst_mode = va->va_mode | e_Vtype_to_Stype(va->va_type);
	hpsb->hst_nlink = va->va_nlink;
	hpsb->hst_ouid = (u_short)va->va_uid;
	hpsb->hst_ogid = (u_short)va->va_gid;
	hpsb->hst_uid = va->va_uid;
	hpsb->hst_gid = va->va_gid;
#ifdef parisc
	hpsb->hst_rdev = dev_bsdtohpux(va->va_rdev);

	/*
	 * (XXX) If a graphics or hil device, return HP-UX-style dev no.
	 * N.B. Eeew... this code knows the major device numbers!
	 *
	 * P.S. And we still don't want to talk about it...
	 */
	if (va->va_type == VCHR) {
		switch (major(va->va_rdev)) {
		    case 10:	/* /dev/grf... */
			hpsb->hst_rdev = (12 << 24) | 0x100000 |
				((va->va_rdev & 0x7) << 8);
			break;
		    case 14:	/* /dev/hil... */
			hpsb->hst_rdev = ((va->va_rdev&0xf)==0? 23: 24) << 24 |
				0x203000 | ((va->va_rdev & 0xf) << 4);
			break;
		}
	}
#endif
	hpsb->hst_size = va->va_size;
	hpsb->hst_atime = va->va_atime.ts_sec;
	hpsb->hst_mtime = va->va_mtime.ts_sec;
	hpsb->hst_ctime = va->va_ctime.ts_sec;
	hpsb->hst_blksize = va->va_blocksize;
	hpsb->hst_blocks = va->va_bytes / S_BLKSIZE;
}

void e_vattr_to_linux_shmid_stat(
	struct vattr		*va,
	struct linux_shmid_ds	*sb)
{
	sb->shm_perm.key = -1;	/* XXX file name */
	sb->shm_perm.uid = va->va_uid;
	sb->shm_perm.gid = va->va_gid;
	sb->shm_perm.cuid = va->va_uid;
	sb->shm_perm.cgid = va->va_gid;
	sb->shm_perm.mode = va->va_mode; /* XXX are these the same? */
	sb->shm_perm.seq = va->va_gen; /* XXX? */

	sb->shm_segsz = va->va_size;
	sb->shm_atime = va->va_atime.ts_sec; /* XXX attach time */
	sb->shm_dtime = va->va_mtime.ts_sec; /* XXX detach time */
	sb->shm_ctime = va->va_ctime.ts_sec;
	sb->shm_cpid = 0;	/* XXX */
	sb->shm_lpid = 0;	/* XXX */
	sb->shm_nattch = 0;	/* XXX */
}

/* SUPPORT ROUTINES */

/*
 * Set vnode attributes to VNOVAL
 */
void vattr_clear(va)
	register struct vattr *va;
{

	va->va_type = VNON;
	/* XXX These next two used to be one line, but for a GCC bug. */
	va->va_size = VNOVAL;
	va->va_bytes = VNOVAL;
	va->va_mode = va->va_nlink = va->va_uid = va->va_gid = VNOVAL;
	va->va_fsid = va->va_fileid = va->va_blocksize = VNOVAL;
	va->va_rdev = va->va_atime.ts_sec = va->va_atime.ts_nsec = VNOVAL;
	va->va_mtime.ts_sec = va->va_mtime.ts_nsec = VNOVAL;
	va->va_ctime.ts_sec = va->va_ctime.ts_nsec = va->va_flags = VNOVAL;
	va->va_gen = VNOVAL;
	va->va_vaflags = 0;
}
