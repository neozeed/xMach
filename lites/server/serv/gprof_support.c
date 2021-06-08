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
 * $Log: gprof_support.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:47  mike
 * Initial Lites release from hut.fi
 *
 *
 *	File:	serv/gprof_support.c
 *	Author:	Chris Maeda
 *	Date:	March 1992
 *
 *	Defines routines for in-server profiling a la gprof.
 *
 */

#ifdef GPROF

#include <serv/import_mach.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/gprof.h>
#include <cthreads.h>

/*
 * GPROF for UX.
 *
 * _mcount protects its data structures with a mutex.  Therefore
 * we have to be very careful to not call any functions defined
 * outside of this file since they can recursively call _mcount.
 * Note that the BSD trick of setting a variable to turn off
 * _mcount in recursive calls doesn't work since we're multithreaded.
 *
 * The only functions defined outside of this file are various
 * kernel stubs (eg vm_allocate) from libmach and the timeout
 * function which is used to put the pc sampling function on the
 * callout queue.  However, timeout is never called from _mcount
 * so we're ok.
 */
/*
 * UNFORTUNATELY, we just started profiling ALL libraries, so
 * we can no longer use mutex's.  But spin_try_lock is in gcc
 * asm(""), so that's all we can use.
 */

spin_lock_t gprof_spin = SPIN_LOCK_INITIALIZER;
		/* if you can't get it; drop datum */
int gprof_do_profiling = 0;	/* do profiling when non-zero */
unsigned long gprof_bounds = 0;	/* counter of bounds check errors */
unsigned long gprof_text_low, gprof_text_high;

/*
 * Call graph data structures.
 */
callarc_t **gprof_calls = (callarc_t **) NULL; /* hash table of callarcs */
callarc_t *gprof_callarc_freelist = (callarc_t *) NULL;
callarc_t *gprof_callarc_pagelist = (callarc_t *) NULL;

/*
 * PC histogram data structures.
 */ 
/* this has to be pulled soon too */
struct mutex gprof_sample_mutex;
CHUNK *gprof_pchist;		/* histogram of pc values */
unsigned long gprof_pchist_size;

int gprof_tick_sample;			/* ticks per sample */
int gprof_sample_interval = 4000;	/* task_get_sampled_pcs every 4 sec */
sampled_pc_seqno_t gprof_seqno, gprof_oseqno;
unsigned long gprof_seqnum_gaps = 0;
sampled_pc_t gprof_pc_samples[512];

/*
 * Memory allocator for callarcs.
 */
void
gprof_callarc_get(n)
	int n;			/* this many callarcs */
{
    kern_return_t kr;
    vm_address_t page;
    vm_size_t size;
    int num_structs, i;
    register callarc_t **arcp, *arcs;

    size = n * sizeof(callarc_t);
    kr = vm_allocate(mach_task_self(), &page, size, TRUE);
    if (kr != KERN_SUCCESS) {
	panic("_mcount: allocating callarcs");
    }

    /*
     * First callarc of each page is a pointer to the next region
     * of callarcs.  ca_count holds the size.
     */
    arcs = (callarc_t *)page;
    arcs->ca_count = size;
    arcs->ca_next = gprof_callarc_pagelist;
    gprof_callarc_pagelist = arcs;
    arcs++;			/* point to next callarc */

    /*
     * Build a freelist of callarcs in the new pages.
     */
    num_structs = (size - sizeof(callarc_t)) / sizeof(callarc_t);
    arcp = &gprof_callarc_freelist;	/* freelist should be NULL */
    for (i = 0; i < num_structs; i++) {
	*arcp = arcs;
	arcp = &arcs->ca_next;
	arcs++;
    }
    *arcp = (callarc_t *) NULL; /* null terminate list */
}

/*
 * Allocate memory so we can do sampling.
 */
void
monstartup()
{
    kern_return_t kr;
    mach_port_t mytask;
    unsigned int objtype;
    vm_address_t callarcs;
    extern int _eprol();	/* first function in text seg */
    extern int etext;		/* comes right after text seg */

    mytask = mach_task_self();

    gprof_text_low = (unsigned long) _eprol;
    gprof_text_high = (unsigned long) &etext;
    gprof_pchist_size = (gprof_text_high - gprof_text_low) / CG_RESOL;
    gprof_text_high = gprof_text_low + (gprof_pchist_size - 1) * CG_RESOL;
    
    /*
     * Allocate pc histogram.
     */
    kr = vm_allocate(mytask,
		     (vm_address_t *) &gprof_pchist,
		     gprof_pchist_size * sizeof(CHUNK),
		     TRUE);	/* anywhere */
    if (kr != KERN_SUCCESS)
	panic("monstartup can't allocate pchist");

    /*
     * Allocate callarc table.
     */
    kr = vm_allocate(mytask,
		     (vm_address_t *) &gprof_calls,
		     gprof_pchist_size * sizeof(callarc_t *),
		     TRUE);	/* anywhere */
    if (kr != KERN_SUCCESS)
	panic("monstartup can't allocate callarc table");

    /*
     * Allocate callarcs.
     */
    gprof_callarc_get(512);
	
    mutex_init(&gprof_sample_mutex);
}

/*
 * Deallocate memory so we can do sampling again.
 */
void
monshutdown()
{
    kern_return_t kr;
    mach_port_t mytask = mach_task_self();
    register callarc_t *arcs, *narcs;
    
    /*
     * De_allocate pc histogram.
     */
    kr = vm_deallocate(mytask,
		     (vm_address_t) gprof_pchist,
		     gprof_pchist_size * sizeof(CHUNK));
    if (kr != KERN_SUCCESS)
	panic("monshutdown can't de_allocate pchist");

    /*
     * De_allocate callarc table.
     */
    kr = vm_deallocate(mytask,
		     (vm_address_t) gprof_calls,
		     gprof_pchist_size * sizeof(callarc_t *));
    if (kr != KERN_SUCCESS)
	panic("monshutdown can't de_allocate callarc table");

    /*
     * De_allocate ALL callarcs.
     */
    arcs = gprof_callarc_pagelist;
    gprof_callarc_pagelist = 0;
    do {
	narcs = arcs->ca_next;
	kr = vm_deallocate(mytask, (vm_address_t) arcs, arcs->ca_count);
	if (kr != KERN_SUCCESS)
	    panic("monshutdown can't de_allocate callarcs");
    } while (arcs = narcs);
}

/*
 * mcountaux -- called by _mcount
 */
void
mcountaux(from, to)
	unsigned long from, to;
{
    register callarc_t *carc, *parc;
    register unsigned long idx;

    /*
     * I hope, I hope, I hope ...
     */
    if (!spin_try_lock(&gprof_spin)) {
	register int i;
    	for (i = 50; --i > 0;)
	    if (!spin_lock_locked(&gprof_spin) && spin_try_lock(&gprof_spin))
		break;
	if (i == 0)	/* give up */
	    return;
    }
    /*
     * Find the callarc for the given values of FROM and TO.
     */
    /* Bounds Check */
	
    idx = (from - gprof_text_low) >> CG_SHIFT;
    if (idx >= gprof_pchist_size) {
	    gprof_bounds++;
	    goto mcount_done;
    }
    for (carc = gprof_calls[idx];
         carc != (callarc_t *) NULL;
	 parc = carc, carc = carc->ca_next) {
	if (carc->ca_to == to) {
	    if (carc != gprof_calls[idx]) {
	    	parc->ca_next = carc->ca_next;
		carc->ca_next = gprof_calls[idx];
		gprof_calls[idx] = carc;
	    }
	    (carc->ca_count)++;
	    spin_unlock(&gprof_spin);
	    return;
	}
    }

    /*
     * If we didn't find a callarc, grab one.
     */
    if (gprof_callarc_freelist == (callarc_t *)NULL) {
        gprof_do_profiling = 0; 
	gprof_callarc_get(512);
        gprof_do_profiling = 1; 
	if (gprof_callarc_freelist == (callarc_t *)NULL) {
	    gprof_do_profiling = 0, panic("_mcount: out of callarcs");
	}
    }

    /* get a callarc from freelist */
    carc = gprof_callarc_freelist;
    gprof_callarc_freelist = carc->ca_next;
    
    /* initialize the new callarc */
    carc->ca_from = from;
    carc->ca_to = to;
    carc->ca_count = 1;
    carc->ca_next = gprof_calls[idx];
    gprof_calls[idx] = carc;

mcount_done:
    spin_unlock(&gprof_spin);
    return;
}

/*
 * Takes pc samples.  Called from softclock.
 */
/*ARGSUSED*/
int
gprof_take_samples(arg)
	caddr_t arg;
{
    kern_return_t kr;
    int samplecnt, i;
    vm_offset_t pc;

    mutex_lock(&gprof_sample_mutex);

    if (gprof_do_profiling == 0) {
	mutex_unlock(&gprof_sample_mutex);
	return 0;
    }

    samplecnt = sizeof(gprof_pc_samples) / sizeof(sampled_pc_t);

    kr = task_get_sampled_pcs(mach_task_self(), &gprof_seqno,
			      gprof_pc_samples, &samplecnt);

    if (kr != KERN_SUCCESS) {
	printf("Can not get sampled pcs %x.  task_get_sampled_pcs failed\n", kr);
	mutex_unlock(&gprof_sample_mutex);
	return 0;
    }

    /* store samples for our task in the pc histogram */
    for (i = 0; i < samplecnt; i++) {
	    pc = gprof_pc_samples[i].pc;

	    if ((pc >= gprof_text_low) && (pc <= gprof_text_high))
		(gprof_pchist[(pc-gprof_text_low)>>CG_SHIFT])++;
    }

    /* see if we missed any samples */
    if ((gprof_oseqno + samplecnt) != gprof_seqno) {
	printf("gprof_take_samples: gap from %d to %d\n",
	       gprof_oseqno, gprof_seqno - samplecnt);
	if (gprof_sample_interval > 500)
		gprof_sample_interval >>= 1;
    }

    gprof_oseqno = gprof_seqno;

    mutex_unlock(&gprof_sample_mutex);

    /* put ourselves back on the callout queue */
    timeout(gprof_take_samples, 0, gprof_sample_interval);
}


kern_return_t
gprof_start_profiling()
{
	register int i, samplecnt;
	register callarc_t *carc, *carc_next;
	kern_return_t kr;

	if (gprof_do_profiling)
		return KERN_SUCCESS;

	/*
	 * (a) zero histogram
	 * (b) (re)initialize call graph
	 * (c) (re)start pc sampling
	 */
	
	bzero(gprof_pchist, gprof_pchist_size * sizeof(CHUNK));

	for (i = 0; i < gprof_pchist_size; i++) {
		carc = gprof_calls[i];
		while (carc != (callarc_t *) NULL) {
			carc_next = carc->ca_next;
			carc->ca_next = gprof_callarc_freelist;
			gprof_callarc_freelist = carc;
			carc = carc_next;
		}
		gprof_calls[i] = (callarc_t *) NULL;
	}

	gprof_seqno = gprof_oseqno = 0;

	kr = task_enable_pc_sampling(mach_task_self(), &gprof_tick_sample);
	if (kr != KERN_SUCCESS)
	    return kr;

	gprof_do_profiling++;
	return KERN_SUCCESS;
}

kern_return_t
gprof_stop_profiling()
{
    int samplecnt;
    kern_return_t kr;
	
    if (gprof_do_profiling == 0)
	return KERN_SUCCESS;
	
    gprof_do_profiling = 0;

    kr = task_disable_pc_sampling(mach_task_self(), &samplecnt);
    if (kr != KERN_SUCCESS)
	    return kr;

    return KERN_SUCCESS;
}

/*
 * Server monitoring messages from bsd_1.defs
 */

kern_return_t
bsd_mon_switch(proc_port, intr, onoff)
	mach_port_t	proc_port;
	boolean_t	*intr;
	int		*onoff;
{
	int old_value;
	kern_return_t kr;

	mutex_lock(&gprof_sample_mutex);
	spin_lock(&gprof_spin);

	old_value = gprof_do_profiling;

	if (*onoff)
	  kr = gprof_start_profiling();
	else
	  kr = gprof_stop_profiling();

	*onoff = old_value;
	spin_unlock(&gprof_spin);
	mutex_unlock(&gprof_sample_mutex);

	/*
	 * Take gprof_take_samples off the callout queue.
	 * We have to call it here since it will call _mcount.
	 */
	if (gprof_do_profiling == 0)
	    untimeout(gprof_take_samples,0);
	else
	    timeout(gprof_take_samples, 0, gprof_sample_interval);

	*intr = FALSE;
	return kr;
}

kern_return_t
bsd_mon_dump(proc_port, intr, mon_data, mon_data_cnt)
	mach_port_t	proc_port;
	boolean_t	*intr;
	char		**mon_data;
	int		*mon_data_cnt;
{
	vm_address_t	mon_buffer;
	vm_size_t	mon_size;
	int		callarc_cnt, i;
	callarc_t	*ca;
	kern_return_t	kr;
	struct gprof_header *gh;
	CHUNK		*hist;
	int		old_gprof_do_profiling = gprof_do_profiling;

	gprof_do_profiling = 0;

	/* Count callarcs. */
	callarc_cnt = 0;
	for (i = 0; i < gprof_pchist_size; i++)
		for (ca = gprof_calls[i];
		     ca != (callarc_t *) NULL;
		     ca = ca->ca_next)
			callarc_cnt++;

	/* How big is the buffer? */
	mon_size = sizeof(struct gprof_header)
    		+ gprof_pchist_size * sizeof(CHUNK)
			+ callarc_cnt * sizeof(struct gprof_call);
  
	kr = vm_allocate(mach_task_self(), &mon_buffer, mon_size, TRUE);
	if (kr != KERN_SUCCESS) {
		gprof_do_profiling = old_gprof_do_profiling;
		return kr;
	}

	gh = (struct gprof_header *) mon_buffer;
	gh->low = gprof_text_low;
	gh->high = gprof_text_high;
	gh->nbytes = sizeof(struct gprof_header)
		+ gprof_pchist_size * sizeof(CHUNK);

	hist = (CHUNK *) (gh + 1);
	bcopy((char *)gprof_pchist, (char *)hist,
	      gprof_pchist_size * sizeof(CHUNK));

 	hist += gprof_pchist_size;
	{
		register struct gprof_call *gc;

		gc = (struct gprof_call *) hist;

		for (i = 0; i < gprof_pchist_size; i++)
			for (ca = gprof_calls[i];
			     ca != (callarc_t *) NULL;
			     ca = ca->ca_next) {
				/*
				 * We have to use bcopy since gc might
				 * not be word-aligned.
				 */
				bcopy(ca, gc, sizeof(struct gprof_call));
				gc++;
			}
	}

	*mon_data = (char *) mon_buffer;
	*mon_data_cnt = mon_size;

	gprof_do_profiling = old_gprof_do_profiling;
	*intr = FALSE;
	return KERN_SUCCESS;
}

#endif	GPROF
