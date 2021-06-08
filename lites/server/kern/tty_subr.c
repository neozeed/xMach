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
 * $Log: tty_subr.c,v $
 * Revision 1.2  2000/10/27 01:58:45  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.2  1995/03/23  01:16:41  law
 * lites-950323 from jvh.
 *
 */
/* 
 *	File:	tty_subr.c
 *	Author:	Randall W. Dean
 *	Date:	1992
 *
 * 	Clists.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/clist.h>
#include <sys/synch.h>
#include <sys/assert.h>

extern int nclist;

#define dtocbp(x) ((struct cblock *)((vm_offset_t)(x) & ~CROUND))

#define cbdealloc(x) \
do { \
      register struct cblock *bp; \
      bp = dtocbp((x)); \
      bp->c_next = cfreelist; \
      cfreelist = bp; \
 } while (0)

#define clpbytestoread(clp) \
    min(clp->c_cc, \
	(int)((vm_offset_t)dtocbp(clp->c_cf+CBLOCK)-(vm_offset_t)clp->c_cf))

/* only use after incclp so there are some bytes and c_cl points to
 * next writeable
 */
#define clpbytestowrite(clp) \
    (int)((vm_offset_t)dtocbp(clp->c_cl+CBLOCK)-(vm_offset_t)clp->c_cl)


struct cblock *cfree = 0, *cfreelist = 0;

/*
 * Initialize clists.
 */
void clist_init()
{
    register struct cblock *cp;

    cfree = (struct cblock *) malloc(sizeof(struct cblock) * nclist);
    assert(cfree);

    for(cp=dtocbp(cfree+CROUND); cp < &cfree[nclist-1]; cp++) {
	cp->c_next = cfreelist;
	cfreelist = cp;
    }
}

void
clcheck(struct clist *clp)
{
    if (clp->c_cc == 0) {
	cbdealloc(clp->c_cf-1);
	clp->c_cf = clp->c_cl = NULL;
    } else if (((integer_t)clp->c_cf & CROUND) == 0){
	register struct cblock *cbp = dtocbp(clp->c_cf-1);
	clp->c_cf = cbp->c_next->c_info;
	cbdealloc(cbp);
    }
}

/*
 * Get a character from a clist.
 */
getc(struct clist *clp)
{
    int c;
    int s;

    s = spltty();
    if (clp->c_cc == 0) {
	clp->c_cf = clp->c_cl = NULL;
	splx(s);
	return -1;
    }
    c = *(clp->c_cf++) & 0xff;
    clp->c_cc--;
    clcheck(clp);
    splx(s);
    return (c);
}

/*
 * Copy clist to buffer.
 * Return number of bytes moved.
 */
int
q_to_b(struct clist *clp, char *cp, int count)
{
    int s, start_count = count;

    if (count <= 0)
	return (0);
    s = spltty();
    if (clp->c_cc == 0) {
	clp->c_cf = clp->c_cl = NULL;
	splx(s);
	return (0);
    }

    while (count && clp->c_cc) {
	register int bytes;
	bytes = min(count, clpbytestoread(clp));
	bcopy(clp->c_cf, cp, bytes);
	clp->c_cf += bytes;
	clp->c_cc -= bytes;
	count -= bytes;
	cp += bytes;
	clcheck(clp);
    }
    splx(s);
    return (start_count - count);
}

/*
 * Return count of contiguous characters in clist.
 * Stop counting if flag&character is non-null.
 */
int
ndqb(struct clist *clp, boolean_t flag)
{
    int count = 0;
    int s;

    s = spltty();
    if (clp->c_cc == 0) {
	splx(s);
	return 0;
    }
    count = clpbytestoread(clp);
    if (flag) {
	register char *cc = clp->c_cf;
	register char *end = cc + count;

	count = 0;
	while (cc < end) {
	    if (*cc & flag) break;
	    count++;
	    cc++;
	}
    }
    splx(s);
    return (count);
}

/*
 * Flush count bytes from clist.
 */
void ndflush(struct clist *clp, int count)
{
    int s;

    s = spltty();
    if (clp->c_cc == 0) {
	splx(s);
	return;
    }
    while (count > 0 && clp->c_cc) {
	register int cl = clpbytestoread(clp);
	if (cl > count) {
	    clp->c_cf += count;
	    clp->c_cc -= count;
	    count = 0;
	} else {
	    clp->c_cf += cl;
	    clp->c_cc -= cl;
	    count -= cl;
	    clcheck(clp);
	}
    }
    splx(s);
}


int incclp(struct clist *clp)
{
    struct cblock *cbp;

    if ((clp->c_cl) == NULL) {
	if ((cbp = cfreelist) == NULL)
	    return (-1);
	cfreelist = cbp->c_next;
	cbp->c_next = NULL;
	clp->c_cf = clp->c_cl = cbp->c_info;
	clp->c_cc = 1;
    } else {
	if (((integer_t)(clp->c_cl+1) & CROUND) == 0) {
	    cbp = dtocbp(clp->c_cl - 1);
	    if ((cbp->c_next = cfreelist) == NULL)
		return (-1);
	    cbp = cbp->c_next;
	    cfreelist = cbp->c_next;
	    cbp->c_next = NULL;
	    clp->c_cl = cbp->c_info;
	} else
	    clp->c_cl++;
	clp->c_cc++;
    }
    return 0;
}

/*
 * Put a character into the output queue.
 */
int
putc(int c, struct clist *clp)
{
    int s;

    s = spltty();
    if (incclp(clp) == -1) {
	splx(s);
	return -1;
    }
    *clp->c_cl = c;
    splx(s);
    return (0);
}

/*
 * Copy buffer to clist.
 * Return number of bytes not transfered.
 */
int
b_to_q(char *cp, int count, struct clist *clp)
{
	int s;
	int ts;

	if (count <= 0)
		return (0);
	s = spltty();
	while(count) {
	    if (incclp(clp) == -1) {
		splx(s);
		return count;
	    }
	    ts = min(clpbytestowrite(clp),count);
	    bcopy(cp, clp->c_cl, ts);
	    clp->c_cl += ts-1;
	    clp->c_cc += ts-1;
	    count -= ts;
	    cp += ts;
	}
	splx(s);
	return (0);
}

/*
 * Given a non-NULL pointer into the clist return the pointer
 * to the next character in the list or return NULL if no more chars.
 *
 * Callers must not allow getc's to happen between nextc's so that the
 * pointer becomes invalid.  Note that interrupts are NOT masked.
 */
char *
nextc(struct clist *clp, char *cp, int *c)
{

	if (clp->c_cc && cp++ != clp->c_cl) {
	    if (((integer_t)cp & CROUND) == 0)
		cp = dtocbp(cp-1)->c_next->c_info;
	    *c = *cp & 0xff;
	    return (cp);
	}
	return 0;
}

/*
 * Remove the last character in the list and return it.
 */
unputc(struct clist *clp)
{
    int s;
    char c;
    register struct cblock *cbp;

    s = spltty();
    if (clp->c_cc == 0) {
	splx(s);
	return (-1);
    }
    c = *clp->c_cl;
    clp->c_cc -= 1;
    if (clp->c_cl == (cbp = dtocbp(clp->c_cl))->c_info) {
	cbdealloc(cbp);
	if (clp->c_cc == 0) {
	    clp->c_cl = clp->c_cf = NULL;
	} else {
	    register struct cblock *ocbp;
	    for(ocbp=dtocbp(clp->c_cf);ocbp->c_next!=cbp;ocbp=ocbp->c_next);
	    ocbp->c_next = NULL;
	    clp->c_cl = &ocbp->c_info[CBSIZE-1];
	}
    } else
	clp->c_cl -= 1;
    splx(s);
    return c & 0xff;
}

/*
 * Put the chars in the from queue on the end of the to queue.
 */
void catq(struct clist *from, struct clist *to)
{
	int c;

	while ((c = getc(from)) >= 0)
		putc(c, to);
}

