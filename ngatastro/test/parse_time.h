/* parse_time.h
*/

#ifndef PARSE_TIME_H
#define PARSE_TIME_H
/**
 * @file
 * @brief Header file containing the external API for common routines for the ngat astrometry library test programs. 
 * @author Chris Mottram
 * @version $Revision$
 */

#include <time.h>

/* external functions */
extern int Parse_Time(char *date_string,struct timespec *time);

#endif
