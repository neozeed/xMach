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
 * $Log: e_uname.c,v $
 * Revision 1.3  2000/11/07 00:41:25  welchd
 *
 * Added support for executing dynamically linked Linux ELF binaries
 *
 * Revision 1.2  2000/10/27 01:55:27  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:28  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	emulator/e_uname.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1994.
 *
 *	Uname in a few different flavors.
 */
/*-
 * Copyright (c) 1994
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)uname.c	8.1 (Berkeley) 1/4/94";
#endif /* LIBC_SCCS and not lint */

#include <e_defs.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

struct lite_utsname {
	char	sysname[256];	/* Name of this OS. */
	char	nodename[256];	/* Name of this network node. */
	char	release[256];	/* Release level. */
	char	version[256];	/* Version level. */
	char	machine[256];	/* Hardware type. */
};

struct bnr_utsname {
	char	sysname[32];	/* Name of this OS. */
	char	nodename[32];	/* Name of this network node. */
	char	release[32];	/* Release level. */
	char	version[32];	/* Version level. */
	char	machine[32];	/* Hardware type. */
};

struct linux_utsname {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
        char domainname[65];
};

errno_t e_lite_uname(struct lite_utsname *name)
{
	int mib[2];
	size_t len;
	char *p;
	errno_t err, rval;

	rval = 0;

	mib[0] = CTL_KERN;
	mib[1] = KERN_OSTYPE;
	len = sizeof(name->sysname);
	if (err = e_sysctl(mib, 2, &name->sysname, &len, NULL, 0))
		rval = err;

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	len = sizeof(name->nodename);
	if (err = e_sysctl(mib, 2, &name->nodename, &len, NULL, 0))
		rval = err;

	mib[0] = CTL_KERN;
	mib[1] = KERN_OSRELEASE;
	len = sizeof(name->release);
	if (err = e_sysctl(mib, 2, &name->release, &len, NULL, 0))
		rval = err;

	/* The version may have newlines in it, turn them into spaces. */
	mib[0] = CTL_KERN;
	mib[1] = KERN_VERSION;
	len = sizeof(name->version);
	if (err = e_sysctl(mib, 2, &name->version, &len, NULL, 0))
		rval = err;
	else
		for (p = name->version; len--; ++p)
			if (*p == '\n' || *p == '\t')
				if (len > 1)
					*p = ' ';
				else
					*p = '\0';

	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	len = sizeof(name->machine);
	if (err = e_sysctl(mib, 2, &name->machine, &len, NULL, 0))
		rval = err;
	return rval;
}

errno_t e_bnr_uname(struct bnr_utsname *name)
{
	int mib[2];
	size_t len;
	char *p;
	errno_t err, rval;
	char version[256];

	rval = 0;

	mib[0] = CTL_KERN;
	mib[1] = KERN_OSTYPE;
	len = sizeof(name->sysname);
	if (err = e_sysctl(mib, 2, &name->sysname, &len, NULL, 0))
		rval = err;

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	len = sizeof(name->nodename);
	if (err = e_sysctl(mib, 2, &name->nodename, &len, NULL, 0))
		rval = err;

	mib[0] = CTL_KERN;
	mib[1] = KERN_OSRELEASE;
	len = sizeof(name->release);
	if (err = e_sysctl(mib, 2, &name->release, &len, NULL, 0))
		rval = err;

	/* The version may have newlines in it, turn them into spaces. */
	/* 
	 * sysctl doesn't copy anything if the space is too short.
	 * Here get as much as we can instead.
	 */
	mib[0] = CTL_KERN;
	mib[1] = KERN_VERSION;
	len = sizeof(version);
	if (err = e_sysctl(mib, 2, version, &len, NULL, 0)) {
		rval = err;
	} else {
		for (p = version; len--; ++p)
			if (*p == '\n' || *p == '\t')
				if (len > 1)
					*p = ' ';
				else
					*p = '\0';
		version[31] = '\0';
		bcopy(version, name->version, 32);
		
	}
	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	len = sizeof(name->machine);
	if (err = e_sysctl(mib, 2, &name->machine, &len, NULL, 0))
		rval = err;
	return ESUCCESS;	/* XXX */
}

errno_t e_linux_uname(struct linux_utsname *name)
{
	int mib[2];
	size_t len;
	char *p;
	errno_t err, rval;

	rval = 0;

#if 0   
	mib[0] = CTL_KERN;
	mib[1] = KERN_OSTYPE;
	len = sizeof(name->sysname);
	if (err = e_sysctl(mib, 2, &name->sysname, &len, NULL, 0)) {
		rval = err;
		strcpy((void *) &name->sysname, "sysname");
	}
#endif
        strcpy((void *) &name->sysname, "Linux");

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	len = sizeof(name->nodename);
	if (err = e_sysctl(mib, 2, &name->nodename, &len, NULL, 0)) {
		rval = err;
		strcpy((void *) &name->nodename, "nodename");
	}

#if 0   
	mib[0] = CTL_KERN;
	mib[1] = KERN_OSRELEASE;
	len = sizeof(name->release);
	if (err = e_sysctl(mib, 2, &name->release, &len, NULL, 0)) {
		rval = err;
		strcpy((void *) &name->release, "sysname");
	}
#endif
        strcpy(&name->release, "2.4.0-test8");
   
#if 0   
	/* The version may have newlines in it, turn them into spaces. */
	mib[0] = CTL_KERN;
	mib[1] = KERN_VERSION;
	len = sizeof(name->version);
	if (err = e_sysctl(mib, 2, &name->version, &len, NULL, 0)) {
		rval = err;
		strcpy((void *) &name->version, "version");
	} else
		for (p = name->version; len--; ++p)
			if (*p == '\n' || *p == '\t')
				if (len > 1)
					*p = ' ';
				else
					*p = '\0';
#endif
        strcpy((void *) &name->version, "#9");

#if 0   
	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	len = sizeof(name->machine);
	if (err = e_sysctl(mib, 2, &name->machine, &len, NULL, 0)) {
		rval = err;
		strcpy((void *) &name->sysname, "machine");
	}
#endif
        strcpy((void *) &name->sysname, "i386");
   
	name->domainname[0] = '\0';

	return ESUCCESS;	/* XXX */
}
