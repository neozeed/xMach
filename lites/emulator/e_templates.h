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
 * $Log: e_templates.h,v $
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:15:35  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	emulator/e_templates.h
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *	Date:	May 1994
 *
 *	Templates for stat calls.
 */

#define DEF_STAT(KIND) \
struct KIND ## _stat;		/* fwd */ \
errno_t e_ ## KIND ## _stat(const char *path, struct KIND ## _stat *buf) \
{ \
	kern_return_t kr; \
	struct vattr	va; \
\
	kr = bsd_path_getattr(process_self(), TRUE,	\
			      path, strlen(path)+1, &va); \
	if (kr == KERN_SUCCESS) \
	    e_vattr_to_ ## KIND ## _stat(&va, buf); \
	return e_mach_error_to_errno(kr); \
}

#define DEF_LSTAT(KIND) \
struct KIND ## _stat;		/* fwd */ \
errno_t e_ ## KIND ## _lstat(const char *path, struct KIND ## _stat *buf) \
{ \
	kern_return_t kr; \
	struct vattr	va; \
\
	kr = bsd_path_getattr(process_self(), FALSE,	\
			      path, strlen(path)+1, &va); \
	if (kr == KERN_SUCCESS) \
	    e_vattr_to_ ## KIND ## _stat(&va, buf); \
	return e_mach_error_to_errno(kr); \
}

#define DEF_FSTAT(KIND) \
struct KIND ## _stat;		/* fwd */ \
errno_t e_ ## KIND ## _fstat(int fd, struct KIND ## _stat *buf) \
{ \
	kern_return_t kr; \
	struct vattr va; \
\
	kr = bsd_getattr(process_self(), fd, &va); \
	if (kr == KERN_SUCCESS) \
	    e_vattr_to_ ## KIND ## _stat(&va, buf); \
\
	return kr ? e_mach_error_to_errno(kr) : 0; \
}
