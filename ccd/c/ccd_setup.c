/* ccd_setup.c
** Low level ccd library
*/
/**
 * @file
 * @brief ccd_setup.c contains routines for performing an setup with the CCD Controller.
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
/**
 * Define this to enable strdup in 'string.h', which is a BSD 4.3 prototypes.
 */
#define _BSD_SOURCE 1
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
#include "ccd_general.h"
#include "ccd_setup.h"


/**
 * The length of the camera head model name string in the Setup_Struct
 * @see #Setup_Struct
 */
#define CAMERA_HEAD_MODEL_NAME_LENGTH   (128)

/* data types */
/**
 * Data type holding local data to andor_setup. This consists of the following:
 * <dl>
 * <dt>Config_Dir</dt> <dd>Pointer to a string containing the Andor config directory used for initialisation.</dd>
 * <dt>Selected_Camera</dt> <dd>A long, saying which camera to use. Usually '0'.
 * <dt>Camera_Handle</dt> <dd>The Andor library camera handle.</dd>
 * <dt>Camera_Head_Model_Name</dt> <dd>The camera's head model name, as read off the camera head to 
 *                                     identify the camera. Of length CAMERA_HEAD_MODEL_NAME_LENGTH.</dd>
 * <dt>Camera_Serial_Number</dt> <dd>The camera's serial number, read off the head to identify the camera.</dd>
 * <dt>Detector_X_Pixel_Count</dt> <dd>The number of pixels on the detector in the X direction.</dd>
 * <dt>Detector_Y_Pixel_Count</dt> <dd>The number of pixels on the detector in the Y direction.</dd>
 * <dt>Horizontal_Bin</dt> <dd>Horizontal (X) binning factor.</dd>
 * <dt>Vertical_Bin</dt> <dd>Vertical (Y) binning factor.</dd>
 * <dt>Is_Window</dt> <dd>Boolean, TRUE if the current config is a windowed one, FALSE if full frame.</dd>
 * <dt>Horizontal_Start</dt> <dd>Horizontal (X) start pixel of the imaging window (inclusive).</dd>
 * <dt>Horizontal_End</dt> <dd>Horizontal (X) end pixel of the imaging window (inclusive).</dd>
 * <dt>Vertical_Start</dt> <dd>Vertical (Y) start pixel of the imaging window (inclusive).</dd>
 * <dt>Vertical_End</dt> <dd>Vertical (Y) end pixel of the imaging window (inclusive).</dd>
 * </dl>
 * @see #CAMERA_HEAD_MODEL_NAME_LENGTH
 */
struct Setup_Struct
{
	char *Config_Dir;
	int Selected_Camera;
	int Camera_Handle;
	char Camera_Head_Model_Name[CAMERA_HEAD_MODEL_NAME_LENGTH];
	int Camera_Serial_Number;
	int Detector_X_Pixel_Count;
	int Detector_Y_Pixel_Count;
	int Horizontal_Bin;
	int Vertical_Bin;
	int Is_Window;
	int Horizontal_Start;
	int Horizontal_End;
	int Vertical_Start;
	int Vertical_End;
};


/* external variables */

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id";

/**
 * Variable holding error code of last operation performed by ccd_setup.
 */
static int Setup_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Setup_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";
/**
 * Instance of the setup data.
 * @see #Setup_Struct
 */
static struct Setup_Struct Setup_Data = 
{
	NULL,0,0,"",0,0,0,0,0,FALSE,0,0,0,0
};

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Set the Andor config dir used as a parameter to the Andor library Initialize function.
 * @param directory A character pointer to the string to use.
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Setup_Data
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int CCD_Setup_Config_Directory_Set(char *directory)
{
	Setup_Error_Number = 0;
#if LOGGING > 5
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Config_Directory_Set",LOG_VERBOSITY_INTERMEDIATE,"CCD",
			"CCD_Setup_Config_Directory_Set Started.");
#endif /* LOGGING */
	if(directory == NULL)
	{
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"CCD_Setup_Config_Directory_Set: directory was NULL.");
		return FALSE;
	}
	/* free old config directory if it has previously been set. */
	if(Setup_Data.Config_Dir != NULL)
		free(Setup_Data.Config_Dir);
	/* Use strdup to allocate new config directory */
	Setup_Data.Config_Dir = strdup(directory);
	if(Setup_Data.Config_Dir == NULL)
	{
		Setup_Error_Number = 2;
		sprintf(Setup_Error_String,"CCD_Setup_Config_Directory_Set: Failed to copy directoey '%s'.",directory);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Config_Directory_Set",LOG_VERBOSITY_INTERMEDIATE,"CCD",
			"CCD_Setup_Config_Directory_Set Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Do initial setup and configuration for an Andor CCD.
 * <ul>
 * <li>Uses <b>GetAvailableCameras</b> to get the available number of cameras.
 * <li>Uses <b>GetCameraHandle</b> to get a camera handle for the selected camera and store it in 
 *          Setup_Data.Camera_Handle.
 * <li>Uses <b>SetCurrentCamera</b> to set the Andor libraries current camera.
 * <li>Call <b>Initialize</b> to initialise the andor library using the selected config directory.
 * <li>Calls <b>SetReadMode</b> to set the Andor library read mode to image.
 * <li>Calls <b>SetAcquisitionMode</b> to set the Andor library to acquire a single image at a time.
 * <li>We call <b>GetNumberVSSpeeds, GetVSSpeed </b>
 *     to examine vertical shift speeds.
 * <li>Calls <b>GetNumberHSSpeeds,GetHSSpeed </b> to examine horzontal readout speeds.
 * <li>We call <b>GetNumberPreAmpGains</b>, <b>GetPreAmpGain</b> to log the pre-amp gains available.
 * <li>We call <b>GetNumberADChannels</b> to log the A/D channels available.
 * <li>Calls <b>SetBaselineClamp(1)</b> to set the baseline clamp on.
 * <li>Calls <b>GetDetector</b> to get the detector dimensions and save then to <b>Setup_Data</b>.
 * <li>Calls <b>SetShutter</b> to set the Andor library shutter settings to auto with no shutter delay.
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Setup_Data
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see CCD_General_Andor_ErrorCode_To_String
 * @see CCD_General_Log
 */
int CCD_Setup_Startup(void)
{
	long selected_camera;
	unsigned int andor_retval;
	int retval,camera_count,speed_count,i,pre_amp_gain_count;
	float speed,pre_amp_gain;

	Setup_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Startup Started.");
#endif /* LOGGING */
	/* sort out selected camera. */
	andor_retval = GetAvailableCameras(&camera_count);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetAvailableCameras() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,NULL,
			       "Andor library reports %d cameras.",camera_count);
#endif /* LOGGING */
	if((Setup_Data.Selected_Camera >= camera_count) || (Setup_Data.Selected_Camera < 0))
	{
		Setup_Error_Number = 4;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: Selected camera %d out of range [0..%d].",
			Setup_Data.Selected_Camera,camera_count);
		return FALSE;
	}
	/* get andor camera handle */
	andor_retval = GetCameraHandle(Setup_Data.Selected_Camera,&Setup_Data.Camera_Handle);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 5;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetCameraHandle(%d) failed %s(%u).",
			Setup_Data.Selected_Camera,CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
	/* set current camera */
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,NULL,
			       "SetCurrentCamera(%d).",Setup_Data.Camera_Handle);
#endif /* LOGGING */
	andor_retval = SetCurrentCamera(Setup_Data.Camera_Handle);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 6;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: SetCurrentCamera() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			       "Calling Andor Initialize(%s).",Setup_Data.Config_Dir);
#endif /* LOGGING */
	andor_retval = Initialize(Setup_Data.Config_Dir);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 7;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: Initialize(%s) failed %s(%u).",
			Setup_Data.Config_Dir,CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Sleeping whilst waiting for Initialize to complete.");
#endif /* LOGGING */
	sleep(2);
	/* get camera head model */
#if LOGGING > 5
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Getting camera head model.");
#endif /* LOGGING */
	andor_retval = GetHeadModel(Setup_Data.Camera_Head_Model_Name);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 12;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetHeadModel failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}				     
	/* get camera serial number */
#if LOGGING > 5
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Getting camera serial number.");
#endif /* LOGGING */
	andor_retval = GetCameraSerialNumber(&(Setup_Data.Camera_Serial_Number));
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 14;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetCameraSerialNumber failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}				     
	/* Set read mode to image */
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Calling SetReadMode(4) (image).");
#endif /* LOGGING */
	andor_retval = SetReadMode(4);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 8;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: SetReadMode(4) failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
	/* Set acquisition mode to single scan */
#if LOGGING > 3
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Calling SetAcquisitionMode(1) (single scan).");
#endif /* LOGGING */
	andor_retval = SetAcquisitionMode(1);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 9;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: SetAcquisitionMode(1) failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
	/* log vertical speed data */
	andor_retval = GetNumberVSSpeeds(&speed_count);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 24;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetNumberVSSpeeds() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 9
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"GetNumberVSSpeeds() returned %d speeds.",speed_count);
#endif /* LOGGING */
	for(i=0;i < speed_count; i++)
	{
		andor_retval = GetVSSpeed(i,&speed);
		if(andor_retval != DRV_SUCCESS)
		{
			Setup_Error_Number = 25;
			sprintf(Setup_Error_String,"CCD_Setup_Startup: GetVSSpeed(%d) failed %s(%u).",i,
				CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
			return FALSE;
		}
#if LOGGING > 9
		CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
				       "GetVSSpeed(index=%d) returned %.2f microseconds/pixel shift.",i,speed);
#endif /* LOGGING */
	}
	/* log horizontal readout speed data */
	andor_retval = GetNumberHSSpeeds(0,0,&speed_count);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 28;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetNumberHSSpeeds(0,0) failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 9
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"GetNumberHSSpeeds(channel=0,type=0) returned %d speeds.",
			speed_count);
#endif /* LOGGING */
	for(i=0;i < speed_count; i++)
	{
		andor_retval = GetHSSpeed(0,0,i,&speed);
		if(andor_retval != DRV_SUCCESS)
		{
			Setup_Error_Number = 29;
			sprintf(Setup_Error_String,"CCD_Setup_Startup: GetHSSpeed(0,0,%d) failed %s(%u).",i,
				CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
			return FALSE;
		}
#if LOGGING > 9
		CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
				      "GetHSSpeed(channel=0,type=0,index=%d) returned %.2f.",
				       i,speed);
#endif /* LOGGING */
		/* see below, we select index 0 */
		/*
		if(i == 0) 
			Setup_Data.HSSpeed = speed;
		*/
	}
	/* get A/D channel count */
	andor_retval = GetNumberADChannels(&speed_count);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 30;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetNumberADChannels() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 9
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			       "GetNumberADChannels() returned %d A/D channels.",speed_count);
#endif /* LOGGING */
	/* get the number of pre-amp gains */
	andor_retval = GetNumberPreAmpGains(&pre_amp_gain_count);
	if(andor_retval!=DRV_SUCCESS)
	{
		Setup_Error_Number = 16;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetNumberPreAmpGains() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 9
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			       "GetNumberPreAmpGains() returned %d gains.",pre_amp_gain_count);
#endif /* LOGGING */
	for(i=0;i < pre_amp_gain_count; i++)
	{
		andor_retval = GetPreAmpGain(i,&pre_amp_gain);
		if(andor_retval!=DRV_SUCCESS)
		{
			Setup_Error_Number = 17;
			sprintf(Setup_Error_String,"CCD_Setup_Startup: GetPreAmpGain(%d) failed %s(%u).",
				i,CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
			return FALSE;
		}
#if LOGGING > 9
		CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
				       "PreAmpGain index %d is %.2f.",i,pre_amp_gain);
#endif /* LOGGING */
	} /* end for on pre_amp_gain_count */
	/* set baseline clamp */
#if LOGGING > 3
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Calling SetBaselineClamp(1).");
#endif /* LOGGING */
	andor_retval = SetBaselineClamp(1);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 21;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: SetBaselineClamp(1) failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
	/* get the detector dimensions */
#if LOGGING > 3
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Calling GetDetector.");
#endif /* LOGGING */
	andor_retval = GetDetector(&Setup_Data.Detector_X_Pixel_Count,&Setup_Data.Detector_Y_Pixel_Count);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 10;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: GetDetector() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
	/* initialise the shutter  - is this needed? See ccd_exposure.c */
#if LOGGING > 3
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
			"Calling SetShutter(1,0,0,0).");
#endif /* LOGGING */
	andor_retval = SetShutter(1,0,0,0);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 11;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: SetShutter() failed %s(%u).",
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Startup Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Shutdown setup to the Andor CCD. Calls Andor library routine <b>ShutDown</b>.
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Setup_Error_Number
 * @see CCD_General_Log
 */
int CCD_Setup_Shutdown(void)
{
	Setup_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Shutdown",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Shutdown Started.");
#endif /* LOGGING */
#if LOGGING > 3
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Shutdown",LOG_VERBOSITY_TERSE,"CCD",
			"Calling Shutdown.");
#endif /* LOGGING */
	ShutDown();
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Shutdown",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Shutdown Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set up which part of the CCD to readout. Calls Andor library <b>SetImage</b>.
 * @param ncols Number of image columns (X).
 * @param nrows Number of image rows (Y).
 * @param hbin Binning in X.
 * @param vbin Binning in Y.
 * @param window_flags Whether to use the specified window or not.
 * @param window A structure containing window data.
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see CCD_General_Log
 * @see CCD_General_Andor_ErrorCode_To_String
 * @see #CCD_Setup_Window_Struct
 */
int CCD_Setup_Dimensions(int ncols,int nrows,int hbin,int vbin,int window_flags,
			 struct CCD_Setup_Window_Struct window)
{
	unsigned int andor_retval;

	Setup_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Dimensions Started.");
#endif /* LOGGING */
#if LOGGING > 1
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_TERSE,"CCD",
			       "CCD_Setup_Dimensions(ncols=%d,nrows=%d,hbin=%d,vbin=%d,window_flags=%d,"
			       "{xstart=%d,ystart=%d,xend=%d,yend=%d}).",ncols,nrows,hbin,vbin,window_flags,
			       window.X_Start,window.Y_Start,window.X_End,window.Y_End);
#endif /* LOGGING */
	if(window_flags > 0)
	{
		Setup_Data.Is_Window = TRUE;
		Setup_Data.Horizontal_Bin = hbin;
		Setup_Data.Vertical_Bin = vbin;
		Setup_Data.Horizontal_Start = window.X_Start;
		Setup_Data.Horizontal_End = window.X_End;
		Setup_Data.Vertical_Start = window.Y_Start;
		Setup_Data.Vertical_End = window.Y_End;
	}
	else
	{
		Setup_Data.Is_Window = FALSE;
		Setup_Data.Horizontal_Bin = hbin;
		Setup_Data.Vertical_Bin = vbin;
		Setup_Data.Horizontal_Start = 1;
		Setup_Data.Horizontal_End = ncols;
		Setup_Data.Vertical_Start = 1;
		Setup_Data.Vertical_End = nrows;
	}
	/* check unbinned window can be binned into a whole number of pixels.
	** Otherwise Andor GetImage code fails with P2_INVALID. */
#if LOGGING > 9
	CCD_General_Log_Format("ccd","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_VERY_VERBOSE,NULL,
			       "Check window can be binned into a whole number of pixels:"
			       "(((hend %d - hstart %d)+1)%%hbin %d) = %d.",
			       Setup_Data.Horizontal_End,Setup_Data.Horizontal_Start,Setup_Data.Horizontal_Bin,
			      (((Setup_Data.Horizontal_End-Setup_Data.Horizontal_Start)+1)%Setup_Data.Horizontal_Bin));
#endif /* LOGGING */
	if((((Setup_Data.Horizontal_End-Setup_Data.Horizontal_Start)+1)%Setup_Data.Horizontal_Bin) != 0)
	{
		Setup_Error_Number = 38;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions:Horizontal window size not exact multiple of binning:"
			"(((hend %d - hstart %d)+1)%%hbin %d) != 0.",
			Setup_Data.Horizontal_End,Setup_Data.Horizontal_Start,Setup_Data.Horizontal_Bin);
		return FALSE;
	}
#if LOGGING > 9
	CCD_General_Log_Format("ccd","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_VERY_VERBOSE,NULL,
			       "Check window can be binned into a whole number of pixels:"
			       "(((vend %d - vstart %d)+1)%%vbin %d) = %d.",
			       Setup_Data.Vertical_End,Setup_Data.Vertical_Start,Setup_Data.Vertical_Bin,
			       (((Setup_Data.Vertical_End-Setup_Data.Vertical_Start)+1)%Setup_Data.Vertical_Bin));
#endif /* LOGGING */
	if((((Setup_Data.Vertical_End-Setup_Data.Vertical_Start)+1)%Setup_Data.Vertical_Bin) != 0)
	{
		Setup_Error_Number = 39;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions:Vertical window size not exact multiple of binning:"
			"(((vend %d - vstart %d)+1)%%vbin %d) != 0.",
			Setup_Data.Vertical_End,Setup_Data.Vertical_Start,Setup_Data.Vertical_Bin);
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log_Format("ccd","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_VERBOSE,NULL,
			       "Calling SetImage(hbin=%d,vbin=%d,hstart=%d,hend=%d,vstart=%d,vend=%d).",
			       Setup_Data.Horizontal_Bin,Setup_Data.Vertical_Bin,
			       Setup_Data.Horizontal_Start,Setup_Data.Horizontal_End,
			       Setup_Data.Vertical_Start,Setup_Data.Vertical_End);
#endif /* LOGGING */
        andor_retval = SetImage(Setup_Data.Horizontal_Bin,Setup_Data.Vertical_Bin,
				Setup_Data.Horizontal_Start,Setup_Data.Horizontal_End,
				Setup_Data.Vertical_Start,Setup_Data.Vertical_End);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 13;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: SetImage(hbin=%d,vbin=%d,"
			"hstart=%d,hend=%d,vstart=%d,vend=%d) failed %s(%u).",
			Setup_Data.Horizontal_Bin,Setup_Data.Vertical_Bin,
			Setup_Data.Horizontal_Start,Setup_Data.Horizontal_End,
			Setup_Data.Vertical_Start,Setup_Data.Vertical_End,
			CCD_General_Andor_ErrorCode_To_String(andor_retval),andor_retval);
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Dimensions Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Abort a setup. Currently does nothing.
 */
void CCD_Setup_Abort(void)
{
	Setup_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Abort",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Abort Started.");
#endif /* LOGGING */

#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Abort",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Abort Finished.");
#endif /* LOGGING */
}

/**
 * Get the number of columns setup to be read out from the last CCD_Setup_Dimensions.
 * Currently, (Setup_Data.Horizontal_End - Setup_Data.Horizontal_Start)+1.
 * Plus 1 as dimensions are inclusive. This number is unbinned.
 * @return The number of unbinned columns.
 * @see #Setup_Data
 */
int CCD_Setup_Get_NCols(void)
{
	return (Setup_Data.Horizontal_End - Setup_Data.Horizontal_Start)+1;
}

/**
 * Get the number of rows setup to be read out from the last CCD_Setup_Dimensions.
 * Currently, (Setup_Data.Vertical_End - Setup_Data.Vertical_Start)+1.
 * Plus 1 as dimensions are inclusive. This number is unbinned.
 * @return The number of unbinned rows.
 * @see #Setup_Data
 */
int CCD_Setup_Get_NRows(void)
{
	return (Setup_Data.Vertical_End - Setup_Data.Vertical_Start)+1;
}

/**
 * Routine that returns the column binning factor the last dimension setup has set the CCD Controller to. 
 * This is the number passed into CCD_Setup_Dimensions.
 * @return The columns binning number.
 */
int CCD_Setup_Get_Bin_X(void)
{
	return Setup_Data.Horizontal_Bin;
}

/**
 * Routine that returns the row binning factor the last dimension setup has set the CCD Controller to. 
 * This is the number passed into CCD_Setup_Dimensions.
 * @return The rows binning number.
 */
int CCD_Setup_Get_Bin_Y(void)
{
	return Setup_Data.Vertical_Bin;
}

/**
 * Return whether the CCD is being windowed or not.
 * @return TRUE if the CCD is being windowed, FALSE otherwise.
 * @see #Setup_Data
 */
int CCD_Setup_Is_Window(void)
{
	return Setup_Data.Is_Window;
}

/**
 * Routine that returns the horizontal start pixel of the readout. Usually 1 for full-frame readout,
 * and the window start pixel (unbinned) for windowed readout.
 * @return The horizontal start pixel of readout. This is an unbinned value.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Horizontal_Start(void)
{
	return Setup_Data.Horizontal_Start;
}

/**
 * Routine that returns the horizontal end pixel of the readout. Usually ncols for full-frame readout,
 * and the window end pixel (unbinned) for windowed readout.
 * @return The horizontal end pixel of readout. This is an unbinned value.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Horizontal_End(void)
{
	return Setup_Data.Horizontal_End;
}

/**
 * Routine that returns the vertical start pixel of the readout. Usually 1 for full-frame readout,
 * and the window start pixel (unbinned) for windowed readout.
 * @return The vertical start pixel of readout. This is a unbinned value.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Vertical_Start(void)
{
	return Setup_Data.Vertical_Start;
}

/**
 * Routine that returns the vertical end pixel of the readout. Usually nrows for full-frame readout,
 * and the window end pixel (unbinned) for windowed readout.
 * @return The vertical end pixel of readout. This is an unbinned value.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Vertical_End(void)
{
	return Setup_Data.Vertical_End;
}

/**
 * Get the number of pixels on the (unbinned) detector, in X. Retrieved using the stored
 * Detector_X_Pixel_Count data retrieved during startup (GetDetector).
 * @return The number of pixels on the detector in X.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Detector_Pixel_Count_X(void)
{
	return Setup_Data.Detector_X_Pixel_Count;
}

/**
 * Get the number of pixels on the (unbinned) detector, in Y. Retrieved using the stored
 * Detector_Y_Pixel_Count data retrieved during startup (GetDetector).
 * @return The number of pixels on the detector in Y.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Detector_Pixel_Count_Y(void)
{
	return Setup_Data.Detector_Y_Pixel_Count;
}

/**
 * Get the camera head model name. Retrieved from the stored Camera_Head_Model_Name data read during CCD_Setup_Startup.
 * @param name A pre-allocated character array to store the camera head model name in.
 * @param name_length The number of characters allocated for the name array.
 * @return The routine returns TRUE if the camera haed model name was copied into name successfully, and FALSE
 *         if it failed (the camera head model name is too long for the allocated name array).
 * @see #Setup_Data
 * @see #CCD_Setup_Startup
 */
int CCD_Setup_Get_Camera_Head_Model_Name(char *name,int name_length)
{
	if(strlen(Setup_Data.Camera_Head_Model_Name) >= name_length)
	{
		Setup_Error_Number = 15;
		sprintf(Setup_Error_String,"CCD_Setup_Get_Camera_Head_Model_Name:name buffer too short (%d vs %ld).",
			name_length,strlen(Setup_Data.Camera_Head_Model_Name));
		return FALSE;
	}
	strcpy(name,Setup_Data.Camera_Head_Model_Name);
	return TRUE;
}

/**
 * Get the camera serial number read off the head in CCD_Setup_Startup.
 * @return An integer, the camera serial number.
 * @see #CCD_Setup_Startup
 * @see #Setup_Data
 */
int CCD_Setup_Get_Camera_Serial_Number(void)
{
	return Setup_Data.Camera_Serial_Number;
}

/**
 * Return the length of buffer required to hold one image with the current setup.
 * @param buffer_length The address of a size_t to hold required length of buffer in pixels. 
 *        These will be binned pixels.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #CCD_Setup_Get_NCols
 * @see #CCD_Setup_Get_NRows
 * @see #CCD_Setup_Get_Bin_X
 * @see #CCD_Setup_Get_Bin_Y
 */
int CCD_Setup_Get_Buffer_Length(size_t *buffer_length)
{
	if(buffer_length == NULL)
	{
		Setup_Error_Number = 34;
		sprintf(Setup_Error_String,"CCD_Setup_Get_Buffer_Length:buffer_length is NULL.");
		return FALSE;
	}
	if(CCD_Setup_Get_Bin_X() == 0)
	{
		Setup_Error_Number = 35;
		sprintf(Setup_Error_String,"CCD_Setup_Get_Buffer_Length:X Binning is 0.");
		return FALSE;
	}
	if(CCD_Setup_Get_Bin_Y() == 0)
	{
		Setup_Error_Number = 36;
		sprintf(Setup_Error_String,"CCD_Setup_Get_Buffer_Length:Y Binning is 0.");
		return FALSE;
	}
	(*buffer_length) = (size_t)((CCD_Setup_Get_NCols() * CCD_Setup_Get_NRows())/
				    (CCD_Setup_Get_Bin_X() * CCD_Setup_Get_Bin_Y()));
#if LOGGING > 9
	CCD_General_Log_Format("ccd","ccd_setup.c","CCD_Setup_Get_Buffer_Length",LOG_VERBOSITY_VERBOSE,"CCD",
			       "buffer_length %ld pixels = (ncols %d x nrows %d) / (binx %d x biny %d).",
			       (*buffer_length),CCD_Setup_Get_NCols(),CCD_Setup_Get_NRows(),
			       CCD_Setup_Get_Bin_X(),CCD_Setup_Get_Bin_Y());
#endif
	return TRUE;
}

/**
 * Allocate memory to hold a single image using the current setup.
 * @param buffer The address of a pointer to store the location of the allocated memory.
 * @param buffer_length The address of a size_t to hold the length of the buffer in bytes.
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #CCD_Setup_Get_Buffer_Length
 */
int CCD_Setup_Allocate_Image_Buffer(void **buffer,size_t *buffer_length)
{
	size_t binned_pixel_count;

#if LOGGING > 0
	CCD_General_Log("ccd","ccd_setup.c","CCD_Setup_Allocate_Image_Buffer",LOG_VERBOSITY_VERBOSE,"ANDOR",
			"started.");
#endif
	if(buffer == NULL)
	{
		Setup_Error_Number = 31;
		sprintf(Setup_Error_String,"CCD_Setup_Allocate_Image_Buffer: buffer was NULL.");
		return FALSE;
	}
	if(buffer_length == NULL)
	{
		Setup_Error_Number = 32;
		sprintf(Setup_Error_String,"CCD_Setup_Allocate_Image_Buffer: buffer_length was NULL.");
		return FALSE;
	}
	if(!CCD_Setup_Get_Buffer_Length(&binned_pixel_count))
		return FALSE;
	(*buffer_length) = binned_pixel_count*sizeof(unsigned short);
	(*buffer) = (void *)malloc((*buffer_length));
	if((*buffer) == NULL)
	{
		Setup_Error_Number = 33;
		sprintf(Setup_Error_String,"CCD_Setup_Allocate_Image_Buffer: Failed to allocate buffer (%ld).",
			(*buffer_length));
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log("ccd","ccd_setup.c","CCD_Setup_Allocate_Image_Buffer",LOG_VERBOSITY_VERBOSE,"ANDOR",
			"finished.");
#endif
	return TRUE;
}

/**
 * Get the current value of ccd_setup's error number.
 * @return The current value of ccd_setup's error number.
 * @see #Setup_Error_Number
 */
int CCD_Setup_Get_Error_Number(void)
{
	return Setup_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_setup in a standard way.
 * @see CCD_General_Get_Current_Time_String
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
void CCD_Setup_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Setup:Error(%d) : %s\n",time_string,Setup_Error_Number,Setup_Error_String);
}

/**
 * The error routine that reports any errors occuring in ccd_setup in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see CCD_General_Get_Current_Time_String
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
void CCD_Setup_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Setup:Error(%d) : %s\n",time_string,
		Setup_Error_Number,Setup_Error_String);
}

