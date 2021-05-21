/** @file   mkd_log.cpp
  *
  * @brief  Logging functions 
  *
  * @author asp
  *
  * @date   2021-05-21
  *
  * @version $Id$
  */

#include "mkd.h"

using std::cout;
using std::cerr;
using std::endl;
using std::exception;
using namespace log4cxx;
static LoggerPtr logger(Logger::getLogger("mookodi.instrument.server.Logging"));


/** @brief     OBSOLETE: Use new mkd_log() instead
  *            Log message to screen. 
  *            Intended to be used in-line, returning true | false.
  *
  * @param[in] ret = boolean return value
  * @param[in] lvl = severity 
  * @param[in] fac = facility ID
  * @param[in] fmt = vprintf(...) format and data to be displayed
  *
  * @return    ret = value passed in for return by this function
  */
int mkd_log_obsolete( int ret, int lvl, int fac, const char *fmt, ... )
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

//      Reset screen colour if not writing to a file 
        if ( log_colour[ lvl ] && log_fp == stdout )
            fprintf( log_fp, "%s", COL_RESET );

//      Force output
        fflush( log_fp );
    }
    return ret;
}


/** @brief     Wrapper to log to log4cxx. 
  *            Used in-line, returning true | false.
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
    char  str[1024];   

//  Only output messages for current log level and facility   
    if ((log_lvl < 0 && abs(log_lvl) == fac)||  // -ve == Facility
        (    lvl <= log_lvl                )  ) // +ve == Level
    {
	va_start( args, fmt );

//      Print log string and pass to standard logger
        sprintf( str, fmt, args );
        log_to_log4cxx( (char *)PROC_NAME, fac_lvls[fac], NULL, LOG_VERBOSITY_TERSE, log_lvls[lvl], str); 
    }

    return ret;
}


/** From CJM
 * C function conforming to the CCD library logging interface.
 * This causes messages to be logged to cout, in the form:
 *   "function : string".
 * @param sub_system The sub system. Can be NULL.
 * @param source_filename The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param string The log message to be logged. 
 */
static void log_to_stdout(char *sub_system, char *source_filename, char *function,
			  int level, char *category, char *string)
{
	cout << function << ":" << string << endl;
}


/** From CJM
 * A C function conforming to the CCD library logging interface. This causes messages to be logged to log4cxx logger ,
 * in the form:
 * category << ":" << sub_system << ":" << source_filename << ":" << "function : string".
 * Log levels LOG_VERBOSITY_VERY_TERSE,LOG_VERBOSITY_TERSE and  
 * LOG_VERBOSITY_INTERMEDIATE are logged using LOG4CXX_INFO,
 * log level LOG_VERBOSITY_VERBOSE is logged as LOG4CXX_DEBUG and LOG_VERBOSITY_VERY_VERBOSE is logged as LOG4CXX_TRACE.
 * @param sub_system The sub system. Can be NULL.
 * @param source_filename The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param string The log message to be logged. 
 * @see logger
 * @see LOG4CXX_INFO
 * @see LOG4CXX_DEBUG
 * @see LOG4CXX_TRACE
 * @see LOG_VERBOSITY_VERY_TERSE
 * @see LOG_VERBOSITY_TERSE
 * @see LOG_VERBOSITY_INTERMEDIATE
 * @see LOG_VERBOSITY_VERBOSE
 * @see LOG_VERBOSITY_VERY_VERBOSE
 */
static void log_to_log4cxx(char *sub_system, char *source_filename, char *function,
			   int level, char *category, char *string)
{
	switch(level)
	{
		case LOG_VERBOSITY_VERY_TERSE:
		case LOG_VERBOSITY_TERSE:
		case LOG_VERBOSITY_INTERMEDIATE:
			LOG4CXX_INFO(logger,category << ":" << sub_system << ":" << source_filename <<
				     ":" << function << ":" << string);
			break;
		case LOG_VERBOSITY_VERBOSE:
			LOG4CXX_DEBUG(logger,category << ":" << sub_system << ":" << source_filename <<
				     ":" << function << ":" << string);
			break;
		case LOG_VERBOSITY_VERY_VERBOSE:
			LOG4CXX_TRACE(logger,category << ":" << sub_system << ":" << source_filename <<
				     ":" << function << ":" << string);
			break;
		default:
			LOG4CXX_TRACE(logger,category << ":" << sub_system << ":" << source_filename <<
				     ":" << function << ":" << string);
			break;
	}
}
