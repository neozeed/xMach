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
 * $Log: user_copy.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File: serv/user_copy.c
 *	Author: Randall W. Dean
 *	Date: 1992
 *
 * 	copyin/copyout.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <ufs/ufs/dir.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/parallel.h>

#include <serv/import_mach.h>

extern char *extend_reply_msg();
extern void extend_current_output();

int
copyout(from, to, len)
	void	*from, *to;
	u_int	len;

{
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;

	if (pk->k_reply_msg == 0) {
	    /*UIO_SYSSPACE*/
		bcopy(from, to, len);
	}
	else {
	    /*UIO_USERSPACE*/
		register caddr_t	user_addr;

		user_addr = extend_reply_msg(to, len, len);
		bcopy(from, user_addr, len);
		extend_current_output(len);
	}
	return (0);
}
int
subyte(addr, byte)
	void *addr;
	int byte;
{
	return (copyout( &byte, addr, sizeof(char)) == 0 ? 0 : -1);
}

int copyin(from, to, len)
	void *	from;
	void *	to;
	u_int	len;
{
	vm_offset_t	start, end;
	char		*data;
	vm_size_t	data_len;
	kern_return_t	result;
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;

	if (pk->k_reply_msg == 0) {
	    /*UIO_SYSSPACE*/
		bcopy(from, to, len);
		return (0);
	}
	    /*UIO_USERSPACE*/

	start = trunc_page((vm_offset_t)from);
	end   = round_page((vm_offset_t)from + len);

	/*
	 * Unlock master lock to touch user data.
	 */
	if (pk->k_master_lock)
	    master_unlock();

	result = vm_read(p->p_task,
			 start,
			 (vm_size_t)(end - start),
			 (pointer_t *)  &data,
			 &data_len);
	if (result == KERN_SUCCESS) {
	    bcopy(data + ((vm_offset_t)from - start),
		  to,
		  len);
	    (void) vm_deallocate(mach_task_self(),
				(vm_offset_t)data,
				data_len);
	}
	else {
	    result = EFAULT;
	}

	/*
	 * Re-lock master lock.
	 */
	if (pk->k_master_lock)
	    master_lock();

	return (result);
}

int fuword(addr)
	void	*addr;
{
	int	word;

	if (copyin(addr, &word, sizeof(word)))
	    return (-1);
	return (word);
}

int fubyte(addr)
	void	*addr;
{
	char	byte;

	if (copyin(addr, &byte, sizeof(byte)))
	    return (-1);
	return ((int)byte & 0xff);
}

copyinstr(from, to, max_len, len_copied)
	void	*from;
	register void *to;
	u_int	max_len;
	u_int	*len_copied;
{
	vm_offset_t	start, end;
	char		* data;
	vm_size_t	data_len;
	kern_return_t	result;

	register int	count, cur_max;
	register char	* cp;
	proc_invocation_t pk = get_proc_invocation();
	struct proc *p = pk->k_p;

	if (pk->k_reply_msg == 0) {
	    /*UIO_SYSSPACE*/
		return (copystr(from, to, max_len, len_copied));
	}
	    /*UIO_USERSPACE*/

	/*
	 * Unlock master lock to touch user data.
	 */
	if (pk->k_master_lock)
	    master_unlock();

	count = 0;

	while (count < max_len) {
	    start = trunc_page((vm_offset_t)from);
	    end   = start + vm_page_size;

	    result = vm_read(p->p_task,
			     start,
			     vm_page_size,
			     (pointer_t *) &data,
			     &data_len);
	    if (result != KERN_SUCCESS) {
		/*
		 * Re-lock master lock.
		 */
		if (pk->k_master_lock)
		    master_lock();

		return (EFAULT);
	    }

	    cur_max = end - (vm_offset_t)from;
	    if (cur_max > max_len)
		cur_max = max_len;

	    cp = data + ((vm_offset_t)from - start);
	    while (count < cur_max) {
		count++;
		if ((*((char *)to)++ = *cp++) == 0) {
		    goto done;
		}
	    }
	    (void) vm_deallocate(mach_task_self(),
				 (vm_offset_t)data,
				 data_len);
	    from = (void *)end;
	}

	/*
	 * Re-lock master lock.
	 */
	if (pk->k_master_lock)
	    master_lock();

	return (ENOENT);

    done:
	if (len_copied)
	    *len_copied = count;

	(void) vm_deallocate(mach_task_self(),
			     (vm_offset_t)data,
			     data_len);
	/*
	 * Re-lock master lock.
	 */
	if (pk->k_master_lock)
	    master_lock();

	return (0);
}

/*
 * Check address.
 * Given virtual address, byte count, and flag (nonzero for READ).
 * Returns 0 on no access.
 */
useracc(user_addr, len, direction)
	caddr_t	user_addr;
	u_int	len;
	int	direction;
{
	/*
	 * We take the 'hit' in the emulator (SIGSEGV) instead of the
	 * UX server (EFAULT).  If we fault, the system call will have
	 * been completed: this is a change to the existing semantics,
	 * but only brain-damaged programs should be able to tell
	 * the difference.
	 */
	return (1);	/* always */
}
