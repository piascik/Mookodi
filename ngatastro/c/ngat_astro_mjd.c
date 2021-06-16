/* ngat_astro_mjd.c
** Next Generation Astronomical Telescope Astrometric library. Modified Julian Date routines.
*/
/**
 * @file
 * @brief ngat_astro_mjd.c contains the Modified Julian Date routines for the NGAT astrometry library.
 * @author Chris Mottram
 * @version $Revision: 1.4 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L
/**
 * This hash define is needed to make header files include X/Open UNIX entensions.
 * This allows us to use BSD/SVr4 function calls even when  _POSIX_C_SOURCE is defined.
 * If these lines are not included, the fact that _POSIX_C_SOURCE is defined
 * causes struct timeval not to be defined in time.h, and then resource.h complains about this (under Solaris).
 */
#define _XOPEN_SOURCE		(1)
/**
 * This hash define is needed to make header files include X/Open UNIX entensions.
 * This allows us to use BSD/SVr4 function calls even when  _POSIX_C_SOURCE is defined.
 * If these lines are not included, the fact that _POSIX_C_SOURCE is defined
 * causes struct timeval not to be defined in time.h, and then resource.h complains about this (under Solaris).
 */
#define _XOPEN_SOURCE_EXTENDED 	(1)

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include "ngat_astro_mjd.h"
#include "ngat_astro.h"
#include "ngat_astro_private.h"

/* hash definitions */

/* internal data types */

/* internal data */

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to calculate the MJD representation of the specified time.
 * @param time A struct timespec, containing the time.
 * @param mjd The address of a double, to store the modified Julian date (JD-2400000.5).
 * @param leap_second_correction A number representing whether a leap second will occur. This is normally zero,
 * 	which means no leap second will occur. It can be 1, which means the last minute of the day has 61 seconds,
 *	i.e. there are 86401 seconds in the day. It can be -1,which means the last minute of the day has 59 seconds,
 *	i.e. there are 86399 seconds in the day.
 * @return The routine returns TRUE if it succeeds, FALSE if it fails. The library error code and
 *         error string are set if an error occurs.
 * @see #NGAT_Astro_Year_Month_Day_To_MJD
 * @see #NGAT_Astro_Hour_Minute_Second_To_Day_Fraction
 * @see Astro_Error_Number
 * @see Astro_Error_String
 */
int NGAT_Astro_Timespec_To_MJD(struct timespec time,int leap_second_correction,double *mjd)
{
	struct tm tm_time;
	int year,month,day,retval;
	double day_fraction;

	Astro_Error_Number = 0;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Timespec_To_MJD(time=%ld.%ld,leap second=%d)",
			      time.tv_sec,time.tv_nsec,leap_second_correction);
#endif
/* check leap_second_correction in range */
	if((leap_second_correction < -1) || (leap_second_correction > 1))
	{
		Astro_Error_Number = 5;
		sprintf(Astro_Error_String,"NGAT_Astro_Timespec_To_MJD:Leap Second correction %d out of range (-1,1).",
			leap_second_correction);
		return FALSE;
	}
/* timespec to year/month/day */
/* convert time to ymdhms*/
	gmtime_r(&(time.tv_sec),&tm_time);
/* convert tm_time data to format suitable for NGAT_Astro_Year_Month_Day_To_MJD */
/* tm_year is years since 1900:NGAT_Astro_Year_Month_Day_To_MJD wants full year.*/
	year = tm_time.tm_year+1900; 
	month = tm_time.tm_mon+1;/* tm_mon is 0..11 : NGAT_Astro_Year_Month_Day_To_MJD wants 1..12 */
	day = tm_time.tm_mday;
/* call NGAT_Astro_Year_Month_Day_To_MJD to get MJD for 0hr */
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Timespec_To_MJD:year = %d,month = %d,day = %d",
			      year,month,day);
#endif
	retval = NGAT_Astro_Year_Month_Day_To_MJD(year,month,day,mjd);
	if(retval == FALSE)
		return FALSE;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Timespec_To_MJD:"
			      "NGAT_Astro_Year_Month_Day_To_MJD returnd %.2f",(*mjd));
#endif
	/* get day fraction */
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Timespec_To_MJD:hour = %d,minute = %d,second = %d,"
			      "nsec = %ld, leap second = %d",tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,
			      time.tv_nsec,leap_second_correction);
#endif
	retval = NGAT_Astro_Hour_Minute_Second_To_Day_Fraction(tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,
							       time.tv_nsec,leap_second_correction,&day_fraction);
	if(retval == FALSE)
		return FALSE;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Timespec_To_MJD:"
			      "NGAT_Astro_Hour_Minute_Second_To_Day_Fraction returned %.3f.",day_fraction);
#endif
/* add day_fraction to mjd */
	(*mjd) += day_fraction;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Timespec_To_MJD:returning MJD %.3f.",(*mjd));
#endif
	return TRUE;
}

/**
 * Routine to calculate the MJD representation of the specified year,month and day.
 * Algorithm taken from The Quarterly Journal of the Royal Astronomical Society, Vol 25, No1.
 * Page 53-55, Simple Formulae for Julian Day Numbers and Calendar Dates (D. A. Hatcher).
 * Note this returns the MJD at the start of the day, you may wish to add a day fraction on.
 * Note JD's start at midday, but MJD's start at midnight.
 * @param year The year, which must be greater than -4712.
 * @param month A number representing the month, where January is 1 and December is 12.
 * @param day The day number in the month.
 * @param mjd The address of a double, to store the modified Julian date (JD-2400000.5).
 * @return The routine returns TRUE if it succeeds, FALSE if it fails. The library error code and
 *         error string are set if an error occurs.
 * @see Astro_Error_Number
 * @see Astro_Error_String
 */
int NGAT_Astro_Year_Month_Day_To_MJD(int year,int month,int day,double *mjd)
{
	static int month_day_count[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	double d1,g1,g3;
	int a_dash,m_dash,y,d,N,g,g2;

	Astro_Error_Number = 0;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD(year=%d,month=%d,day=%d.",
			      year,month,day);
#endif
	/* check parameters */
	if(mjd == NULL)
	{
		Astro_Error_Number = 1;
		sprintf(Astro_Error_String,"NGAT_Astro_Year_Month_Day_To_MJD: MJD address was NULL.");
		return FALSE;
	}
	if(year < -4712)
	{
		Astro_Error_Number = 2;
		sprintf(Astro_Error_String,"NGAT_Astro_Year_Month_Day_To_MJD: Year %d out of range.",year);
		return FALSE;
	}
	if((month < 1)||(month > 12))
	{
		Astro_Error_Number = 3;
		sprintf(Astro_Error_String,"NGAT_Astro_Year_Month_Day_To_MJD: Month %d out of range (1..12).",month);
		return FALSE;
	}
	/* day validation */
	if(month == 2)
	{
		if(((year % 4 ) == 0 ) && (((year % 100) != 0) || ((year % 400) == 0)))
			month_day_count[month-1] = 29;
		else
			month_day_count[month-1] = 28;
	}
	if((day < 0)||(day > month_day_count[month-1]))
	{
		Astro_Error_Number = 4;
		sprintf(Astro_Error_String,"NGAT_Astro_Year_Month_Day_To_MJD: Day %d out of range (1..%d).",
			day,month_day_count[month-1]);
		return FALSE;
	}
	/* note year is equivalent to a, month is equivalent to m in Hatcher's algorithm. */
	/* a_dash is the same as the year for march to december, it is one less than the year in January and
	** February. a_dash is a march centred year. (1) */
	a_dash = year-((12-month)/10);
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:a_dash = %d.",a_dash);
#endif
	/* m_dash is a month number based on a march centred year (2) */
	m_dash = (month+9)%12;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:m_dash = %d.",m_dash);
#endif
	/* (5) */
	y = (int)((365.25 * (((double)a_dash) + 4712.0)));
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:y = %d.",y);
#endif
	/* (6) */
	d1 = (30.6 * ((double)m_dash)) + 0.5;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:d1 = %.3f.",d1);
#endif
	d = (int)d1;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:d = %d.",d);
#endif
	/* (7) */
	N = y + d + day + 59;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:N = %d.",N);
#endif
	/* Calculate Gregorian Offset g (9) */
	g1 = ((double)a_dash) / 100.0;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:g1 = %.2f.",g1);
#endif
	g2 = (int)(g1 + 49.0);
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:g2 = %d.",g2);
#endif
	g3 = (((double)g2) * 0.75);
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:g3 = %.2f.",g3);
#endif
	g = ((int)g3)-38;
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:g = %d.",g);
#endif
	/* Julian date is N - g. But is this starting at 12 H?
	** Modified Julian date is JD - 2400000.5  */
	(*mjd) = floor(N - g - 2400000.5);
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Year_Month_Day_To_MJD:returning MJD = %.3f.",(*mjd));
#endif
	return TRUE;
}

/**
 * Routine to calculate the day fraction, given the current time within the day.
 * @param hours The number of hours, from 0 to 23.
 * @param minutes The number of minutes, in the range (0..59).
 * @param seconds The number of seconds, in the range (0..61) (to allow for leap seconds).
 * @param nano_seconds The number of nano_seconds, in the range (0..1x10^9).
 * @param leap_second_correction A number representing whether a leap second will occur. This is normally zero,
 * 	which means no leap second will occur. It can be 1, which means the last minute of the day has 61 seconds,
 *	i.e. there are 86401 seconds in the day. It can be -1,which means the last minute of the day has 59 seconds,
 *	i.e. there are 86399 seconds in the day.
 * @param day_fraction The address of a double, to store the returned day fraction. This will return in the range
 *        (0..1).
 * @return The routine returns TRUE if it succeeds, FALSE if it fails. The library error code and
 *         error string are set if an error occurs.
 * @see Astro_Error_Number
 * @see Astro_Error_String
 */
int NGAT_Astro_Hour_Minute_Second_To_Day_Fraction(int hours,int minutes,int seconds,int nano_seconds,
						  int leap_second_correction,double *day_fraction)
{
	double seconds_in_day = 86400.0;
	double elapsed_seconds;

#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction(hours=%d,minutes=%d,"
			      "seconds=%d,nano_seconds=%d,leap second=%d.",
			      hours,minutes,seconds,nano_seconds,leap_second_correction);
#endif
/* check parameters */
	if((hours < 0)||(hours > 23))
	{
		Astro_Error_Number = 6;
		sprintf(Astro_Error_String,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction:"
			"Hours %d out of range (0,23).",hours);
		return FALSE;
	}
	if((minutes < 0)||(minutes > 59))
	{
		Astro_Error_Number = 7;
		sprintf(Astro_Error_String,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction:"
			"Minutes %d out of range (0,59).",minutes);
		return FALSE;
	}
	if((seconds < 0)||(seconds > 61))/* allow for leap seconds : See 'man gmtime' */
	{
		Astro_Error_Number = 8;
		sprintf(Astro_Error_String,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction:"
			"Seconds %d out of range (0,60).",seconds);
		return FALSE;
	}
	if((nano_seconds < 0)||(nano_seconds >= 1.0E+09))
	{
		Astro_Error_Number = 9;
		sprintf(Astro_Error_String,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction:"
			"Nanoseconds %d out of range (0,1x10^9).",nano_seconds);
		return FALSE;
	}
	if((leap_second_correction < -1) || (leap_second_correction > 1))
	{
		Astro_Error_Number = 10;
		sprintf(Astro_Error_String,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction:"
			"Leap Second correction %d out of range (-1,1).",leap_second_correction);
		return FALSE;
	}
	if(day_fraction == NULL)
	{
		Astro_Error_Number = 11;
		sprintf(Astro_Error_String,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction:Day fraction was NULL.");
		return FALSE;
	}
/* how many seconds were in the day */
	seconds_in_day = 86400.0;
	seconds_in_day += (double)leap_second_correction;
/* calculate the number of elapsed seconds in the day */
	elapsed_seconds = (double)seconds + (((double)nano_seconds) / 1.0E+09);
	elapsed_seconds += ((double)minutes) * 60.0;
	elapsed_seconds += ((double)hours) * 3600.0;
/* calculate day fraction */
	(*day_fraction) = elapsed_seconds / seconds_in_day;
	if(((*day_fraction) < 0.0) || ((*day_fraction) > 1.0))
	{
		Astro_Error_Number = 12;
		sprintf(Astro_Error_String,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction:"
			"Calculated Day fraction %.2f out of range (0..1).",(*day_fraction));
		return FALSE;
	}
#if LOGGING > 9
	NGAT_Astro_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"NGAT_Astro_Hour_Minute_Second_To_Day_Fraction returns "
			      "day_fraction %.3f.",day_fraction);
#endif
	return TRUE;
}
