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
 * $Log: inittodr.c,v $
 * Revision 1.2  2000/10/27 01:58:49  welchd
 *
 * Updated to latest source
 *
 * Revision 1.1.1.1  1995/03/02  21:49:47  mike
 * Initial Lites release from hut.fi
 *
 * Revision 2.2  93/02/26  12:56:08  rwd
 * 	Include sys/systm.h for printf prototypes.
 * 	[92/12/09            rwd]
 * 
 * Revision 2.1  92/04/21  17:10:58  rwd
 * BSDSS
 * 
 *
 */

/*
 * Routines to guess the time.
 */
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

/*
 * Time conversion.
 */

#define	SECS_PER_MIN	60
#define	MINS_PER_HOUR	60
#define	HOURS_PER_DAY	24
#define	DAYS_PER_WEEK	7
#define	DAYS_PER_NYEAR	365
#define	DAYS_PER_LYEAR	366
#define	SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define	SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define	MONS_PER_YEAR	12

#define	TM_SUNDAY	0
#define	TM_MONDAY	1
#define	TM_TUESDAY	2
#define	TM_WEDNESDAY	3
#define	TM_THURSDAY	4
#define	TM_FRIDAY	5
#define	TM_SATURDAY	6

#define	TM_JANUARY	0
#define	TM_FEBRUARY	1
#define	TM_MARCH	2
#define	TM_APRIL	3
#define	TM_MAY		4
#define	TM_JUNE		5
#define	TM_JULY		6
#define	TM_AUGUST	7
#define	TM_SEPTEMBER	8
#define	TM_OCTOBER	9
#define	TM_NOVEMBER	10
#define	TM_DECEMBER	11

#define	TM_YEAR_BASE	1900

#define	EPOCH_YEAR	1970
#define	EPOCH_WDAY	TM_THURSDAY

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define	isleap(y) (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

int	mon_lengths[2][MONS_PER_YEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};


void cvttime(
	time_t			clock,		/* seconds since 1 Jan 1970 */
	register struct tm *	tmp)
{
	register long		days;
	register long		rem;
	register int		y;
	register int		yleap;
	register int *		ip;
	struct tm		tm;

	days = (clock - tz.tz_minuteswest*SECS_PER_MIN) / SECS_PER_DAY;
	rem  = (clock - tz.tz_minuteswest*SECS_PER_MIN) % SECS_PER_DAY;
	while (rem < 0) {
	    rem += SECS_PER_DAY;
	    --days;
	}
	while (rem >= SECS_PER_DAY) {
	    rem -= SECS_PER_DAY;
	    ++days;
	}

	tm.tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tm.tm_min = (int) (rem / SECS_PER_MIN);
	tm.tm_sec = (int) (rem % SECS_PER_MIN);

	tm.tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tm.tm_wday < 0)
	    tm.tm_wday += DAYS_PER_WEEK;

	y = EPOCH_YEAR;
	if (days >= 0) {
	    for (;;) {
		yleap = isleap(y);
		if (days < (long) year_lengths[yleap])
		    break;
		++y;
		days = days - (long) year_lengths[yleap];
	    }
	}
	else {
	    do {
		--y;
		yleap = isleap(y);
		days = days + (long) year_lengths[yleap];
	    } while (days < 0);
	}
	tm.tm_year = y - TM_YEAR_BASE;
	tm.tm_yday = (int) days;

	ip = mon_lengths[yleap];
	for (tm.tm_mon = 0;
	     days >= (long) ip[tm.tm_mon];
	     ++tm.tm_mon) {
	    days = days - (long) ip[tm.tm_mon];
	}
	tm.tm_mday = (int) (days + 1);
	tm.tm_isdst = 0;

	*tmp = tm;
}

void fmttime(
	register struct tm *timeptr,
	char		str[26])
{
	static char	wday_name[DAYS_PER_WEEK][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static char	mon_name[MONS_PER_YEAR][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	sprintf(str, "%s %s %d %d:%d:%d %d",
		wday_name[timeptr->tm_wday],
		mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		TM_YEAR_BASE + timeptr->tm_year);
}	

void print_time(
	char *	fmt,
	time_t	clock)
{
	struct tm	tm;
	char		timestr[26];

	cvttime(clock, &tm);
	fmttime(&tm, timestr);
	printf(fmt, timestr);
}

/*
 * Initialize the time of day.
 */
#define	SECYR		((unsigned)(365*SECS_PER_DAY))	/* per common year */

/*
 * If the hardware clock does not keep the current year, it
 * sets the year to 1982.  If there is no hardware clock at all,
 * the year will be about 1970.
 */
#define	NOYEAR		((1982 - EPOCH_YEAR) * SECYR + 3 * SECS_PER_DAY)
			/* 1972, 1976, 1980 were leap years */

/*
 * Initialze the time of day register, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within six months,
 * and the time of year clock provides the rest.
 */
void inittodr(base)
	time_t base;
{
	long		deltat;
	int		year = EPOCH_YEAR;

	struct timeval	cur_time, time;

	time.tv_usec = 0;

  print_time("Base is %s\n", base);

	if (base < 5*SECYR) {
	    printf("WARNING: preposterous time in file system");
	    time.tv_sec = 6*SECYR + 186*SECS_PER_DAY + SECS_PER_DAY/2;
	    goto check;
	}

	get_time(&cur_time);
	boottime = cur_time;

  print_time("Current time is %s\n", cur_time.tv_sec);
	/*
	 * If the hardware does not provide the year,
	 * the time will be stuck at 1982.	XXX
	 */
	if (cur_time.tv_sec < NOYEAR) {
	    printf("WARNING: hardware does not know what time it is");
	    /*
	     * Believe the time in the file system for lack of
	     * anything better.
	     */
	    time.tv_sec = base;
	    time.tv_usec = 0;
	    goto check;
	}

	/*
	 * Sneak to within 6 month of the time in the filesystem,
	 * by starting with the time of the year suggested by the TODR,
	 * and advancing through succesive years.  Adding the number of
	 * seconds in the current year takes us to the end of the current year
	 * and then around into the next year to the same position.
	 */
	time.tv_sec = cur_time.tv_sec - NOYEAR;
	while (time.tv_sec < base-SECYR/2) {
	    if (isleap(year))
		time.tv_sec += SECS_PER_DAY;
	    year++;
	    time.tv_sec += SECYR;
	}

	/*
	 * See if we gained/lost two or more days;
	 * if so, assume something is amiss.
	 */
	deltat = time.tv_sec - base;
	if (deltat < 0)
	    deltat = -deltat;
	if (deltat < 2*SECS_PER_DAY) {
	    /*
	     * Time seems to correspond.
	     */
	    goto set_the_time;
	}

	printf("WARNING: clock %s %d days",
	    time.tv_sec < base ? "lost" : "gained", deltat / SECS_PER_DAY);
check:
	printf(" -- CHECK AND RESET THE DATE!\n");
	boottime.tv_sec = time.tv_sec;

set_the_time:
  print_time("Time is set to %s\n", time.tv_sec);
	set_time(&time);
}

