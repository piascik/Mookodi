#include "mkd.h"

/** @brief     Log message to screen. 
  *            Intended to be used in-line, returning true | false.
  *
  * @param[in] ret = boolean return value
  * @param[in] lvl = severity 
  * @param[in] fac = facility ID
  * @param[in] fmt = vprintf(...) format and data to be displayed
  *
  * @return    ret = value passed in for return by this function
  */
int mkd_log( int ret, int lvl, int fac, const char *fmt, ... )
{
    va_list args;
    char    dtm[32]; // Day/time string, YYYY-MM-DDThh:mm:ss 
    char    sec[16]; // Fractional seconds, 0.nnn 

    struct timeval t; 

//  Only output messages for current log level    
    if ((log_lvl < 0 && abs(log_lvl) == fac)||  // -ve == Facility
        (    lvl <= log_lvl                )  ) // +ve == Level
    {
	va_start( args, fmt );

//      Get timestamp and check if fractional part rounds up 
        gettimeofday( &t, NULL );
        if (t.tv_usec >= 950000)
        {
            t.tv_sec++;           // round up integer seconds
            t.tv_usec -= 950000;  // round down fractional part 
        }

//      Set screen colour when writing to screen 
        if ( log_colour[ lvl ] && log_fp == stdout )            
            fprintf( log_fp, "%s", log_colour[ lvl ] );

//      Create timestamp string
	strftime( dtm, sizeof(dtm)-1, "%Y-%m-%dT%H:%M:%S", localtime(&t.tv_sec));
        snprintf( sec, sizeof(sec)-1, "%2.3f", t.tv_usec/TIM_MICROSECOND );

//      Print log line
//      <prefix> YYYY-MM-DDThh:mm:ss.sss <log-level>: <facility> <message ...> <OK | Fail> 
        fprintf ( log_fp, "%s%s%s %s: %s %-4.4i ", log_pfx, dtm, &sec[1], log_lvls[lvl], fac_lvls[fac], ret  );
        vfprintf( log_fp, fmt, args );
        fprintf ( log_fp, "\n"); 
//        fprintf ( log_fp, " %s", ret ? "OK\n":"Fail\n"); 

//      Reset screen colour if not writing to a file 
        if ( log_colour[ lvl ] && log_fp == stdout )
            fprintf( log_fp, "%s", COL_RESET );

//      Force output
        fflush( log_fp );
    }
    return ret;
}

