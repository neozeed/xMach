/* 
 * Mach Operating System
 * Copyright (c) 1995 Johannes Helander
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
 * $Log: proc_stats.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:48  mike
 * Initial Lites release from hut.fi
 *
 */
/* 
 *	File:	serv/proc_stats.c
 *	Author:	Johannes Helander, Helsinki University of Technology, 1995.
 *
 *	Collect statistics for process.
 */

#include <serv/server_defs.h>

/* Convert Mach time to unix time */
void time_value_to_timeval(time_value_t *t_v, struct timeval *tv)
{
	tv->tv_sec = t_v->seconds;
	tv->tv_usec = t_v->microseconds;
}

/* Collect statistics for live and suspended process */
mach_error_t proc_get_stats(struct proc *p)
{
	struct rusage *ru = &p->p_stats->p_ru;
	task_t task = p->p_task;

	struct task_basic_info tbi;
	struct task_events_info tei;
	struct task_thread_times_info tti;
	kern_return_t kr;
	mach_msg_type_number_t count;

	struct timeval tv;

	/* Query the kernel */
	count = TASK_BASIC_INFO_COUNT;
	kr = task_info(task, TASK_BASIC_INFO, (task_info_t) &tbi, &count);
	if (kr != KERN_SUCCESS)
	    return kr;
	assert(count == TASK_BASIC_INFO_COUNT);

	count = TASK_EVENTS_INFO_COUNT;
	kr = task_info(task, TASK_EVENTS_INFO, (task_info_t) &tei, &count);
	if (kr == KERN_INVALID_ARGUMENT) {
		/* unimplemented */
		bzero(&tei, sizeof(tei));
	} else {
		if (kr != KERN_SUCCESS)
		    return kr;
		assert(count == TASK_EVENTS_INFO_COUNT);
	}

	count = TASK_THREAD_TIMES_INFO_COUNT;
	kr = task_info(task, TASK_THREAD_TIMES_INFO,
		       (task_info_t) &tti, &count);
	if (kr != KERN_SUCCESS)
	    return kr;
	assert(count == TASK_THREAD_TIMES_INFO_COUNT);

	/* Translate */

	/* user time */
	time_value_to_timeval(&tbi.user_time, &tv);	/* terminated */
	ru->ru_utime = tv;

	time_value_to_timeval(&tti.user_time, &tv);	/* + live threads */
	timevaladd(&ru->ru_utime, &tv);

	/* system time */
	time_value_to_timeval(&tbi.system_time, &tv);	/* terminated */
	ru->ru_stime = tv;

	time_value_to_timeval(&tti.system_time, &tv);	/* + live threads */
	timevaladd(&ru->ru_stime, &tv);

	/* max resident size: use current. XXX */
	ru->ru_maxrss = tbi.resident_size / 1024;

	/* sizes: put the whole resident size in "text" size */
	ru->ru_ixrss = tbi.virtual_size / 1024;
	ru->ru_idrss = 0;
	ru->ru_isrss = 0;

	ru->ru_minflt = tei.reactivations;
	ru->ru_majflt = tei.faults;
	ru->ru_nswap = tei.pageins;

	/* ru_inblock, ru_oublock: calculated elsewhere */

#if 0
	/* messages: Mach messages seem more interesting than sockets */
	ru->ru_msgsnd = tei.messages_sent;
	ru->ru_msgrcv = tei.messages_received;
#endif

	/* ru_nsignals: calculated by bsd_sigreturn */

	/* Context switches: info not available, set to one. XXX */
	ru->ru_nvcsw = 1;
	ru->ru_nivcsw = 1;

	return KERN_SUCCESS;
}
