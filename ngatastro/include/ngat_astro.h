/* ngat_astro.h
** $Header: /home/dev/src/ngatastro/include/RCS/ngat_astro.h,v 1.3 2009/02/06 11:21:26 cjm Exp $
*/

#ifndef NGAT_ASTRO_H
#define NGAT_ASTRO_H
/**
 * @file
 * @brief ngat_astro.h contains the externally declared API for general routines in the NGATAstro library 
 *        (logging/error handling etc).
 * @author Chris Mottram
 * @version $Id$
 */
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
#define NGAT_ASTRO_IS_BOOLEAN(value)	(((value) == TRUE)||((value) == FALSE))

/**
 * This is the length of error string of modules in the library.
 */
#define NGAT_ASTRO_ERROR_STRING_LENGTH	256

/**
 * The number of nanoseconds in one second. A struct timespec has fields in nanoseconds.
 */
#define NGAT_ASTRO_ONE_SECOND_NS	(1000000000)
/**
 * The number of nanoseconds in one millisecond. A struct timespec has fields in nanoseconds.
 */
#define NGAT_ASTRO_ONE_MILLISECOND_NS	(1000000)
/**
 * The number of milliseconds in one second.
 */
#define NGAT_ASTRO_ONE_SECOND_MS	(1000)
/**
 * The number of nanoseconds in one microsecond.
 */
#define NGAT_ASTRO_ONE_MICROSECOND_NS	(1000)

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

/* external functions */

extern void NGAT_Astro_Error(void);
extern void NGAT_Astro_Error_String(char *error_string);

/* routine used by other modules error code */
extern void NGAT_Astro_Get_Current_Time_String(char *time_string,int string_length);

/* logging routines */
extern void NGAT_Astro_Log_Format(int level,char *format,...);
extern void NGAT_Astro_Log(int level,char *string);
extern void NGAT_Astro_Set_Log_Handler_Function(void (*log_fn)(int level,char *string));
extern void NGAT_Astro_Set_Log_Filter_Function(int (*filter_fn)(int level,char *string));
extern void NGAT_Astro_Log_Handler_Stdout(int level,char *string);
extern void NGAT_Astro_Set_Log_Filter_Level(int level);
extern int NGAT_Astro_Log_Filter_Level_Absolute(int level,char *string);
extern int NGAT_Astro_Log_Filter_Level_Bitwise(int level,char *string);

#endif
