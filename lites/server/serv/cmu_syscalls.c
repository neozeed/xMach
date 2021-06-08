/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * 13-Jan-94  Johannes Helander (jvh) at Helsinki University of Technology
 *	Added TBL_BOOTTIME.
 *
 * 03-Dec-93  Johannes Helander (jvh) at Helsinki University of Technology
 *	For TBL_PROCINFO default parent pid to zero if there is no parent.
 *	Don't gather data from null procs.
 *
 * $Log: cmu_syscalls.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:47  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.3  93/05/14  15:17:37  rvb
 * 	Make Gcc happy -> less warnings
 * 
 * Revision 2.2  93/02/26  12:49:30  rwd
 * 	Copy only PI_COMLEN into table command struct NOT MAXCOMLEN.
 * 	[92/12/14            rwd]
 * 
 * Revision 2.1  92/04/21  17:13:33  rwd
 * BSDSS
 * 
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/table.h>
#include <sys/syscall.h>

#include <serv/import_mach.h>
#include <machine/param.h>

/*
 *  table - get/set element(s) from system table
 *
 *  This call is intended as a general purpose mechanism for retrieving or
 *  updating individual or sequential elements of various system tables and
 *  data structures.
 *
 *  One potential future use might be to make most of the standard system
 *  tables available via this mechanism so as to permit non-privileged programs
 *  to access these common SYSTAT types of data.
 *
 *  Parameters:
 *
 *  id		= an identifer indicating the table in question
 *  index	= an index into this table specifying the starting
 *		  position at which to begin the data copy
 *  addr	= address in user space to receive/supply the data
 *  nel		= number of table elements to retrieve/update
 *  lel		= expected size of a single element of the table.  If this
 *		  is smaller than the actual size, extra data will be
 *		  truncated from the end.  If it is larger, holes will be
 *		  left between elements copied to/from the user address space.
 *
 *		  The intent of separately distinguishing these final two
 *		  arguments is to insulate user programs as much as possible
 *		  from the common change in the size of system data structures
 *		  when a new field is added.  This works so long as new fields
 *		  are added only to the end, none are removed, and all fields
 *		  remain a fixed size.
 *
 *  Returns:
 *
 *  val1	= number of elements retrieved/updated (this may be fewer than
 *		  requested if more elements are requested than exist in
 *		  the table from the specified index).
 *
 *  Note:
 *
 *  A call with lel == 0 and nel == MAXSHORT can be used to determine the
 *  length of a table (in elements) before actually requesting any of the
 *  data.
 */

#define MAXLEL	(sizeof(long))	/* maximum element length (for set) */

extern int max_pid;
extern int system_procs;

int table(cp, args, retval)
	struct proc *cp;
	void *args;
	int *retval;
{
	register struct args {
		int id;
		int index;
		caddr_t addr;
		int nel;	/* >0 ==> get, <0 ==> set */
		u_int lel;
	} *uap = (struct args *) args;
	struct proc *p;
	caddr_t data;
	unsigned size;
	int error = 0;
	int set;
	vm_offset_t	arg_addr;
	vm_size_t	arg_size;
	vm_offset_t	copy_start;
	vm_offset_t	copy_end;
	vm_size_t	copy_size;
	vm_offset_t	dealloc_start;	/* area to remove from kernel map */
	vm_size_t	dealloc_end;

	/*
	 *  Verify that any set request is appropriate.
	 */
	set = 0;
	if (uap->nel < 0) {
		switch (uap->id) {
		case TBL_MAXUPRC:
			if (error = suser(cp->p_ucred, &cp->p_acflag))
				return (error);
		        break;
		default:
			return (EINVAL);
		}
		set++;
		uap->nel = -(uap->nel);
	}

	*retval = 0;

	/*
	 *  Verify common case of only current process index.
	 */
	switch (uap->id) {
	case TBL_U_TTYD:
	case TBL_MAXUPRC:
		if ((uap->index != cp->p_pid && uap->index != 0) ||
				(uap->nel != 1))
			goto bad;
		break;
        case TBL_PROCINFO:
		if (uap->lel == 0) {
		    *retval = min(max_pid+1+system_procs,uap->nel);
		    return (0);
		}
		break;
	}
	
	/*
	 *  Main loop for each element.
	 */

	while (uap->nel > 0) {
		dev_t nottyd;
		int iv;
		struct tty *ttp;
		union {
			struct tbl_loadavg t_u_tl;
			struct tbl_procinfo t_u_tp;
			struct tbl_sysinfo t_u_ts;
			struct tbl_intr t_u_ti;
			struct tbl_dkinfo t_u_td;
			struct tbl_ttyinfo t_u_tt;
		} tbl_data;
#define	tl	tbl_data.t_u_tl
#define	tp	tbl_data.t_u_tp
#define ts	tbl_data.t_u_ts
#define ti	tbl_data.t_u_ti
#define td	tbl_data.t_u_td
#define tt	tbl_data.t_u_tt

		extern mach_port_t host_port;
		host_load_info_data_t load_info;
		int count;
                
		dealloc_start = (vm_offset_t) 0;
		dealloc_end = (vm_offset_t) 0;
		switch (uap->id) {
		case TBL_U_TTYD:
			if ((cp->p_flag & P_CONTROLT) && (ttp = cp->p_session->s_ttyp) != NULL)
				data = (caddr_t)&ttp->t_dev;
			else {
				nottyd = -1;
				data = (caddr_t)&nottyd;
			}
			size = sizeof (dev_t);
			break;
		case TBL_LOADAVG:
			if (uap->index != 0 || uap->nel != 1)
				goto bad;
			count = HOST_LOAD_INFO_COUNT;
			(void) host_info(host_port, HOST_LOAD_INFO,
					 (host_info_t)&load_info,
					 (mach_msg_type_number_t *)&count);
			/* XXX Memory leak ? */
			bcopy((caddr_t)load_info.avenrun,
			      (caddr_t)&tl.tl_avenrun,
			      sizeof(tl.tl_avenrun));
			tl.tl_lscale = 1000;/*XXXXXXX*/
			bcopy((caddr_t)load_info.mach_factor,
			      (caddr_t)&tl.tl_mach_factor,
			      sizeof(tl.tl_mach_factor));
			data = (caddr_t)&tl;
			size = sizeof (tl);
			break;
		case TBL_INTR:
			printf("table(TBL_INTR) not implemented\n");
			goto bad;
		case TBL_INCLUDE_VERSION:
			if (uap->index != 0 || uap->nel != 1)
				goto bad;
			iv = 50;/*XXXXXXXXX*/
			data = (caddr_t)&iv;
			size = sizeof(iv);
			break;
		case TBL_MAXUPRC:
			printf("TBL_MAXUPRC not supported\n");
			goto bad;
		case TBL_UAREA:
			printf("table(TBL_UAREA) not implemented\n");
			goto bad;
		case TBL_ARGUMENTS:
                   {
			/*
			 *	Returns the first N bytes of the user args,
			 *	Odd data structure is for compatibility.
			 */
			/*
			 *	Lookup process by pid
			 */
			p = pfind(uap->index);
			if (p == (struct proc *)0) {
			/*
			 *	Proc 0 isn't in the hash table
			 */
				if (uap->index == 0)
					p = &proc0;
				else {
					/*
					 *	No such process
					 */
					return (ESRCH);
				}
			 }
			if (p != cp &&
			    (error = suser(cp->p_ucred, &cp->p_acflag)))
				return (error);
                        /*
                         *      Get task struct
                         */

			/*
			 *	If the user expects no more than N bytes of
			 *	argument list, use that as a guess for the
			 *	size.
			 */
			if ((arg_size = uap->lel) == 0) {
				error = EINVAL;
				goto bad;
			}
#if 0	/* not working */
			arg_addr = (vm_offset_t)p->p_argp;
                        copy_size = min(utaskp->uu_arg_size, arg_size);
#else
			arg_addr = 0;
			goto bad;
#endif

                        /*
                         * If there are no arguments then return empty
                         * buffer
                         */
                        if (copy_size == 0 || arg_addr == 0) {
                                return(0);
                        }
                        
			if (vm_read(p->p_task,
				    arg_addr,
				    copy_size,
				    &copy_start,
				    &copy_size) != KERN_SUCCESS)
			  goto bad;
                        /*
                         * Return arguments
                         */
                        data = (caddr_t) copy_start;
                        size = (vm_size_t) copy_size;
                        
			copy_end = copy_start + copy_size;

			dealloc_start = copy_start;
			dealloc_end = copy_end;
                        break;
                    }
		case TBL_ENVIRONMENT:
                   {
			/*
			 *	Returns the first N bytes of the environment,
			 *	Odd data structure is for compatibility.
			 */
			/*
			 *	Lookup process by pid
			 */
			p = pfind(uap->index);
			if (p == (struct proc *)0) {
			/*
			 *	Proc 0 isn't in the hash table
			 */
				if (uap->index == 0)
					p = &proc0;
				else {
					/*
					 *	No such process
					 */
					return (ESRCH);
				}
			 }
			if (p != cp &&
			    (error = suser(cp->p_ucred, &cp->p_acflag)))
				return (error);

			/*
			 *	If the user expects no more than N bytes of
			 *	argument list, use that as a guess for the
			 *	size.
			 */
			if ((arg_size = uap->lel) == 0) {
				error = EINVAL;
				goto bad;
			}
#if 0
			arg_addr = (vm_offset_t)utaskp->uu_envp;
                        copy_size = min(utaskp->uu_env_size, arg_size);
#else
			arg_addr = 0;
#endif 0

                        /*
                         * If there is no environment then return empty
                         * buffer
                         */
                        if (copy_size == 0 || arg_addr == 0) {
                                return (0);
                        }
                        
			if (vm_read(p->p_task,
				    arg_addr,
				    copy_size,
				    &copy_start,
				    &copy_size) != KERN_SUCCESS)
			  goto bad;
			copy_end = copy_start + copy_size;

                        /*
                         * Return arguments
                         */
                        data = (caddr_t) copy_start;
                        size = (vm_size_t) copy_size;
                        
			copy_end = copy_start + copy_size;

			dealloc_start = copy_start;
			dealloc_end = copy_end;
                        break;
                    }
		case TBL_PROCINFO:
		    {
			/*
			 *	Index is entry number in proc table.
			 */
			if (uap->index > max_pid+1+system_procs ||
			    uap->index < 0)
			    goto bad;

			if (uap->index-system_procs == 0)
			    p = &proc0;
			else
			    p = pfind(uap->index-system_procs);
			if (p != cp
			    && (error = suser(cp->p_ucred, &cp->p_acflag)))
				return (error);
			if (p == (struct proc *)0) {
			    bzero((caddr_t)&tp, sizeof(tp));
			    tp.pi_status = PI_EMPTY;
			} else if ((p->p_stat == 0) || (p->p_stat == SIDL)) {
			    bzero((caddr_t)&tp, sizeof(tp));
			    tp.pi_status = PI_EMPTY;
			} else {
			    mutex_lock(&p->p_lock);
			    tp.pi_uid	= p->p_ucred->cr_uid;
                            tp.pi_ruid  = p->p_cred->p_ruid;
                            tp.pi_svuid = p->p_cred->p_svuid;
                            tp.pi_rgid  = p->p_cred->p_rgid;
                            tp.pi_svgid = p->p_cred->p_svgid;
			    mutex_unlock(&p->p_lock);
			    tp.pi_pid	= p->p_pid;
			    tp.pi_ppid	= p->p_pptr ? p->p_pptr->p_pid : 0;
                            if (p->p_pgrp) {
                                    tp.pi_pgrp  = p->p_pgrp->pg_id;
                                    tp.pi_jobc  = p->p_pgrp->pg_jobc;
			    } else {
                                    tp.pi_pgrp  = 0;
                                    tp.pi_jobc  = 0;
                            }
			    if (p->p_session == NULL)
				panic("session == NULL");
                            if (p->p_session && p->p_session->s_leader)
                                    tp.pi_session = p->p_session->s_leader->p_pid;
                            else
                                    tp.pi_session = 0;
			    tp.pi_flag	= p->p_flag;
                            tp.pi_cursig = p->p_siglist;
                            tp.pi_sig   = p->p_siglist;
                            tp.pi_sigmask = p->p_sigmask;
                            tp.pi_sigignore = p->p_sigignore;
                            tp.pi_sigcatch = p->p_sigcatch;
                            
			    if (p->p_task == MACH_PORT_NULL)   {
				tp.pi_status = PI_ZOMBIE;
			    } else {
				if (!(p->p_flag & P_CONTROLT) ||
				    (ttp = p->p_session->s_ttyp) == NULL) {
				    tp.pi_ttyd = NODEV;
                                    tp.pi_tsession = 0;
                                    tp.pi_tpgrp = 0;
                                } else {
				    tp.pi_ttyd = ttp->t_dev;
                                    if (ttp->t_session && ttp->t_session->s_leader) {
                                            
                                        tp.pi_tsession = ttp->t_session->s_leader->p_pid;
                                            
                                        tp.pi_tpgrp = ttp->t_session->s_leader->p_pgrp->pg_id;
                                    } else {
                                        tp.pi_tsession = 0;
                                        tp.pi_tpgrp = 0;
                                    }
                                }
				bcopy(p->p_comm, tp.pi_comm, PI_COMLEN);
				tp.pi_comm[PI_COMLEN] = '\0';

				if (p->p_flag & P_WEXIT)
				    tp.pi_status = PI_EXITING;
				else
				    tp.pi_status = PI_ACTIVE;
			    }
			}

			data = (caddr_t)&tp;
			size = sizeof(tp);
			break;
		    }
		case TBL_BOOTTIME:
			if (uap->index != 0 || uap->nel != 1)
			    goto bad;
			data = (caddr_t)&boottime;
			size = sizeof boottime;
			break;
                case TBL_SYSINFO:
			printf("table(TBL_SYSINFO) not implemented\n");
			goto bad;
                case TBL_DKINFO:
			printf("table(TBL_DKINFO) not implemented\n");
			goto bad;
                case TBL_TTYINFO:
			printf("table(TBL_TTYINFO) not implemented\n");
			goto bad;
 		case TBL_MSGDS:
			printf("table(TBL_MSGDS) not implemented\n");
			goto bad;
		case TBL_SEMDS:
			printf("table(TBL_SEMDS) not implemented\n");
			goto bad;
		case TBL_SHMDS:
			printf("table(TBL_SHMDS) not implemented\n");
			goto bad;
		case TBL_MSGINFO:
			printf("table(TBL_MSGINFO) not implemented\n");
			goto bad;
		case TBL_SEMINFO:
			printf("table(TBL_SEMINFO) not implemented\n");
			goto bad;
		case TBL_SHMINFO:
			printf("table(TBL_SHMINFO) not implemented\n");
			goto bad;
		default:
		bad:
			/*
			 *	Return error only if all indices
			 *	are invalid.
			 */
			if (*retval == 0)
				error = EINVAL;
			else
				error = 0;
			return (error);
		}
		/*
		 * This code should be generalized if/when other tables
		 * are added to handle single element copies where the
		 * actual and expected sizes differ or the table entries
		 * are not contiguous in kernel memory (as with TTYLOC)
		 * and also efficiently copy multiple element
		 * tables when contiguous and the sizes match.
		 */
		size = min(size, uap->lel);
		if (size) {
			if (set) {
				char buff[MAXLEL];

			        error = copyin(uap->addr, buff, size);
				if (error == 0)
					bcopy(buff, data, size);
			}
			else {
				error = copyout(data, uap->addr, size);
			}
		}
		if (dealloc_start != (vm_offset_t) 0) {
			(void) vm_deallocate(mach_task_self(),
					     dealloc_start,
					     dealloc_end - dealloc_start);
		}
		if (error)
			return (error);
		uap->addr += uap->lel;
		uap->nel -= 1;
		uap->index += 1;
		*retval += 1;
	}
	return(0);
}

