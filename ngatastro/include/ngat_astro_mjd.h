/* ngat_astro_mjd.h
** $Header: /home/dev/src/ngatastro/include/RCS/ngat_astro_mjd.h,v 1.2 2006/05/16 18:52:29 cjm Exp $
*/

#ifndef NGAT_ASTRO_MJD_H
#define NGAT_ASTRO_MJD_H
/**
 * @file
 * @brief ngat_astro_mjd.h contains the externally declared API for the MJD (Modified Julian Date) 
 *        routines in the NGATAstro library.
 * @author Chris Mottram
 * @version $Id$
 */

/* hash defines */

/* external functions */
extern int NGAT_Astro_Timespec_To_MJD(struct timespec time,int leap_second_correction,double *mjd);
extern int NGAT_Astro_Year_Month_Day_To_MJD(int year,int month,int day,double *mjd);
extern int NGAT_Astro_Hour_Minute_Second_To_Day_Fraction(int hours,int minutes,int seconds,int nano_seconds,
						  int leap_second_correction, double *day_fraction);

#endif
