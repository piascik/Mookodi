/* ngat_astro.c
** Next Generation Astronomical Telescope Astrometric library.
*/
/**
 * @file
 * @brief ngat_astro.c contains general routines for the NGAT astrometry library.
 * @author Chris Mottram
 * @version $Id$
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
#include "ngat_astro.h"

/* hash definitions */

/* external variables */
/**
 * External Variable holding error code of last operation performed by the NGAT astro library.
 */
int Astro_Error_Number = 0;
/**
 * External variable holding description of the last error that occured.
 * @see #NGAT_ASTRO_ERROR_STRING_LENGTH
 */
char Astro_Error_String[NGAT_ASTRO_ERROR_STRING_LENGTH] = "";

/* internal data types */
/**
 * Data type holding local data to ngat_astro.
 * @see #NGAT_Astro_Log
 * @see #NGAT_Astro_Set_Log_Filter_Level
 * @see #NGAT_Astro_Log_Filter_Level_Absolute
 * @see #NGAT_Astro_Log_Filter_Level_Bitwise
 */
struct Astro_Struct
{
	/** Function pointer to the routine that will log messages passed to it. */
	void (*Astro_Log_Handler)(int level,char *string);
	/** Function pointer to the routine that will filter log messages passed to it.
	 *  The funtion will return TRUE if the message should be logged, and FALSE if it shouldn't. */
	int (*Astro_Log_Filter)(int level,char *string);
	/** A globally maintained log filter level. 
	 * This is set using NGAT_Astro_Set_Log_Filter_Level.
	 * NGAT_Astro_Log_Filter_Level_Absolute and NGAT_Astro_Log_Filter_Level_Bitwise test it against
	 * message levels to determine whether to log messages. */
	int Astro_Log_Filter_Level;
};

/* internal data */
/**
 * The instance of Astro_Struct that contains local data for this module.
 * This is statically initialised to the following:
 * <dl>
 * <dt>Astro_Log_Handler</dt> <dd>NULL</dd>
 * <dt>Astro_Log_Filter</dt> <dd>NULL</dd>
 * <dt>Astro_Log_Filter_Level</dt> <dd>0</dd>
 * </dl>
 * @see #Astro_Struct
 */
static struct Astro_Struct Astro_Data = 
{
	NULL,NULL,0
};

/**
 * General buffer used for string formatting during logging.
 * @see #NGAT_ASTRO_ERROR_STRING_LENGTH
 */
static char Astro_Buff[NGAT_ASTRO_ERROR_STRING_LENGTH];

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * A general error routine. 
 * <b>Note</b> you cannot call both NGAT_Astro_Error and NGAT_Astro_Error_String to print the error string and 
 * get a string copy of it, only one of the error routines can be called after libngatastro has generated an error.
 * A second call to one of these routines will generate a 'Error not found' error!.
 * @see #NGAT_Astro_Get_Current_Time_String
 * @see #Astro_Error_Number
 * @see #Astro_Error_String
 */
void NGAT_Astro_Error(void)
{
	char time_string[32];
	int found = FALSE;

	if(Astro_Error_Number != 0)
	{
		found = TRUE;
		NGAT_Astro_Get_Current_Time_String(time_string,32);
		fprintf(stderr,"%s NGAT_Astro:Error(%d) : %s\n",time_string,Astro_Error_Number,Astro_Error_String);
	}
	if(!found)
	{
		fprintf(stderr,"Error:NGAT_Astro_Error:Error not found\n");
	}
}

/**
 * A general error routine. 
 * <b>Note</b> you cannot call both NGAT_Astro_Error and NGAT_Astro_Error_String to print the error string and 
 * get a string copy of it, only one of the error routines can be called after libngatastro has generated an error.
 * A second call to one of these routines will generate a 'Error not found' error!.
 * @param error_string A character buffer big enough to store the longest possible error message. It is
 * recomended that it is at least 1024 bytes in size.
 * @see #NGAT_Astro_Get_Current_Time_String
 * @see #Astro_Error_Number
 * @see #Astro_Error_String
 */
void NGAT_Astro_Error_String(char *error_string)
{
	char time_string[32];

	strcpy(error_string,"");
	if(Astro_Error_Number != 0)
	{
		NGAT_Astro_Get_Current_Time_String(time_string,32);
		sprintf(error_string+strlen(error_string),"%s NGAT_Astro:Error(%d) : %s\n",time_string,
			Astro_Error_Number,Astro_Error_String);
	}
	if(strlen(error_string) == 0)
	{
		strcat(error_string,"Error:NGAT_Astro_Error:Error not found\n");
	}
}

/**
 * Routine to get the current time in a string. The string is returned in the format
 * '01/01/2000 13:59:59', or the string "Unknown time" if the routine failed.
 * The time is in UTC.
 * @param time_string The string to fill with the current time.
 * @param string_length The length of the buffer passed in. It is recommended the length is at least 20 characters.
 */
void NGAT_Astro_Get_Current_Time_String(char *time_string,int string_length)
{
	time_t current_time;
	struct tm utc_time;

	if(time(&current_time) > -1)
	{
		gmtime_r(&current_time,&utc_time);
		strftime(time_string,string_length,"%d/%m/%Y %H:%M:%S",&utc_time);
	}
	else
		strncpy(time_string,"Unknown time",string_length);
}

/**
 * Routine to log a message to a defined logging mechanism. This routine has an arbitary number of arguments,
 * and uses vsprintf to format them i.e. like fprintf. The Astro_Buff is used to hold the created string,
 * therefore the total length of the generated string should not be longer than NGAT_ASTRO_ERROR_STRING_LENGTH.
 * NGAT_Astro_Log is then called to handle the log message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param format A string, with formatting statements the same as fprintf would use to determine the type
 * 	of the following arguments.
 * @see #NGAT_Astro_Log
 * @see #Astro_Buff
 * @see #NGAT_ASTRO_ERROR_STRING_LENGTH
 */
void NGAT_Astro_Log_Format(int level,char *format,...)
{
	va_list ap;

/* format the arguments */
	va_start(ap,format);
	vsprintf(Astro_Buff,format,ap);
	va_end(ap);
/* call the log routine to log the results */
	NGAT_Astro_Log(level,Astro_Buff);
}

/**
 * Routine to log a message to a defined logging mechanism. If the string or Astro_Data.Astro_Log_Handler are NULL
 * the routine does not log the message. If the Astro_Data.Astro_Log_Filter function pointer is non-NULL, the
 * message is passed to it to determoine whether to log the message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param string The message to log.
 * @see #Astro_Data
 */
void NGAT_Astro_Log(int level,char *string)
{
/* If the string is NULL, don't log. */
	if(string == NULL)
		return;
/* If there is no log handler, return */
	if(Astro_Data.Astro_Log_Handler == NULL)
		return;
/* If there's a log filter, check it returns TRUE for this message */
	if(Astro_Data.Astro_Log_Filter != NULL)
	{
		if(Astro_Data.Astro_Log_Filter(level,string) == FALSE)
			return;
	}
/* We can log the message */
	(*Astro_Data.Astro_Log_Handler)(level,string);
}

/**
 * Routine to set the Astro_Data.Astro_Log_Handler used by NGAT_Astro_Log.
 * @param log_fn A function pointer to a suitable handler.
 * @see #Astro_Data
 * @see #NGAT_Astro_Log
 */
void NGAT_Astro_Set_Log_Handler_Function(void (*log_fn)(int level,char *string))
{
	Astro_Data.Astro_Log_Handler = log_fn;
}

/**
 * Routine to set the Astro_Data.Astro_Log_Filter used by NGAT_Astro_Log.
 * @param filter_fn A function pointer to a suitable filter function.
 * @see #Astro_Data
 * @see #NGAT_Astro_Log
 */
void NGAT_Astro_Set_Log_Filter_Function(int (*filter_fn)(int level,char *string))
{
	Astro_Data.Astro_Log_Filter = filter_fn;
}

/**
 * A log handler to be used for the Astro_Data.Astro_Log_Handler function.
 * Just prints the message to stdout, terminated by a newline.
 * @param level The log level for this message.
 * @param string The log message to be logged. 
 */
void NGAT_Astro_Log_Handler_Stdout(int level,char *string)
{
	if(string == NULL)
		return;
	fprintf(stdout,"%s\n",string);
}

/**
 * Routine to set the Astro_Data.Astro_Log_Filter_Level.
 * @see #Astro_Data
 */
void NGAT_Astro_Set_Log_Filter_Level(int level)
{
	Astro_Data.Astro_Log_Filter_Level = level;
}

/**
 * A log message filter routine, to be used for the Astro_Data.Astro_Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level is less than or equal to the Astro_Data.Astro_Log_Filter_Level,
 * 	otherwise it returns FALSE.
 * @see #Astro_Data
 */
int NGAT_Astro_Log_Filter_Level_Absolute(int level,char *string)
{
	return (level <= Astro_Data.Astro_Log_Filter_Level);
}

/**
 * A log message filter routine, to be used for the Astro_Data.Astro_Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level has bits set that are also set in the 
 * 	Astro_Data.Astro_Log_Filter_Level, otherwise it returns FALSE.
 * @see #Astro_Data
 */
int NGAT_Astro_Log_Filter_Level_Bitwise(int level,char *string)
{
	return ((level & Astro_Data.Astro_Log_Filter_Level) > 0);
}
