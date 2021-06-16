/* date_to_ms.c
*/
/**
 * @file
 * @brief Test program, that converts an input date into a the number of milliseconds since the epoch.
 * <pre>
 * date_to_ms &lt;date&gt;
 * </pre>
 * The input date is in the format : YYYY-MM-DDThh:mm:ss.sss
 * i.e. 2003-02-13T19:33:12.661
 * The special keyword "now" uses the current system time.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
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
#include <string.h>
#include <time.h>
#include "ngat_astro.h"
#include "parse_time.h"

/* internal variables */

/* internal functions */

/* external routines */
/**
 * Main program. Reads a date argument, converts to a struct timespec, and prints out the number of milliseconds
 * since the epoch.
 * @param argc The number of arguments, should be 2.
 * @param argv The string array of arguments.
 * @return The program returns zero on success, and non-zero to indicate a failure.
 * @see Parse_Time
 * @see NGAT_ASTRO_ONE_SECOND_MS
 * @see NGAT_ASTRO_ONE_MILLISECOND_NS
 */
int main(int argc,char *argv[])
{
	struct timespec time;
	int retval;
	double msd;

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
			(int)(time.tv_nsec/NGAT_ASTRO_ONE_MILLISECOND_NS));
	}
	else
	{
		if(Parse_Time(argv[1],&time) == FALSE)
		{
			return 2;
		}
	}
	/*
	fprintf(stdout,"secs since epoch: %lu\n",time.tv_sec);
	fprintf(stdout,"nsecs: %lu\n",time.tv_nsec);
	fprintf(stdout,"nsecs is ms: %lu\n",time.tv_nsec/NGAT_ASTRO_ONE_MILLISECOND_NS);
	fprintf(stdout,"nsecs is ms(double): %.0lf\n",((double)(time.tv_nsec/(double)NGAT_ASTRO_ONE_MILLISECOND_NS)));
	*/
	msd = ((double)((double)time.tv_sec*(double)NGAT_ASTRO_ONE_SECOND_MS))+((double)(time.tv_nsec/(double)NGAT_ASTRO_ONE_MILLISECOND_NS));
	fprintf(stdout,"%.0lf\n",msd);
	return 0;
}
