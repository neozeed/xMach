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
 * $Log: error_codes.c,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.4  1995/09/02  05:38:31  mike
 * XXX parisc ifdefs to avoid compilation problems on i386
 *
 * Revision 1.3  1995/09/01  23:27:54  mike
 * HP-UX compatibility support from Jeff F.
 *
 * Revision 1.2  1995/03/23  01:38:15  law
 * Update to 950323 snapshot + utah changes
 *
 * Revision 1.1.1.2  1995/03/22  23:24:20  law
 * Pure lites-950316 snapshot.
 *
 */
/* 
 *	File:	emulator/error_codes.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	errno conversions.
 */

#include <e_defs.h>
#include <device/device_types.h>

char *mach_error_string(mach_error_t);

noreturn e_bad_mach_error(mach_error_t kr)
{
	char *str = mach_error_string(kr);

	e_emulator_error("libemul: Can't handle error code x%x: \"%s\". %s.\n",
			 kr, str ? str : "(null)", "Terminating");
	(noreturn) task_terminate(mach_task_self());
	/*NOTREACHED*/
}

mach_error_t e_kernel_error_to_lites_error(mach_error_t kr)
{
	switch (kr) {
	      case KERN_INVALID_ADDRESS:
	      case KERN_PROTECTION_FAILURE:
		return LITES_EFAULT;
	      case KERN_NO_SPACE:
		goto bad;
	      case KERN_INVALID_ARGUMENT:
		return LITES_EINVAL;
	      case KERN_FAILURE:
		goto bad;
	      case KERN_RESOURCE_SHORTAGE:
		return LITES_EAGAIN;
	      case KERN_NOT_RECEIVER:
	      case KERN_NO_ACCESS:
	      case KERN_MEMORY_FAILURE:
	      case KERN_MEMORY_ERROR:
	      case KERN_NOT_IN_SET:
	      case KERN_NAME_EXISTS:
	      case KERN_ABORTED:
		goto bad;
	      case KERN_INVALID_NAME:
	      case KERN_INVALID_TASK:
	      case KERN_INVALID_RIGHT:
	      case KERN_INVALID_VALUE:
		return LITES_EINVAL;
	      case KERN_UREFS_OVERFLOW:
	      case KERN_INVALID_CAPABILITY:
	      case KERN_RIGHT_EXISTS:
	      case KERN_INVALID_HOST:
	      case KERN_MEMORY_PRESENT:
		goto bad;

	      case D_IO_ERROR:
		return LITES_EIO;
	      case D_WOULD_BLOCK:
		return LITES_EWOULDBLOCK;
	      case D_NO_SUCH_DEVICE:
		return LITES_ENXIO;
	      case D_ALREADY_OPEN:
		return LITES_EBUSY;
	      case D_DEVICE_DOWN:
		return LITES_ENETDOWN;
	      case D_INVALID_OPERATION:
		return LITES_ENOTTY;
	      case D_INVALID_RECNUM:
	      case D_INVALID_SIZE:
		return LITES_EINVAL;
	      case D_NO_MEMORY:
	      case D_READ_ONLY:
		return LITES_EIO;
#ifdef D_NO_SPACE
	      case D_NO_SPACE:
		return (LITES_ENOSPC);
#endif
	      case MIG_TYPE_ERROR:
	      case MIG_REPLY_MISMATCH:
	      case MIG_REMOTE_ERROR:
	      case MIG_BAD_ID:
	      case MIG_BAD_ARGUMENTS:
	      case MIG_ARRAY_TOO_LARGE:
		return LITES_EINVAL;

	      case MIG_NO_REPLY:
	      case MIG_EXCEPTION:
	      case MIG_SERVER_DIED:
#ifdef MIG_DESTROY_REQUEST
	      case MIG_DESTROY_REQUEST:
#endif
	      default:
		return kr;
	}
      bad:
	return kr;
}

errno_t e_mach_error_to_errno(mach_error_t kr)
{
	if (kr == KERN_SUCCESS)
	    return 0;

	if (kr == ERESTART || kr == EJUSTRETURN)
	    return kr;

	/* First collapse all errors into Lites errors */
	kr = e_kernel_error_to_lites_error(kr);

	switch (e_my_binary_type) {
	      case BT_LINUX:
	      case BT_LINUX_SHLIB:
	      case BT_LINUX_O:
	      case BT_LINUX_ELF:
		return e_mach_error_to_linux_errno(kr);
#ifdef parisc
	      case BT_HPUX:
		return errno_bsdtohpux(kr);
#endif
	      default:
		/* use the code below */
	}
	/* Then we are BSD XXX */
	/* Check for encapsulated errno */
	if ((kr & (system_emask|sub_emask)) == unix_err(0))
	    return err_get_code(kr);

	/* All else failed. Get zapped. */
	e_bad_mach_error(kr);
	/*NOTREACHED*/
	return kr;		/* avoid bogus gcc 2.5.8 warning */
}

errno_t errno_bsd_to_linux_table[___ELAST+1] = {
	0,
	1,			/* 1 EPERM */
	2,			/* 2 ENOENT */
	3,			/* 3 ESRCH */
	4,			/* 4 EINTR */
	5,			/* 5 EIO */
	6,			/* 6 ENXIO */
	7,			/* 7 E2BIG */
	8,			/* 8 ENOEXEC */
	9,			/* 9 EBADF */
	10,			/* 10 ECHILD */
	35,			/* 11 EDEADLK */
	12,			/* 12 ENOMEM */
	13,			/* 13 EACCES */
	14,			/* 14 EFAULT */
	15,			/* 15 ENOTBLK */
	16,			/* 16 EBUSY */
	17,			/* 17 EEXIST */
	18,			/* 18 EXDEV */
	19,			/* 19 ENODEV */
	20,			/* 20 ENOTDIR */
	21,			/* 21 EISDIR */
	22,			/* 22 EINVAL */
	23,			/* 23 ENFILE */
	24,			/* 24 EMFILE */
	25,			/* 25 ENOTTY */
	26,			/* 26 ETXTBSY */
	27,			/* 27 EFBIG */
	28,			/* 28 ENOSPC */
	29,			/* 29 ESPIPE */
	30,			/* 30 EROFS */
	31,			/* 31 EMLINK */
	32,			/* 32 EPIPE */
	33,			/* 33 EDOM */
	34,			/* 34 ERANGE */
	11,			/* 35 EAGAIN */
	115,			/* 36 EINPROGRESS */
	114,			/* 37 EALREADY */
	88,			/* 38 ENOTSOCK */
	89,			/* 39 EDESTADDRREQ */
	90,			/* 40 EMSGSIZE */
	91,			/* 41 EPROTOTYPE */
	92,			/* 24 ENOPROTOOPT */
	93,			/* 43 EPROTONOSUPPORT */
	94,			/* 44 ESOCKTNOSUPPORT */
	95,			/* 45 EOPNOTSUPP */
	96,			/* 46 EPFNOSUPPORT */
	97,			/* 47 EAFNOSUPPORT */
	98,			/* 48 EADDRINUSE */
	99,			/* 49 EADDRNOTAVAIL */
	100,			/* 50 ENETDOWN */
	101,			/* 51 ENETUNREACH */
	102,			/* 52 ENETRESET */
	103,			/* 53 ECONNABORTED */
	104,			/* 54 ECONNRESET */
	105,			/* 55 ENOBUFS */
	106,			/* 56 EISCONN */
	107,			/* 57 ENOTCONN */
	108,			/* 58 ESHUTDOWN */
	109,			/* 59 ETOOMANYREFS */
	110,			/* 60 ETIMEDOUT */
	111,			/* 61 ECONNREFUSED */
	40,			/* 62 ELOOP */
	36,			/* 63 ENAMETOOLONG */
	112,			/* 64 EHOSTDOWN */
	113,			/* 65 EHOSTUNREACH */
	39,			/* 66 ENOTEMPTY */
	87,			/* 67 EPROCLIM XXX --> EUSERS */
	87,			/* 68 EUSERS */
	122,			/* 69 EDQUOT */
	116,			/* 70 ESTALE */
	66,			/* 71 EREMOTE */
	71,			/* 72 EBADRPC  XXX --> EPROTO */
	71,			/* 73 ERPCMISMATCH  XXX --> EPROTO */
	95,			/* 74 EPROGUNAVAIL  XXX --> EOPNOTSUPP */
	95,			/* 75 EPROGMISMATCH  XXX --> EOPNOTSUPP */
	95,			/* 76 EPROCUNAVAIL  XXX --> EOPNOTSUPP */
	37,			/* 77 ENOLCK */
	38,			/* 78 ENOSYS */
	20,		/* 79 EFTYPE XXX --> ENOTDIR Chmod On Sticky Non-Dir */
	13,			/* 80 EAUTH  XXX --> EACCES */
	13			/* 81 ENEEDAUTH  XXX --> EACCES */
};

errno_t e_mach_error_to_linux_errno(mach_error_t kr)
{
	errno_t err;

	if ((kr & (system_emask|sub_emask)) == unix_err(0)
	    && kr <= LITES_ELAST)
	{
		return errno_bsd_to_linux_table[err_get_code(kr)];
	}
	e_bad_mach_error(kr);
	/*NOTREACHED*/
	return kr;		/* avoid bogus gcc 2.5.8 warnings */
}
