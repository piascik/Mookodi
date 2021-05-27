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
#define _DEFAULT_SOURCE 1
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
 * Data type holding local data to ccd_setup. 
 * @see #CAMERA_HEAD_MODEL_NAME_LENGTH
 */
struct Setup_Struct
{
	/** Pointer to a string containing the Andor config directory used for initialisation. */
	char *Config_Dir;
	/** A long, saying which camera to use. Usually '0'. */
	int Selected_Camera;
	/** The Andor library camera handle. */
	int Camera_Handle;
	/** The camera's head model name, as read off the camera head to identify the camera. 
	 * Of length CAMERA_HEAD_MODEL_NAME_LENGTH.*/
	char Camera_Head_Model_Name[CAMERA_HEAD_MODEL_NAME_LENGTH];
	/** The camera's serial number, read off the head to identify the camera. */
	int Camera_Serial_Number;
	/** The number of pixels on the detector in the X direction. */
	int Detector_X_Pixel_Count;
	/** The number of pixels on the detector in the Y direction. */
	int Detector_Y_Pixel_Count;
	/** Horizontal (X) binning factor. */
	int Horizontal_Bin;
	/** Vertical (Y) binning factor. */
	int Vertical_Bin;
	/** Boolean, TRUE if the current config is a windowed one, FALSE if full frame. */
	int Is_Window;
	/** Horizontal (X) start pixel of the imaging window (inclusive). */
	int Horizontal_Start;
	/** Horizontal (X) end pixel of the imaging window (inclusive). */
	int Horizontal_End;
	/** Vertical (Y) start pixel of the imaging window (inclusive). */
	int Vertical_Start;
	/** Vertical (Y) end pixel of the imaging window (inclusive). */
	int Vertical_End;
	/** The horizontal speed index to use when setting the horizontal readout speed. */
	int HS_Speed_Index;
	/** The horizontal speed for the specified HS_Speed_Index, in MHz. */
	float HS_Speed;
	/** The vertical speed index to use when setting the vertical readout speed. */
	int VS_Speed_Index;
	/** The vertical speed for the specified VS_Speed_Index, in microseconds/pixel. */
	float VS_Speed;
	/** The pre-amp gain index to use when setting the pre-amp gain. */
	int Pre_Amp_Gain_Index;
	/** The actual pre-amp gain for the specified Pre_Amp_Gain_Index. */
	float Pre_Amp_Gain;
	/** A boolean - if true the exposure code will flip the image in the horizontal/X direction. */
	int Flip_X;
	/** A boolean - if true the exposure code wil flip the image in the vertical/Y direction. */
	int Flip_Y;
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
	NULL,0,0,"",0,0,0,0,0,FALSE,0,0,0,0,0,0.0,0,0.0,0,FALSE,FALSE
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
 * </ul>
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
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
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
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Startup",LOG_VERBOSITY_VERBOSE,"CCD",
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
 * @param ncols Number of unbinned image columns (X).
 * @param nrows Number of unbinned image rows (Y).
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
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_VERY_VERBOSE,"CCD",
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
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_VERY_VERBOSE,"CCD",
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
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Dimensions",LOG_VERBOSITY_VERBOSE,"ANDOR",
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
 * Set the horizontal shift speed to the setting represented by the specified index in a list of horizontal
 * shift speeds. This affects the reasdout speed of the detector.
 * @param hs_speed_index The index into the list of speeds. Valid values are 0 to GetNumberHSSpeeds()-1.
 *        For the Andor iKon M934 the there are 4 speeds indexs (0..3), and these translate to the following:
 *        <ul>
 *        <li><b>0</b> 5MHz
 *        <li><b>1</b> 3MHz
 *        <li><b>2</b> 1MHz
 *        <li><b>3</b> 0.05MHz
 *        </ul>
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see CCD_General_Log
 * @see CCD_General_Andor_ErrorCode_To_String
 */
int CCD_Setup_Set_HS_Speed(int hs_speed_index)
{
	unsigned int andor_retval;

	Setup_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Set_HS_Speed",LOG_VERBOSITY_TERSE,"CCD",
			       "CCD_Setup_Set_HS_Speed(hs_speed_index = %d) Started.",hs_speed_index);
#endif /* LOGGING */
	/* A/D chanel is always 0 for Andor iKon M934 (there is only 1 channel) */
	andor_retval = SetHSSpeed(0,hs_speed_index);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 18;
		sprintf(Setup_Error_String,"CCD_Setup_Set_HS_Speed: SetHSSpeed(A/D channel=0,hs_speed_index=%d) "
			"failed %s(%u).",hs_speed_index,CCD_General_Andor_ErrorCode_To_String(andor_retval),
			andor_retval);
		return FALSE;
	}
	Setup_Data.HS_Speed_Index = hs_speed_index;
	/* retrieve what the actual speed is in MHz, and store in Setup_Data.HS_Speed 
	** We default to A/D channel 0 and typ 0. The Andor iKon M934 has only 1 A/D channel / amplifier. */
	andor_retval = GetHSSpeed(0,0,hs_speed_index,&(Setup_Data.HS_Speed));
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 22;
		sprintf(Setup_Error_String,"CCD_Setup_Set_HS_Speed: GetHSSpeed(A/D channel=0,typ=0,hs_speed_index=%d) "
			"failed %s(%u).",hs_speed_index,CCD_General_Andor_ErrorCode_To_String(andor_retval),
			andor_retval);
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Set_HS_Speed",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Set_HS_Speed Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the vertical shift speed to the setting represented by the specified index in a list of vertical
 * shift speeds. This affects the reasdout speed of the detector. After setting the vertical shift speed
 * we retrieve the actual speed and store it in Setup_Data.VS_Speed.
 * @param vs_speed_index The index into the list of speeds. Valid values are 0 to GetNumberVSSpeeds()-1.
 *        For the Andor iKon M934 the there are 6 speeds indexs (0..5), and these translate to the following:
 *        <ul>
 *        <li><b>0</b> 2.25 microseconds/pixel
 *        <li><b>1</b> 4.25 microseconds/pixel
 *        <li><b>2</b> 8.25 microseconds/pixel
 *        <li><b>3</b> 16.25 microseconds/pixel
 *        <li><b>4</b> 32.25 microseconds/pixel
 *        <li><b>5</b> 64.25 microseconds/pixel
 *        </ul>
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see CCD_General_Log
 * @see CCD_General_Andor_ErrorCode_To_String
 */
int CCD_Setup_Set_VS_Speed(int vs_speed_index)
{
	unsigned int andor_retval;

	Setup_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Set_VS_Speed",LOG_VERBOSITY_TERSE,"CCD",
			       "CCD_Setup_Set_VS_Speed(vs_speed_index = %d) Started.",vs_speed_index);
#endif /* LOGGING */
	andor_retval = SetVSSpeed(vs_speed_index);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 19;
		sprintf(Setup_Error_String,"CCD_Setup_Set_VS_Speed: SetVSSpeed(vs_speed_index=%d) "
			"failed %s(%u).",vs_speed_index,CCD_General_Andor_ErrorCode_To_String(andor_retval),
			andor_retval);
		return FALSE;
	}
	Setup_Data.VS_Speed_Index = vs_speed_index;
	/* retrieve what the actual vertical speed is for the specified index and store it for future use */
	andor_retval = GetVSSpeed(vs_speed_index,&(Setup_Data.VS_Speed));
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 23;
		sprintf(Setup_Error_String,"CCD_Setup_Set_VS_Speed: GetVSSpeed(vs_speed_index=%d) "
			"failed %s(%u).",vs_speed_index,CCD_General_Andor_ErrorCode_To_String(andor_retval),
			andor_retval);
		return FALSE;
	}	
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Set_VS_Speed",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Set_VS_Speed Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the pre amp gain to the setting represented by the specified index in a list of pre amp gains.
 * After setting the pre-amp gain to the specified index we retrieve the actual gain factor 
 * and store it in Setup_Data.Pre_Amp_Gain.
 * @param pre_amp_gain_index The index into the list of gains. Valid values are 0 to GetNumberPreAmpGains()-1.
 *        For the Andor iKon M934 the there are 3 pre amp gain indexs (0..2), and these translate to the following:
 *        <ul>
 *        <li><b>0</b> 1.0 gain factor
 *        <li><b>1</b> 2.0 gain factor
 *        <li><b>2</b> 4.0 gain factor
 *        </ul>
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see CCD_General_Log
 * @see CCD_General_Andor_ErrorCode_To_String
 */
int CCD_Setup_Set_Pre_Amp_Gain(int pre_amp_gain_index)
{
	unsigned int andor_retval;

	Setup_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Set_Pre_Amp_Gain",LOG_VERBOSITY_TERSE,"CCD",
			       "CCD_Setup_Set_Pre_Amp_Gain(pre_amp_gain_index = %d) Started.",pre_amp_gain_index);
#endif /* LOGGING */
	andor_retval = SetPreAmpGain(pre_amp_gain_index);
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 20;
		sprintf(Setup_Error_String,"CCD_Setup_Set_Pre_Amp_Gain: SetPreAmpGain(pre_amp_gain_index=%d) "
			"failed %s(%u).",pre_amp_gain_index,CCD_General_Andor_ErrorCode_To_String(andor_retval),
			andor_retval);
		return FALSE;
	}
	Setup_Data.Pre_Amp_Gain_Index = pre_amp_gain_index;
	/* get the actual pre-amp gain factor as reported by the camera and store it in Setup_Data.Pre_Amp_Gain */
	andor_retval = GetPreAmpGain(pre_amp_gain_index,&(Setup_Data.Pre_Amp_Gain));
	if(andor_retval != DRV_SUCCESS)
	{
		Setup_Error_Number = 26;
		sprintf(Setup_Error_String,"CCD_Setup_Set_Pre_Amp_Gain: GetPreAmpGain(pre_amp_gain_index=%d) "
			"failed %s(%u).",pre_amp_gain_index,CCD_General_Andor_ErrorCode_To_String(andor_retval),
			andor_retval);
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Set_Pre_Amp_Gain",LOG_VERBOSITY_TERSE,"CCD",
			"CCD_Setup_Set_Pre_Amp_Gain Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to set whether the exposure code will flip the read out image in the horizontal/X direction.
 * This just sets a flag in Setup_Data which is retrieved by the exposure code.
 * @param flip_x Whether to flip the readout data in the horizontal/X direction - a boolean.
 * @see #Setup_Data
 * @see CCD_GENERAL_IS_BOOLEAN
 */
int CCD_Setup_Set_Flip_X(int flip_x)
{
	if(!CCD_GENERAL_IS_BOOLEAN(flip_x))
	{
		Setup_Error_Number = 27;
		sprintf(Setup_Error_String,"CCD_Setup_Set_Flip_X: Argument flip_x (%d) was not a boolean.",flip_x);
		return FALSE;
	}
	Setup_Data.Flip_X = flip_x;
	return TRUE;
}

/**
 * Routine to set whether the exposure code will flip the read out image in the vertical/Y direction.
 * This just sets a flag in Setup_Data which is retrieved by the exposure code.
 * @param flip_y Whether to flip the readout data in the vertical/Y direction - a boolean.
 * @see #Setup_Data
 * @see CCD_GENERAL_IS_BOOLEAN
 */
int CCD_Setup_Set_Flip_Y(int flip_y)
{
	if(!CCD_GENERAL_IS_BOOLEAN(flip_y))
	{
		Setup_Error_Number = 37;
		sprintf(Setup_Error_String,"CCD_Setup_Set_Flip_Y: Argument flip_y (%d) was not a boolean.",flip_y);
		return FALSE;
	}
	Setup_Data.Flip_Y = flip_y;
	return TRUE;
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
 * Return whether or not to flip the read out image in the horizontal/X direction.
 * @return A boolean, if TRUE flip the read out image in the horizontal/X direction, otherwise don't.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Flip_X(void)
{
	return Setup_Data.Flip_X;
}

/**
 * Return whether or not to flip the read out image in the vertical/Y direction.
 * @return A boolean, if TRUE flip the read out image in the vertical/Y direction, otherwise don't.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Flip_Y(void)
{
	return Setup_Data.Flip_Y;
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
 * Return the current horizontal shift speed, last configured using CCD_Setup_Set_HS_Speed.
 * CCD_Setup_Set_HS_Speed stores this value in Setup_Data, and it is this that is returned.
 * @return The current horizontal shift speed in use by the camera, in MHz.
 * @see #Setup_Data
 * @see CCD_Setup_Set_HS_Speed
 */
float CCD_Setup_Get_HS_Speed(void)
{
	return Setup_Data.HS_Speed;
}

/**
 * Return the current horizontal shift speed index, which was used to configure the camera in CCD_Setup_Set_HS_Speed.
 * CCD_Setup_Set_HS_Speed stores this value in Setup_Data, and it is this that is returned.
 * @return The current horizontal shift speed index.
 * @see #Setup_Data
 * @see CCD_Setup_Set_HS_Speed
 */
int CCD_Setup_Get_HS_Speed_Index(void)
{
	return Setup_Data.HS_Speed_Index;
}

/**
 * Return the current vertical shift speed, last configured using CCD_Setup_Set_VS_Speed.
 * CCD_Setup_Set_VS_Speed stores this value in Setup_Data, and it is this that is returned.
 * @return The current vertical shift speed in use by the camera, in microseconds/pixel.
 * @see #Setup_Data
 * @see CCD_Setup_Set_VS_Speed
 */
float CCD_Setup_Get_VS_Speed(void)
{
	return Setup_Data.VS_Speed;
}

/**
 * Return the current vertical shift speed index, which was used to configure the camera in CCD_Setup_Set_VS_Speed.
 * CCD_Setup_Set_VS_Speed stores this value in Setup_Data, and it is this that is returned.
 * @return The current vertical shift speed index.
 * @see #Setup_Data
 * @see CCD_Setup_Set_VS_Speed
 */
int CCD_Setup_Get_VS_Speed_Index(void)
{
	return Setup_Data.VS_Speed_Index;
}

/**
 * Return the current pre amp gain factor, last configured using CCD_Setup_Set_Pre_Amp_Gain.
 * CCD_Setup_Set_Pre_Amp_Gain stores this value in Setup_Data, and it is this that is returned.
 * @return The current pre-amp gain factor in use by the camera.
 * @see #Setup_Data
 * @see CCD_Setup_Set_Pre_Amp_Gain
 */
float CCD_Setup_Get_Pre_Amp_Gain(void)
{
	return Setup_Data.Pre_Amp_Gain;
}

/**
 * Return the current pre-amp gain index, which was used to configure the camera in CCD_Setup_Set_Pre_Amp_Gain.
 * CCD_Setup_Set_Pre_Amp_Gain stores this value in Setup_Data, and it is this that is returned.
 * @return The current pre-amp gain index.
 * @see #Setup_Data
 * @see CCD_Setup_Set_Pre_Amp_Gain
 */
int CCD_Setup_Get_Pre_Amp_Gain_Index(void)
{
	return Setup_Data.Pre_Amp_Gain_Index;
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
	CCD_General_Log_Format("setup","ccd_setup.c","CCD_Setup_Get_Buffer_Length",LOG_VERBOSITY_VERBOSE,"CCD",
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
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Allocate_Image_Buffer",LOG_VERBOSITY_VERBOSE,"CCD",
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
	CCD_General_Log("setup","ccd_setup.c","CCD_Setup_Allocate_Image_Buffer",LOG_VERBOSITY_VERBOSE,"CCD",
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

