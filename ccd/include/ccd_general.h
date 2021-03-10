/* ccd_general.h */
#ifndef CCD_GENERAL_H
#define CCD_GENERAL_H
/**
 * @file
 * @brief ccd_general.h contains the externally declared API for general routines in the CCD library 
 *        (logging/error handling etc).
 * @author Chris Mottram
 * @version $Id$
 */

#ifdef __cplusplus
extern "C" {
#endif

/* for timespec definition */
#include <time.h>

/* hash defines */
/**
 * TRUE is the value usually returned from routines to indicate success.
 */
#ifndef TRUE
#define TRUE 1
#endif
/**
 * FALSE is the value usually returned from routines to indicate failure.
 */
#ifndef FALSE
#define FALSE 0
#endif

/**
 * Macro to check whether the parameter is either TRUE or FALSE.
 */
#define CCD_GENERAL_IS_BOOLEAN(value)	(((value) == TRUE)||((value) == FALSE))

/**
 * This is the length of error string of modules in the library.
 */
#define CCD_GENERAL_ERROR_STRING_LENGTH	(1024)

/**
 * The number of nanoseconds in one second. A struct timespec has fields in nanoseconds.
 */
#define CCD_GENERAL_ONE_SECOND_NS	(1000000000)
/**
 * The number of nanoseconds in one millisecond. A struct timespec has fields in nanoseconds.
 */
#define CCD_GENERAL_ONE_MILLISECOND_NS	(1000000)
/**
 * The number of microseconds in one millisecond. The Andor library reports horizontal/vertical transfer speeds
 * in microseconds.
 */
#define CCD_GENERAL_ONE_MILLISECOND_MICROSECOND	(1000)
/**
 * The number of milliseconds in one second.
 */
#define CCD_GENERAL_ONE_SECOND_MS	(1000)
/**
 * The number of nanoseconds in one microsecond.
 */
#define CCD_GENERAL_ONE_MICROSECOND_NS	(1000)

/* enums */
/**
 * This enum describes a verbosity filtering level of a log message. The idea is that the high priority/
 * terse level messages are always displayed, whilst the detail/very verbose messages can be filtered out.
 * This enum copied from log_udp.h, so we can remove log_udp dependancy from mookodi.
 * <dl>
 * <dt>LOG_VERBOSITY_VERY_TERSE</dt> <dd>High priority/top level message.</dd>
 * <dt>LOG_VERBOSITY_TERSE</dt> <dd> Higher priority message.</dd>
 * <dt>LOG_VERBOSITY_INTERMEDIATE</dt> <dd>Intermediate level message.</dd>
 * <dt>LOG_VERBOSITY_VERBOSE</dt> <dd>Lower priority/more detailed/verbose message.</dd>
 * <dt>LOG_VERBOSITY_VERY_VERBOSE</dt> <dd>Lowest level/most verbose message.</dd>
 * </dl>
 */
enum LOG_VERBOSITY
{
	LOG_VERBOSITY_VERY_TERSE=1,
	LOG_VERBOSITY_TERSE=2,
	LOG_VERBOSITY_INTERMEDIATE=3,
	LOG_VERBOSITY_VERBOSE=4,
	LOG_VERBOSITY_VERY_VERBOSE=5
};

#ifndef fdifftime
/**
 * Return double difference (in seconds) between two struct timespec's.
 * @param t0 A struct timespec.
 * @param t1 A struct timespec.
 * @return A double, in seconds, representing the time elapsed from t0 to t1.
 * @see #CCD_GENERAL_ONE_SECOND_NS
 */
#define fdifftime(t1, t0) (((double)(((t1).tv_sec)-((t0).tv_sec))+(double)(((t1).tv_nsec)-((t0).tv_nsec))/CCD_GENERAL_ONE_SECOND_NS))
#endif

/* external functions */
extern char* CCD_General_Andor_ErrorCode_To_String(unsigned int error_code);
extern void CCD_General_Error(void);
extern void CCD_General_Error_To_String(char *error_string);
extern int CCD_General_Is_Error(void);

/* routine used by other modules error code */
extern void CCD_General_Get_Current_Time_String(char *time_string,int string_length);
extern void CCD_General_Get_Time_String(struct timespec time,char *time_string,int string_length);

/* logging routines */
extern void CCD_General_Log_Format(char *sub_system,char *source_filename,char *function,int level,
				   char *category,char *format,...);
extern void CCD_General_Log(char *sub_system,char *source_filename,char *function,int level,
			    char *category,char *string);
extern void CCD_General_Set_Log_Handler_Function(void (*log_fn)(char *sub_system,char *source_filename,char *function,
								int level,char *category,char *string));
extern void CCD_General_Set_Log_Filter_Function(int (*filter_fn)(char *sub_system,char *source_filename,char *function,
								 int level,char *category,char *string));
extern void CCD_General_Log_Handler_Stdout(char *sub_system,char *source_filename,char *function,int level,
					   char *category,char *string);
extern void CCD_General_Set_Log_Filter_Level(int level);
extern int CCD_General_Log_Filter_Level_Absolute(char *sub_system,char *source_filename,char *function,int level,
						 char *category,char *string);

#ifdef __cplusplus
}
#endif

#endif
