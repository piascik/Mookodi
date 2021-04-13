/**
 * @file
 * @brief Camera.cpp provides an implementation of the camera detector thrift interface. This implementation
 *        makes the relevant calls to the CCD library for each thrift API call.
 * @author Chris Mottram
 * @version $Id$
 */
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
namespace po = boost::program_options;

/**
 * The default target temperature to cool the CCD to.
 */
#define DEFAULT_CCD_TARGET_TEMPERATURE (-60.0)
/**
 * The default directory storing the installed Andor CCD library configuration files.
 */
#define DEFAULT_ANDOR_CONFIG_DIR ("/usr/local/etc/andor")
/**
 * Camera is an iKon M934, Full frame is 1024 x 1024.
 */
#define DEFAULT_FULL_FRAME_X_PIXEL_COUNT (1024)
/**
 * Camera is an iKon M934, Full frame is 1024 x 1024.
 */
#define DEFAULT_FULL_FRAME_Y_PIXEL_COUNT (1024)
/**
 * The default FITS instrument code, used when generating FITS filenames to specify 'mookodi'.
 * This is used for the filename (not the directory) and is by convention in capitals.
 */
#define DEFAULT_FITS_INSTRUMENT_CODE     ("MKD")
/**
 * The default FITS image data directory root,
 * used to construct the directory structure where generated FITS images are stored.
 */
#define DEFAULT_FITS_DATA_DIR_ROOT       ("/data")
/**
 * The default FITS image data directory telescope component, 
 * used to construct the directory structure where generated FITS images are stored.
 */
#define DEFAULT_FITS_DATA_DIR_TELESCOPE  ("lesedi")
/**
 * The default FITS image data directory instrument component, 
 * used to construct the directory structure where generated FITS images are stored.
 * This is used for the directory (not the filename) and is by convention in lower case.
 */
#define DEFAULT_FITS_DATA_DIR_INSTRUMENT ("mkd")
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
static void log_to_stdout(char *sub_system,char *source_filename,char *function,
			  int level,char *category,char *string);
static void log_to_log4cxx(char *sub_system,char *source_filename,char *function,
			   int level,char *category,char *string);

/**
 * Constructor for the Camera object. This calls initialize to initialise the camera.
 * @see Camera::initialize
 */
Camera::Camera()
{
	initialize();
}

/**
 * Destructor for the Camera object. Does nothing.
 */
Camera::~Camera()
{
}

/**
 * Method to set the configuration filename to load config from in initialize.
 * @param config_filename The configuration filename as a string.
 * @see Camera::mConfigFilename
 * @see Camera::initialize
 * @see Camera::load_config
 */
void Camera::set_config_filename(const std::string & config_filename)
{
	mConfigFilename = config_filename;
}

/**
 * Method to load configuration data from the previously specified configuration filename (mConfigFilename),
 * into the boost variables map mConfigFileVM.
 * @see Camera::mConfigFilename
 * @see Camera::mConfigFileVM
 * @see logger
 * @see LOG4CXX_INFO
 * @see #DEFAULT_CCD_TARGET_TEMPERATURE
 * @see #DEFAULT_ANDOR_CONFIG_DIR
 * @see #DEFAULT_FULL_FRAME_X_PIXEL_COUNT
 * @see #DEFAULT_FULL_FRAME_Y_PIXEL_COUNT
 * @see #DEFAULT_FITS_INSTRUMENT_CODE
 * @see #DEFAULT_FITS_DATA_DIR_ROOT
 * @see #DEFAULT_FITS_DATA_DIR_TELESCOPE
 * @see #DEFAULT_FITS_DATA_DIR_INSTRUMENT
 */
void Camera::load_config()
{
	po::options_description config_file_options("Configuration");

	cout << "load_config using configuration filename " << mConfigFilename << "." << endl;
	LOG4CXX_INFO(logger,"load_config using configuration filename " << mConfigFilename);
	// setup the list of values in the configuration filename
	config_file_options.add_options()
            ("ccd.ncols",po::value<int>()->default_value(DEFAULT_FULL_FRAME_X_PIXEL_COUNT),
             "The number of horizontal/x/column pixels in the full imaging frame.")
            ("ccd.nrows",po::value<int>()->default_value(DEFAULT_FULL_FRAME_Y_PIXEL_COUNT),
             "The number of vertical/y/row pixels in the full imaging frame.")
            ("ccd.target_temperature",po::value<double>()->default_value(DEFAULT_CCD_TARGET_TEMPERATURE),
             "Temperature to try and cool CCD down to, in degrees centigrade.")
            ("andor.config_dir",po::value<string>()->default_value(DEFAULT_ANDOR_CONFIG_DIR),
             "The directory containing the Andor library configuration files.")
            ("fits.instrument_code",po::value<string>()->default_value(DEFAULT_FITS_INSTRUMENT_CODE),
             "The FITS instrument code used when generating FITS filenames.")
            ("fits.data_dir.root",po::value<string>()->default_value(DEFAULT_FITS_DATA_DIR_ROOT),
             "This is the root of the directory structure to put generated FITS images into.")
            ("fits.data_dir.telescope",po::value<string>()->default_value(DEFAULT_FITS_DATA_DIR_TELESCOPE),
             "This string is the telescope part of the directory structure to put generated FITS images into.")
            ("fits.data_dir.instrument",po::value<string>()->default_value(DEFAULT_FITS_DATA_DIR_INSTRUMENT),
             "This string is the instrument part of the directory structure to put generated FITS images into.")
            ;
	// load the configuration variable map from the config filename.	
	std::ifstream ifs(mConfigFilename);
        store(parse_config_file(ifs, config_file_options), mConfigFileVM);
        notify(mConfigFileVM);
}

/**
 * Initialisation method for the camera object. This does the following:
 * <ul>
 * <li>We set the CCD library log handler function to log to stdout.
 * <li>We call load_config to load configuration values from the config file.
 * <li>We retrieve the "andor.config_dir" configuration value and use it to set the Andor config directory in the
 *     CCD library using CCD_Setup_Config_Directory_Set.
 * <li>We connect to and initialise the camera using CCD_Setup_Startup.
 * <li>We retrieve the fits filename instrument code "fits.instrument_code", 
 *     the data directory root "fits.data_dir.root", 
 *     the telescope component of the data directory "fits.data_dir.telescope",
 *     and the instument component of the data directory "fits.data_dir.instrument",
 *     from the config file values in mConfigFileVM, and use them to initialise FITS filename generation using 
 *     CCD_Fits_Filename_Initialise.
 * <li>We initialise the FITS headers (stored in mFitsHeader) using CCD_Fits_Header_Initialise.
 * <li>We setup the cached image data (used to configure the CCD windowing/binning). Some of the
 *     values are read from the config file variable map mConfigFileVM.
 * <li>We configure the detector readout dimensions to the cached ones using CCD_Setup_Dimensions.
 * <li>We initialise mExposureCount and mExposureIndex to zero.
 * </ul>
 * If a CCD library routine fails we call create_ccd_library_exception to create a CameraException that is then thrown.
 * @see Camera::mConfigFileVM
 * @see Camera::mFitsHeader
 * @see Camera::mCachedNCols
 * @see Camera::mCachedNRows
 * @see Camera::mCachedHBin
 * @see Camera::mCachedVBin
 * @see Camera::mCachedWindowFlags
 * @see Camera::mCachedWindow
 * @see Camera::mExposureCount
 * @see Camera::mExposureIndex
 * @see Camera::load_config
 * @see Camera::create_ccd_library_exception
 * @see logger
 * @see LOG4CXX_INFO
 * @see CCD_General_Set_Log_Handler_Function
 * @see CCD_General_Log_Handler_Stdout
 * @see CCD_Setup_Config_Directory_Set
 * @see CCD_Setup_Startup
 * @see CCD_Setup_Dimensions
 * @see CCD_Fits_Filename_Initialise
 * @see CCD_Fits_Header_Initialise
 * @see log_to_log4cxx
 */
void Camera::initialize()
{
	CameraException ce;
	char *config_dir;
	char *fits_data_dir_root;
	char *fits_data_dir_telescope;
	char *fits_data_dir_instrument;
	char *instrument_code;
	int retval;
	
	cout << "Initialising Camera." << endl;
	LOG4CXX_INFO(logger,"Initialising Camera.");
	/* setup loggers */
	CCD_General_Set_Log_Handler_Function(log_to_log4cxx);
	/* load configuration from the config file */
	load_config();
	/* setup andor config directory */
	config_dir = (char *)(mConfigFileVM["andor.config_dir"].as<std::string>().c_str());
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
	/* initialise FITS filename generation code */
	instrument_code = (char *)(mConfigFileVM["fits.instrument_code"].as<std::string>().c_str());
	fits_data_dir_root = (char *)(mConfigFileVM["fits.data_dir.root"].as<std::string>().c_str());
	fits_data_dir_telescope = (char *)(mConfigFileVM["fits.data_dir.telescope"].as<std::string>().c_str());
	fits_data_dir_instrument = (char *)(mConfigFileVM["fits.data_dir.instrument"].as<std::string>().c_str());
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
	mCachedNCols = mConfigFileVM["ccd.ncols"].as<int>(); 
	mCachedNRows = mConfigFileVM["ccd.nrows"].as<int>();
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
	mExposureCount = 0;
	mExposureIndex = 0;
}

/**
 * Set the binning to use when reading out the CCD.
 * <ul>
 * <li>We set the cached binning variables mCachedNCols and mCachedNRows to the input parameters.
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
	
	cout << "Set binning to " << unsigned(xbin) << ", " << unsigned(ybin) << endl;
	LOG4CXX_INFO(logger,"Set binning to " << unsigned(xbin) << ", " << unsigned(ybin));
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
 * @param speed The readout speed, of type ReadoutSpeed.
 * @see ReadoutSpeed
 */
void Camera::set_readout_speed(const ReadoutSpeed::type speed)
{
	cout << "Set readout speed to " << to_string(speed) << "." << endl;
	LOG4CXX_INFO(logger,"Set readout speed to " << to_string(speed) << ".");
	/* TODO */
}

/**
 * Set the gain of the camera.
 * @param gain_number The gain factor to configure the camera with os type Gain.
 * @see Gain
 */
void Camera::set_gain(const Gain::type gain_number)
{
	cout << "Set gain to " << to_string(gain_number) << "." << endl;
	LOG4CXX_INFO(logger,"Set gain to " << to_string(gain_number) << ".");
	/* TODO */
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
 * @param exptype What kind of exposure we are doing, of type ExposureType. Only the EXPOSURE, STANDARD, ARC and LAMP
 *       types are supported by this method, BIAS and DARK are rejected, and should be acquired by using start_multbias
 *       or start_multdark instead.
 * @param exposure_count The number of frames to take. Must be at least one.
 * @param exposure_length The exposure length of each frame in milliseconds. Must be at least 1 ms.
 * @see ExposureType
 * @see Camera::multrun_thread
 * @see logger
 * @see LOG4CXX_INFO
 */
void Camera::start_multrun(const ExposureType::type exptype,const int32_t exposure_count,const int32_t exposure_length)
{
	CameraException ce;

	cout << "Starting multrun thread with exposure type " << to_string(exptype) <<
		", exposure count " << exposure_count << ", exposure length " << exposure_length << "ms." << endl;
	LOG4CXX_INFO(logger,"Starting multrun thread with exposure type " << to_string(exptype) <<
		     ", exposure count " << exposure_count << ", exposure length " << exposure_length << "ms.");
	if((exptype == ExposureType::BIAS) || (exptype == ExposureType::DARK))
	{
		ce.message = "start_multrun called with illegal exptype " + to_string(exptype) + ".";
		throw ce;
	}	
	std::thread thrd(&Camera::multrun_thread, this, exptype, exposure_count, exposure_length);
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
	/* to be implemented */
	state.readout_speed = ReadoutSpeed::SLOW;
	state.gain = Gain::ONE;
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
 */
void Camera::get_last_image_filename(std::string &filename)
{
	/* TODO */
}

/**
 * Return a list of FITS image filenames from the current  / last multbias / multdark / multrun 
 * performed by the camera server.
 * @param filename_list A vector list containing strings. On return of this method, this lisit will contain 
 *                      a list of strings representing FITS image filnames of the last multbias / multdark / multrun 
 *                      performed by the camera server.
 */
void Camera::get_image_filenames(std::vector<std::string> &filename_list)
{
	/* TODO */
}


/**
 * Start cooling down the camera.
 * <ul>
 * <li>We retrieve the target temperature from the config file variable map mConfigFileVM,
 *     "ccd.target_temperature" value.
 * <li>We set the target temperature using CCD_Temperature_Set.
 * <li>We turn the cooler on using CCD_Temperature_Cooler_On.
 * </ul>
 * If setting the CCD temperature fails the method can throw a CameraException 
 * (created using create_ccd_library_exception).
 * @see Camera::mConfigFileVM
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
	target_temperature = mConfigFileVM["ccd.target_temperature"].as<double>();
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
 * This method is run as a separate thread to actually create a series of bias frames.
 * <ul>
 * <li>We get the length of the image buffer we need by calling CCD_Setup_Get_Buffer_Length, 
 *     and then resize mImageBuf to suit.
 * <li>We also get the number of binned columns and rows in the image by calling 
 *     CCD_Setup_Get_NCols / CCD_Setup_Get_Bin_X / CCD_Setup_Get_NRows / CCD_Setup_Get_Bin_Y.
 * <li>We set mExposureCount to the exposure_count parameter, so the status reflects how many images 
 *     we are going to take.
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
			add_camera_fits_headers(ExposureType::BIAS,image_index,exposure_count,0);
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
		}/* end for on exposure_count */
	}
	catch(TException&e)
	{
		cerr << "Caught TException: " << e.what() << "." << endl;
		LOG4CXX_ERROR(logger,"Caught TException: " << e.what() << ".");
	}
	catch(exception& e)
	{
		cerr << "Caught Exception: " << e.what()  << "." << endl;
		LOG4CXX_FATAL(logger,"Caught Exception: " << e.what()  << ".");
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
 * <li>We loop over the exposure_count:
 *     <ul>
 *     <li>We set mExposureIndex to the current exposure index (for status propagation).
 *     <li>We call CCD_Exposure_Expose with the exposure length parameter to tell the camera to take a
 *         dark exposure of the required length, and read out the image and store it in mImageBuf.
 *     <li>We increment the FITS filename run number by calling CCD_Fits_Filename_Next_Run.
 *     <li>We call CCD_Fits_Filename_Get_Filename to generate a FITS filename.
 *     <li>We call add_camera_fits_headers to add the internally generated camera FITS headers to mFitsHeader.
 *     <li>We call CCD_Exposure_Save to save the read out data in mImageBuf to the generated FITS filename with the FITS headers
 *         from mFitsHeader.
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
			add_camera_fits_headers(ExposureType::DARK,image_index,exposure_count,exposure_length);
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
		}/* end for on exposure_count */
	}
	catch(TException&e)
	{
		cerr << "Caught TException: " << e.what() << "." << endl;
		LOG4CXX_ERROR(logger,"Caught TException: " << e.what() << ".");
	}
	catch(exception& e)
	{
		cerr << "Caught Exception: " << e.what()  << "." << endl;
		LOG4CXX_FATAL(logger,"Caught Exception: " << e.what()  << ".");
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
void Camera::multrun_thread(const ExposureType::type exptype,int32_t exposure_count,int32_t exposure_length)
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
			add_camera_fits_headers(exptype,image_index,exposure_count,
						exposure_length);
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
		}/* end for on exposure_count */
	}
	catch(TException&e)
	{
		cerr << "Caught TException: " << e.what() << "." << endl;
		LOG4CXX_ERROR(logger,"Caught TException: " << e.what() << ".");
	}
	catch(exception& e)
	{
		cerr << "Caught Exception: " << e.what()  << "." << endl;
		LOG4CXX_FATAL(logger,"Caught Exception: " << e.what()  << ".");
	}		
}

/**
 * Method to add some of the internal FITS headers generatde from within the camera to mFitsHeader,
 * which are then saved to the generated FITS images. Headers added are:
 * <ul>
 * <li><b>EXPTYPE</b> String describing exposure type: BIAS DARK EXPOSURE ACQUIRE ARC SKYFLAT STANDARD LAMPFLAT
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
 * <li><b>IMGRECT / SUBRECT</b> We figure out the active image area. If we are windowing (mCachedWindowFlags is true), 
 *        we use the image dimensions in mCachedWindow. If we are not windowing (mCachedWindowFlags is false), 
 *        we use mCachedNCols,mCachedNRows. We construct a string "sx, sy, ex, ey" 
 *        and set the "IMGRECT" and "SUBRECT" headers to this value.
 * <li><b>UTSTART</b> We retrieve the start exposure timestamp using CCD_Exposure_Start_Time_Get. We use 
 *                   CCD_Fits_Header_TimeSpec_To_UtStart_String to format the string.
 * <li><b>DATE-OBS</b> We retrieve the start exposure timestamp using CCD_Exposure_Start_Time_Get. We use 
 *                   CCD_Fits_Header_TimeSpec_To_Date_Obs_String to format the string.
 * </ul>
 * @param exptype What kind of exposure this is, of type ExposureType.
 * @param exposure_type What kind of exposure we are doing, from the ExposureType thrift enumeration.
 * @param image_index Which image out of the exposure_count number of images we are currently doing.
 * @param exposure_count The number of images in the Multrun/Multbias/MultDark.
 * @param The exposure length, in milliseconds, of this image.
 * @see Camera::create_ccd_library_exception
 * @see ExposureType
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
 * @see CCD_Fits_Header_Add_Units
 * @see CCD_Fits_Header_TimeSpec_To_UtStart_String
 * @see CCD_Fits_Header_TimeSpec_To_Date_Obs_String
 * @see CCD_Setup_Get_Bin_X
 * @see CCD_Setup_Get_Bin_Y
 * @see CCD_Setup_Get_Camera_Head_Model_Name
 * @see CCD_Setup_Get_Camera_Serial_Number
 * @see CCD_TEMPERATURE_STATUS
 * @see CCD_Temperature_Get
 */
void Camera::add_camera_fits_headers(const ExposureType::type exptype,
				    int image_index,int32_t exposure_count,int32_t exposure_length)
{
	CameraException ce;
	enum CCD_TEMPERATURE_STATUS temperature_status;
	struct timespec start_time;
	char camera_head_model_name[128];
	char img_rect_buff[128];
	char time_string[32];
	double temperature;
	int retval,xs,ys,xe,ye;
	
	/* EXPTYPE */
	retval = CCD_Fits_Header_Add_String(&mFitsHeader,"EXPTYPE",to_string(exptype).c_str(),"");
	if(retval == FALSE)
	{
		ce = create_ccd_library_exception();
		throw ce;
	}
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
}

/**
 * This method creates a camera exception, and populates the message with an aggregation of error messasges found
 * in the CCD library.
 * @return The created camera exception. The message is generated by CCD_General_Error_To_String.
 * @see #ERROR_BUFFER_LENGTH
 * @see CameraException
 * @see CCD_General_Error_To_String
 */
CameraException Camera::create_ccd_library_exception()
{
	CameraException ce;
	char error_buffer[ERROR_BUFFER_LENGTH];
	
	CCD_General_Error_To_String(error_buffer);
	std::string str(error_buffer);
	ce.message = str;
	return ce;
}

/**
 * A C function conforming to the CCD library logging interface. This causes messages to be logged to cout, in the form:
 * "function : string".
 * @param sub_system The sub system. Can be NULL.
 * @param source_filename The source filename. Can be NULL.
 * @param function The function calling the log. Can be NULL.
 * @param level At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. Designed to be used as a filter. Can be NULL.
 * @param string The log message to be logged. 
 */
static void log_to_stdout(char *sub_system,char *source_filename,char *function,
			   int level,char *category,char *string)
{
	cout << function << ":" << string << endl;
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
	
