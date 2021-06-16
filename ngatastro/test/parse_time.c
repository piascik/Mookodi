/* parse_time.c
*/
/**
 * @file
 * @brief Common routines for the ngat astrometry library test programs. 
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "ngat_astro.h"

/* internal variables */

/* internal functions */

/* external routines */
/**
 * Parse a date string, of the form YYYY-MM-DDThh:mm:ss.sss, into a struct timespec.
 * @param date_string The string, of form YYYY-MM-DDThh:mm:ss.sss.
 * @param time The address of a timespec struct to fill.
 * @return The routine returns TRUE on success, FALSE on failure.
 */
int Parse_Time(char *date_string,struct timespec *time)
{
	struct tm tm_time;
	time_t seconds_since_the_epoch;
	int retval,year,month,day,hour,minute;
	double second,second_fraction;

	if(time == NULL)
	{
		fprintf(stderr,"Parse_Time failed, time was NULL.\n");
		return FALSE;
	}
	retval = sscanf(date_string,"%d-%d-%dT%d:%d:%lf",&year,&month,&day,&hour,&minute,&second);
	if(retval != 6)
	{
		fprintf(stderr,"Parse_Time failed, only %d of 6 matched for %s.\n",retval,date_string);
		return FALSE;
	}
	tm_time.tm_sec = (int)second;
	tm_time.tm_min = minute;
	tm_time.tm_hour = hour;
	tm_time.tm_mday = day;
	tm_time.tm_mon = month-1;/* tm_mon goes from 0-11 */
	tm_time.tm_year = year-1900;/* tm_year is year from 1900 */
	seconds_since_the_epoch = mktime(&tm_time);
	if(seconds_since_the_epoch == -1)
	{
		fprintf(stderr,"Parse_Time failed, mktime failed.\n");
		return FALSE;
	}
	time->tv_sec = seconds_since_the_epoch;
	/*fprintf(stdout,"Seconds since the epoch:%ld.\n",time->tv_sec);*/
	second_fraction = second-((double)tm_time.tm_sec);
	/*fprintf(stdout,"Second fraction %.3f = %.3f - %ld.\n",second_fraction,second,tm_time.tm_sec);*/
	time->tv_nsec = (long)(second_fraction*NGAT_ASTRO_ONE_SECOND_NS);
	/*fprintf(stdout,"Nano-Seconds: %ld = %.3f * %d.\n",time->tv_nsec,second_fraction,NGAT_ASTRO_ONE_SECOND_NS);*/
	/*fprintf(stdout,"Time parsed as:%s.%3d\n",ctime(&(time->tv_sec)),(time->tv_nsec/NGAT_ASTRO_ONE_MILLISECOND_NS));*/
	return TRUE;
}
