/**
 * @file
 * @brief Camera.cpp provides an implementation of the camera detector thrift interface. This implementation
 *        makes the relevant calls to the CCD library for each thrift API call.
 * @author Chris Mottram
 * @version $Id$
 */
#include "CameraConfig.h"
#include "Camera.h"
#include <thread>
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thrift/Thrift.h>
#include <boost/program_options.hpp>
#include "log4cxx/logger.h"

#include <stdio.h>
#include <time.h>

#include "ccd_exposure.h"
#include "ccd_fits_filename.h"
#include "ccd_fits_header.h"
#include "ccd_general.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

using std::cout;
using std::cerr;
using std::endl;
using std::exception;
using namespace ::apache::thrift;
using namespace log4cxx;

/**
 * The configuration file section to use for retrieving configuration ("camera").
 */
#define CONFIG_CAMERA_SECTION ("Camera")
/**
 * Convertion from degrees Centigrade to degrees Kelvin.
 */
#define DEGREES_CENTIGRADE_TO_KELVIN      (273.15)
/**
 * The length of an error buffer to use when retrieving errors from the CCD library.
 */
#define ERROR_BUFFER_LENGTH  (1024)

/**
 * Logger instance for the Andor Camera (Camera.cpp).
 */
static LoggerPtr logger(Logger::getLogger("mookodi.camera.server.Camera"));

/* internal C functions */
static void log_to_log4cxx(char *sub_system,char *source_filename,char *function,
			   int level,char *category,char *string);

/**
 * Constructor for the Camera object.
 */
Camera::Camera()
{
}

/**
 * Destructor for the Camera object. Does nothing.
 */
Camera::~Camera()
{
}

/**
 * Method to set the configuration object to load config from in initialize.
 * @param config The configuration object.
 * @see Camera::mCameraConfig
 */
void Camera::set_config(CameraConfig & config)
{
	mCameraConfig = config;
}

/**
 * Initialisation method for the camera object. This does the following:
 * <ul>
 * <li>We set the CCD library log handler function to log to stdout.
 * <li>We retrieve the "andor.config_dir" configuration value and use it to set the Andor config directory in the
 *     CCD library using CCD_Setup_Config_Directory_Set.
 * <li>We connect to and initialise the camera using CCD_Setup_Startup.
 * <li>We configure the initial readout speed to SLOW using set_readout_speed, and pre-amp gain to ONE using set_gain.
 * <li>We retrieve the fits filename instrument code "fits.instrument_code", 
 *     the data directory root "fits.data_dir.root", 
 *     the telescope component of the data directory "fits.data_dir.telescope",
 *     and the instument component of the data directory "fits.data_dir.instrument",
 *     from the config file values in mCameraConfig, and use them to initialise FITS filename generation using 
 *     CCD_Fits_Filename_Initialise.
 * <li>We initialise the FITS headers (stored in mFitsHeader) using CCD_Fits_Header_Initialise.
 * <li>We setup the cached image data (used to configure the CCD windowing/binning). Some of the
 *     values are read from the config object ("ccd.ncols" / "ccd.nrows").
 * <li>We configure the detector readout dimensions to the cached ones using CCD_Setup_Dimensions.
 * <li>We configure how the read out images are flipped after readout, by retrieving from config the 
 *     "ccd.image.flip.x" / "ccd.image.flip.y" booleans and using CCD_Setup_Set_Flip_X / CCD_Setup_Set_Flip_Y 
 *     to configure the CCD library appropriately.
 * <li>We initialise mExposureCount and mExposureIndex to zero.
 * <li>We initialise mImageBufNCols / mImageBufNRows to zero.
 * <li>We initialise mLastImageFilename and mImageFilenameList to empty strings/vectors.
 * </ul>
 * If a CCD library routine fails we call create_ccd_library_exception to create a CameraException that is then thrown.
 * @see #CONFIG_CAMERA_SECTION
 * @see Camera::mCameraConfig
 * @see Camera::mFitsHeader
 * @see Camera::mCachedNCols
 * @see Camera::mCachedNRows
 * @see Camera::mCachedHBin
 * @see Camera::mCachedVBin
 * @see Camera::mCachedWindowFlags
 * @see Camera::mCachedWindow
 * @see Camera::mExposureCount
 * @see Camera::mExposureIndex
 * @see Camera::mImageBufNCols
 * @see Camera::mImageBufNRows
 * @see Camera::mLastImageFilename
 * @see Camera::mImageFilenameList
 * @see Camera::set_readout_speed
 * @see Camera::set_gain
 * @see Camera::create_ccd_library_exception
 * @see CameraConfig::get_config_string
 * @see CameraConfig::get_config_int
 * @see CameraConfig::get_config_boolean
 * @see ReadoutSpeed
 * @see Gain
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_General_Set_Log_Handler_Function
 * @see CCD_General_Log_Handler_Stdout
 * @see CCD_Setup_Config_Directory_Set
 * @see CCD_Setup_Startup
 * @see CCD_Setup_Dimensions
 * @see CCD_Setup_Set_Flip_X
 * @see CCD_Setup_Set_Flip_Y
 * @see CCD_Fits_Filename_Initialise
 * @see CCD_Fits_Header_Initialise
 * @see log_to_log4cxx
 */
void Camera::initialize()
{
	CameraException ce;
	char config_dir[256];
	char fits_data_dir_root[32];
	char fits_data_dir_telescope[32];
	char fits_data_dir_instrument[32];
	char instrument_code[32];
	int retval,flip_x,flip_y;
	
	cout << "Initialising Camera." << endl;
	LOG4CXX_INFO(logger,"Initialising Camera.");
	/* setup loggers */
	CCD_General_Set_Log_Handler_Function(log_to_log4cxx);
	/* setup andor config directory */
	mCameraConfig.get_config_string(CONFIG_CAMERA_SECTION,"andor.config_dir",config_dir,256);
	retval = CCD_Setup_Config_Directory_Set(config_dir);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* connect to and initialise camera head */
	retval = CCD_Setup_Startup();
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* configure initial readout speed and pre-amp gain */
	set_readout_speed(ReadoutSpeed::SLOW);
	set_gain(Gain::ONE);
	/* initialise FITS filename generation code */
	mCameraConfig.get_config_string(CONFIG_CAMERA_SECTION,"fits.instrument_code",instrument_code,32);
	mCameraConfig.get_config_string(CONFIG_CAMERA_SECTION,"fits.data_dir.root",fits_data_dir_root,32);
	mCameraConfig.get_config_string(CONFIG_CAMERA_SECTION,"fits.data_dir.telescope",fits_data_dir_telescope,32);
	mCameraConfig.get_config_string(CONFIG_CAMERA_SECTION,"fits.data_dir.instrument",fits_data_dir_instrument,32);
	retval = CCD_Fits_Filename_Initialise(instrument_code,fits_data_dir_root,fits_data_dir_telescope,
					      fits_data_dir_instrument);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* initialise FITS header data */
	retval = CCD_Fits_Header_Initialise(&mFitsHeader);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* setup cached image dimension data */
	mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.ncols",&mCachedNCols);
	mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.nrows",&mCachedNRows);
	mCachedHBin = 1;
	mCachedVBin = 1;
	mCachedWindowFlags = false;
	mCachedWindow.X_Start = 0;
	mCachedWindow.Y_Start = 0;
	mCachedWindow.X_End = 0;
	mCachedWindow.Y_End = 0;
	/* set the image dimensions to these defaults */
	cout << "Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		" binning ( " << mCachedHBin << ", " << mCachedVBin <<
		" ), Use Window " << mCachedWindowFlags <<
		", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		mCachedWindow.X_End << "," << mCachedWindow.Y_End << ")." << endl;
	LOG4CXX_INFO(logger,"Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		" binning ( " << mCachedHBin << ", " << mCachedVBin <<
		" ), Use Window " << mCachedWindowFlags <<
		", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		mCachedWindow.X_End << "," << mCachedWindow.Y_End << ").");
	retval = CCD_Setup_Dimensions(mCachedNCols,mCachedNRows,mCachedHBin,mCachedVBin,mCachedWindowFlags,
				      mCachedWindow);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* configure read out image flipping code from the config file */
	mCameraConfig.get_config_boolean(CONFIG_CAMERA_SECTION,"ccd.image.flip.x",&flip_x);
	mCameraConfig.get_config_boolean(CONFIG_CAMERA_SECTION,"ccd.image.flip.y",&flip_y);
	retval = CCD_Setup_Set_Flip_X(flip_x);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	retval = CCD_Setup_Set_Flip_Y(flip_y);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* initialise camera status variables */
	mExposureCount = 0;
	mExposureIndex = 0;
	mImageBufNCols = 0;
	mImageBufNRows = 0;
	mLastImageFilename = "";
	mImageFilenameList = {};	
}

/**
 * Set the binning to use when reading out the CCD.
 * <ul>
 * <li>We set the cached binning variables mCachedHBin and mCachedVBin to the input parameters.
 * <li>We call CCD_Setup_Dimensions with the cached detector binning/window dimensions to configure
 *     the detector to the new dimensions.
 * </ul>
 * If CCD_Setup_Dimensions fails we call create_ccd_library_exception to create a CameraException that is then thrown.
 * @param xbin The binning to use in the X/horizontal direction. Should be at least 1.
 * @param ybin The binning to use in the Y/vertical direction. Should be at least 1.
 * @see Camera::mCachedNCols
 * @see Camera::mCachedNRows
 * @see Camera::mCachedHBin
 * @see Camera::mCachedVBin
 * @see Camera::mCachedWindowFlags
 * @see Camera::mCachedWindow
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CameraException
 * @see CCD_Setup_Dimensions
 */
void Camera::set_binning(const int8_t xbin, const int8_t ybin)
{
	CameraException ce;
	int retval;
	
	cout << "Set binning to " << int(xbin) << ", " << int(ybin) << endl;
	LOG4CXX_INFO(logger,"Set binning to " << int(xbin) << ", " << int(ybin));
	mCachedHBin = xbin;
	mCachedVBin = ybin;
	cout << "Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		" binning ( " << mCachedHBin << ", " << mCachedVBin <<
		" ), Use Window " << mCachedWindowFlags <<
		", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		mCachedWindow.X_End << "," << mCachedWindow.Y_End << ")." << endl;
	LOG4CXX_INFO(logger,"Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		" binning ( " << mCachedHBin << ", " << mCachedVBin <<
		" ), Use Window " << mCachedWindowFlags <<
		", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		mCachedWindow.X_End << "," << mCachedWindow.Y_End << ").");
	retval = CCD_Setup_Dimensions(mCachedNCols,mCachedNRows,mCachedHBin,mCachedVBin,mCachedWindowFlags,
				      mCachedWindow);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
}

/**
 * Set the window (region of interest) to use when reading out the CCD.
 * <ul>
 * <li>We set mCachedWindowFlags to true, to tell CCD_Setup_Dimensions to use the window.
 * <li>We setup the cached window mCachedWindow based on the input parameters.
 * <li>We call CCD_Setup_Dimensions with the cached detector binning/window dimensions to configure
 *     the detector to the new dimensions.
 * </ul>
 * If CCD_Setup_Dimensions fails we call create_ccd_library_exception to create a CameraException that is then thrown.
 * @param x_start The start X pixel position of the sub-window. Should be at least 1, 
 *        and less than the X size of the detector.
 * @param y_start The start Y pixel position of the sub-window. Should be at least 1, 
 *        and less than the Y size of the detector.
 * @param x_end The end X pixel position of the sub-window. Should be at least 1, 
 *        less than the X size of the detector, and greater than x_start.
 * @param y_end The end Y pixel position of the sub-window. Should be at least 1, 
 *        less than the Y size of the detector, and greater than y_start.
 * @see Camera::mCachedNCols
 * @see Camera::mCachedNRows
 * @see Camera::mCachedHBin
 * @see Camera::mCachedVBin
 * @see Camera::mCachedWindowFlags
 * @see Camera::mCachedWindow
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CameraException
 * @see CCD_Setup_Dimensions
 */
void Camera::set_window(const int32_t x_start, const int32_t y_start, const int32_t x_end, const int32_t y_end)
{
	CameraException ce;
	int retval;

	cout << "Set window to start position ( " << x_start << ", " << y_start <<
		" ), end position ( " << x_end << ", " << y_end << " )."<< endl;
	LOG4CXX_INFO(logger,"Set window to start position ( " << x_start << ", " << y_start <<
		     " ), end position ( " << x_end << ", " << y_end << " ).");
	mCachedWindowFlags = true;
	mCachedWindow.X_Start = x_start;
	mCachedWindow.Y_Start = y_start;
	mCachedWindow.X_End = x_end;
	mCachedWindow.Y_End = y_end;
	cout << "Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		" binning ( " << mCachedHBin << ", " << mCachedVBin <<
		" ), Use Window " << mCachedWindowFlags <<
		", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		mCachedWindow.X_End << "," << mCachedWindow.Y_End << ")." << endl;
	LOG4CXX_INFO(logger,"Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		     " binning ( " << mCachedHBin << ", " << mCachedVBin <<
		     " ), Use Window " << mCachedWindowFlags <<
		     ", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		     mCachedWindow.X_End << "," << mCachedWindow.Y_End << ").");
	retval = CCD_Setup_Dimensions(mCachedNCols,mCachedNRows,mCachedHBin,mCachedVBin,mCachedWindowFlags,
				      mCachedWindow);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
}

/**
 * Clear a previously set sub-window i.e. re-configure the deterctor to read out full-frame.
 * <ul>
 * <li>We set mCachedWindowFlags to false, to tell CCD_Setup_Dimensions to not use the window data.
 * <li>We call CCD_Setup_Dimensions with the cached detector binning/window dimensions to configure
 *     the detector to the new dimensions.
 * </ul>
 * If CCD_Setup_Dimensions fails we call create_ccd_library_exception to create a CameraException that is then thrown.
 * @see Camera::mCachedNCols
 * @see Camera::mCachedNRows
 * @see Camera::mCachedHBin
 * @see Camera::mCachedVBin
 * @see Camera::mCachedWindowFlags
 * @see Camera::mCachedWindow
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Setup_Dimensions
 */
void Camera::clear_window()
{
	CameraException ce;
	int retval;

	cout << "Clear window." << endl;
	LOG4CXX_INFO(logger,"Clear window.");
	mCachedWindowFlags = false;
	cout << "Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		" binning ( " << mCachedHBin << ", " << mCachedVBin <<
		" ), Use Window " << mCachedWindowFlags <<
		", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		mCachedWindow.X_End << "," << mCachedWindow.Y_End << ")." << endl;
	LOG4CXX_INFO(logger,"Configure CCD using ncols " << mCachedNCols << ", nrows " << mCachedNRows <<
		" binning ( " << mCachedHBin << ", " << mCachedVBin <<
		" ), Use Window " << mCachedWindowFlags <<
		", Window (" << mCachedWindow.X_Start << "," << mCachedWindow.Y_Start << "," <<
		     mCachedWindow.X_End << "," << mCachedWindow.Y_End << ").");
	retval = CCD_Setup_Dimensions(mCachedNCols,mCachedNRows,mCachedHBin,mCachedVBin,mCachedWindowFlags,
				      mCachedWindow);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
}

/**
 * Set the readout speed of the camera (either SLOW or FAST).
 * <ul>
 * <li>We retrieve the horizontal shift speed index from the config (mCameraConfig), using the camera section key
 *     "ccd.readout_speed.hs_speed_index.SLOW|FAST".
 * <li>We retrieve the vertical shift speed index from the config (mCameraConfig), using the camera section key
 *     "ccd.readout_speed.vs_speed_index.SLOW|FAST".
 * <li>We retrieve the vertical clock voltage amplitude from the config (mCameraConfig), using the camera section key
 *     "ccd.readout_speed.vs_amplitude.SLOW|FAST".
 * <li>We configure the camera's horizontal shift speed by calling. CCD_Setup_Set_HS_Speed.
 * <li>We configure the camera's vertical shift speed by calling. CCD_Setup_Set_VS_Speed.
 * <li>We configure the camera's vertical clock speed amplitude by calling. CCD_Setup_Set_VS_Amplitude.
 * <li>We update mCachedReadoutSpeed to reflect the newly configured readout speed.
 * </ul>
 * If an error occurs configuring the camera or retrieving the config, a CameraException is thrown.
 * @param speed The readout speed, of type ReadoutSpeed.
 * @see Camera::mCameraConfig
 * @see Camera::mCachedReadoutSpeed
 * @see #CONFIG_CAMERA_SECTION
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see LOG4CXX_DEBUG
 * @see ReadoutSpeed
 * @see CCD_Setup_Set_HS_Speed
 * @see CCD_Setup_Set_VS_Speed
 * @see CCD_Setup_Set_VS_Amplitude
 */
void Camera::set_readout_speed(const ReadoutSpeed::type speed)
{
	CameraException ce;
	std::string keyword;
	int retval;
	int hs_speed_index,vs_speed_index,vs_amplitude;
	
	cout << "Set readout speed to " << to_string(speed) << "." << endl;
	LOG4CXX_INFO(logger,"Set readout speed to " << to_string(speed) << ".");
	/* Get the horizontal and vertical shift speeds configured for this readout speed */
	keyword = "ccd.readout_speed.hs_speed_index."+to_string(speed);
	mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,keyword.c_str(),&hs_speed_index);
	keyword = "ccd.readout_speed.vs_speed_index."+to_string(speed);
	mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,keyword.c_str(),&vs_speed_index);
	keyword = "ccd.readout_speed.vs_amplitude."+to_string(speed);
	mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,keyword.c_str(),&vs_amplitude);
	/* configure the camera appropriately */
	LOG4CXX_DEBUG(logger,"Using horizontal shift speed index " << std::to_string(hs_speed_index) << ".");
	retval = CCD_Setup_Set_HS_Speed(hs_speed_index);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	LOG4CXX_DEBUG(logger,"Using vertical shift speed index " << std::to_string(vs_speed_index) << ".");
	retval = CCD_Setup_Set_VS_Speed(vs_speed_index);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	LOG4CXX_DEBUG(logger,"Using vertical clock amplitude " << std::to_string(vs_amplitude) << ".");
	retval = CCD_Setup_Set_VS_Amplitude(vs_amplitude);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* we update the cached value, used for status */
	mCachedReadoutSpeed = speed;
	LOG4CXX_INFO(logger,"Readout speed set to " << to_string(speed) << ".");
}

/**
 * Set the gain of the camera.
 * <ul>
 * <li>We convert the gain_number gain factor to a pre amp gain index as follows (for an Andor iKon M934);
 *     <ul>
 *     <li><b>Pre-amp gain index     "gain factor"  Gain (gain_number)</b>
 *     <li>0                      1.0            ONE
 *     <li>1                      2.0            TWO
 *     <li>2                      4.0            FOUR
 *     </ul>
 * <li>We call CCD_Setup_Set_Pre_Amp_Gain to configure the camera's gain.
 * </ul>
 * @param gain_number The gain factor to configure the camera with of type Gain.
 * @see Gain
 * @see CCD_Setup_Set_Pre_Amp_Gain
 * @see #mCachedGain
 */
void Camera::set_gain(const Gain::type gain_number)
{
	CameraException ce;
	int retval,pre_amp_gain_index;
	
	cout << "Set gain to " << to_string(gain_number) << "." << endl;
	LOG4CXX_INFO(logger,"Set gain to " << to_string(gain_number) << ".");
	/* CCD_Setup_Set_Pre_Amp_Gain takes a pre amp gain index, which according to test_andor_readout_speed_gains.c
	** has the following values for an Andor iKon M934:
	** Pre-amp gain index     "gain factor"  Gain (gain_number)
	** 0                      1.0            ONE
	** 1                      2.0            TWO
	** 2                      4.0            FOUR
	*/
	switch(gain_number)
	{
		case Gain::ONE:
			pre_amp_gain_index = 0;
			break;
		case Gain::TWO:
			pre_amp_gain_index = 1;
			break;
		case Gain::FOUR:
			pre_amp_gain_index = 2;
			break;
		default:
			ce.message = "set_gain: gain_number "+ to_string(gain_number) +" is not supported.";
			throw ce;
			break;
	}
	LOG4CXX_DEBUG(logger,"Gain " << to_string(gain_number) << " has pre-amp gain index of " <<
		      std::to_string(pre_amp_gain_index) << ".");
	retval = CCD_Setup_Set_Pre_Amp_Gain(pre_amp_gain_index);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* update the cached gain value (used for status serving */
	mCachedGain = gain_number;
	LOG4CXX_INFO(logger,"Gain now set to " << to_string(gain_number) <<
		     " , pre-amp gain index " << std::to_string(pre_amp_gain_index) <<".");
}

/**
 * Add a list of  FITS header to the list of FITS headers to be saved to FITS images.
 * If A FITS header with the specified keyword already exists in the list of headers, it's contents are updated.
 * @param fits_info The list of FITS header cards to add to the FITS header.
 * @see FitsHeaderCard
 * @see logger
 * @see LOG4CXX_INFO
 * @see Camera::add_fits_header
 */
void Camera::set_fits_headers(const std::vector<FitsHeaderCard> & fits_info)
{
	cout << "Set FITS headers." << endl;
	LOG4CXX_INFO(logger,"Set FITS headers.");
	for (auto it = begin(fits_info); it != end(fits_info); ++it )
	{
		add_fits_header(it->key,it->valtype,it->val,it->comment);
	}
}

/**
 * Add a FITS header to the list of FITS headers to be saved to FITS images.
 * If A FITS header with the specified keyword already exists in the list of headers, it's contents are updated.
 * @param keyword The FITS header card keyword.
 * @param valtype What kind of value this header takes, of type FitsCardType.
 * @param value A string representation of the value.
 * @param comment A string containing a comment for this header.
 * @see FitsCardType
 * @see Camera::mFitsHeader
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Fits_Header_Add_Int
 * @see CCD_Fits_Header_Add_Float
 * @see CCD_Fits_Header_Add_String
 */
void Camera::add_fits_header(const std::string & keyword, const FitsCardType::type valtype,const std::string & value,
			 const std::string & comment)
{
	CameraException ce;
	int retval,ivalue;
	double dvalue;
	
	cout << "Add FITS header " << keyword  << " of type " << to_string(valtype) << " and value " << value << endl;
	LOG4CXX_INFO(logger,"Add FITS header " << keyword  << " of type " << to_string(valtype) <<
		     " and value " << value );
	switch(valtype)
	{
		case FitsCardType::INTEGER:
			retval = sscanf(value.c_str(),"%d",&ivalue);
			if(retval != 1)
			{
				ce.message = "add_fits_header: Failed to parse string "+ value +" to an integer ("+
					std::to_string(retval) +").";
				throw ce;
			}
			retval = CCD_Fits_Header_Add_Int(&mFitsHeader,(char*)(keyword.c_str()),ivalue,
							 (char*)(comment.c_str()));
			if(retval == FALSE)
			{
				ce = create_ccd_library_exception();
				throw ce;
			}	
			break;
		case FitsCardType::FLOAT:
			retval = sscanf(value.c_str(),"%lf",&dvalue);
			if(retval != 1)
			{
				ce.message = "add_fits_header: Failed to parse string "+ value +" to a double ("+
					std::to_string(retval) +").";
				throw ce;
			}
			retval = CCD_Fits_Header_Add_Float(&mFitsHeader,(char*)(keyword.c_str()),dvalue,
							   (char*)(comment.c_str()));
			if(retval == FALSE)
			{
				ce = create_ccd_library_exception();
				throw ce;
			}	
			break;
		case FitsCardType::STRING:
			retval = CCD_Fits_Header_Add_String(&mFitsHeader,(char*)(keyword.c_str()),(char*)(value.c_str()),
							    (char*)(comment.c_str()));
			if(retval == FALSE)
			{
				ce = create_ccd_library_exception();
				throw ce;
			}	
			break;
		default:
			break;
	}
}

/**
 * Entry point to a routine to clear out FITS headers.
 * @see Camera::mFitsHeader
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Fits_Header_Clear
 */
void Camera::clear_fits_headers()
{
	CameraException ce;
	int retval;
	
	cout << "Clear FITS headers." << endl;
	LOG4CXX_INFO(logger,"Clear FITS headers.");
	retval = CCD_Fits_Header_Clear(&mFitsHeader);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}	
}

/**
 * thrift entry point to take an exposure with the camera. 
 * A new thread running an instance of expose_thread is started.
 * @param exposure_length The exposure length of each frame in milliseconds. Must be at least 1 ms.
 * @param save_image A boolean, if true save the image in a FITS filename, otherwise don't 
 *        (the image data can be retrieved using the get_image_data method).
 * @see Camera::expose_thread
 * @see Camera::get_image_data
 * @see logger
 * @see LOG4CXX_INFO
 */
void Camera::start_expose(const int32_t exposure_length, const bool save_image)
{
	CameraException ce;

	cout << "Starting expose thread with exposure length " << exposure_length <<
		"ms and save_image " << save_image << "." << endl;
	LOG4CXX_INFO(logger,"Starting expose thread with exposure length " << exposure_length <<
		     "ms and save_image " << save_image << ".");
	std::thread thrd(&Camera::expose_thread, this, exposure_length, save_image);
	thrd.detach();
}

/**
 * thrift entry point to start taking multiple biases. A new thread running an instance of multbias_thread is started.
 * @param exposure_count The number of biases to take. Must be at least one.
 * @see Camera::multbias_thread
 */
void Camera::start_multbias(const int32_t exposure_count)
{
	cout << "Starting multbias thread with exposure count " << exposure_count << "." << endl;
	LOG4CXX_INFO(logger,"Starting multbias thread with exposure count " << exposure_count << ".");
	std::thread thrd(&Camera::multbias_thread, this, exposure_count);
	thrd.detach();
}

/**
 * thrift entry point to start taking multiple dark frames. A new thread running an instance of multdark_thread is started.
 * @param exposure_count The number of dark frames to take. Must be at least one.
 * @param exposure_length The exposure length of each dark in milliseconds. Must be at least 1 ms.
 * @see Camera::multdark_thread
 * @see logger
 * @see LOG4CXX_INFO
 */
void Camera::start_multdark(const int32_t exposure_count,const int32_t exposure_length)
{
	cout << "Starting multdark thread with exposure count " << exposure_count <<
		", exposure length " << exposure_length << "ms." << endl;
	LOG4CXX_INFO(logger,"Starting multdark thread with exposure count " << exposure_count <<
		     ", exposure length " << exposure_length << "ms.");
	std::thread thrd(&Camera::multdark_thread, this, exposure_count, exposure_length);
	thrd.detach();
}

/**
 * thrift entry point to start taking multiple science frames. 
 * A new thread running an instance of multrun_thread is started.
 * @param exposure_count The number of frames to take. Must be at least one.
 * @param exposure_length The exposure length of each frame in milliseconds. Must be at least 1 ms.
 * @see Camera::multrun_thread
 * @see logger
 * @see LOG4CXX_INFO
 */
void Camera::start_multrun(const int32_t exposure_count,const int32_t exposure_length)
{
	CameraException ce;

	cout << "Starting multrun thread with exposure count " << exposure_count <<
		", exposure length " << exposure_length << "ms." << endl;
	LOG4CXX_INFO(logger,"Starting multrun thread with exposure count " << exposure_count <<
		     ", exposure length " << exposure_length << "ms.");
	std::thread thrd(&Camera::multrun_thread, this, exposure_count, exposure_length);
	thrd.detach();
}

/**
 * Abort a running multrun/multdark/multbias. 
 * This calls CCD_Exposure_Abort to attempt to stop a running multrun/multdark/multbias.
 * If CCD_Exposure_Abort fails we call create_ccd_library_exception to create a CameraException that is then thrown.
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Exposure_Abort
 */
void Camera::abort_exposure()
{
	CameraException ce;
	int retval;
	
	cout << "Abort exposure." << endl;
	LOG4CXX_INFO(logger,"Abort exposure.");
	retval = CCD_Exposure_Abort();
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}	
}

/**
 * Get the current state of the camera.
 * <ul>
 * <li>We call various CCD_Setup library routines to get the currently configured image dimensions 
 *     (CCD_Setup_Get_Bin_X / CCD_Setup_Get_Bin_Y / CCD_Setup_Is_Window / 
 *     CCD_Setup_Get_Horizontal_Start / CCD_Setup_Get_Vertical_Start / 
 *     CCD_Setup_Get_Horizontal_End / CCD_Setup_Get_Vertical_End).
 * <li>We call CCD_Exposure_Length_Get to get the current exposure length.
 * <li>We fill in the exposure_count and exposure_index status from mExposureCount/mExposureIndex
 * <li>We call CCD_Exposure_Start_Time_Get to get a timestamp of when the last exposure started.
 * <li>We call clock_gettime to get a current timestamp.
 * <li>We call CCD_Exposure_Status_Get to get the CCD library's exposure status.
 * <li>Based on the exposure status, exposure length, exposure start time and current time, we set 
 *     the status's exposure_state, elapsed_exposure_length and remaining_exposure_length state.
 * <li>Based on the exposure status, we either retrieve the current CCD temperature using CCD_Temperature_Get, or if
 *     the detector is currently exposing or reading out, a cached temperature using CCD_Temperature_Get_Cached_Temperature.
 * <li>We set the readout speed status to mCachedReadoutSpeed.
 * <li>We set the gain status to mCachedGain.
 * </ul>
 * If retrieving the CCD temperature fails the method can throw a CameraException 
 * (created using create_ccd_library_exception).
 * @param state An instance of CameraState that we set the member values to the current status.
 * @see Camera::mCachedNCols
 * @see Camera::mCachedNRows
 * @see Camera::mCachedHBin
 * @see Camera::mCachedVBin
 * @see Camera::mCachedWindowFlags
 * @see Camera::mCachedWindow
 * @see Camera::mCachedReadoutSpeed
 * @see Camera::mCachedGain
 * @see Camera::mExposureCount
 * @see Camera::mExposureIndex
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_EXPOSURE_STATUS
 * @see CCD_Exposure_Length_Get
 * @see CCD_Exposure_Status_Get
 * @see CCD_GENERAL_ONE_SECOND_MS
 * @see CCD_Setup_Get_Bin_X
 * @see CCD_Setup_Get_Bin_Y
 * @see CCD_Setup_Is_Window
 * @see CCD_Setup_Get_Horizontal_Start
 * @see CCD_Setup_Get_Vertical_Start
 * @see CCD_Setup_Get_Horizontal_End
 * @see CCD_Setup_Get_Vertical_End
 * @see CCD_TEMPERATURE_STATUS
 * @see CCD_Temperature_Get
 * @see CCD_Temperature_Get_Cached_Temperature
 * @see CameraState
 */
void Camera::get_state(CameraState &state)
{
	CameraException ce;
	enum CCD_EXPOSURE_STATUS ccd_library_exposure_status;
	enum CCD_TEMPERATURE_STATUS temperature_status;
	struct timespec start_time,current_time,cache_date_stamp;
	int retval;
	
	cout << "Get camera state." << endl;
	LOG4CXX_INFO(logger,"Get camera state.");
	state.xbin = CCD_Setup_Get_Bin_X();
	state.ybin = CCD_Setup_Get_Bin_Y();
	state.use_window = CCD_Setup_Is_Window();
	state.window.x_start = CCD_Setup_Get_Horizontal_Start();
	state.window.y_start = CCD_Setup_Get_Vertical_Start();
	state.window.x_end = CCD_Setup_Get_Horizontal_End();
	state.window.y_end = CCD_Setup_Get_Vertical_End();
	state.exposure_length = CCD_Exposure_Length_Get();
	state.exposure_count = mExposureCount;
	state.exposure_index = mExposureIndex;
	CCD_Exposure_Start_Time_Get(&start_time);
	clock_gettime(CLOCK_REALTIME,&current_time);
	ccd_library_exposure_status = CCD_Exposure_Status_Get();
	switch(ccd_library_exposure_status)
	{
		case CCD_EXPOSURE_STATUS_NONE:
			state.exposure_state = ExposureState::IDLE;
			state.elapsed_exposure_length = 0;
			state.remaining_exposure_length = 0;
			break;
		case CCD_EXPOSURE_STATUS_WAIT_START:
			state.exposure_state = ExposureState::SETUP;
			state.elapsed_exposure_length = 0;
			state.remaining_exposure_length = 0;
			break;
		case CCD_EXPOSURE_STATUS_EXPOSE:
			state.exposure_state = ExposureState::EXPOSING;
			/* fdifftime returns difference between timestamps in seconds.
			** exposure_length state is in milliseconds. */
			state.elapsed_exposure_length = fdifftime(current_time,start_time)*
				((double)CCD_GENERAL_ONE_SECOND_MS);
			state.remaining_exposure_length = state.exposure_length-state.elapsed_exposure_length;
			/* check we have not already reached the end of the exposure,
			** but not started reading out yet */
			if(state.elapsed_exposure_length > state.exposure_length)
			{
				state.elapsed_exposure_length = state.exposure_length;
				state.remaining_exposure_length = 0;
			}
			break;
		case CCD_EXPOSURE_STATUS_READOUT:
			state.exposure_state = ExposureState::READOUT;
			state.elapsed_exposure_length = state.exposure_length;
			state.remaining_exposure_length = 0;
			break;
		default:
			/* we could throw an exception here */
			break;
	}/* end switch */
	/* we can only get the current temperature when the detector is not exposing or reading out 
	** The temperature is in degrees centigrade. */
	switch(ccd_library_exposure_status)
	{
		case CCD_EXPOSURE_STATUS_NONE:
		case CCD_EXPOSURE_STATUS_WAIT_START:
			retval = CCD_Temperature_Get(&(state.ccd_temperature),&temperature_status);
			if(retval == FALSE)
			{
				ce = create_ccd_library_exception();
				throw ce;
			}	
			break;
		case CCD_EXPOSURE_STATUS_EXPOSE:
		case CCD_EXPOSURE_STATUS_READOUT:
			retval = CCD_Temperature_Get_Cached_Temperature(&(state.ccd_temperature),&temperature_status,
									&cache_date_stamp);
			if(retval == FALSE)
			{
				ce = create_ccd_library_exception();
				throw ce;
			}	
			break;
		default:
			/* we could throw an exception here */
			break;	
	}/* end switch */
	/* Set readout speed and gain from cached values */
	state.readout_speed = mCachedReadoutSpeed;
	state.gain = mCachedGain;
}

/**
 * Get a copy of the image data.
 * @param img_data An ImageData instance to fill in with the returned image data.
 * @see Camera::mImageBuf
 * @see Camera::mImageBufNCols
 * @see Camera::mImageBufNRows
 * @see logger
 * @see LOG4CXX_INFO
 * @see ImageData
 */
void Camera::get_image_data(ImageData &img_data)
{
	cout << "Get image data." << endl;
	LOG4CXX_INFO(logger,"Get image data.");
	std::vector<int16_t>::const_iterator first = mImageBuf.begin();
	std::vector<int16_t>::const_iterator last = mImageBuf.end();
	img_data.data.assign(first, last);
	img_data.x_size = mImageBufNCols;
	img_data.y_size = mImageBufNRows;
}

/**
 * Return the image filename of the last FITS image saved by the camera server.
 * @param filename On return of this method, the filename will contain a string representation of 
 *                 the last FITS image filename saved by the camera server.
 * @see Camera::mLastImageFilename
 */
void Camera::get_last_image_filename(std::string &filename)
{
	filename = mLastImageFilename;
}

/**
 * Return a list of FITS image filenames from the current  / last multbias / multdark / multrun 
 * performed by the camera server.
 * @param filename_list A vector list containing strings. On return of this method, this lisit will contain 
 *                      a list of strings representing FITS image filnames of the last multbias / multdark / multrun 
 *                      performed by the camera server.
 * @see Camera::mImageFilenameList
 */
void Camera::get_image_filenames(std::vector<std::string> &filename_list)
{
	filename_list = mImageFilenameList;
}


/**
 * Start cooling down the camera.
 * <ul>
 * <li>We retrieve the target temperature from the config file object mCameraConfig,
 *     "ccd.target_temperature" value.
 * <li>We set the target temperature using CCD_Temperature_Set.
 * <li>We turn the cooler on using CCD_Temperature_Cooler_On.
 * </ul>
 * If setting the CCD temperature fails the method can throw a CameraException 
 * (created using create_ccd_library_exception).
 * @see Camera::mCameraConfig
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Temperature_Set
 * @see CCD_Temperature_Cooler_On
 */
void Camera::cool_down()
{
	CameraException ce;
	double target_temperature;
	int retval;

	cout << "Cool down the camera." << endl;
	LOG4CXX_INFO(logger,"Cool down the camera.");
	/* get the target temperature */
	mCameraConfig.get_config_double(CONFIG_CAMERA_SECTION,"ccd.target_temperature",&target_temperature);
	LOG4CXX_INFO(logger,"Camera temperature setpoint is " << target_temperature << ".");
	/* set the camera set-point */
	retval = CCD_Temperature_Set(target_temperature); 
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* turn the cooler on */
	retval = CCD_Temperature_Cooler_On();
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}	
}

/**
 * Start warming up the camera.
 * <ul>
 * <li>We turn the cooler off using CCD_Temperature_Cooler_Off.
 * </ul>
 * Note we do _not_ set the temperature to a warm setpoint, this camera's firmware will not 
 * allow us to call SetTemperature with a warm value (i.e. 10 C).
 * If turning the cooler off fails the method can throw a CameraException 
 * (created using create_ccd_library_exception).
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Temperature_Cooler_Off
 */
void Camera::warm_up()
{
	CameraException ce;
	int retval;

	cout << "Warm up the camera." << endl;
	LOG4CXX_INFO(logger,"Warm up the camera.");
	/* turn the cooler off */
	retval = CCD_Temperature_Cooler_Off();
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}	
}

/**
 * This method is run as a separate thread to take a single exposure.
 * <ul>
 * <li>We get the length of the image buffer we need by calling CCD_Setup_Get_Buffer_Length, 
 *     and then resize mImageBuf to suit.
 * <li>We also get the number of binned columns and rows in the image by calling 
 *     CCD_Setup_Get_NCols / CCD_Setup_Get_Bin_X / CCD_Setup_Get_NRows / CCD_Setup_Get_Bin_Y.
 * <li>We set the start_time to zero, so the exposure starts immediately.
 * <li>We set mExposureCount to 1, so the status reflects how many images we are going to take.
 * <li>We clear the mImageFilenameList vector.
 * <li>We set mExposureIndex to 0 (for status propagation).
 * <li>We call CCD_Exposure_Expose with the exposure length parameter to tell the camera to take an 
 *     exposure of the required length, and read out the image and store it in mImageBuf.
 * <li>If save_image is true we thrn do the following:
 *     <ul>
 *     <li>We increment the FITS filename run number by calling CCD_Fits_Filename_Next_Run.
 *     <li>We call CCD_Fits_Filename_Get_Filename to generate a FITS filename.
 *     <li>We call add_camera_fits_headers to add the internally generated camera FITS headers to mFitsHeader.
 *     <li>We call CCD_Exposure_Save to save the read out data in mImageBuf to the generated FITS filename with the 
 *         FITS headers from mFitsHeader.
 *     <li>We update mLastImageFilename with the newly saved FITS filename, 
 *         and add the filename to the mImageFilenameList list.
 *     </ul>
 * </ul>
 * If any of the CCD library calls fail, we use create_ccd_library_exception to create a 
 * CameraException with a suitable error message, and then throw the exception.
 * @param exposure_length The length of one exposure in milliseconds. Should be at least 1.
 * @param save_image A boolean, if true the acquired image is saved to a FITS file, 
 *        otherwise the acquired data is left in mImageBuf to potentially be accessed by  get_image_data.
 * @see Camera::mImageBuf
 * @see Camera::mImageBufNCols
 * @see Camera::mImageBufNRows
 * @see Camera::mExposureCount
 * @see Camera::mExposureIndex
 * @see Camera::mLastImageFilename
 * @see Camera::mImageFilenameList
 * @see Camera::mFitsHeader
 * @see Camera::add_camera_fits_headers
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Exposure_Expose
 * @see CCD_Exposure_Save
 * @see CCD_Fits_Filename_Next_Run
 * @see CCD_Fits_Filename_Get_Filename
 * @see CCD_Setup_Get_Buffer_Length
 * @see CCD_Setup_Get_NCols
 * @see CCD_Setup_Get_Bin_X
 * @see CCD_Setup_Get_NRows
 * @see CCD_Setup_Get_Bin_Y
 */
void Camera::expose_thread(int32_t exposure_length, bool save_image)
{
	CameraException ce;
	struct timespec start_time;
	char filename[256];
	size_t image_buffer_length = 0;
	int retval,binned_ncols,binned_nrows;

	try
	{
		cout << "expose thread with exposure length " << exposure_length <<
			" ms and save_image " << save_image << "." << endl;
		LOG4CXX_INFO(logger,"expose thread with exposure length " << exposure_length <<
			     " ms and save_image " << save_image << ".");
		/* setup image buffer */
		retval = CCD_Setup_Get_Buffer_Length(&image_buffer_length);
		if(retval == FALSE)
		{
			ce = create_ccd_library_exception();
			throw ce;
		}	
		mImageBuf.resize(image_buffer_length);
		binned_ncols = CCD_Setup_Get_NCols()/CCD_Setup_Get_Bin_X();
		binned_nrows = CCD_Setup_Get_NRows()/CCD_Setup_Get_Bin_Y();
		mImageBufNCols = binned_ncols;
		mImageBufNRows = binned_nrows;
		/* start time is now */
		start_time.tv_sec = 0;
		start_time.tv_nsec = 0;
		/* initialise exposure count/index status */
		mExposureCount = 1;
		mImageFilenameList.clear();
		/* update exposure index status data */
		mExposureIndex = 0;
		/* take the image */
		retval = CCD_Exposure_Expose(TRUE,start_time,exposure_length,(void*)(mImageBuf.data()),
					     image_buffer_length);
		if(retval == FALSE)
		{
			mExposureCount = 0;
			mExposureIndex = 0;
			ce = create_ccd_library_exception();
			throw ce;
		}
		if(save_image)
		{
			/* increment the filename run number */
			retval = CCD_Fits_Filename_Next_Run();
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* get the filename to save to */
			retval = CCD_Fits_Filename_Get_Filename(filename,256);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* Add internally generated FITS headers to mFitsHeader */
			add_camera_fits_headers(0,1,exposure_length);
			/* save the image */
			retval = CCD_Exposure_Save(filename,(void*)(mImageBuf.data()),image_buffer_length,
						   binned_ncols,binned_nrows,mFitsHeader);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* update last image filename and add generated filename to list of FITS images */
			mLastImageFilename = filename;
			mImageFilenameList.push_back(std::string(filename));
		}/* end if save_image */
	}
	catch(TException&e)
	{
		cerr << "expose_thread: Caught TException: " << e.what() << "." << endl;
		LOG4CXX_ERROR(logger,"expose_thread:Caught TException: " << e.what() << ".");
	}
	catch(exception& e)
	{
		cerr << "expose_thread:Caught Exception: " << e.what()  << "." << endl;
		LOG4CXX_FATAL(logger,"expose_thread:Caught Exception: " << e.what()  << ".");
	}		
}

/**
 * This method is run as a separate thread to actually create a series of bias frames.
 * <ul>
 * <li>We get the length of the image buffer we need by calling CCD_Setup_Get_Buffer_Length, 
 *     and then resize mImageBuf to suit.
 * <li>We also get the number of binned columns and rows in the image by calling 
 *     CCD_Setup_Get_NCols / CCD_Setup_Get_Bin_X / CCD_Setup_Get_NRows / CCD_Setup_Get_Bin_Y.
 * <li>We set mExposureCount to the exposure_count parameter, so the status reflects how many images 
 *     we are going to take.
 * <li>We clear the mImageFilenameList vector.
 * <li>We loop over the exposure_count:
 *     <ul>
 *     <li>We set mExposureIndex to the current exposure index (for status propagation).
 *     <li>We call CCD_Exposure_Bias to tell the camera to take a
 *         bias frame, and read out the image and store it in mImageBuf.
 *     <li>We increment the FITS filename run number by calling CCD_Fits_Filename_Next_Run.
 *     <li>We call CCD_Fits_Filename_Get_Filename to generate a FITS filename.
 *     <li>We call add_camera_fits_headers to add the internally generated camera FITS headers to mFitsHeader.
 *     <li>We call CCD_Exposure_Save to save the read out data in mImageBuf to the generated FITS filename with the 
 *         FITS headers from mFitsHeader.
 *     <li>We update mLastImageFilename with the newly saved FITS filename, 
 *         and add the filename to the mImageFilenameList list.
 *     </ul>
 * <li>
 * </ul>
 * If any of the CCD library calls fail, we use create_ccd_library_exception to create a 
 * CameraException with a suitable error message, and then throw the exception.
 * @param exposure_count The number of bias exposures to take. Should be at least 1.
 * @see Camera::mImageBuf
 * @see Camera::mImageBufNCols
 * @see Camera::mImageBufNRows
 * @see Camera::mExposureCount
 * @see Camera::mExposureIndex
 * @see Camera::mLastImageFilename
 * @see Camera::mImageFilenameList
 * @see Camera::mFitsHeader
 * @see Camera::add_camera_fits_headers
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Exposure_Bias
 * @see CCD_Exposure_Save
 * @see CCD_Fits_Filename_Next_Run
 * @see CCD_Fits_Filename_Get_Filename
 * @see CCD_Setup_Get_Buffer_Length
 * @see CCD_Setup_Get_NCols
 * @see CCD_Setup_Get_Bin_X
 * @see CCD_Setup_Get_NRows
 * @see CCD_Setup_Get_Bin_Y
 */
void Camera::multbias_thread(int32_t exposure_count)
{
	CameraException ce;
	char filename[256];
	size_t image_buffer_length = 0;
	int retval,binned_ncols,binned_nrows;

	try
	{
		cout << "multbias thread with exposure count " << exposure_count << "." << endl;
		LOG4CXX_INFO(logger,"multbias thread with exposure count " << exposure_count << ".");
		/* setup image buffer */
		retval = CCD_Setup_Get_Buffer_Length(&image_buffer_length);
		if(retval == FALSE)
		{
			ce = create_ccd_library_exception();
			throw ce;
		}	
		mImageBuf.resize(image_buffer_length);
		binned_ncols = CCD_Setup_Get_NCols()/CCD_Setup_Get_Bin_X();
		binned_nrows = CCD_Setup_Get_NRows()/CCD_Setup_Get_Bin_Y();
		mImageBufNCols = binned_ncols;
		mImageBufNRows = binned_nrows;
		/* initialise exposure count/index status */
		mExposureCount = exposure_count;
		mImageFilenameList.clear();
		/* loop over number of exposure to take */
		for(int image_index = 0; image_index < exposure_count; image_index++)
		{
			/* update exposure index status data */
			mExposureIndex = image_index;
			/* take the image */
			retval = CCD_Exposure_Bias((void*)(mImageBuf.data()),image_buffer_length);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* increment the filename run number */
			retval = CCD_Fits_Filename_Next_Run();
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* get the filename to save to */
			retval = CCD_Fits_Filename_Get_Filename(filename,256);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* Add internally generated FITS headers to mFitsHeader */
			add_camera_fits_headers(image_index,exposure_count,0);
			/* save the image */
			retval = CCD_Exposure_Save(filename,(void*)(mImageBuf.data()),image_buffer_length,
						   binned_ncols,binned_nrows,mFitsHeader);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* update last image filename and add generated filename to list of FITS images */
			mLastImageFilename = filename;
			mImageFilenameList.push_back(std::string(filename));
		}/* end for on exposure_count */
	}
	catch(TException&e)
	{
		cerr << "bias_thread: Caught TException: " << e.what() << "." << endl;
		LOG4CXX_ERROR(logger,"bias_thread: Caught TException: " << e.what() << ".");
	}
	catch(exception& e)
	{
		cerr << "bias_thread: Caught Exception: " << e.what()  << "." << endl;
		LOG4CXX_FATAL(logger,"bias_thread: Caught Exception: " << e.what()  << ".");
	}		
}

/**
 * This method is run as a separate thread to actually create a series of dark frames.
 * <ul>
 * <li>We get the length of the image buffer we need by calling CCD_Setup_Get_Buffer_Length, 
 *     and then resize mImageBuf to suit.
 * <li>We also get the number of binned columns and rows in the image by calling 
 *     CCD_Setup_Get_NCols / CCD_Setup_Get_Bin_X / CCD_Setup_Get_NRows / CCD_Setup_Get_Bin_Y.
 * <li>We set the start_time to zero, so the exposure starts immediately.
 * <li>We set mExposureCount to the exposure_count parameter, so the status reflects how many images we are going to take.
 * <li>We clear the mImageFilenameList vector.
 * <li>We loop over the exposure_count:
 *     <ul>
 *     <li>We set mExposureIndex to the current exposure index (for status propagation).
 *     <li>We call CCD_Exposure_Expose with the exposure length parameter to tell the camera to take a
 *         dark exposure of the required length, and read out the image and store it in mImageBuf.
 *     <li>We increment the FITS filename run number by calling CCD_Fits_Filename_Next_Run.
 *     <li>We call CCD_Fits_Filename_Get_Filename to generate a FITS filename.
 *     <li>We call add_camera_fits_headers to add the internally generated camera FITS headers to mFitsHeader.
 *     <li>We call CCD_Exposure_Save to save the read out data in mImageBuf to the generated FITS filename 
 *         with the FITS headers from mFitsHeader.
 *     <li>We update mLastImageFilename with the newly saved FITS filename, 
 *         and add the filename to the mImageFilenameList list.
 *     </ul>
 * <li>
 * </ul>
 * If any of the CCD library calls fail, we use create_ccd_library_exception to create a 
 * CameraException with a suitable error message, and then throw the exception.
 * @param exposure_count The number of dark exposures to take. Should be at least 1.
 * @param exposure_length The length of one exposure in milliseconds. Should be at least 1.
 * @see Camera::mImageBuf
 * @see Camera::mImageBufNCols
 * @see Camera::mImageBufNRows
 * @see Camera::mExposureCount
 * @see Camera::mExposureIndex
 * @see Camera::mLastImageFilename
 * @see Camera::mImageFilenameList
 * @see Camera::mFitsHeader
 * @see Camera::add_camera_fits_headers
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_Exposure_Expose
 * @see CCD_Exposure_Save
 * @see CCD_Fits_Filename_Next_Run
 * @see CCD_Fits_Filename_Get_Filename
 * @see CCD_Setup_Get_Buffer_Length
 * @see CCD_Setup_Get_NCols
 * @see CCD_Setup_Get_Bin_X
 * @see CCD_Setup_Get_NRows
 * @see CCD_Setup_Get_Bin_Y
 */
void Camera::multdark_thread(int32_t exposure_count,int32_t exposure_length)
{
	CameraException ce;
	struct timespec start_time;
	char filename[256];
	size_t image_buffer_length = 0;
	int retval,binned_ncols,binned_nrows;

	try
	{
		cout << "multdark thread with exposure count " << exposure_count << ", exposure length " <<
			exposure_length << "ms." << endl;
		LOG4CXX_INFO(logger,"multdark thread with exposure count " << exposure_count <<
			     ", exposure length " << exposure_length << "ms.");
		/* setup image buffer */
		retval = CCD_Setup_Get_Buffer_Length(&image_buffer_length);
		if(retval == FALSE)
		{
			ce = create_ccd_library_exception();
			throw ce;
		}	
		mImageBuf.resize(image_buffer_length);
		binned_ncols = CCD_Setup_Get_NCols()/CCD_Setup_Get_Bin_X();
		binned_nrows = CCD_Setup_Get_NRows()/CCD_Setup_Get_Bin_Y();
		mImageBufNCols = binned_ncols;
		mImageBufNRows = binned_nrows;
		/* start time is now */
		start_time.tv_sec = 0;
		start_time.tv_nsec = 0;
		/* initialise exposure count/index status */
		mExposureCount = exposure_count;
		mImageFilenameList.clear();
		/* loop over number of exposure to take */
		for(int image_index = 0; image_index < exposure_count; image_index++)
		{
			/* update exposure index status data */
			mExposureIndex = image_index;
			/* take the image */
			retval = CCD_Exposure_Expose(FALSE,start_time,exposure_length,(void*)(mImageBuf.data()),
						     image_buffer_length);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* increment the filename run number */
			retval = CCD_Fits_Filename_Next_Run();
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* get the filename to save to */
			retval = CCD_Fits_Filename_Get_Filename(filename,256);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* Add internally generated FITS headers to mFitsHeader */
			add_camera_fits_headers(image_index,exposure_count,exposure_length);
			/* save the image */
			retval = CCD_Exposure_Save(filename,(void*)(mImageBuf.data()),image_buffer_length,
						   binned_ncols,binned_nrows,mFitsHeader);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* update last image filename and add generated filename to list of FITS images */
			mLastImageFilename = filename;
			mImageFilenameList.push_back(std::string(filename));
		}/* end for on exposure_count */
	}
	catch(TException&e)
	{
		cerr << "dark_thread: Caught TException: " << e.what() << "." << endl;
		LOG4CXX_ERROR(logger,"dark_thread:Caught TException: " << e.what() << ".");
	}
	catch(exception& e)
	{
		cerr << "dark_thread: Caught Exception: " << e.what()  << "." << endl;
		LOG4CXX_FATAL(logger,"dark_thread: Caught Exception: " << e.what()  << ".");
	}		
}

/**
 * This method is run as a separate thread to actually create a series of science frames.
 * <ul>
 * <li>We get the length of the image buffer we need by calling CCD_Setup_Get_Buffer_Length, 
 *     and then resize mImageBuf to suit.
 * <li>We also get the number of binned columns and rows in the image by calling 
 *     CCD_Setup_Get_NCols / CCD_Setup_Get_Bin_X / CCD_Setup_Get_NRows / CCD_Setup_Get_Bin_Y.
 * <li>We set the start_time to zero, so the exposure starts immediately.
 * <li>We set mExposureCount to the exposure_count parameter, so the status reflects how many images 
 *     we are going to take.
 * <li>We clear the mImageFilenameList vector.
 * <li>We loop over the exposure_count:
 *     <ul>
 *     <li>We set mExposureIndex to the current exposure index (for status propagation).
 *     <li>We call CCD_Exposure_Expose with the exposure length parameter to tell the camera to take an 
 *         exposure of the required length, and read out the image and store it in mImageBuf.
 *     <li>We increment the FITS filename run number by calling CCD_Fits_Filename_Next_Run.
 *     <li>We call CCD_Fits_Filename_Get_Filename to generate a FITS filename.
 *     <li>We call add_camera_fits_headers to add the internally generated camera FITS headers to mFitsHeader.
 *     <li>We call CCD_Exposure_Save to save the read out data in mImageBuf to the generated FITS filename with the 
 *         FITS headers from mFitsHeader.
 *     <li>We update mLastImageFilename with the newly saved FITS filename, 
 *         and add the filename to the mImageFilenameList list.
 *     </ul>
 * <li>
 * </ul>
 * If any of the CCD library calls fail, we use create_ccd_library_exception to create a 
 * CameraException with a suitable error message, and then throw the exception.
 * @param exposure_count The number of science exposures to take. Should be at least 1.
 * @param exposure_length The length of one exposure in milliseconds. Should be at least 1.
 * @see Camera::mImageBuf
 * @see Camera::mImageBufNCols
 * @see Camera::mImageBufNRows
 * @see Camera::mExposureCount
 * @see Camera::mExposureIndex
 * @see Camera::mLastImageFilename
 * @see Camera::mImageFilenameList
 * @see Camera::mFitsHeader
 * @see Camera::add_camera_fits_headers
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see LOG4CXX_ERROR
 * @see LOG4CXX_FATAL
 * @see CCD_Exposure_Expose
 * @see CCD_Exposure_Save
 * @see CCD_Fits_Filename_Next_Run
 * @see CCD_Fits_Filename_Get_Filename
 * @see CCD_Setup_Get_Buffer_Length
 * @see CCD_Setup_Get_NCols
 * @see CCD_Setup_Get_Bin_X
 * @see CCD_Setup_Get_NRows
 * @see CCD_Setup_Get_Bin_Y
 */
void Camera::multrun_thread(int32_t exposure_count,int32_t exposure_length)
{
	CameraException ce;
	struct timespec start_time;
	char filename[256];
	size_t image_buffer_length = 0;
	int retval,binned_ncols,binned_nrows;

	try
	{
		cout << "multrun thread with exposure count " << exposure_count <<
			", exposure length " << exposure_length << "ms." << endl;
		LOG4CXX_INFO(logger,"multrun thread with exposure count " << exposure_count <<
			     ", exposure length " << exposure_length << "ms.");
		/* setup image buffer */
		retval = CCD_Setup_Get_Buffer_Length(&image_buffer_length);
		if(retval == FALSE)
		{
			ce = create_ccd_library_exception();
			throw ce;
		}	
		mImageBuf.resize(image_buffer_length);
		binned_ncols = CCD_Setup_Get_NCols()/CCD_Setup_Get_Bin_X();
		binned_nrows = CCD_Setup_Get_NRows()/CCD_Setup_Get_Bin_Y();
		mImageBufNCols = binned_ncols;
		mImageBufNRows = binned_nrows;
		/* start time is now */
		start_time.tv_sec = 0;
		start_time.tv_nsec = 0;
		/* initialise exposure count/index status */
		mExposureCount = exposure_count;
		mImageFilenameList.clear();
		/* loop over number of exposure to take */
		for(int image_index = 0; image_index < exposure_count; image_index++)
		{
			/* update exposure index status data */
			mExposureIndex = image_index;
			/* take the image */
			retval = CCD_Exposure_Expose(TRUE,start_time,exposure_length,(void*)(mImageBuf.data()),
						     image_buffer_length);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* increment the filename run number */
			retval = CCD_Fits_Filename_Next_Run();
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* get the filename to save to */
			retval = CCD_Fits_Filename_Get_Filename(filename,256);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* Add internally generated FITS headers to mFitsHeader */
			add_camera_fits_headers(image_index,exposure_count,exposure_length);
			/* save the image */
			retval = CCD_Exposure_Save(filename,(void*)(mImageBuf.data()),image_buffer_length,
						   binned_ncols,binned_nrows,mFitsHeader);
			if(retval == FALSE)
			{
				mExposureCount = 0;
				mExposureIndex = 0;
				ce = create_ccd_library_exception();
				throw ce;
			}
			/* update last image filename and add generated filename to list of FITS images */
			mLastImageFilename = filename;
			mImageFilenameList.push_back(std::string(filename));
		}/* end for on exposure_count */
	}
	catch(TException&e)
	{
		cerr << "multrun_thread: Caught TException: " << e.what() << "." << endl;
		LOG4CXX_ERROR(logger,"multrun_thread:Caught TException: " << e.what() << ".");
	}
	catch(exception& e)
	{
		cerr << "multrun_thread: Caught Exception: " << e.what()  << "." << endl;
		LOG4CXX_FATAL(logger,"multrun_thread: Caught Exception: " << e.what()  << ".");
	}		
}

/**
 * Method to add some of the internal FITS headers generated from within the camera to mFitsHeader,
 * which are then saved to the generated FITS images. Headers added are:
 * <ul>
 * <li><b>EXPTIME</b> Length of exposure in seconds.
 * <li><b>EXPOSURE</b> Length of exposure in seconds.
 * <li><b>HBIN</b> Horizontal / X binning, retrieved from the CCD library using CCD_Setup_Get_Bin_X.
 * <li><b>VBIN</b> Vertical / Y binning, retrieved from the CCD library using CCD_Setup_Get_Bin_Y.
 * <li><b>CCDTEMP</b> The camera temperature in degrees Kelvin, this is retrieved from the camera in degrees centigrade 
 *                   using CCD_Temperature_Get, and then has DEGREES_CENTIGRADE_TO_KELVIN added to it.
 * <li><b>HEAD</b> The camera head model name, retrieved from the CCD library using 
 *                 CCD_Setup_Get_Camera_Head_Model_Name.
 * <li><b>SERNO</b> The camera head serial number, retrieved from the CCD library using 
 *                  CCD_Setup_Get_Camera_Serial_Number.
 * <li><b>FLIPX</b> A boolean, whether we have flipped the readout in the Horizontal / X direction, 
 *                  retrieved from the CCD library using CCD_Setup_Get_Flip_X.
 * <li><b>FLIPY</b> A boolean, whether we have flipped the readout in the Vertical / Y direction, 
 *                  retrieved from the CCD library using CCD_Setup_Get_Flip_Y.
 * <li><b>IMGRECT / SUBRECT</b> We figure out the active image area. If we are windowing (mCachedWindowFlags is true), 
 *        we use the image dimensions in mCachedWindow. If we are not windowing (mCachedWindowFlags is false), 
 *        we use mCachedNCols,mCachedNRows. We construct a string "sx, sy, ex, ey" 
 *        and set the "IMGRECT" and "SUBRECT" headers to this value.
 * <li><b>UTSTART</b> We retrieve the start exposure timestamp using CCD_Exposure_Start_Time_Get. We use 
 *                   CCD_Fits_Header_TimeSpec_To_UtStart_String to format the string.
 * <li><b>DATE-OBS</b> We retrieve the start exposure timestamp using CCD_Exposure_Start_Time_Get. We use 
 *                   CCD_Fits_Header_TimeSpec_To_Date_Obs_String to format the string.
 * <li><b>VSHIFT</b> The vertical shift speed in microseconds/pixel, retrieved from the CCD library using 
 *                   CCD_Setup_Get_VS_Speed.
 * <li><b>VSHIFTI</b> The vertical shift speed index used to configure the vertical shift speed, 
 *                    retrieved from the CCD library using CCD_Setup_Get_VS_Speed_Index.
 * <li><b>VSAMP</b> The vertical clock voltage amplitude, retrieved from the CCD library using 
 *                  CCD_Setup_Get_VS_Amplitude, where '0' is normal and '1'-'4' indicates an increased clock
 *                  voltage. 
 * <li><b>HSHIFT</b> The horizontal shift speed in MHz, retrieved from the CCD library using CCD_Setup_Get_HS_Speed.
 * <li><b>HSHIFTI</b> The horizontal shift speed index used to configure the horizontal shift speed, 
 *                    retrieved from the CCD library using CCD_Setup_Get_HS_Speed_Index.
 * </ul>
 * @param image_index Which image out of the exposure_count number of images we are currently doing.
 * @param exposure_count The number of images in the Multrun/Multbias/MultDark.
 * @param exposure_length The exposure length, in milliseconds, of this image.
 * @see Camera::create_ccd_library_exception
 * @see mFitsHeader
 * @see Camera::mCachedNCols
 * @see Camera::mCachedNRows
 * @see Camera::mCachedWindowFlags
 * @see Camera::mCachedWindow
 * @see DEGREES_CENTIGRADE_TO_KELVIN
 * @see CCD_Exposure_Start_Time_Get
 * @see CCD_Fits_Header_Add_String
 * @see CCD_Fits_Header_Add_Float
 * @see CCD_Fits_Header_Add_Int
 * @see CCD_Fits_Header_Add_Logical
 * @see CCD_Fits_Header_Add_Units
 * @see CCD_Fits_Header_TimeSpec_To_UtStart_String
 * @see CCD_Fits_Header_TimeSpec_To_Date_Obs_String
 * @see CCD_Setup_Get_Bin_X
 * @see CCD_Setup_Get_Bin_Y
 * @see CCD_Setup_Get_Flip_X
 * @see CCD_Setup_Get_Flip_Y
 * @see CCD_Setup_Get_Camera_Head_Model_Name
 * @see CCD_Setup_Get_Camera_Serial_Number
 * @see CCD_TEMPERATURE_STATUS
 * @see CCD_Temperature_Get
 * @see CCD_Setup_Get_VS_Speed
 * @see CCD_Setup_Get_VS_Speed_Index
 * @see CCD_Setup_Get_HS_Speed
 * @see CCD_Setup_Get_HS_Speed_Index
 * @see CCD_Setup_Get_VS_Amplitude
 */
void Camera::add_camera_fits_headers(int image_index,int32_t exposure_count,int32_t exposure_length)
{
	CameraException ce;
	enum CCD_TEMPERATURE_STATUS temperature_status;
	struct timespec start_time;
	char camera_head_model_name[128];
	char img_rect_buff[128];
	char time_string[32];
	double temperature;
	float vs_speed,hs_speed,pre_amp_gain;
	int retval,xs,ys,xe,ye,vs_speed_index,hs_speed_index,vs_amplitude;
	
	/* EXPTIME  double in secs */
	retval = CCD_Fits_Header_Add_Float(&mFitsHeader,"EXPTIME",
					   ((double)exposure_length)/((double)CCD_GENERAL_ONE_SECOND_MS),
					   "Exposure length in decimal seconds");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	retval = CCD_Fits_Header_Add_Units(&mFitsHeader,"EXPTIME","s");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}	
	/* EXPOSURE double in secs */
	retval = CCD_Fits_Header_Add_Float(&mFitsHeader,"EXPOSURE",
					   ((double)exposure_length)/((double)CCD_GENERAL_ONE_SECOND_MS),
					   "Exposure length in decimal seconds");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	retval = CCD_Fits_Header_Add_Units(&mFitsHeader,"EXPOSURE","s");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* retrieve start of exposure timestamp */
	CCD_Exposure_Start_Time_Get(&start_time);
	/* UTSTART HH:MM:SS */
	CCD_Fits_Header_TimeSpec_To_UtStart_String(start_time,time_string);
	retval = CCD_Fits_Header_Add_String(&mFitsHeader,"UTSTART",time_string,"Start time of the observation");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* LCSTART HH:MM:SS */
	/* DATE-OBS YYYY-MM-DD */
	/* DATE-OBS YYYY-MM-DDTHH:MM:SS.sss */
	CCD_Fits_Header_TimeSpec_To_Date_Obs_String(start_time,time_string);
	retval = CCD_Fits_Header_Add_String(&mFitsHeader,"DATE-OBS",time_string,"Start time of the observation");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* HJD-OBS double (Heliocentric Julian Date of start) */
	/* HBIN */
	retval = CCD_Fits_Header_Add_Int(&mFitsHeader,"HBIN",CCD_Setup_Get_Bin_X(),"Horizontal/X binning");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}		
	/* VBIN */
	retval = CCD_Fits_Header_Add_Int(&mFitsHeader,"VBIN",CCD_Setup_Get_Bin_X(),"Vertical/Y binning");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}		
	/* BINNING vxh ! */
	/* GAINVAL */
	/* SGAIN CCD Mode GAIN (Faint)*/
	/* SRDNOISE CCD Mode RDNoise (Slow) */
	/* NOISEADU */
	/* NOISEEL */
	/* CCDTEMP double Kelvin */
	retval = CCD_Temperature_Get(&temperature,&temperature_status);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}		
	retval = CCD_Fits_Header_Add_Float(&mFitsHeader,"CCDTEMP",temperature+DEGREES_CENTIGRADE_TO_KELVIN,
					   "CCD temperature");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	retval = CCD_Fits_Header_Add_Units(&mFitsHeader,"CCDTEMP","Kelvin");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* RUN-NO */
	/* HEAD string CCD camera head string DU8201_UVB etc */
	retval = CCD_Setup_Get_Camera_Head_Model_Name(camera_head_model_name,128);
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}		
	retval = CCD_Fits_Header_Add_String(&mFitsHeader,"HEAD",camera_head_model_name,"Camera head model name");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* SERNO camera serial number */
	retval = CCD_Fits_Header_Add_Int(&mFitsHeader,"SERNO",CCD_Setup_Get_Camera_Serial_Number(),
					 "Camera serial number");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* FLIPX */
	retval = CCD_Fits_Header_Add_Logical(&mFitsHeader,"FLIPX",CCD_Setup_Get_Flip_X(),
					     "Camera readout flipped horizontally");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* FLIPY */
	retval = CCD_Fits_Header_Add_Logical(&mFitsHeader,"FLIPY",CCD_Setup_Get_Flip_Y(),
					     "Camera readout flipped vertically");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* Note IMGRECT / SUBRECT may need to be modified to account for read out flipping */
	/* IMGRECT 1, 1024, 1024, 1 */
	/* SUBRECT 1, 1024, 1024, 1 */
	if(mCachedWindowFlags)
	{
		xs = mCachedWindow.X_Start;
		ys = mCachedWindow.Y_Start;
		xe = mCachedWindow.X_End;
		ye = mCachedWindow.Y_End;
	}
	else
	{
		xs = 1;
		ys = 1;
		xe = mCachedNCols;
		ye = mCachedNRows;
	}
	sprintf(img_rect_buff,"%d, %d, %d, %d",xs,ys,xe,ye);
	retval = CCD_Fits_Header_Add_String(&mFitsHeader,"IMGRECT",img_rect_buff,"Imaging area");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	retval = CCD_Fits_Header_Add_String(&mFitsHeader,"SUBRECT",img_rect_buff,"Sub-maging area");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* VSHIFT */
	vs_speed = CCD_Setup_Get_VS_Speed();
	retval = CCD_Fits_Header_Add_Float(&mFitsHeader,"VSHIFT",(double)vs_speed,"vertical shift speed");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	retval = CCD_Fits_Header_Add_Units(&mFitsHeader,"VSHIFT","us/pixel");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* VSHIFTI */
	vs_speed_index = CCD_Setup_Get_VS_Speed_Index();
	retval = CCD_Fits_Header_Add_Int(&mFitsHeader,"VSHIFTI",vs_speed_index,"vertical shift speed index");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* VSAMP */
	vs_amplitude = CCD_Setup_Get_VS_Amplitude();
	retval = CCD_Fits_Header_Add_Int(&mFitsHeader,"VSAMP",vs_amplitude,"vertical clock voltage amplitude");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* HSHIFT */
	hs_speed = CCD_Setup_Get_HS_Speed();
	retval = CCD_Fits_Header_Add_Float(&mFitsHeader,"HSHIFT",(double)hs_speed,"horizontal shift speed");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	retval = CCD_Fits_Header_Add_Units(&mFitsHeader,"HSHIFT","MHz");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* HSHIFTI */
	hs_speed_index = CCD_Setup_Get_HS_Speed_Index();
	retval = CCD_Fits_Header_Add_Int(&mFitsHeader,"HSHIFTI",hs_speed_index,"horizontal shift speed index");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
	/* GAIN */
	pre_amp_gain = CCD_Setup_Get_Pre_Amp_Gain();
	retval = CCD_Fits_Header_Add_Float(&mFitsHeader,"GAIN",(double)pre_amp_gain,"pre-amp gain factor");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
}

/**
 * This method creates a camera exception, and populates the message with an aggregation of error messasges found
 * in the CCD library. We also log the created error to the log file.
 * @return The created camera exception. The message is generated by CCD_General_Error_To_String.
 * @see #ERROR_BUFFER_LENGTH
 * @see #logger
 * @see CameraException
 * @see CCD_General_Error_To_String
 * @see LOG4CXX_ERROR
 */
CameraException Camera::create_ccd_library_exception()
{
	CameraException ce;
	char error_buffer[ERROR_BUFFER_LENGTH];
	
	CCD_General_Error_To_String(error_buffer);
	std::string str(error_buffer);
	ce.message = str;
	LOG4CXX_ERROR(logger,"Creating CCD library exception:" + str);
	return ce;
}

/**
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
static void log_to_log4cxx(char *sub_system,char *source_filename,char *function,
			   int level,char *category,char *string)
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
	
