/* ccd_exposure.c
** Low level ccd library
*/
/**
 * @file
 * @brief ccd_exposure.c contains routines for performing an exposure with the CCD Controller.
 * @author Chris Mottram
 * @version $Id$
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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef _POSIX_TIMERS
#include <sys/time.h>
#endif
#include <time.h>
#include "atmcdLXd.h"
#include "fitsio.h"
#include "ccd_general.h"
#include "ccd_exposure.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

/* hash defines */
/**
 * Number of seconds to wait after an exposure is meant to have finished, before we abort with a timeout signal.
 */
#define EXPOSURE_TIMEOUT_SECS     (30.0)

/* data types */
/**
 * Structure used to hold local data to ccd_exposure.
 * <dl>
 * <dt>Exposure_Status</dt> <dd>Whether an operation is being performed to CLEAR, EXPOSE or READOUT the CCD.</dd>
 * <dt>Start_Time</dt> <dd>The time stamp when an exposure was started.</dd>
 * <dt>Exposure_Length</dt> <dd>The last exposure length to be set (ms).</dd>
 * <dt>Abort</dt> <dd>Whether to abort an exposure.</dd>
 * <dt>Exposure_Index</dt> <dd>The current exposure being done.</dd>
 * <dt>Exposure_Count</dt> <dd>The number of exposures to be done in the MULTRUN.</dd>
 * <dt>Accumulation</dt> <dd>The current Andor accumulation.</dd>
 * <dt>Series</dt> <dd>The current Andor series.</dd>
 * <dt>Exposure_Loop_Pause_Length</dt> <dd>An amount of time to pause/sleep, in milliseconds, each time
 *     round the loop whilst waiting for an exposure to be done (DRV_ACQUIRING -> DRV_IDLE).
 * </dl>
 * @see #CCD_EXPOSURE_STATUS
 */
struct Exposure_Struct
{
	enum CCD_EXPOSURE_STATUS Exposure_Status;
	struct timespec Start_Time;
	volatile int Exposure_Length;
	volatile int Abort;/* This is volatile as a different thread may change this variable. */
	volatile int Exposure_Index;
	volatile int Exposure_Count;
	volatile int Accumulation;
	volatile int Series;
	int Exposure_Loop_Pause_Length;
};

/* external variables */

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Data holding the current status of ccd_exposure.
 * @see #Exposure_Struct
 * @see #CCD_EXPOSURE_STATUS
 */
static struct Exposure_Struct Exposure_Data = 
{
	CCD_EXPOSURE_STATUS_NONE,
	{0L,0L},
	0,FALSE,
	-1,-1,-1,-1,
	1
};

/**
 * Variable holding error code of last operation performed by ccd_exposure.
 */
static int Exposure_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Exposure_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */
static int Exposure_Wait_For_Start_Time(struct timespec start_time);
static void Exposure_Debug_Buffer(char *description,unsigned short *buffer,size_t buffer_length);
static int fexist(char *filename);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * This routine sets up ccd_exposure internal variables.
 * It should be called at startup.
 */
void CCD_Exposure_Initialise(void)
{
	Exposure_Error_Number = 0;
/* print some compile time information to stdout */
	fprintf(stdout,"CCD_Exposure_Initialise:%s.\n",rcsid);
}

/**
 * Do an exposure.
 * @param open_shutter A boolean, TRUE to open the shutter, FALSE to leave it closed (dark).
 * @param start_time The time to start the exposure. If both the fields in the <i>struct timespec</i> are zero,
 * 	the exposure can be started at any convenient time.
 * @param exposure_length The length of time to open the shutter for in milliseconds. This must be greater than zero.
 * @param buffer A pointer to a previously allocated area of memory, of length buffer_length. This should have the
 *        correct size to save the read out image into.
 * @param buffer_length The length of the buffer in <b>pixels</b>.
 * @return Returns TRUE if the exposure succeeds and the data read out into the buffer, returns FALSE if an error
 *	occurs or the exposure is aborted.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see #Exposure_Debug_Buffer
 * @see CCD_General_Log
 * @see CCD_General_Andor_ErrorCode_To_String
 * @see CCD_Setup_Get_Buffer_Length
 */
int CCD_Exposure_Expose(int open_shutter,struct timespec start_time,int exposure_length,
			void *buffer,size_t buffer_length)
{
	struct timespec sleep_time,current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	at_u32 andor_pixel_count;
	size_t pixel_count;
	unsigned int andor_retval;
	int accumulation,series,exposure_status,acquisition_counter,done;

	Exposure_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_INTERMEDIATE,NULL,
			"CCD_Exposure_Expose started.");
#endif
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_INTERMEDIATE,NULL,
			       "CCD_Exposure_Expose(open_shutter=%d,start_time=%ld,exposure_length=%d,buffer=%p,"
			       "buffer_length=%ld).",open_shutter,start_time.tv_sec,exposure_length,
			       buffer,buffer_length);
#endif
	/* set acquisition mode to single scan */
#if LOGGING > 0
	CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_VERBOSE,"ANDOR",
		       "CCD_Exposure_Expose:SetAcquisitionMode(1):single scan.");
#endif
	andor_retval = SetAcquisitionMode(1);
	if(andor_retval != DRV_SUCCESS)
	{
		Exposure_Error_Number = 38;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:SetAcquisitionMode(1) failed %d(%s).",
			andor_retval,CCD_General_Andor_ErrorCode_To_String(andor_retval));
		return FALSE;
	}
	/* set shutter */
	if(open_shutter)
	{
#if LOGGING > 5
		CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_INTERMEDIATE,"ANDOR",
				"SetShutter(1,0,0,0).");
#endif
		andor_retval = SetShutter(1,0,0,0);
		if(andor_retval != DRV_SUCCESS)
		{
			Exposure_Error_Number = 6;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose: SetShutter() failed %s(%u).",
				CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
			return FALSE;
		}
	}
	else
	{
#if LOGGING > 5
		CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_INTERMEDIATE,"ANDOR",
				"SetShutter(1,2,0,0).");
#endif
		andor_retval = SetShutter(1,2,0,0);/* 2 means close */
		if(andor_retval != DRV_SUCCESS)
		{
			Exposure_Error_Number = 7;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose: SetShutter() failed %s(%u).",
				CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
			return FALSE;
		}
	}
	/* set exposure length */
	Exposure_Data.Exposure_Length = exposure_length;
	/* set other parameters used for status reporting */
	Exposure_Data.Exposure_Count = 1;
	Exposure_Data.Exposure_Index = 0;
	Exposure_Data.Accumulation = -1;
	Exposure_Data.Series = -1;
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_INTERMEDIATE,"ANDOR",
			"SetExposureTime(%.2f).",((float)exposure_length)/1000.0f);
#endif
	andor_retval = SetExposureTime(((float)exposure_length)/1000.0f);/* in seconds */
	if(andor_retval != DRV_SUCCESS)
	{
		Exposure_Error_Number = 8;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose: SetExposureTime(%f) failed %s(%u).",
			(((float)exposure_length)/1000.0f),CCD_General_Andor_ErrorCode_To_String(andor_retval),
			andor_retval);
		return FALSE;
	}
	/* check buffer details */
	if(buffer == NULL)
	{
		Exposure_Error_Number = 9;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose: buffer was NULL.");
		return FALSE;
	}
	if(!CCD_Setup_Get_Buffer_Length(&pixel_count))
	{
		Exposure_Error_Number = 40;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose: CCD_Setup_Get_Buffer_Length failed.");
		return FALSE;
	}
	andor_pixel_count = (at_u32)pixel_count;
	if(buffer_length < pixel_count)
	{
		Exposure_Error_Number = 10;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose: buffer_length (%ld) was too small (%ld).",
			buffer_length,pixel_count);
		return FALSE;
	}
	/* reset abort */
	Exposure_Data.Abort = FALSE;
	/* wait for start_time, if applicable */
	if(start_time.tv_sec > 0)
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_WAIT_START;
		done = FALSE;
		while(done == FALSE)
		{
#ifdef _POSIX_TIMERS
			clock_gettime(CLOCK_REALTIME,&current_time);
#else
			gettimeofday(&gtod_current_time,NULL);
			current_time.tv_sec = gtod_current_time.tv_sec;
			current_time.tv_nsec = gtod_current_time.tv_usec*CCD_GLOBAL_ONE_MICROSECOND_NS;
#endif
#if LOGGING > 3
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERBOSE,NULL,
					       "Waiting for exposure start time (%ld,%ld).",
					       current_time.tv_sec,start_time.tv_sec);
#endif
		/* if we've time, sleep for a second */
			if((start_time.tv_sec - current_time.tv_sec) > 0)
			{
				sleep_time.tv_sec = 1;
				sleep_time.tv_nsec = 0;
				nanosleep(&sleep_time,NULL);
			}
			else
			{
				/* sleep for remaining sub-second time (if it exists!) */
				sleep_time.tv_sec = start_time.tv_sec - current_time.tv_sec;
				sleep_time.tv_nsec = start_time.tv_nsec - current_time.tv_nsec;
				if((sleep_time.tv_sec == 0)&&(sleep_time.tv_nsec > 0))
					nanosleep(&sleep_time,NULL);
				/* exit the wait to start loop */
				done = TRUE;
			}
		/* check - have we been aborted? */
			if(Exposure_Data.Abort)
			{
				Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
				Exposure_Error_Number = 11;
				sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted.");
				return FALSE;
			}
		}/* end while */
	}/* end if wait for start_time */
	/* start the exposure */
#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&(Exposure_Data.Start_Time));
#else
	gettimeofday(&gtod_current_time,NULL);
	Exposure_Data.Start_Time.tv_sec = gtod_current_time.tv_sec;
	Exposure_Data.Start_Time.tv_nsec = gtod_current_time.tv_usec*CCD_GLOBAL_ONE_MICROSECOND_NS;
#endif
	Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_EXPOSE;
#if LOGGING > 5
	CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_INTERMEDIATE,"ANDOR",
			"StartAcquisition().");
#endif
	andor_retval = StartAcquisition();
	if(andor_retval != DRV_SUCCESS)
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
		Exposure_Error_Number = 12;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose: StartAcquisition() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
	/* wait until acquisition complete */
	acquisition_counter = 0;
	do
	{
		/* sleep a (very small (configurable)) bit */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = Exposure_Data.Exposure_Loop_Pause_Length*CCD_GENERAL_ONE_MILLISECOND_NS;
		nanosleep(&sleep_time,NULL);
		/* get the status */
		andor_retval = GetStatus(&exposure_status);
		if(andor_retval != DRV_SUCCESS)
		{
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			Exposure_Error_Number = 13;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose: GetStatus() failed %s(%u).",
				CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
			return FALSE;
		}
#if LOGGING > 3
		if((acquisition_counter%1000)==0)
		{
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERBOSE,NULL,
					       "Current Acquisition Status after %d loops is %s(%u).",
					       acquisition_counter,
					       CCD_General_Andor_ErrorCode_To_String(exposure_status),
					       exposure_status);
		}
#endif
		acquisition_counter++;
		/* check - have we been aborted? */
		if(Exposure_Data.Abort)
	        {
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERBOSE,"ANDOR",
					       "Abort detected, attempting Andor AbortAcquisition.");
			andor_retval = AbortAcquisition();
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERBOSE,"ANDOR",
					       "AbortAcquisition() return %u.",andor_retval);
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			Exposure_Error_Number = 14;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted.");
			return FALSE;
		}
		/* timeout */
#ifdef _POSIX_TIMERS
		clock_gettime(CLOCK_REALTIME,&current_time);
#else
		gettimeofday(&gtod_current_time,NULL);
		current_time.tv_sec = gtod_current_time.tv_sec;
		current_time.tv_nsec = gtod_current_time.tv_usec*CCD_GLOBAL_ONE_MICROSECOND_NS;
#endif
#if LOGGING > 3
		if((acquisition_counter%1000)==0)
		{
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERBOSE,NULL,
					       "Start_Time = %s, current_time = %s, fdifftime = %.2f s,"
					       "difftime = %.2f s, exposure length = %d ms, timeout length = %.2f s, "
					       "is a timeout = %d.",ctime(&(Exposure_Data.Start_Time.tv_sec)),
					       ctime(&(current_time.tv_sec)),
					       fdifftime(current_time,Exposure_Data.Start_Time),
					       difftime(current_time.tv_sec,Exposure_Data.Start_Time.tv_sec),
					       Exposure_Data.Exposure_Length,
					       ((((double)Exposure_Data.Exposure_Length)/1000.0)+
						EXPOSURE_TIMEOUT_SECS),
					       fdifftime(current_time,Exposure_Data.Start_Time) > 
					       ((((double)Exposure_Data.Exposure_Length)/1000.0)+
						EXPOSURE_TIMEOUT_SECS));
		}
#endif
		if(fdifftime(current_time,Exposure_Data.Start_Time) >
		   ((((double)Exposure_Data.Exposure_Length)/1000.0)+EXPOSURE_TIMEOUT_SECS))
		{
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERBOSE,"ANDOR",
					       "Timeout detected, attempting Andor AbortAcquisition.");
			andor_retval = AbortAcquisition();
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERBOSE,"ANDOR",
					       "AbortAcquisition() return %u.",andor_retval);
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			Exposure_Error_Number = 15;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose:"
				"Timeout (Andor library stuck in DRV_ACQUIRING).");
			CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",
					       LOG_VERBOSITY_VERY_TERSE,NULL,
					       "Timeout (Andor library stuck in DRV_ACQUIRING).");
			return FALSE;
		}
	}
	while(exposure_status==DRV_ACQUIRING);
#if LOGGING > 3
	CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_VERBOSE,NULL,
			       "Acquisition Status after %d loops is %s(%u).",acquisition_counter,
			       CCD_General_Andor_ErrorCode_To_String(exposure_status),exposure_status);
#endif
	/* get data */
#if LOGGING > 3
	CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_VERBOSE,"ANDOR",
			       "Calling GetAcquiredData16(%p,%lu).",buffer,andor_pixel_count);
#endif
	Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_READOUT;
	andor_retval = GetAcquiredData16((unsigned short*)buffer,andor_pixel_count);
	if(andor_retval != DRV_SUCCESS)
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
		Exposure_Error_Number = 16;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose: GetAcquiredData16(%p,%u) failed %s(%u).",
			buffer,andor_pixel_count,CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
	Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
	Exposure_Data.Exposure_Index++;
	andor_retval = GetAcquisitionProgress(&(accumulation),&(series));
	if(andor_retval == DRV_SUCCESS)
	{
		Exposure_Data.Accumulation = accumulation;
		Exposure_Data.Series = series;
	}
#if LOGGING > 9
	Exposure_Debug_Buffer("CCD_Exposure_Expose",(unsigned short*)buffer,buffer_length);
#endif
#if LOGGING > 1
	CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Expose",LOG_VERBOSITY_INTERMEDIATE,NULL,
			"CCD_Exposure_Expose finished.");
#endif
	return TRUE;
}

/**
 * Take a bias frame.
 * @param buffer A pointer to a previously allocated area of memory, of length buffer_length. This should have the
 *        correct size to save the read out image into.
 * @param buffer_length The length of the buffer in <b>pixels</b>.
 * @return Returns TRUE if the exposure succeeds and the data read out into the buffer, returns FALSE if an error
 *	occurs or the exposure is aborted.
 * @see #CCD_Exposure_Expose
 */
int CCD_Exposure_Bias(void *buffer,size_t buffer_length)
{
	struct timespec start_time = {0,0};
	int retval;

	Exposure_Error_Number = 0;
	retval = CCD_Exposure_Expose(FALSE,start_time,0,buffer,buffer_length);
	return retval;
}

/**
 * This routine aborts an exposure currently underway, whether it is reading out or not.
 * @return Returns TRUE if the abort succeeds  returns FALSE if an error occurs.
 * @see #Exposure_Data
 */
int CCD_Exposure_Abort(void)
{
	Exposure_Error_Number = 0;
	Exposure_Data.Abort = TRUE;
	return TRUE;
}

/**
 * Get the current exposure status.
 * @return The current exposure status.
 * @see #CCD_EXPOSURE_STATUS
 * @see #Exposure_Data
 */
enum CCD_EXPOSURE_STATUS CCD_Exposure_Status_Get(void)
{
	return Exposure_Data.Exposure_Status;
}

/**
 * Get the current Andor accumulation.
 * @return The current Andor accumulation.
 * @see #Exposure_Data
 */
int CCD_Exposure_Accumulation_Get(void)
{
	return Exposure_Data.Accumulation;
}

/**
 * Get the current Andor series.
 * @return The current Andor series.
 * @see #Exposure_Data
 */
int CCD_Exposure_Series_Get(void)
{
	return Exposure_Data.Series;
}

/**
 * Get the current exposure index.
 * @return The current exposure index.
 * @see #Exposure_Data
 */
int CCD_Exposure_Index_Get(void)
{
	return Exposure_Data.Exposure_Index;
}

/**
 * Get the current exposure count.
 * @return The current exposure count. 
 * @see #Exposure_Data
 */
int CCD_Exposure_Count_Get(void)
{
	return Exposure_Data.Exposure_Count;
}

/**
 * Get the current exposure length.
 * @return The current exposure length, in milliseconds. 
 * @see #Exposure_Data
 */
int CCD_Exposure_Length_Get(void)
{
	return Exposure_Data.Exposure_Length;
}

/**
 * This routine gets the time stamp for the start of the exposure.
 * @param timespec The time stamp for the start of the exposure.
 * @return Returns TRUE on success, and FALSE if an error occurs.
 * @see #Exposure_Data
 */
int CCD_Exposure_Start_Time_Get(struct timespec* timespec)
{
	Exposure_Error_Number = 0;
	if(timespec == NULL)
	{
		Exposure_Error_Number = 17;
		sprintf(Exposure_Error_String,"CCD_Exposure_Start_Time_Get: timespec was NULL.");
		return FALSE;
	}
	(*timespec) = Exposure_Data.Start_Time;
	return TRUE;
}

/**
 * Save the exposure to disk.
 * @param filename The name of the file to save the image into. If it does not exist, it is created.
 * @param buffer Pointer to a previously allocated array of unsigned shorts containing the image pixel values.
 * @param buffer_length The length of the buffer in bytes.
 * @param ncols The number of binned image columns (the X size/width of the image).
 * @param nrows The number of binned image rows (the Y size/height of the image).
 * @param header A list of FITS header cards to write to the output filename's FITS header.
 * @return Returns TRUE on success, and FALSE if an error occurs.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see #Exposure_Debug_Buffer
 * @see CCD_General_Log
 * @see CCD_Fits_Header_Write_To_Fits
 * @see #fexist
 */
int CCD_Exposure_Save(char *filename,void *buffer,size_t buffer_length,int ncols,int nrows,
		      struct Fits_Header_Struct header)
{
	static fitsfile *fits_fp = NULL;
	char buff[32]; /* fits_get_errstatus returns 30 chars max */
	long axes[2];
	int status = 0,retval,ivalue;
	double dvalue;

#if LOGGING > 5
	CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Save",LOG_VERBOSITY_INTERMEDIATE,NULL,"started.");
#endif
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_exposure.c","CCD_Exposure_Save",LOG_VERBOSITY_INTERMEDIATE,NULL,
			       "Saving to '%s', buffer of length %ld with dimensions %d x %d.",filename,
			       buffer_length,ncols,nrows);
#endif
	/* check existence of FITS image and create or append as appropriate? */
	if(fexist(filename))
	{
		retval = fits_open_file(&fits_fp,filename,READWRITE,&status);
		if(retval)
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			Exposure_Error_Number = 1;
			sprintf(Exposure_Error_String,"CCD_Exposure_Save: File open failed(%s,%d,%s).",
				filename,status,buff);
			return FALSE;
		}
	}
	else
	{
		/* open file */
		if(fits_create_file(&fits_fp,filename,&status))
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			Exposure_Error_Number = 2;
			sprintf(Exposure_Error_String,"CCD_Exposure_Save: File create failed(%s,%d,%s).",
				filename,status,buff);
			return FALSE;
		}
		/* create image block */
		axes[0] = nrows;
		axes[1] = ncols;
		retval = fits_create_img(fits_fp,USHORT_IMG,2,axes,&status);
		if(retval)
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			fits_close_file(fits_fp,&status);
			Exposure_Error_Number = 3;
			sprintf(Exposure_Error_String,"CCD_Exposure_Save: Create image failed(%s,%d,%s).",
				filename,status,buff);
			return FALSE;
		}
	}
	/* write the FITS headers */
	if(!CCD_Fits_Header_Write_To_Fits(header,fits_fp))
	{
		Exposure_Error_Number = 41;
		sprintf(Exposure_Error_String,"CCD_Exposure_Save: Writing FITS headers to disk failed(%s).",
			filename);
		return FALSE;
	}
	/* debug whats in the buffer */
#if LOGGING > 9
	Exposure_Debug_Buffer("CCD_Exposure_Save",(unsigned short*)buffer,buffer_length);
#endif
	/* write the data */
	retval = fits_write_img(fits_fp,TUSHORT,1,ncols*nrows,buffer,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Exposure_Error_Number = 4;
		sprintf(Exposure_Error_String,"CCD_Exposure_Save: File write image failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
	/* diddly time stamp etc*/
	/* ensure data we have written is in the actual data buffer, not CFITSIO's internal buffers */
	/* closing the file ensures this. */ 
	retval = fits_close_file(fits_fp,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Exposure_Error_Number = 5;
		sprintf(Exposure_Error_String,"CCD_Exposure_Save: File close file failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log("ccd","ccd_exposure.c","CCD_Exposure_Save",LOG_VERBOSITY_INTERMEDIATE,NULL,"finished.");
#endif
	return TRUE;
}

/**
 * Get the current value of ccd_exposures's error number.
 * @return The current value of ccd_exposure's error number.
 * @see #Exposure_Error_Number
 */
int CCD_Exposure_Get_Error_Number(void)
{
	return Exposure_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_exposure in a standard way.
 * @see CCD_General_Get_Current_Time_String
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 */
void CCD_Exposure_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Exposure:Error(%d) : %s\n",time_string,Exposure_Error_Number,Exposure_Error_String);
}

/**
 * The error routine that reports any errors occuring in ccd_exposure in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see CCD_General_Get_Current_Time_String
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 */
void CCD_Exposure_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Exposure:Error(%d) : %s\n",time_string,
		Exposure_Error_Number,Exposure_Error_String);
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to wait until the specified start time.
 * @param start_time The start time.
 * @return The routine returns TRUE on success and FALSE for failure.
 * @see #Exposure_Data
 * @see #CCD_EXPOSURE_STATUS
 * @see CCD_GENERAL_ONE_SECOND_NS
 * @see CCD_GENERAL_ONE_MICROSECOND_NS
 */
static int Exposure_Wait_For_Start_Time(struct timespec start_time)
{
	struct timespec sleep_time,current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	long remaining_sec,remaining_ns;
	int done;

	if(start_time.tv_sec > 0)
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_WAIT_START;
		done = FALSE;
		while(done == FALSE)
		{
#ifdef _POSIX_TIMERS
			clock_gettime(CLOCK_REALTIME,&current_time);
#else
			gettimeofday(&gtod_current_time,NULL);
			current_time.tv_sec = gtod_current_time.tv_sec;
			current_time.tv_nsec = gtod_current_time.tv_usec*CCD_GENERAL_ONE_MICROSECOND_NS;
#endif
			remaining_sec = start_time.tv_sec - current_time.tv_sec;
#if LOGGING > 4
			CCD_General_Log_Format("ccd","ccd_exposure.c","Exposure_Wait_For_Start_Time",
					      LOG_VERBOSITY_INTERMEDIATE,"TIMING","Exposure_Wait_For_Start_Time:"
					      "Waiting for exposure start time (%ld,%ld) = %ld secs.",
					      current_time.tv_sec,start_time.tv_sec,remaining_sec);
#endif
		/* if we have over a second before wait_time, sleep for a second. */
			if(remaining_sec > 1)
			{
				sleep_time.tv_sec = 1;
				sleep_time.tv_nsec = 0;
				nanosleep(&sleep_time,NULL);
			}
			else if(remaining_sec > -1)
			{
				remaining_ns = (start_time.tv_nsec - current_time.tv_nsec);
				if(remaining_ns < 0)
				{
					remaining_sec--;
					remaining_ns += CCD_GENERAL_ONE_SECOND_NS;
				}
				done = TRUE;
				if(remaining_sec > -1)
				{
					sleep_time.tv_sec = remaining_sec;
					sleep_time.tv_nsec = remaining_ns;
					nanosleep(&sleep_time,NULL);
				}
			}
			else
				done = TRUE;
		/* if an abort has occured, stop sleeping. The following command send will catch the abort. */
			if(Exposure_Data.Abort)
				done = TRUE;
		}/* end while */
	}/* end if */
	if(Exposure_Data.Abort)
	{
		Exposure_Error_Number = 33;
		sprintf(Exposure_Error_String,"Exposure_Wait_For_Start_Time:Operation was aborted.");
		return FALSE;
	}
/* switch status to exposing and store the actual time the exposure is going to start */
#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&(Exposure_Data.Start_Time));
#else
	gettimeofday(&gtod_current_time,NULL);
	Exposure_Data.Start_Time.tv_sec = gtod_current_time.tv_sec;
	Exposure_Data.Start_Time.tv_nsec = gtod_current_time.tv_usec*CCD_GENERAL_ONE_MICROSECOND_NS;
#endif
	return TRUE;
}

/**
 * Debug routine to print out part of the readout buffer.
 * @param description A string containing a description of the buffer.
 * @param buffer The readout buffer.
 * @param buffer_length The length of the readout buffer in pixels.
 * @see CCD_General_Log
 */
static void Exposure_Debug_Buffer(char *description,unsigned short *buffer,size_t buffer_length)
{
	char buff[1024];
	int i;

	strcpy(buff,description);
	strcat(buff," : ");
	for(i=0; i < 10; i++)
	{
		sprintf(buff+strlen(buff),"[%d] = %hu,",i,buffer[i]);
	}
	strcat(buff," ... ");
	for(i=(buffer_length-10); i < buffer_length; i++)
	{
		sprintf(buff+strlen(buff),"[%d] = %hu,",i,buffer[i]);
	}
#if LOGGING > 9
	CCD_General_Log("ccd","ccd_exposure.c","Exposure_Debug_Buffer",LOG_VERBOSITY_INTERMEDIATE,NULL,buff);
#endif	
}

/**
 * Return whether the specified filename exists or not.
 * @param filename A string representing the filename to test.
 * @return The routine returns TRUE if the filename exists, and FALSE if it does not exist. 
 */
static int fexist(char *filename)
{
	FILE *fptr = NULL;

	fptr = fopen(filename,"r");
	if(fptr == NULL )
		return FALSE;
	fclose(fptr);
	return TRUE;
}

