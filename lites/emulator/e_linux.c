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
 * $Log: e_linux.c,v $
 * Revision 1.4  2000/11/26 01:35:55  welchd
 *
 * Fixed to getdents interface
 *
 * Revision 1.3  2000/11/07 00:41:25  welchd
 *
 * Added support for executing dynamically linked Linux ELF binaries
 *
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.2  1995/09/03  21:22:05  gback
 * added support for Linux mount and reboot
 *
 * Revision 1.1.1.2  1995/03/23  01:15:34  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/e_linux_stubs.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	February 1994
 *
 *	Linux system call handler functions.
 */

#include <e_defs.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <device/tty_status.h>

typedef int linux_pid_t;

static sigset_t sigmask_linux_to_bsd(sigset_t omask);
static sigset_t sigmask_bsd_to_linux(sigset_t omask);
inline int signal_number_bsd_to_linux(int old_sig);
inline int signal_number_linux_to_bsd(int old_sig);
extern int syscall_debug;

errno_t e_linux_pause(int* retval)
{
   return(e_sigsuspend(0xffff));
}

errno_t e_linux_nanosleep(const struct timespec* req, struct timespec* rem,
			  int* retval)
{
   
}

/* Linux mount support */
/*
 * These are the fs-independent mount-flags: up to 16 flags are supported
 */
#define MS_RDONLY        1 /* mount read-only */
#define MS_NOSUID        2 /* ignore suid and sgid bits */
#define MS_NODEV         4 /* disallow access to device special files */
#define MS_NOEXEC        8 /* disallow program execution */
#define MS_SYNCHRONOUS  16 /* writes are synced at once */
#define MS_REMOUNT      32 /* alter flags of a mounted FS */

errno_t 
e_linux_mount(char *special, char *dir, char *type, int flags 
		/*, void *extraoptions, int *retval */)
{
	int bsd_flags = 0, bsd_type = 0;
	struct ufs_args args;
	struct msdosfs_args msdosargs;
	void *data = (void*)&args;
	int error;

	if (syscall_debug > 2)
		e_emulator_error("e_linux_mount(%s, %s, %s, x%x, _)\n",
			special, dir, type, flags);
	if (!strcmp(type, "ext2")) {
		bsd_type = MOUNT_EXT2FS;
		args.fspec = special;
	} else
	if (!strcmp(type, "msdos")) {
		bsd_type = MOUNT_MSDOS;
		/* XXX parse *extraoptions ... */
		msdosargs.uid = 0;
		msdosargs.gid = 0;
		msdosargs.mask = 0755;
		msdosargs.fspec = special;
		data = (void*)&msdosargs;
	} else
	if (!strcmp(type, "ufs")) {
		bsd_type = MOUNT_UFS;
		args.fspec = special;
	} else
		return EINVAL;	

	bsd_flags |= (flags & MS_RDONLY)	? MNT_RDONLY : 0;
	bsd_flags |= (flags & MS_NOSUID)	? MNT_NOSUID : 0;
	bsd_flags |= (flags & MS_NODEV)		? MNT_NODEV : 0;
	bsd_flags |= (flags & MS_NOEXEC)	? MNT_NOEXEC : 0;
	bsd_flags |= (flags & MS_SYNCHRONOUS)	? MNT_SYNCHRONOUS : 0;
	bsd_flags |= (flags & MS_REMOUNT)	? MNT_UPDATE : 0;

	if (syscall_debug > 2)
		e_emulator_error("e_mount(%d, %s, x%x, %s)\n",
			bsd_type, dir, bsd_flags, args.fspec);
	return e_mount(bsd_type, dir, bsd_flags, data);
}

/* Linux reboot - a general purpose call. Sets ctrl-alt-del action,
   remote controls color TVs and much more...
 */
errno_t e_linux_reboot(int magic, int magic_too, int flag/*, int *retval */)
{
	if(magic != 0xfee1dead || magic_too != 672274793)
		return EINVAL;
	if(flag == 0x1234567)
		return e_reboot(RB_AUTOBOOT);
	if(flag == 0xCDEF0123)
		; /* XXX halt the system */
	e_emulator_error("special reboot options not implemented\n");
	return EINVAL;
}

/* Linux select args are on the stack. */
errno_t e_linux_select(unsigned int *args, int *nready)
{
	return e_select(args[0], (fd_set *) args[1], (fd_set *) args[2],
			(fd_set *) args[3], (struct timeval *) args[4],
			nready);
}

/* Shared library loader */
errno_t e_linux_uselib(const char *library)
{
	union exec_data exech;
	enum binary_type binary_type;
	struct exec_load_info li_data;
	errno_t err;
	int fd;

	if (syscall_debug > 2)
		e_emulator_error("e_linux_uselib(%s)\n", library);
	err = emul_exec_open(library, &fd, &binary_type, &exech, 0, 0,
			     MACH_PORT_NULL);
	if (err)
	    return err;
	if (binary_type != BT_LINUX_SHLIB) {
		if (syscall_debug > 0)
		    e_emulator_error("e_linux_uselib: \"%s\" is not a %s\n",
				     library, "Linux shared library");
		e_close(fd);
		return ENOEXEC;
	}
	err = emul_exec_load(fd, binary_type, &exech, &li_data, 0,
			     MACH_PORT_NULL);
	if (err)
	    return err;
	return ESUCCESS;
}

struct linux_mmap_args {
	unsigned long addr;
	unsigned long len;
	unsigned long prot;
	unsigned long flags;
	unsigned long fd;
	unsigned long off;
};

#define LINUX_MAP_SHARED     (0x1)
#define LINUX_MAP_PRIVATE    (0x2)
#define LINUX_MAP_FIXED      (0x10)
#define LINUX_MAP_ANON       (0x20)
#define LINUX_MAP_GROWSDOWN  (0x100)
#define LINUX_MAP_DENYWRITE  (0x800)
#define LINUX_MAP_EXECUTABLE (0x1000)
#define LINUX_MAP_LOCKED     (0x2000)
#define LINUX_MAP_NORESERVE  (0x4000)

errno_t e_linux_mmap(struct linux_mmap_args *args, caddr_t *addr)
{  
   errno_t err;
   int flags;
   unsigned long iaddr;
   
#if 0
   e_emulator_error("e_linux_mmap(start 0x%x, length %d, prot %x, flags %x, "
		    "fd %d, off %x", args->addr, args->len, args->prot,
		    args->flags, args->fd, args->off);
#endif   
   
   flags = 0;
   if (args->flags & LINUX_MAP_SHARED)
     {
	flags = flags | MAP_SHARED;
     }
   if (args->flags & LINUX_MAP_PRIVATE)
     {
	flags = flags | MAP_PRIVATE;
     }
   if (args->flags & LINUX_MAP_FIXED)
     {
	flags = flags | MAP_FIXED;
     }
   if (args->flags & LINUX_MAP_ANON)
     {
	flags = flags | MAP_ANON;
     }
   /* FIXME: Handle the other flags to the linux system call */
   
   iaddr = args->addr;
   if (iaddr == 0)
     {
	iaddr = 0x40000000;
     }
   
   err = e_mmap(iaddr,
		args->len,
		args->prot,  /* Protection constants are same */
		flags,
		args->fd,
		args->off,
		addr);
   if (err)
     {
	*addr = (caddr_t *)-1;
	return(err);
     }
#if 0
   e_emulator_error("e_linux_mmap() = 0x%x", *addr);
#endif   
   return(err);
}

extern vm_offset_t heap_end;

errno_t e_linux_brk(void *new, int *retval)
{
	errno_t err;
	if (new == 0) {
		*retval = heap_end;
		return ESUCCESS;
	}
	err = e_obreak(new, 0);
	if (err)
	    return err;
	/* Linux's tcsh doesn't work if the address is rounded */
	*retval = (int) new;

	return ESUCCESS;
}

struct linux_dirent {
        long		d_ino;
        unsigned int 	d_off;
        unsigned short	d_reclen;
        char		d_name[256];
};

#include <sys/dirent.h>
#undef d_ino

/* 
 * Keep track of directory seek position. Should be separate for each
 * file and should be more efficient. But the efficient version
 * belongs to the shared lib where readdir calls getdirentries. But
 * for that to work there must be opendir etc. and they are not called
 * here.
 */

int readdir_last_fd = 0;
int readdir_seek = 0;

boolean_t linux_readdir_debug = FALSE;


errno_t e_linux_readdir(
	int			fd,
	struct linux_dirent	*linux_dirent,
	unsigned int		count,
	int			*retval)
{
	long offset;
	char buffer[512];

	errno_t err;
	kern_return_t kr;
	int nread;
	struct dirent *de;

	struct {
		int fd;
		char *buf;
		int nbytes;
		long *basep;
	} a;

	if (fd != readdir_last_fd) {
		readdir_last_fd = fd;
		readdir_seek = 0;
	}

	/* Round 512 up, else down */
	e_bnr_lseek(fd, ((readdir_seek + 1) & ~511), 0, 0);

	err = e_lite_getdirentries(fd, buffer, sizeof(buffer),
				   &offset, &nread);

	if (err)
	    return err;

	if (nread <= 0) {
		*retval = 0;
		return ESUCCESS;
	}
	de = (struct dirent *) ((vm_offset_t)buffer + (readdir_seek & 511));
	/* Checks XXX */
	readdir_seek += de->d_reclen;
	if (de->d_reclen == 0) {
		/* It looks like this is another way of saying EOF. */
		*retval = 0;
		return ESUCCESS;
	}
	
	bcopy(de->d_name, linux_dirent->d_name, de->d_namlen);
	linux_dirent->d_name[de->d_namlen] = '\0';
	linux_dirent->d_reclen = de->d_namlen;
	linux_dirent->d_off = readdir_seek;
	linux_dirent->d_ino = de->d_fileno;

	if (linux_readdir_debug)
	    e_emulator_error("e_linux_readdir got \"%s\" i=%d rl=%d nl=%d seek=%d",
			     linux_dirent->d_name, de->d_fileno, de->d_reclen,
			     de->d_namlen, readdir_seek);
	*retval = 1;
	return ESUCCESS;
}

static unsigned int convert_dents(char* in_buf,
				  char* out_buf,
				  unsigned int nread,
				  unsigned int count,
				  unsigned int* last_offset)
{
   unsigned int j;
   unsigned int i;
   unsigned int k;
   struct linux_dirent* lde;
   struct dirent* de;
   
#if 0
   j = 0;
   while (j < nread)
     {
	de = (struct dirent *)(in_buf + j);
	
	if (de->d_reclen == 0)
	  {
	     break;
	  }
	
	e_emulator_error("%d %d %s %d", 
			 j, de->d_fileno, de->d_name, de->d_reclen);
	
	j = j + de->d_reclen;
     }
#endif
   
   j = 0;
   k = 0;
   memset(out_buf, 0, count);
   while (j < nread)
     {
	de = (struct dirent *)(in_buf + j);
	lde = (struct linux_dirent *)(out_buf + k);
	
	if (k + 10 + de->d_namlen + 1 >= count ||
	    de->d_reclen == 0)
	  {
	     break;
	  }
	
	memcpy(lde->d_name, de->d_name, de->d_namlen);
	lde->d_name[de->d_namlen] = 0;
	lde->d_ino = de->d_fileno;
	lde->d_off = j + de->d_reclen; 
	lde->d_reclen = 10 + de->d_namlen + 1;
	if ((lde->d_reclen % 4) != 0)
	  {
	     lde->d_reclen = lde->d_reclen + 4 - (lde->d_reclen % 4);
	  }
	
	j = j + de->d_reclen;
	k = k + lde->d_reclen;
     }
   *last_offset = j;
   
#if 0
   i = 0;
   while (i < k)
     {
	lde = (struct linux_dirent *)(out_buf + i);
	
	e_emulator_error("%d %d %d %d %s", 
			 i, lde->d_ino, lde->d_off, lde->d_reclen, 
			 lde->d_name);
	
	i = i + lde->d_reclen;
     }
#endif   
   
   return(k);
}

errno_t e_linux_getdents(unsigned int fd, struct linux_dirent* dirp, 
			 unsigned int count, unsigned int* retval)
{
   long offset;
   int nread;
   errno_t err;
   void* tmp_buffer;   
   unsigned int k;
   bnr_off_t start_offset;
   unsigned int last_offset;
   
//   e_emulator_error("e_linux_getdents(fd %d, dirp %x, count %d)",
//		    fd, dirp, count);
   
   tmp_buffer = alloca(count);
   if (tmp_buffer == NULL)
     {
	return(ENOMEM);
     }
   
   err = e_bnr_lseek(fd, 0, SEEK_CUR, &start_offset);
   if (err)
     {
	e_emulator_error("e_bnr_seek failed %d", err);
	return(err);
     }
//   e_emulator_error("start_offset %d", start_offset);
   
   err = e_lite_getdirentries(fd, tmp_buffer, count, &offset, &nread);
   
//   e_emulator_error("nread %d err %d", nread, err);
   
   if (err)
     {
	return(err);
     }
   
   if (nread <= 0)
     {
	*retval = 0;
	return(ESUCCESS);
     }
   
   k = convert_dents(tmp_buffer, (char*)dirp, nread, count, &last_offset);
   
   last_offset = last_offset + start_offset;
//   e_emulator_error("last_offset %d", last_offset);
   err = e_bnr_lseek(fd, last_offset, SEEK_SET, &start_offset);
   if (err)
     {
	e_emulator_error("e_bnr_seek failed 2 err %d", err);
	return(err);
     }
   
//   e_emulator_error("k %d", k);
   
   *retval = k;
  
#if 0   
   do
     {
	unsigned char c;
	unsigned int n;
	
	(void)e_read(1, &c, 1, &n);
     } while(0);
#endif   
   
   return(ESUCCESS);
}

errno_t e_linux_open(const char *path, int flags, int mode, int *fd)
{
   errno_t err;
   
   err = e_open(path, flags, mode, fd);
   if (*fd == readdir_last_fd)
     readdir_seek = 0;
   return err;
}

#include <e_templates.h>
DEF_STAT(linux)
DEF_LSTAT(linux)
DEF_FSTAT(linux)

struct stat64
{
   unsigned long long st_dev;
   unsigned short int  __pad1;
   unsigned long  __st_ino;
   
   unsigned int   st_mode;
   unsigned int   st_nlink;
   unsigned int   st_uid;
   unsigned int   st_gid;
   
   unsigned long long st_rdev;
   unsigned short int  __pad2;
   
   long int      st_size;
   long int st_blksize;
   
   long int  st_blocks; /* Number 512-byte blocks allocated */
   
   long int  linux_stat64_st_atime;
   unsigned long int  __pad5;
   
   long int  linux_stat64_st_mtime;
   unsigned long  __pad6;
   
   long int  linux_stat64_st_ctime;
   unsigned long  __pad7;
   
   unsigned long int unused1;
   unsigned long int usused2;
};

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

errno_t e_linux_fstat64(unsigned long fd, struct stat64* statbuf, long flags,
			errno_t* rerr)
{
   struct linux_stat tstatbuf;
   errno_t err;
   
   err = e_linux_fstat(fd, &tstatbuf);
   if (err)
     {
	*rerr = err;
	return(err);
     }
   
   statbuf->st_dev = tstatbuf.st_dev;
   statbuf->__st_ino = tstatbuf.st_ino;
   statbuf->st_mode = tstatbuf.st_mode;
   statbuf->st_nlink = tstatbuf.st_nlink;
   statbuf->st_uid = tstatbuf.st_uid;
   statbuf->st_gid = tstatbuf.st_gid;
   statbuf->st_rdev = tstatbuf.st_rdev;
   statbuf->st_size = tstatbuf.st_size;
   statbuf->st_blksize = tstatbuf.st_blksize;
   statbuf->st_blocks = tstatbuf.st_blocks;
   statbuf->linux_stat64_st_atime = tstatbuf.linux_st_atime;
   statbuf->linux_stat64_st_mtime = tstatbuf.linux_st_mtime;
   statbuf->linux_stat64_st_ctime = tstatbuf.linux_st_ctime;
//   statbuf->st_ino = tstatbuf.st_ino;
   
   return(ESUCCESS);
}

errno_t e_linux_lstat64(char* filename, struct stat64* statbuf, long flags,
			errno_t* rerr)
{
   struct linux_stat tstatbuf;
   errno_t err;
   
   err = e_linux_lstat(filename, &tstatbuf);
   if (err)
     {
	*rerr = err;
	return(err);
     }
   
   statbuf->st_dev = tstatbuf.st_dev;
   statbuf->__st_ino = tstatbuf.st_ino;
   statbuf->st_mode = tstatbuf.st_mode;
   statbuf->st_nlink = tstatbuf.st_nlink;
   statbuf->st_uid = tstatbuf.st_uid;
   statbuf->st_gid = tstatbuf.st_gid;
   statbuf->st_rdev = tstatbuf.st_rdev;
   statbuf->st_size = tstatbuf.st_size;
   statbuf->st_blksize = tstatbuf.st_blksize;
   statbuf->st_blocks = tstatbuf.st_blocks;
   statbuf->linux_stat64_st_atime = tstatbuf.linux_st_atime;
   statbuf->linux_stat64_st_mtime = tstatbuf.linux_st_mtime;
   statbuf->linux_stat64_st_ctime = tstatbuf.linux_st_ctime;
//   statbuf->st_ino = tstatbuf.st_ino;
   
   return(ESUCCESS);
}

errno_t e_linux_stat64(char* filename, struct stat64* statbuf, long flags,
			errno_t* rerr)
{
   struct linux_stat tstatbuf;
   errno_t err;
   
   err = e_linux_stat(filename, &tstatbuf);
   if (err)
     {
	*rerr = err;
	return(err);
     }
   
   statbuf->st_dev = tstatbuf.st_dev;
   statbuf->__st_ino = tstatbuf.st_ino;
   statbuf->st_mode = tstatbuf.st_mode;
   statbuf->st_nlink = tstatbuf.st_nlink;
   statbuf->st_uid = tstatbuf.st_uid;
   statbuf->st_gid = tstatbuf.st_gid;
   statbuf->st_rdev = tstatbuf.st_rdev;
   statbuf->st_size = tstatbuf.st_size;
   statbuf->st_blksize = tstatbuf.st_blksize;
   statbuf->st_blocks = tstatbuf.st_blocks;
   statbuf->linux_stat64_st_atime = tstatbuf.linux_st_atime;
   statbuf->linux_stat64_st_mtime = tstatbuf.linux_st_mtime;
   statbuf->linux_stat64_st_ctime = tstatbuf.linux_st_ctime;
//   statbuf->st_ino = tstatbuf.st_ino;
   
   return(ESUCCESS);
}

errno_t e_linux_lchown32(const char* filename, unsigned long user, 
			 unsigned long group, errno_t* rerr)
{
   errno_t err;
   
   err = e_chown(filename, user, group);
   *rerr = err;
   
   return(err);
}

errno_t e_linux_fcntl64(unsigned int fd,
			unsigned int cmd,
			unsigned long arg,
			int* ret)
{
   errno_t err;
   
   err = e_fcntl(fd, cmd, arg, ret);
   
   return(err);
}

errno_t e_linux_kill(linux_pid_t pid, int sig)
{
	int bsd_sig = signal_number_linux_to_bsd(sig);

	return e_kill((pid_t) pid, bsd_sig);
}

/* struct rusage is the same on Linux. Signal numbers need conversion. */
errno_t e_linux_wait4(
	linux_pid_t	pid,
	int		*status_out,
	int		options,
	struct rusage	*rusage,
	linux_pid_t	*wpid_out)
{
	errno_t err;
	pid_t wpid;
	int status;
        
	err = e_lite_wait4((pid_t) pid, &status, options, rusage, &wpid);
	if (err)
	    return err;
	if (wpid_out)
	    *wpid_out = wpid;
	if (status_out) {
		/* what a mess wait is! */
		if (WIFSTOPPED(status)) {
			int sig = WSTOPSIG(status);
			sig = signal_number_bsd_to_linux(sig);
			*status_out = W_STOPCODE(sig);
		} else if (WIFSIGNALED(status)) {
			int sig = WTERMSIG(status);
			sig = signal_number_bsd_to_linux(sig);
			*status_out = W_EXITCODE(WEXITSTATUS(status), sig);
		} else {
			/* Normal exit */
			*status_out = status;
		}
	}
	return ESUCCESS;
}

errno_t e_linux_sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	errno_t err;
	int newhow = 0;
	sigset_t nset;
	sigset_t *nset_p = (sigset_t *)NULL;

	/* Convert how from Linux to native */
	switch (how) {
	      case 0:
		newhow = SIG_BLOCK;
		break;
	      case 1:
		newhow = SIG_UNBLOCK;
		break;
	      case 2:
		newhow = SIG_SETMASK;
		break;
	}
	if (set) {
	  	nset = sigmask_linux_to_bsd(*set);
		nset_p = &nset;
	}
	
	err = e_sigprocmask(newhow, nset_p, oset);
	if (!err && oset)
	    *oset = sigmask_bsd_to_linux(*oset);
	return err;
}

boolean_t linux_one_shot[33] = {0 };

errno_t e_linux_one_shot_signal_check_and_eliminate(int sig)
{
	sigset_t mask;
	struct sigaction sa;
	if (linux_one_shot[sig]) {
		linux_one_shot[sig] = FALSE;
		e_sigprocmask(SIG_BLOCK, 0, &mask);
		/* mask |= sigmask(sig); */
		sa.sa_mask = mask;
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		return e_sigaction(sig, &sa, 0);
	}
	return ESUCCESS;
}

/* Convert Linux action to native action and back */
errno_t e_linux_sigaction(
	int			sig,
	struct sigaction	*act,
	struct sigaction	*oact)
{
	struct sigaction sa, osa;
	errno_t err;
	int flags = 0;
	sigset_t mask;

	mask = sigmask_linux_to_bsd(act->sa_mask);
	sig = signal_number_linux_to_bsd(sig);

	if (act->sa_flags & 1)
	    flags |= SA_NOCLDSTOP;
	if (act->sa_flags & 0x10000000)
	    flags |= SA_RESTART;
	if (act->sa_flags & 0x08000000)
	    flags |= SA_ONSTACK;

	if (act->sa_flags & 0x40000000)	/* SA_NOMASK */
	    e_sigprocmask(SIG_BLOCK, 0, &mask);

	if (act->sa_flags & 0x80000000)
	    linux_one_shot[sig] = TRUE;
#if 0
	/* SA_ONESHOT 0x80000000 is still unhandled */

	/* Check for unsupported flags */
	/* 0x20000000 is SA_INTERRUPT which is a no op */
	if (act->sa_flags
	    & ~(1 | 0x10000000 | 0x08000000 | 0x40000000 | 0x20000000))
	{
		return EINVAL;
	}
#endif
	if ((int) act->sa_handler == -1) {
#if 0
		sigset_t set = sigmask(sig);
		sa.sa_handler = 0;
		e_sigprocmask(SIG_IGN, &set, &mask);
#else
		sa.sa_handler = SIG_DFL;
#endif
	} else {
		sa.sa_handler = act->sa_handler;
	}
	sa.sa_flags = flags;
	sa.sa_mask = mask;
	err = e_sigaction(sig, &sa, &osa);
	if (err)
	    return err;
	if (oact) {
		oact->sa_handler = osa.sa_handler;
		oact->sa_mask = sigmask_bsd_to_linux(osa.sa_mask);
		flags = 0;
		if (osa.sa_flags & SA_NOCLDSTOP)
		    flags |= 1;
		if (osa.sa_flags & SA_RESTART)
		    flags |= 0x10000000;
		if (osa.sa_flags & SA_ONSTACK)
		    flags |= 0x08000000;
		oact->sa_flags = flags;
	}
	return ESUCCESS;
}

/* Linux 1.1 termios struct as printed by gdb */
struct linux_termios {
    long unsigned int c_iflag;
    long unsigned int c_oflag;
    long unsigned int c_cflag;
    long unsigned int c_lflag;
    unsigned char c_line;
    unsigned char c_cc[19];
};

void termios_linux_to_bsd(struct linux_termios *lt, struct termios *t);
void termios_bsd_to_linux(struct termios *t, struct linux_termios *lt);

/* Convert ioctl calls.  Add more conversions as needed */
errno_t e_linux_ioctl(int fd, unsigned int cmd, char *argp)
{
	unsigned int newcmd = cmd;
	struct termios tio;
	errno_t err;

	switch(cmd) {
	      case 0x5401:	/* TCGETS */
		err = e_ioctl(fd, TIOCGETA, (char *) &tio);
		if (err == ESUCCESS)
		    termios_bsd_to_linux(&tio, (struct linux_termios *)argp);
		return err;
	      case 0x5402:	/* TCSETS */
		return e_linux_tcsetattr(fd, TCSANOW, argp);
	      case 0x5403:	/* TCSETSW */
		return e_linux_tcsetattr(fd, TCSADRAIN, argp);
	      case 0x5404:	/* TCSETSF */
		return e_linux_tcsetattr(fd, TCSAFLUSH, argp);
	      case 0x540a:	/* TCXONC */
		switch ((int) argp) {
		      case 1:	/* TCOON */
			/* Start output */
			newcmd = TIOCSTART;
			break;
		      default:
			return EOPNOTSUPP;
		}
	      case 0x540f:	/* TIOCGPGRP */
		newcmd = TIOCGPGRP;
		break;
	      case 0x5410:	/* TIOCSPGRP */
		newcmd = TIOCSPGRP;
		break;
	      case 0x5411:	/* TIOCOUTQ */
		newcmd = TIOCOUTQ;
		break;
	      case 0x5413:	/* TIOCGWINSZ */
		/* struct winsize is compatible */
		newcmd = TIOCGWINSZ;
		break;
	      case 0x5414:	/* TIOCSWINSZ */
		newcmd = TIOCSWINSZ;
		break;
	      case 0x541b:	/* FIONREAD */
		newcmd = FIONREAD;
		break;
	      default:
		return EOPNOTSUPP;
	}
	return e_ioctl(fd, newcmd, argp);
}

errno_t e_linux_tcsetattr(
	int			fd,
	int			action,
	struct linux_termios	*ltio)
{
	struct termios tio;

	termios_linux_to_bsd(ltio, &tio);
	/* action values are the same on BSD and Linux */
	switch (action) {
	      case TCSANOW:
		return e_ioctl(fd, TIOCSETA, (char *) &tio);
	      case TCSADRAIN:
		return e_ioctl(fd, TIOCSETAW, (char *) &tio);
	      case TCSAFLUSH:
		return e_ioctl(fd, TIOCSETAF, (char *) &tio);
	      default:
		return EINVAL;
	}
}

/* 
 * The termios bits are defined here since they are needed twice
 * The bits are logically almost exactly the same as in BSD but the
 * values differ.
 */

/* iflag */
#define LINUX_IGNBRK	1
#define LINUX_BRKINT	2
#define LINUX_IGNPAR	4
#define LINUX_PARMRK	8
#define LINUX_INPCK	0x10
#define LINUX_ISTRIP	0x20
#define LINUX_INLCR	0x40
#define LINUX_IGNCR	0x80
#define LINUX_ICRNL	0x100
#define LINUX_IUCLC	0x200
#define LINUX_IXON	0x400
#define LINUX_IXANY	0x800
#define LINUX_IXOFF	0x1000
#define LINUX_IMAXBEL	0x2000

/* oflag */
#define LINUX_OPOST	1
#define LINUX_ONLCR	4
#define LINUX_XTABS	0x1800

/* cflag */
#define LINUX_CBAUD	0xf
#define LINUX_B0	0
#define LINUX_B50	1
#define LINUX_B75	2
#define LINUX_B110	3
#define LINUX_B134	4
#define LINUX_B150	5
#define LINUX_B200	6
#define LINUX_B300	7
#define LINUX_B600	8
#define LINUX_B1200	9
#define LINUX_B1800	10
#define LINUX_B2400	11
#define LINUX_B4800	12
#define LINUX_B9600	13
#define LINUX_B19200	14
#define LINUX_B38400	15
#define LINUX_CSIZE	0x30
#define LINUX_CS5	0
#define LINUX_CS6	0x10
#define LINUX_CS7	0x20
#define LINUX_CS8	0x30
#define LINUX_CSTOPB	0x40
#define LINUX_CREAD	0x80
#define LINUX_PARENB	0x100
#define LINUX_PARODD	0x200
#define LINUX_HUPCL	0x400
#define LINUX_CLOCAL	0x800

/* lflag */
#define LINUX_ISIG	1
#define LINUX_ICANON	2
#define LINUX_XCASE	4
#define LINUX_ECHO	8
#define LINUX_ECHOE	0x10
#define LINUX_ECHOK	0x20
#define LINUX_ECHONL	0x40
#define LINUX_NOFLSH	0x80
#define LINUX_TOSTOP	0x100
#define LINUX_ECHOCTL	0x200
#define LINUX_ECHOPRT	0x400
#define LINUX_ECHOKE	0x800
#define LINUX_FLUSHO	0x1000
#define LINUX_PENDIN	0x4000
#define LINUX_IEXTEN	0x8000

/* cc */
#define LINUX_VINTR 0
#define LINUX_VQUIT 1
#define LINUX_VERASE 2
#define LINUX_VKILL 3
#define LINUX_VEOF 4
#define LINUX_VTIME 5
#define LINUX_VMIN 6
#define LINUX_VSWTC 7
#define LINUX_VSTART 8
#define LINUX_VSTOP 9
#define LINUX_VSUSP 10
#define LINUX_VEOL 11
#define LINUX_VREPRINT 12
#define LINUX_VDISCARD 13
#define LINUX_VWERASE 14
#define LINUX_VLNEXT 15
#define LINUX_VEOL2 16

/* XXX igncr (iflags) cs5 (cflags) went wrong */
void termios_linux_to_bsd(struct linux_termios *ot, struct termios *nt)
{
	unsigned bit, x;

	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_iflag & (1 << bit)) {
		  case LINUX_IGNBRK:
		    x |= IGNBRK;
		    break;
		  case LINUX_BRKINT:
		    x |= BRKINT;
		    break;
		  case LINUX_IGNPAR:
		    x |= IGNPAR;
		    break;
		  case LINUX_PARMRK:
		    x |= PARMRK;
		    break;
		  case LINUX_INPCK:
		    x |= INPCK;
		    break;
		  case LINUX_ISTRIP:
		    x |= ISTRIP;
		    break;
		  case LINUX_INLCR:
		    x |= INLCR;
		    break;
		  case LINUX_IGNCR:
		    x |= IGNCR;
		    break;
		  case LINUX_ICRNL:
		    x |= ICRNL;
		    break;
		  case LINUX_IUCLC:
		    /* x |= IUCLC; */
		    break;
		  case LINUX_IXON:
		    x |= IXON;
		    break;
		  case LINUX_IXANY:
		    x |= IXANY;
		    break;
		  case LINUX_IXOFF:
		    x |= IXOFF;
		    break;
		  case LINUX_IMAXBEL:
		    x |= IMAXBEL;
		    break;
	    }
	nt->c_iflag = x;

	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_oflag & (1 << bit)) {
		  case LINUX_OPOST:
		    x |= OPOST;
		    break;
		  case LINUX_ONLCR:
		    x |= ONLCR;
		    break;
		  case   LINUX_XTABS:
		    x |= OXTABS;
		    break;
	    }
	nt->c_oflag = x;

	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_cflag & (1 << bit)) {
		  case LINUX_CSTOPB:
		    x |= CSTOPB;
		    break;
		  case LINUX_CREAD:
		    x |= CREAD;
		    break;
		  case LINUX_PARENB:
		    x |= PARENB;
		    break;
		  case LINUX_PARODD:
		    x |= PARODD;
		    break;
		  case LINUX_HUPCL:
		    x |= HUPCL;
		    break;
		  case LINUX_CLOCAL:
		    x |= CLOCAL;
		    break;
	    }
	switch (ot->c_cflag & LINUX_CSIZE) {
	      case LINUX_CS5:
		x |= CS5;
		break;
	      case LINUX_CS6:
		x |= CS6;
		break;
	      case LINUX_CS7:
		x |= CS7;
		break;
	      case LINUX_CS8:
		x |= CS8;
		break;
	}
	nt->c_cflag = x;

	x = 0;
	switch (ot->c_cflag & LINUX_CBAUD) {
	      case LINUX_B0:	/* means hangup */
		x |= B0;
		break;
	      case LINUX_B50:
		x |= B50;
		break;
	      case LINUX_B75:
		x |= B75;
		break;
	      case LINUX_B110:
		x |= B110;
		break;
	      case LINUX_B134:
		x |= B134;
		break;
	      case LINUX_B150:
		x |= B150;
		break;
	      case LINUX_B200:
		x |= B200;
		break;
	      case LINUX_B300:
		x |= B300;
		break;
	      case LINUX_B600:
		x |= B600;
		break;
	      case LINUX_B1200:
		x |= B1200;
		break;
	      case LINUX_B1800:
		x |= B1800;
		break;
	      case LINUX_B2400:
		x |= B2400;
		break;
	      case LINUX_B4800:
		x |= B4800;
		break;
	      case LINUX_B9600:
		x |= B9600;
		break;
	      case LINUX_B19200:
		x |= B19200;
		break;
	      case LINUX_B38400:
		x |= B38400;
		break;
	}
	nt->c_ospeed = x;
	nt->c_ispeed = x;

	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_lflag & (1 << bit)) {
		  case LINUX_ISIG:
		    x |= ISIG;
		    break;
		  case LINUX_ICANON:
		    x |= ICANON;
		    break;
		  case LINUX_ECHO:
		    x |= ECHO;
		    break;
		  case LINUX_ECHOE:
		    x |= ECHOE;
		    break;
		  case LINUX_ECHOK:
		    x |= ECHOK;
		    break;
		  case LINUX_ECHONL:
		    x |= ECHONL;
		    break;
		  case LINUX_NOFLSH:
		    x |= NOFLSH;
		    break;
		  case LINUX_TOSTOP:
		    x |= TOSTOP;
		    break;
		  case LINUX_ECHOCTL:
		    x |= ECHOCTL;
		    break;
		  case LINUX_ECHOPRT:
		    x |= ECHOPRT;
		    break;
		  case LINUX_ECHOKE:
		    x |= ECHOKE;
		    break;
		  case LINUX_FLUSHO:
		    x |= FLUSHO;
		    break;
		  case LINUX_PENDIN:
		    x |= PENDIN;
		    break;
		  case LINUX_IEXTEN:
		    x |= IEXTEN;
		    break;
	    }
	nt->c_lflag = x;

	nt->c_cc[VEOF] = ot->c_cc[LINUX_VEOF];
	nt->c_cc[VEOL] = ot->c_cc[LINUX_VEOL];
	nt->c_cc[VEOL2] = ot->c_cc[LINUX_VEOL2];
	nt->c_cc[VERASE] = ot->c_cc[LINUX_VERASE];
	nt->c_cc[VWERASE] = ot->c_cc[LINUX_VWERASE];
	nt->c_cc[VKILL] = ot->c_cc[LINUX_VKILL];
	nt->c_cc[VREPRINT] = ot->c_cc[LINUX_VREPRINT];
	nt->c_cc[VINTR] = ot->c_cc[LINUX_VINTR];
	nt->c_cc[VQUIT] = ot->c_cc[LINUX_VQUIT];
	nt->c_cc[VSUSP] = ot->c_cc[LINUX_VSUSP];
	nt->c_cc[VSTART] = ot->c_cc[LINUX_VSTART];
	nt->c_cc[VSTOP] = ot->c_cc[LINUX_VSTOP];
	nt->c_cc[VLNEXT] = ot->c_cc[LINUX_VLNEXT];
	nt->c_cc[VDISCARD] = ot->c_cc[LINUX_VDISCARD];
	nt->c_cc[VMIN] = ot->c_cc[LINUX_VMIN];
	nt->c_cc[VTIME] = ot->c_cc[LINUX_VTIME];
	nt->c_cc[VSTATUS] = ot->c_cc[LINUX_VSWTC]; /* XXX? switch VConsole? */
}

void termios_bsd_to_linux(struct termios *ot, struct linux_termios *nt)
{
	unsigned bit, x;
	
	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_iflag & (1 << bit)) {
		  case IGNBRK:
		    x |= LINUX_IGNBRK;
		    break;
		  case BRKINT:
		    x |= LINUX_BRKINT;
		    break;
		  case IGNPAR:
		    x |= LINUX_IGNPAR;
		    break;
		  case PARMRK:
		    x |= LINUX_PARMRK;
		    break;
		  case INPCK:
		    x |= LINUX_INPCK;
		    break;
		  case ISTRIP:
		    x |= LINUX_ISTRIP;
		    break;
		  case INLCR:
		    x |= LINUX_INLCR;
		    break;
		  case IGNCR:
		    x |= LINUX_IGNCR;
		    break;
		  case ICRNL:
		    x |= LINUX_ICRNL;
		    break;
		  case IXON:
		    x |= LINUX_IXON;
		    break;
		  case IXANY:
		    x |= LINUX_IXANY;
		    break;
		  case IXOFF:
		    x |= LINUX_IXOFF;
		    break;
		  case IMAXBEL:
		    x |= LINUX_IMAXBEL;
		    break;
		    /* x |= LINUX_IUCLC; */
	    }
	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_oflag & (1 << bit)) {
		  case OPOST:
		    x |= LINUX_OPOST;
		    break;
		  case ONLCR:
		    x |= LINUX_ONLCR;
		    break;
		  case   OXTABS:
		    x |= LINUX_XTABS;
		    break;
	    }
	nt->c_oflag = x;

	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_cflag & (1 << bit)) {
		  case CSTOPB:
		    x |= LINUX_CSTOPB;
		    break;
		  case CREAD:
		    x |= LINUX_CREAD;
		    break;
		  case PARENB:
		    x |= LINUX_PARENB;
		    break;
		  case PARODD:
		    x |= LINUX_PARODD;
		    break;
		  case HUPCL:
		    x |= LINUX_HUPCL;
		    break;
		  case CLOCAL:
		    x |= LINUX_CLOCAL;
		    break;
	    }
	switch (ot->c_cflag & CSIZE) {
	      case CS5:
		x |= LINUX_CS5;
		break;
	      case CS6:
		x |= LINUX_CS6;
		break;
	      case CS7:
		x |= LINUX_CS7;
		break;
	      case CS8:
		x |= LINUX_CS8;
		break;
	}
	switch (ot->c_ospeed) {
	      case B0:
		x |= LINUX_B0;
		break;
	      case B50:
		x |= LINUX_B50;
		break;
	      case B75:
		x |= LINUX_B75;
		break;
	      case B110:
		x |= LINUX_B110;
		break;
	      case B134:
		x |= LINUX_B134;
		break;
	      case B150:
		x |= LINUX_B150;
		break;
	      case B200:
		x |= LINUX_B200;
		break;
	      case B300:
		x |= LINUX_B300;
		break;
	      case B600:
		x |= LINUX_B600;
		break;
	      case B1200:
		x |= LINUX_B1200;
		break;
	      case B1800:
		x |= LINUX_B1800;
		break;
	      case B2400:
		x |= LINUX_B2400;
		break;
	      case B4800:
		x |= LINUX_B4800;
		break;
	      case B9600:
		x |= LINUX_B9600;
		break;
	      case B19200:
		x |= LINUX_B19200;
		break;
	      case B38400:
		x |= LINUX_B38400;
		break;
	}
	nt->c_cflag = x;

	x = 0;
	for (bit = 0; bit < 32; bit++)
	    switch (ot->c_lflag & (1 << bit)) {
		  case ISIG:
		    x |= LINUX_ISIG;
		    break;
		  case ICANON:
		    x |= LINUX_ICANON;
		    break;
		  case ECHO:
		    x |= LINUX_ECHO;
		    break;
		  case ECHOE:
		    x |= LINUX_ECHOE;
		    break;
		  case ECHOK:
		    x |= LINUX_ECHOK;
		    break;
		  case ECHONL:
		    x |= LINUX_ECHONL;
		    break;
		  case NOFLSH:
		    x |= LINUX_NOFLSH;
		    break;
		  case TOSTOP:
		    x |= LINUX_TOSTOP;
		    break;
		  case ECHOCTL:
		    x |= LINUX_ECHOCTL;
		    break;
		  case ECHOPRT:
		    x |= LINUX_ECHOPRT;
		    break;
		  case ECHOKE:
		    x |= LINUX_ECHOKE;
		    break;
		  case FLUSHO:
		    x |= LINUX_FLUSHO;
		    break;
		  case PENDIN:
		    x |= LINUX_PENDIN;
		    break;
		  case IEXTEN:
		    x |= LINUX_IEXTEN;
		    break;
	    }
	nt->c_lflag = x;

	nt->c_cc[LINUX_VEOF] = ot->c_cc[VEOF];
	nt->c_cc[LINUX_VEOL] = ot->c_cc[VEOL];
	nt->c_cc[LINUX_VEOL2] = ot->c_cc[VEOL2];
	nt->c_cc[LINUX_VERASE] = ot->c_cc[VERASE];
	nt->c_cc[LINUX_VWERASE] = ot->c_cc[VWERASE];
	nt->c_cc[LINUX_VKILL] = ot->c_cc[VKILL];
	nt->c_cc[LINUX_VREPRINT] = ot->c_cc[VREPRINT];
	nt->c_cc[LINUX_VINTR] = ot->c_cc[VINTR];
	nt->c_cc[LINUX_VQUIT] = ot->c_cc[VQUIT];
	nt->c_cc[LINUX_VSUSP] = ot->c_cc[VSUSP];
	nt->c_cc[LINUX_VSTART] = ot->c_cc[VSTART];
	nt->c_cc[LINUX_VSTOP] = ot->c_cc[VSTOP];
	nt->c_cc[LINUX_VLNEXT] = ot->c_cc[VLNEXT];
	nt->c_cc[LINUX_VDISCARD] = ot->c_cc[VDISCARD];
	nt->c_cc[LINUX_VMIN] = ot->c_cc[VMIN];
	nt->c_cc[LINUX_VTIME] = ot->c_cc[VTIME];
	nt->c_cc[LINUX_VSWTC] = ot->c_cc[VSTATUS]; /* XXX? */

	nt->c_line = 0;		/* N_TTY */
}

/* Yet another random interface. sic. */
errno_t e_linux_socketcall(int flavor, unsigned int *args, int *rv)
{
	switch (flavor) {
	      case 1:		/* socket */
		return e_socket(args[0], args[1], args[2], rv);
	      case 2:		/* bind */
		return e_bind(args[0], (struct sockaddr *) args[1], args[2]);
	      case 3:		/* connect */
		return e_connect(args[0], (struct sockaddr *) args[1],
				 args[2]);
	      case 4:		/* listen */
		return e_listen(args[0], args[1]);
	      case 5:		/* accept */
		return e_bnr_accept(args[0], (struct sockaddr *) args[1],
				    (int *) args[2], rv);
	      case 6:		/* getsockname */
		return e_bnr_getsockname(args[0], (struct sockaddr *) args[1],
					 (int *) args[2]);
	      case 7:		/* getpeername */
		return e_bnr_getpeername(args[0], (struct sockaddr *) args[1],
					 (int *) args[2]);
	      case 8:		/* socketpair */
		return e_socketpair(args[0], args[1], args[2],
				    (int *) args[3]);
	      case 9:		/* send */
		return e_send(args[0], (char *) args[1], args[2],
			      args[3], rv);
	      case 10:		/* recv */
		return e_recv(args[0], (void *) args[1], args[2], args[3], rv);
	      case 11:		/* sendto */
		return e_bnr_sendto(args[0], (const void *) args[1],
				    args[2], args[3],
				    (const struct sockaddr *) args[4],
				    args[5], rv);
	      case 12:		/* recvfrom */
		return e_bnr_recvfrom(args[0], (void *) args[1], args[2],
				      args[3], (struct sockaddr *) args[4],
				      (int *) args[5], rv);
	      case 13:		/* shutdown */
		return e_shutdown(args[0], args[1]);
	      case 14:		/* setsockopt */
		return e_setsockopt(args[0], args[1], args[2],
				    (const void *) args[3], args[4]);
	      case 15:		/* getsockopt */
		return e_getsockopt(args[0], args[1], args[2],
				    (void *) args[3], rv);
	      default:
		return EINVAL;
	}
}

/* There are 32 signals only. slot zero is unused */
int signal_number_linux_to_bsd_table[33] = {
	0,
	SIGHUP,			/* 1 HUP */
	SIGINT,			/* 2 INT */
	SIGQUIT,		/* 3 QUIT  */
	SIGILL,			/* 4 ILL */
	SIGTRAP,		/* 5 TRAP */
	SIGABRT,		/* 6 ABRT,IOT */
	SIGEMT,			/* 7 UNUSED */
	SIGFPE,			/* 8 FPE */
	SIGKILL,		/* 9 KILL */
	SIGUSR1,		/* 10 USR1 */
	SIGSEGV,		/* 11 SEGV */
	SIGUSR2,		/* 12 USR2 */
	SIGPIPE,		/* 13 PIPE */
	SIGALRM,		/* 14 ALRM */
	SIGTERM,		/* 15 TERM */
	SIGSEGV,		/* 16 STKFLT */
	SIGCHLD,		/* 17 CHLD */
	SIGCONT,		/* 18 CONT */
	SIGSTOP,		/* 19 STOP */
	SIGTSTP,		/* 20 TSTP */
	SIGTTIN,		/* 21 TTIN */
	SIGTTOU,		/* 22 TTOU */
	SIGIO,			/* 23 IO */
	SIGXCPU,		/* 24 XCPU */
	SIGXFSZ,		/* 25 XFSZ */
	SIGVTALRM,		/* 26 VTALRM */
	SIGPROF,		/* 27 PROF */
	SIGWINCH,		/* 28 WINCH */
	SIGEMT,			/* 29 NONE */
	SIGEMT,			/* 30 PWR */
	SIGEMT,			/* 31 NONE */
	SIGEMT			/* 32 NONE */
    };

int signal_number_bsd_to_linux_table[33] = {
	0,
	1,			/* HUP	1 */
	2,			/* INT	2 */
	3,			/* QUIT	3 */
	4,			/* ILL	4 */
	5,			/* TRAP	5 */
	6,			/* ABRT	6 */
	7,			/* EMT	7 */
	8,			/* FPE	8 */
	9,			/* KILL	9 */
	11,			/* BUS	10 */
	11,			/* SEGV	11 */
	7,			/* SYS	12 */
	13,			/* PIPE	13 */
	14,			/* ALRM	14 */
	15,			/* TERM	15 */
	23,			/* URG	16 */
	19,			/* STOP	17 */
	20,			/* TSTP	18 */
	18,			/* CONT	19 */
	17,			/* CHLD	20 */
	21,			/* TTIN	21 */
	22,			/* TTOU	22 */
	23,			/* IO	23 */
	24,			/* XCPU	24 */
	25,			/* XFSZ	25 */
	26,			/* VTALRM 26 */
	27,			/* PROF	27 */
	28,			/* WINCH 28 */
	7,			/* INFO	29 */
	10,			/* USR1 30 */
	12			/* USR2 31 */
    };

inline int signal_number_bsd_to_linux(int old_sig)
{
	return signal_number_bsd_to_linux_table[old_sig];
}

inline int signal_number_linux_to_bsd(int old_sig)
{
	return signal_number_linux_to_bsd_table[old_sig];
}

static sigset_t sigmask_linux_to_bsd(sigset_t omask)
{
	sigset_t m = 0;
	int bit;

	for (bit = 0; bit < 32; bit++) {
		if (omask & (1 << bit))
		    m |= sigmask(signal_number_linux_to_bsd(bit + 1));
	}
	return m;
}

static sigset_t sigmask_bsd_to_linux(sigset_t omask)
{
	sigset_t m = 0;
	int bit;

	for (bit = 0; bit < 32; bit++) {
		if (omask & (1 << bit))
		    m |= sigmask(signal_number_bsd_to_linux(bit + 1));
	}
	return m;
}

/* SysV ipc interface to Linux */
struct linux_ipc_kludge {
    struct msgbuf *msgp;
    long msgtyp;
};

/* e_linux_sysvipc */
#define LINUX_SEMOP           1
#define LINUX_SEMGET          2
#define LINUX_SEMCTL          3
#define LINUX_MSGSND          11
#define LINUX_MSGRCV          12
#define LINUX_MSGGET          13
#define LINUX_MSGCTL          14
#define LINUX_SHMAT           21
#define LINUX_SHMDT           22
#define LINUX_SHMGET          23
#define LINUX_SHMCTL          24

/* shmget etc */
#define LINUX_IPC_CREAT  00001000
#define LINUX_IPC_EXCL   00002000
#define LINUX_IPC_NOWAIT 00004000

/* shmat */
#define LINUX_SHM_RDONLY      010000

/* shmctl */
#define LINUX_SHM_STAT        13
#define LINUX_SHM_INFO        14

errno_t e_linux_shmget(int key, int size, int shmflg, int *id)
{
	int open_flags = 0;

	if (shmflg & LINUX_IPC_CREAT)
	    open_flags |= O_CREAT;
	if (shmflg & LINUX_IPC_EXCL)
	    open_flags |= O_EXCL;
	if (shmflg & LINUX_IPC_NOWAIT)
	    open_flags |= O_NDELAY;
	open_flags |= O_RDWR;

	return e_shmget(key, size, open_flags, id);
}

errno_t e_linux_shmat(int shmid, char *shmaddr, int shmflg, caddr_t *addr_out)
{
	vm_prot_t prot = VM_PROT_READ|VM_PROT_EXECUTE;

	if ((shmflg & LINUX_SHM_RDONLY) == 0)
	    prot |= VM_PROT_WRITE;
	return e_shmat(shmid, (vm_offset_t) shmaddr, prot, addr_out);
}

struct linux_shmid_ds;

DEF_FSTAT(linux_shmid)

errno_t e_linux_shmctl(
	int	shmid,
	int	cmd,
	struct	linux_shmid_ds *buf)
{
	int fd;
	errno_t err;

	err = e_shmid_to_fd(shmid, &fd);
	if (err)
	    return err;

	switch (cmd) {
	      case LINUX_SHM_STAT:
		return e_linux_shmid_fstat(fd, (struct linux_shmid_stat *)buf);
	      case LINUX_SHM_INFO:
		return e_mach_error_to_errno(LITES_EOPNOTSUPP);
	      default:
		return e_mach_error_to_errno(LITES_EINVAL);
	}
}

errno_t e_linux_sysvipc(int flavor, int a, int b, int c, void *d, int *retval)
{
	switch(flavor) {
	      case LINUX_SEMOP:
		return e_semop(a, d, b);
	      case LINUX_SEMGET:
		return e_semget(a, b, c, retval);
	      case LINUX_SEMCTL:
		return e_linux_shmat(a, d, b, (caddr_t *) retval);
	      case LINUX_MSGSND:
	      case LINUX_MSGRCV:
	      case LINUX_MSGGET:
	      case LINUX_MSGCTL:
		return ENOSYS;
	      case LINUX_SHMAT:
		return e_shmat(a, d, b, retval);
	      case LINUX_SHMDT:
		return e_shmdt(d);
	      case LINUX_SHMGET:
		return e_linux_shmget(a, b, c, retval);
	      case LINUX_SHMCTL:
		return e_linux_shmctl(a, b, d);
	      default:
		return e_mach_error_to_errno(LITES_ENOSYS);
	}
}


