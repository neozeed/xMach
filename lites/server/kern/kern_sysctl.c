/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
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
 *	@(#)kern_sysctl.c	8.4 (Berkeley) 4/14/94
 */

/*
 * sysctl system call.
 */

#include "compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/unistd.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <vm/vm.h>
#include <sys/sysctl.h>

sysctlfn kern_sysctl;
sysctlfn hw_sysctl;
#ifdef DEBUG
sysctlfn debug_sysctl;
#endif
extern sysctlfn vm_sysctl;
extern sysctlfn fs_sysctl;
extern sysctlfn net_sysctl;
extern sysctlfn cpu_sysctl;

/*
 * Locking and stats
 */
static struct sysctl_lock {
	int	sl_lock;
	int	sl_want;
	int	sl_locked;
} memlock;

int
s___sysctl(
	struct proc 	*p,
	int 		*name,
	u_int		namelen,
	void 		*old,
	size_t 		*oldlenp,
	void 		*new,
	size_t 		newlen,
	int 		*retval)
{
	int error, dolock = 1;
	u_int savelen, oldlen = 0;
	sysctlfn *fn;

	if (new != NULL && (error = suser(p->p_ucred, &p->p_acflag)))
		return (error);
	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (namelen > CTL_MAXNAME || namelen < 2)
		return (EINVAL);

	switch (name[0]) {
	case CTL_KERN:
		fn = kern_sysctl;
#ifndef LITES			/* XXX */
		if (name[2] != KERN_VNODE)	/* XXX */
			dolock = 0;
#endif
		break;
	case CTL_HW:
		fn = hw_sysctl;
		break;
	case CTL_VM:
		fn = vm_sysctl;
		break;
	case CTL_NET:
		fn = net_sysctl;
		break;
#ifdef notyet
	case CTL_FS:
		fn = fs_sysctl;
		break;
#endif
	case CTL_MACHDEP:
		fn = cpu_sysctl;
		break;
#ifdef DEBUG
	case CTL_DEBUG:
		fn = debug_sysctl;
		break;
#endif
	default:
		return (EOPNOTSUPP);
	}

	if (oldlenp)
		oldlen = *oldlenp;
	if (old != NULL) {
		while (memlock.sl_lock) {
			memlock.sl_want = 1;
			sleep((caddr_t)&memlock, PRIBIO+1);
			memlock.sl_locked++;
		}
		memlock.sl_lock = 1;
#ifndef LITES			/* XXX */
		if (dolock)
			vslock(old, oldlen);
		savelen = oldlen;
#endif
	}
	error = (*fn)(name + 1, namelen - 1, old, &oldlen, new, newlen, p);
	if (old != NULL) {
#ifndef LITES			/* XXX */
		if (dolock)
			vsunlock(old, savelen, B_WRITE);
#endif
		memlock.sl_lock = 0;
		if (memlock.sl_want) {
			memlock.sl_want = 0;
			wakeup((caddr_t)&memlock);
		}
	}
	if (error)
		return (error);
	if (oldlenp)
		*oldlenp = oldlen;
	*retval = oldlen;
	return (0);
}

/*
 * Attributes stored in the kernel.
 */
char hostname[MAXHOSTNAMELEN] = "";
int hostnamelen = 0;
char domainname[MAXHOSTNAMELEN] = "";
int domainnamelen = 0;
int32_t hostid;
int securelevel;

extern char version_short[];

/*
 * kernel related system variables.
 */
kern_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	int error, level, inthostid;
	extern char ostype[], osrelease[], version[];

	/* all sysctl names at this level are terminal */
	if (namelen != 1 && !(name[0] == KERN_PROC || name[0] == KERN_PROF))
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case KERN_OSTYPE:
		return (sysctl_rdstring(oldp, oldlenp, newp, ostype));
	case KERN_OSRELEASE:
		return (sysctl_rdstring(oldp, oldlenp, newp, osrelease));
	case KERN_OSREV:
		return (sysctl_rdint(oldp, oldlenp, newp, BSD));
	case KERN_VERSION:
		return (sysctl_rdstring(oldp, oldlenp, newp, version_short));
	case KERN_MAXVNODES:
		return(sysctl_int(oldp, oldlenp, newp, newlen, &desiredvnodes));
	case KERN_MAXPROC:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &maxproc));
	case KERN_MAXFILES:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &maxfiles));
	case KERN_ARGMAX:
		return (sysctl_rdint(oldp, oldlenp, newp, ARG_MAX));
	case KERN_SECURELVL:
		level = securelevel;
		if ((error = sysctl_int(oldp, oldlenp, newp, newlen, &level)) ||
		    newp == NULL)
			return (error);
		if (level < securelevel && p->p_pid != 1)
			return (EPERM);
		securelevel = level;
		return (0);
	case KERN_HOSTNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    hostname, sizeof(hostname));
		if (newp && !error)
			hostnamelen = newlen;
		return (error);
	case KERN_DOMAINNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    domainname, sizeof(domainname));
		if (newp && !error)
			domainnamelen = newlen;
		return (error);
	case KERN_HOSTID:
		inthostid = hostid;  /* XXX assumes sizeof long <= sizeof int */
		error =  sysctl_int(oldp, oldlenp, newp, newlen, &inthostid);
		hostid = inthostid;
		return (error);
#ifndef LITES			/* XXX not yet */
	case KERN_CLOCKRATE:
		return (sysctl_clockrate(oldp, oldlenp));
#endif
	case KERN_BOOTTIME:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &boottime,
		    sizeof(struct timeval)));
	case KERN_VNODE:
		return (sysctl_vnode(oldp, oldlenp));
	case KERN_PROC:
		return (sysctl_doproc(name + 1, namelen - 1, oldp, oldlenp));
	case KERN_FILE:
		return (sysctl_file(oldp, oldlenp));
#ifndef LITES
#ifdef GPROF
	case KERN_PROF:
		return (sysctl_doprof(name + 1, namelen - 1, oldp, oldlenp,
		    newp, newlen));
#endif
#endif
	case KERN_POSIX1:
		return (sysctl_rdint(oldp, oldlenp, newp, _POSIX_VERSION));
	case KERN_NGROUPS:
		return (sysctl_rdint(oldp, oldlenp, newp, NGROUPS_MAX));
	case KERN_JOB_CONTROL:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
#else
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
#endif
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * hardware related system variables.
 */
hw_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	extern char machine[], cpu_model[];

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case HW_MACHINE:
		return (sysctl_rdstring(oldp, oldlenp, newp, machine));
#ifdef LITES
#else /*LITES */
	case HW_MODEL:
		return (sysctl_rdstring(oldp, oldlenp, newp, cpu_model));
#endif
	case HW_NCPU:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));	/* XXX */
	case HW_BYTEORDER:
		return (sysctl_rdint(oldp, oldlenp, newp, BYTE_ORDER));
#ifndef LITES
	case HW_PHYSMEM:
		return (sysctl_rdint(oldp, oldlenp, newp, ctob(physmem)));
	case HW_USERMEM:
		return (sysctl_rdint(oldp, oldlenp, newp,
		    ctob(physmem - cnt.v_wire_count)));
#endif /* LITES */
	case HW_PAGESIZE:
		return (sysctl_rdint(oldp, oldlenp, newp, PAGE_SIZE));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

#ifdef LITES

/*
 * CTL_VM identifiers
 */
#define VM_METER        1               /* struct vmmeter */
#define VM_LOADAVG      2               /* struct loadavg */
#define VM_MAXID        3               /* number of valid vm ids */

/*
 * Attributes associated with virtual memory.
 */
vm_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	struct loadavg        averunnable;
	extern mach_port_t    host_port;
	host_load_info_data_t load_info;
	int                   count = HOST_LOAD_INFO_COUNT;
	
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case VM_LOADAVG:
		(void) host_info(host_port, HOST_LOAD_INFO,
				 (host_info_t)&load_info,
				 (mach_msg_type_number_t *)&count);
		averunnable.ldavg[0] = load_info.avenrun[0];
		averunnable.ldavg[1] = load_info.avenrun[1];
		averunnable.ldavg[2] = load_info.avenrun[2];
		averunnable.fscale = LOAD_SCALE;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &averunnable,
		    sizeof(averunnable)));
#if 0
	case VM_METER:
		vmtotal(&vmtotals);
		return (sysctl_rdstruct(oldp, oldlenp, newp, &vmtotals,
		    sizeof(vmtotals)));
#endif
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif

#ifdef DEBUG
/*
 * Debugging related system variables.
 */
struct ctldebug debug0, debug1, debug2, debug3, debug4;
struct ctldebug debug5, debug6, debug7, debug8, debug9;
struct ctldebug debug10, debug11, debug12, debug13, debug14;
struct ctldebug debug15, debug16, debug17, debug18, debug19;
static struct ctldebug *debugvars[CTL_DEBUG_MAXID] = {
	&debug0, &debug1, &debug2, &debug3, &debug4,
	&debug5, &debug6, &debug7, &debug8, &debug9,
	&debug10, &debug11, &debug12, &debug13, &debug14,
	&debug15, &debug16, &debug17, &debug18, &debug19,
};
int
debug_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	struct ctldebug *cdp;

	/* all sysctl names at this level are name and field */
	if (namelen != 2)
		return (ENOTDIR);		/* overloaded */
	cdp = debugvars[name[0]];
	if (cdp->debugname == 0)
		return (EOPNOTSUPP);
	switch (name[1]) {
	case CTL_DEBUG_NAME:
		return (sysctl_rdstring(oldp, oldlenp, newp, cdp->debugname));
	case CTL_DEBUG_VALUE:
		return (sysctl_int(oldp, oldlenp, newp, newlen, cdp->debugvar));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* DEBUG */

/*
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
sysctl_int(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	int *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp && newlen != sizeof(int))
		return (EINVAL);
	*oldlenp = sizeof(int);
	if (oldp)
		*(int *)oldp = *(int *)valp;
	if (error == 0 && newp)
		*(int *)valp = *(int *)newp;
	return (error);
}

/*
 * As above, but read-only.
 */
sysctl_rdint(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	int val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(int);
	if (oldp)
		*(int *)oldp = val;
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a string-valued sysctl function.
 */
sysctl_string(oldp, oldlenp, newp, newlen, str, maxlen)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	char *str;
	int maxlen;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen >= maxlen)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		bcopy(str, oldp, len);
	}
	if (error == 0 && newp) {
		bcopy(newp, str, newlen);
		str[newlen] = 0;
	}
	return (error);
}

/*
 * As above, but read-only.
 */
sysctl_rdstring(oldp, oldlenp, newp, str)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	char *str;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		bcopy(str, oldp, len);
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
sysctl_struct(oldp, oldlenp, newp, newlen, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	void *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen > len)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		bcopy(sp, oldp, len);
	}
	if (error == 0 && newp)
		bcopy(newp, sp, len);
	return (error);
}

/*
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
sysctl_rdstruct(oldp, oldlenp, newp, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp, *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		bcopy(sp, oldp, len);
	return (error);
}

/*
 * Get file structures.
 */
sysctl_file(where, sizep)
	char *where;
	size_t *sizep;
{
	int buflen, error;
	struct file *fp;
	char *start = where;

	buflen = *sizep;
	if (where == NULL) {
		/*
		 * overestimate by 10 files
		 */
		*sizep = sizeof(filehead) + (nfiles + 10) * sizeof(struct file);
		return (0);
	}

	/*
	 * first copyout filehead
	 */
	if (buflen < sizeof(filehead)) {
		*sizep = 0;
		return (0);
	}
	bcopy((caddr_t)&filehead, where, sizeof(filehead));
	buflen -= sizeof(filehead);
	where += sizeof(filehead);

	/*
	 * followed by an array of file structures
	 */
	for (fp = filehead; fp != NULL; fp = fp->f_filef) {
		if (buflen < sizeof(struct file)) {
			*sizep = where - start;
			return (ENOMEM);
		}
		bcopy((caddr_t)fp, where, sizeof (struct file));
		buflen -= sizeof(struct file);
		where += sizeof(struct file);
	}
	*sizep = where - start;
	return (0);
}

/*
 * try over estimating by 5 procs
 */
#define KERN_PROCSLOP	(5 * sizeof (struct kinfo_proc))

sysctl_doproc(name, namelen, where, sizep)
	int *name;
	u_int namelen;
	char *where;
	size_t *sizep;
{
	register struct proc *p;
	register struct kinfo_proc *dp = (struct kinfo_proc *)where;
	register int needed = 0;
	int buflen = where != NULL ? *sizep : 0;
	int doingzomb;
	struct eproc eproc;
	int error = 0;

	if (namelen != 2 && !(namelen == 1 && name[0] == KERN_PROC_ALL))
		return (EINVAL);
	p = (struct proc *)allproc;
	doingzomb = 0;
again:
	for (; p != NULL; p = p->p_next) {
		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;
		/*
		 * TODO - make more efficient (see notes below).
		 * do by session.
		 */
		switch (name[0]) {

		case KERN_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp->pg_id != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_TTY:
			if ((p->p_flag & P_CONTROLT) == 0 ||
			    p->p_session->s_ttyp == NULL ||
			    p->p_session->s_ttyp->t_dev != (dev_t)name[1])
				continue;
			break;

		case KERN_PROC_UID:
			if (p->p_ucred->cr_uid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_RUID:
			if (p->p_cred->p_ruid != (uid_t)name[1])
				continue;
			break;
		}
		if (buflen >= sizeof(struct kinfo_proc)) {
			fill_eproc(p, &eproc);
			bcopy((caddr_t)p, &dp->kp_proc, sizeof (struct proc));
			bcopy((caddr_t)&eproc, &dp->kp_eproc, sizeof (eproc));
			dp++;
			buflen -= sizeof(struct kinfo_proc);
		}
		needed += sizeof(struct kinfo_proc);
	}
	if (doingzomb == 0) {
		p = zombproc;
		doingzomb++;
		goto again;
	}
	if (where != NULL) {
		*sizep = (caddr_t)dp - where;
		if (needed > *sizep)
			return (ENOMEM);
	} else {
		needed += KERN_PROCSLOP;
		*sizep = needed;
	}
	return (0);
}

/*
 * Fill in an eproc structure for the specified process.
 */
void
fill_eproc(p, ep)
	register struct proc *p;
	register struct eproc *ep;
{
	register struct tty *tp;

	ep->e_paddr = p;
	ep->e_sess = p->p_pgrp->pg_session;
	ep->e_pcred = *p->p_cred;
	ep->e_ucred = *p->p_ucred;
	if (p->p_stat == SIDL || p->p_stat == SZOMB) {
		ep->e_vm.vm_rssize = 0;
		ep->e_vm.vm_tsize = 0;
		ep->e_vm.vm_dsize = 0;
		ep->e_vm.vm_ssize = 0;
#ifndef sparc
		/* ep->e_vm.vm_pmap = XXX; */
#endif
	} else {
		register struct vmspace *vm = p->p_vmspace;

#ifdef pmap_resident_count
		ep->e_vm.vm_rssize = pmap_resident_count(&vm->vm_pmap); /*XXX*/
#else
		ep->e_vm.vm_rssize = vm->vm_rssize;
#endif
		ep->e_vm.vm_tsize = vm->vm_tsize;
		ep->e_vm.vm_dsize = vm->vm_dsize;
		ep->e_vm.vm_ssize = vm->vm_ssize;
#ifndef LITES
		ep->e_vm.vm_pmap = vm->vm_pmap;
#endif
	}
	if (p->p_pptr)
		ep->e_ppid = p->p_pptr->p_pid;
	else
		ep->e_ppid = 0;
	ep->e_pgid = p->p_pgrp->pg_id;
	ep->e_jobc = p->p_pgrp->pg_jobc;
	if ((p->p_flag & P_CONTROLT) &&
	     (tp = ep->e_sess->s_ttyp)) {
		ep->e_tdev = tp->t_dev;
		ep->e_tpgid = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PID;
		ep->e_tsess = tp->t_session;
	} else
		ep->e_tdev = NODEV;
	ep->e_flag = ep->e_sess->s_ttyvp ? EPROC_CTTY : 0;
	if (SESS_LEADER(p))
		ep->e_flag |= EPROC_SLEADER;
	if (!queue_empty(&p->p_servers)
	    && ((proc_invocation_t) p->p_servers.next)->k_wmesg) /* XXX */
	{
		strncpy(ep->e_wmesg,
			((proc_invocation_t) p->p_servers.next)->k_wmesg,
			WMESGLEN);
	}
	ep->e_xsize = ep->e_xrssize = 0;
	ep->e_xccount = ep->e_xswrss = 0;
}

#if COMPAT_43
#include <sys/socket.h>
#define	KINFO_PROC		(0<<8)
#define	KINFO_RT		(1<<8)
#define	KINFO_VNODE		(2<<8)
#define	KINFO_FILE		(3<<8)
#define	KINFO_METER		(4<<8)
#define	KINFO_LOADAVG		(5<<8)
#define	KINFO_CLOCKRATE		(6<<8)

struct getkerninfo_args {
	int	op;
	char	*where;
	int	*size;
	int	arg;
};

ogetkerninfo(p, uap, retval)
	struct proc *p;
	register struct getkerninfo_args *uap;
	int *retval;
{
	int error, name[5];
	size_t size;

	if (uap->size &&
	    (error = copyin((caddr_t)uap->size, (caddr_t)&size, sizeof(size))))
		return (error);

	switch (uap->op & 0xff00) {

	case KINFO_RT:
		name[0] = PF_ROUTE;
		name[1] = 0;
		name[2] = (uap->op & 0xff0000) >> 16;
		name[3] = uap->op & 0xff;
		name[4] = uap->arg;
		error = net_sysctl(name, 5, uap->where, &size, NULL, 0, p);
		break;

	case KINFO_VNODE:
		name[0] = KERN_VNODE;
		error = kern_sysctl(name, 1, uap->where, &size, NULL, 0, p);
		break;

	case KINFO_PROC:
		name[0] = KERN_PROC;
		name[1] = uap->op & 0xff;
		name[2] = uap->arg;
		error = kern_sysctl(name, 3, uap->where, &size, NULL, 0, p);
		break;

	case KINFO_FILE:
		name[0] = KERN_FILE;
		error = kern_sysctl(name, 1, uap->where, &size, NULL, 0, p);
		break;

#ifndef LITES			/* XXX for now */
	case KINFO_METER:
		name[0] = VM_METER;
		error = vm_sysctl(name, 1, uap->where, &size, NULL, 0, p);
		break;
#endif /* LITES */

	case KINFO_LOADAVG:
		name[0] = VM_LOADAVG;
		error = vm_sysctl(name, 1, uap->where, &size, NULL, 0, p);
		break;

	case KINFO_CLOCKRATE:
		name[0] = KERN_CLOCKRATE;
		error = kern_sysctl(name, 1, uap->where, &size, NULL, 0, p);
		break;

	default:
		return (EOPNOTSUPP);
	}
	if (error)
		return (error);
	*retval = size;
	if (uap->size)
		error = copyout((caddr_t)&size, (caddr_t)uap->size,
		    sizeof(size));
	return (error);
}
#endif /* COMPAT_43 */
