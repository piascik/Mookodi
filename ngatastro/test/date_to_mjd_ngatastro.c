/* date_to_mjd_ngatastro.c
*/
/**
 * @file
 * @brief Test program, that uses libngatastro to convert an input date into a modified Julian Date
 * <pre>
 * date_to_mjd_ngatastro &lt;date&gt;
 * </pre>
 * The input date is in the format : YYYY-MM-DDThh:mm:ss.sss
 * i.e. 2003-02-13T19:33:12.661
 * The special keyword "now" uses the current system time.
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
#include "ngat_astro_mjd.h"
#include "parse_time.h"

/* internal variables */

/* internal functions */

/* external routines */
/**
 * Main program. Reads a date argument, converts to a struct timespec, and calls NGAT_Astro_Timespec_To_MJD
 * to get an MJD, which is printed out.
 * @param argc The number of arguments, should be 2.
 * @param argv The string array of arguments.
 * @return The program returns zero on success, and non-zero to indicate a failure.
 * @see Parse_Time
 * @see NGAT_Astro_Timespec_To_MJD
 */
int main(int argc,char *argv[])
{
	struct timespec time;
	double mjd;
	int retval;

	if(argc != 2)
	{
		fprintf(stderr,"%s <date>.\n",argv[0]);
		fprintf(stderr,"Date in the form of:YYYY-MM-DDThh:mm:ss.sss.\n");
		fprintf(stderr,"Or use 'now' for current system time.\n");
		return 1;
	}
	if(strcmp(argv[1],"now") == 0)
	{
		clock_gettime(CLOCK_REALTIME,&time);
		fprintf(stdout,"Time parsed as:%s.%3d\n",ctime(&(time.tv_sec)),
			(time.tv_nsec/NGAT_ASTRO_ONE_MILLISECOND_NS));
	}
	else
	{
		if(Parse_Time(argv[1],&time) == FALSE)
		{
			return 2;
		}
	}
	/*NGAT_Astro_Set_Log_Handler_Function(NGAT_Astro_Log_Handler_Stdout);*/
	retval = NGAT_Astro_Timespec_To_MJD(time,0,&mjd);
	if(retval != TRUE)
	{
		NGAT_Astro_Error();
		return 3;
	}
	/*	fprintf(stdout,"%.3f\n",mjd);*/
	fprintf(stdout,"%.8f\n",mjd);
	return 0;
}

