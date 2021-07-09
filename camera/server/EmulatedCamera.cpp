/**
 * @file
 * @brief EmulatedCamera.cpp provides an implementation of the camera detector thrift interface. This implementation
 *        just prints out details of the thrift API call to stdout. There is some basic exposure emulation
 *        so get_status states change in a potentially sensible manner.
 * @author Chris Mottram
 * @version $Id$
 */
#include "CameraConfig.h"
#include "EmulatedCamera.h"
#include <thread>
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>
#include "log4cxx/logger.h"

using std::cout, std::cerr, std::endl;
using namespace log4cxx;

/**
 * The configuration file section to use for retrieving configuration ("camera").
 */
#define CONFIG_CAMERA_SECTION ("Camera")
/**
 * Maximum pixel binning in X.
 */
#define MAX_X_BINNING            (16)
/**
 * Maximum pixel binning in Y.
 */
#define MAX_Y_BINNING            (16)

/**
 * Logger instance for the emulated camera (EmulatedCamera.cpp).
 */
static LoggerPtr logger(Logger::getLogger("mookodi.camera.server.EmulatedCamera"));

/**
 * Constructor for the EmulatedCamera object. 
 */
EmulatedCamera::EmulatedCamera()
{
}

/**
 * Destructor for the Camera object. Does nothing.
 */
EmulatedCamera::~EmulatedCamera()
{
}

/**
 * Method to set the configuration object to load config from in initialize.
 * @param config The configuration object.
 * @see EmulatedCamera::mCameraConfig
 */
void EmulatedCamera::set_config(CameraConfig & config)
{
	mCameraConfig = config;
}

/**
 * Initialisation method for the emulated camera object. This does the following:
 * <ul>
 * <li>We setup the cached state.
 * <li>We set mAbort to false.
 * <li>We initialise mImageBufNCols/mImageBufNRows to 0.
 * </ul>
 * @see EmulatedCamera::mState
 */
void EmulatedCamera::initialize()
{
	mState.xbin = 1;
	mState.ybin = 1;
	mState.use_window = false;
	mState.window.x_start = 0;
	mState.window.y_start = 0;
	mState.window.x_end = 0;
	mState.window.y_end = 0;
	mState.exposure_length = 0;
	mState.elapsed_exposure_length = 0;
	mState.remaining_exposure_length = 0;
	mState.exposure_in_progress = FALSE;
	mState.exposure_state = ExposureState::IDLE;
	mState.ccd_temperature = 0.0;
	mState.readout_speed = ReadoutSpeed::SLOW;
	mState.gain = Gain::ONE;
	mAbort = false;
	mImageBufNCols = 0;
	mImageBufNRows = 0;
	cout << "Detector initialised" << endl;
	LOG4CXX_INFO(logger,"Detector initialised.");
}

/**
 * Set the binning to use when reading out the CCD.
 * <ul>
 * <li>We check the xbin parameter is a legal value between 1 and MAX_X_BINNING, and throw a CamerasException
 *     if it is out of range.
 * <li>We check the ybin parameter is a legal value between 1 and MAX_Y_BINNING, and throw a CamerasException
 *     if it is out of range.
 * <li>We set the cached state binning variables mState.xbin and mState.ybin to the input parameters.
 * </ul>
 * @param xbin The binning to use in the X/horizontal direction. Should be at least 1.
 * @param ybin The binning to use in the Y/vertical direction. Should be at least 1.
 * @see #MAX_X_BINNING
 * @see #MAX_Y_BINNING
 * @see EmulatedCamera::mState
 * @see CameraException
 */
void EmulatedCamera::set_binning(const int8_t xbin, const int8_t ybin)
{
	CameraException ce;

	cout << "Set binning to " << unsigned(xbin) << ", " << unsigned(ybin) << endl;
	LOG4CXX_INFO(logger,"Set binning to " << unsigned(xbin) << ", " << unsigned(ybin));
	if(( xbin < 1)||(xbin > MAX_X_BINNING))
	{
		ce.message = "X binning "+ std::to_string(xbin) +" out of range 1 .. "+
			std::to_string(MAX_X_BINNING)+".";
		throw ce;
	}
	if(( ybin < 1)||(ybin > MAX_Y_BINNING))
	{
		ce.message = "Y binning "+ std::to_string(ybin) +" out of range 1 .. "+
			std::to_string(MAX_Y_BINNING)+".";
		throw ce;
	}
	mState.xbin = xbin;
	mState.ybin = ybin;
}

/**
 * Set the window (region of interest) to use when reading out the CCD.
 * <ul>
 * <li>We retrieve the ncols from the mCameraConfig object, "ccd.ncols" keyword.
 * <li>We retrieve the nrows from the mCameraConfig object, "ccd.nrows" keyword.
 * <li>We check whether the x_start pixel position is less than 1 or greater than the ncols,
 *     and if so we throw an out of range camera exception.
 * <li>We check whether the y_start pixel position is less than 1 or greater than the nrows,
 *     and if so we throw an out of range camera exception.
 * <li>We check whether the x_end pixel position is less than 1, greater than the ncols config file value,
 *     or less than or equal to x_start, and if so we throw an out of range camera exception.
 * <li>We check whether the y_end pixel position is less than 1, greater than the nrows config file value,
 *     or less than or equal to y_start, and if so we throw an out of range camera exception.
 * <li>We set mState's use_window to true, and set mState's window pixel values to the x_start, y_start, x_end 
 *     and y_end parameters.
 * </ul>
 * @param x_start The start X pixel position of the sub-window. Should be at least 1, 
 *        and less than the X size of the detector.
 * @param y_start The start Y pixel position of the sub-window. Should be at least 1, 
 *        and less than the Y size of the detector.
 * @param x_end The end X pixel position of the sub-window. Should be at least 1, 
 *        less than the X size of the detector, and greater than x_start.
 * @param y_end The end Y pixel position of the sub-window. Should be at least 1, 
 *        less than the Y size of the detector, and greater than y_start.
 * @see EmulatedCamera::mCameraConfig
 * @see EmulatedCamera::mState
 * @see CameraException
 */
void EmulatedCamera::set_window(const int32_t x_start,const int32_t y_start,const int32_t x_end,const int32_t y_end)
{
	CameraException ce;
	int ncols, nrows;
	
	cout << "Set window to start position ( " << x_start << ", " << y_start << " ), end position ( " << x_end <<
		", " << y_end << " )." << endl;
	LOG4CXX_INFO(logger,"Set window to start position ( " << x_start << ", " << y_start <<
		     " ), end position ( " << x_end << ", " << y_end << " ).");
	// retrieve needed config
	mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.ncols",&ncols);
	mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.nrows",&nrows);
	// check window is legal
	if((x_start < 0)||(x_start >= ncols))
	{
		ce.message = "Window x_start position "+ std::to_string(x_start) +" out of range 1 .. "+
			std::to_string(ncols)+".";
		throw ce;
	}
	if((y_start < 0)||(y_start >= nrows))
	{
		ce.message = "Window y_start position "+ std::to_string(y_start) +" out of range 1 .. "+
			std::to_string(nrows)+".";
		throw ce;
	}
	if((x_end < 0)||(x_end >= ncols)||(x_end <= x_start))
	{
		ce.message = "Window x_end position "+ std::to_string(x_end) +" out of range "+
			std::to_string(x_start)+" .. "+std::to_string(ncols)+".";
		throw ce;
	}
	if((y_end < 0)||(y_end >= nrows)||(y_end <= y_start))
	{
		ce.message = "Window y_end position "+ std::to_string(y_end) +" out of range "+
			std::to_string(y_start)+" .. "+std::to_string(nrows)+".";
		throw ce;
	}
	// set window state
	mState.use_window = true;
	mState.window.x_start = x_start;
	mState.window.y_start = y_start;
	mState.window.x_end = x_end;
	mState.window.y_end = y_end;
}

/**
 * Clear a previously set sub-window i.e. re-configure the deterctor to read out full-frame.
 * <ul>
 * <li>We set mState.use_window to false.
 * </ul>
 * @see EmulatedCamera::mState
 */
void EmulatedCamera::clear_window()
{
	cout << "Clear window." << endl;
	LOG4CXX_INFO(logger,"Clear window.");
	mState.use_window = false;
}

/**
 * Set the readout speed of the camera (either SLOW or FAST).
 * @param speed The readout speed, of type ReadoutSpeed.
 * @see ReadoutSpeed
 * @see #mState
 */
void EmulatedCamera::set_readout_speed(const ReadoutSpeed::type speed)
{
	cout << "Set readout speed to " << to_string(speed) << "." << endl;
	LOG4CXX_INFO(logger,"Set readout speed to " << to_string(speed) << ".");
	mState.readout_speed = speed;
}

/**
 * Set the gain of the camera.
 * @param gain_number The gain factor to configure the camera with os type Gain.
 * @see Gain
 * @see #mState
 */
void EmulatedCamera::set_gain(const Gain::type gain_number)
{
	cout << "Set gain to " << to_string(gain_number) << "." << endl;
	LOG4CXX_INFO(logger,"Set gain to " << to_string(gain_number) << ".");
	mState.gain = gain_number;
}

/**
 * Method to set the FITS headers. Currently a blank implementation.
 * @param fits_info The list of FITS header cards to add to the FITS header.
 * @see FitsHeaderCard 
 */
void EmulatedCamera::set_fits_headers(const std::vector<FitsHeaderCard> & fits_info)
{
	cout << "Set FITS headers." << endl;
	LOG4CXX_INFO(logger,"Set FITS headers.");
}

/**
 * Add a FITS header to the list of FITS headers to be saved to FITS images. Currently a blank implementation.
 * @param keyword The FITS header card keyword.
 * @param valtype What kind of value this header takes, of type FitsCardType.
 * @param value A string representation of the value.
 * @param comment A string containing a comment for this header.
 * @see FitsCardType
 */
void EmulatedCamera::add_fits_header(const std::string & keyword, const FitsCardType::type valtype,
				     const std::string & value,const std::string & comment)
{
	cout << "Add FITS header " << keyword  << " of type " << to_string(valtype) << " and value " << value << endl;
	LOG4CXX_INFO(logger,"Add FITS header " << keyword  << " of type " << to_string(valtype) <<
		     " and value " << value );
}

/**
 * Entry point to a routine to clear out FITS headers. Currently a blank implementation.
 */
void EmulatedCamera::clear_fits_headers()
{
	cout << "Clear FITS headers." << endl;
	LOG4CXX_INFO(logger,"Clear FITS headers.");
}

/**
 * Thrift entry point to set the exposure length to use for subsequent darks and exposures.
 * @param exposure_length The exposure length to use, in milliseconds.
 * @see EmulatedCamera::mState
 */
void set_exposure_length(const int32_t exposure_length)
{
	cout << "Setting exposure length to " << exposure_length << "ms a." << endl;
	LOG4CXX_INFO(logger,"Setting exposure length to  " << exposure_length << "ms.");
	if(exposure_length < 0)
	{
		ce.message = "set_exposure_length: exposure length " << exposure_length << " is too small.";
		LOG4CXX_ERROR(logger,ce.message);
		throw ce;
	}
	mState.exposure_length = exposure_length;
}

/**
 * thrift entry point to take an exposure with the camera. We set mState's exposure_in_progress to TRUE to show
 * an exposure is in progress.
 * A new thread running an instance of expose_thread is started.
 * @param save_image A boolean, if true save the image in a FITS filename, otherwise don't 
 *        (the image data can be retrieved using the get_image_data method).
 * @see EmulatedCamera::mState
 * @see EmulatedCamera::expose_thread
 * @see EmulatedCamera::get_image_data
 * @see logger
 * @see LOG4CXX_INFO
 */
void EmulatedCamera::start_expose(const bool save_image)
{
	CameraException ce;

	cout << "Starting expose thread with exposure length " << mState.exposure_length <<
		"ms and save_image " << save_image << "." << endl;
	LOG4CXX_INFO(logger,"Starting expose thread with exposure length " << mState.exposure_length <<
		     "ms and save_image " << save_image << ".");
	if(mState.exposure_length < 1)
	{
		ce.message = "Exposure length "+ std::to_string(mState.exposure_length) +" too small.";
		throw ce;
	}
	mState.exposure_in_progress = TRUE;
	std::thread thrd(&EmulatedCamera::expose_thread, this, mState.exposure_length, save_image);
	thrd.detach();
}

/**
 * thrift entry point to start taking a bias frame. We set mState's exposure_in_progress to TRUE to show
 * a bias is in progress. A new thread running an instance of bias_thread is started.
 * @see EmulatedCamera::mState
 * @see EmulatedCamera::bias_thread
 * @see CameraException
 */
void EmulatedCamera::start_bias()
{
	CameraException ce;

	cout << "Starting bias thread." << endl;
	LOG4CXX_INFO(logger,"Starting bias thread.");
	mState.exposure_in_progress = TRUE;
	std::thread thrd(&EmulatedCamera::bias_thread, this);
	thrd.detach();
}

/**
 * thrift entry point to start taking a dark frame. We set mState's exposure_in_progress to TRUE to show
 * a dark is in progress. A new thread running an instance of dark_thread is started.
 * @see EmulatedCamera::mState
 * @see EmulatedCamera::dark_thread
 * @see CameraException
 */
void EmulatedCamera::start_dark()
{
	CameraException ce;

	if(mState.exposure_length < 1)
	{
		ce.message = "Exposure length "+ std::to_string(mState.exposure_length) +" too small.";
		throw ce;
	}
	cout << "Starting dark thread with exposure length " << mState.exposure_length << "ms." << endl;
	LOG4CXX_INFO(logger,"Starting dark thread with exposure length " << mState.exposure_length << "ms.");
	mState.exposure_in_progress = TRUE;
	std::thread thrd(&EmulatedCamera::dark_thread, this, mState.exposure_length);
	thrd.detach();
}


/**
 * Abort a running expose/dark/bias. 
 * This set mAbort to true.
 * @see EmulatedCamera::mAbort
 */
void EmulatedCamera::abort_exposure()
{
	cout << "Abort exposure." << endl;
	LOG4CXX_INFO(logger,"Abort exposure.");
	mAbort = true;
}

/**
 * Get the current state of the camera. Here we set state to the mState member variable.
 * @param state An instance of CameraState that we set the member values to the current status.
 * @see EmulatedCamera::mState
 */
void EmulatedCamera::get_state(CameraState &state)
{
	cout << "Get camera state." << endl;
	LOG4CXX_INFO(logger,"Get camera state.");
	state = mState;
}

/**
 * Get a copy of the image data.
 * @param img_data An ImageData instance to fill in with the returned image data.
 * @see EmulatedCamera::mImageBuf
 * @see EmulatedCamera::mImageBufNCols
 * @see EmulatedCamera::mImageBufNRows
 * @see ImageData
 */
void EmulatedCamera::get_image_data(ImageData &img_data)
{
	cout << "Get image data." << endl;
	LOG4CXX_INFO(logger,"Get image data.");
	std::vector<int32_t>::const_iterator first = mImageBuf.begin();
	std::vector<int32_t>::const_iterator last = mImageBuf.end();
	img_data.data.assign(first, last);
	img_data.x_size = mImageBufNCols;
	img_data.y_size = mImageBufNRows;
}

/**
 * Return the image filename of the last FITS image saved by the camera server.
 * @param filename On return of this method, the filename will contain a string representation of 
 *                 the last FITS image filename saved by the camera server.
 */
void EmulatedCamera::get_last_image_filename(std::string &filename)
{
	/* TODO */
	filename = "/data/lesedi/mkd/2021/0413/MKD_20210413.0001.fits";
}

/**
 * thrift entry point to start cooling down the camera. 
 * We retrieve the target temperature from the config file object mCameraConfig,
 * "ccd.target_temperature" value, and set the mState.ccd_temperature to this to emulate a cooled CCD.
 * @see EmulatedCamera::mCameraConfig
 * @see EmulatedCamera::mState
 */
void EmulatedCamera::cool_down()
{
	double target_temperature;

	cout << "Cool down the camera." << endl;
	LOG4CXX_INFO(logger,"Cool down the camera.");
	mCameraConfig.get_config_double(CONFIG_CAMERA_SECTION,"ccd.target_temperature",&target_temperature);
	mState.ccd_temperature = target_temperature;
	cout << "Camera temperature setpoint is " << mState.ccd_temperature << "." << endl;
	LOG4CXX_INFO(logger,"Camera temperature setpoint is " << mState.ccd_temperature << ".");
}

/**
 * thrift entry point to start warming up the camera. 
 * We set the mState.ccd_temperature to 10.0 to emulate the CCD warmed up.
 * @see EmulatedCamera::mState
 */
void EmulatedCamera::warm_up()
{
	cout << "Warm up the camera." << endl;
	LOG4CXX_INFO(logger,"Warm up the camera.");
	mState.ccd_temperature = 10.0;
	cout << "Camera warmed up to " << mState.ccd_temperature << " C." << endl;
	LOG4CXX_INFO(logger,"Camera warmed up to " << mState.ccd_temperature << " C.");
}

/**
 * Thread to emulate the taking of an image.
 * <ul>
 * <li>We use mState.use_window to determine whether to are reading out a window or full frame,
 *     and based on that use either mState.window or mCameraConfig config value "ccd.ncols"/mCameraConfig config value "ccd.nrows" 
 *     to calculate the image dimensions.
 * <li>We compute the total number of pixels in the readout image using the data calculated.
 * <li>We intialise mState's exposure_length to the expose_thread's exposure_length parameter.
 * <li>We initialise mAbort to false.
 * <li>We set mState's exposure_state to exposing, 
 *     elapsed_exposure_length to 0 and remaining_exposure_length to the exposure_length parameter.
 * <li>We enter a while loop, while mState's remaining_exposure_length is greater than 0 and mAbort is false:
 *     <ul>
 *     <li>We sleep for 1 second.
 *     <li>We decrement mState's remaining_exposure_length by 1000 ms.
 *     <li>We increment mState's elapsed_exposure_length by 1000 ms.
 *     </ul>
 * <li>We check whether mAbort is set true, and if so reset mState's exposure_state to idle and exit the thread.
 * <li>We set mState's exposure_state to readout, and sleep for a second to emulate the readout.
 * <li>We resize mImageBuf to the computed total number of pixels in the readout image.
 * <li>We loop over the image dimensions setting the pixel value in mImageBuf.
 * <li>We sleep for another second.
 * <li>We check whether mAbort is set true, and if so reset mState's exposure_state to idle and exit the thread.
 * <li>We reset mState's exposure_state to idle.
 * </ul>
 * @param exposure_length The length of the exposure in milliseconds. Should be at least 1.
 * @param save_image A boolean, whether to save the taken image to disc - ignored by the camera emulator.
 * @see EmulatedCamera::mState
 * @see EmulatedCamera::mCameraConfig
 * @see EmulatedCamera::mAbort
 * @see EmulatedCamera::mImageBuf
 * @see EmulatedCamera::mImageBufNCols
 * @see EmulatedCamera::mImageBufNRows
 */
void EmulatedCamera::expose_thread(int32_t exposure_length, bool save_image)
{
	int reg_width;
	int reg_height;
	int total_pixels;

	mState.exposure_in_progress = TRUE;
	// setup image dimensions
	if(mState.use_window)
	{
		reg_width = (mState.window.x_end - mState.window.x_start)+1;
		reg_height = (mState.window.y_end - mState.window.y_start)+1; 	
	}
	else
	{
		mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.ncols",&reg_width);
		mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.nrows",&reg_height);
	}
	mImageBufNCols = reg_width;
	mImageBufNRows = reg_height;
	total_pixels = reg_width * reg_height;
 	cout << "expose thread with exposure length " << exposure_length << "ms." << endl;
	LOG4CXX_INFO(logger,"expose thread with exposure length " << exposure_length << "ms.");
	mState.exposure_length = exposure_length;
	mAbort = false;
	mState.exposure_state = ExposureState::EXPOSING;
	mState.elapsed_exposure_length = 0;
	mState.remaining_exposure_length = exposure_length;
	cout << "Starting exposure of length " << exposure_length << " ms." << endl;
	LOG4CXX_INFO(logger,"Starting exposure of length " << exposure_length << " ms.");
	// Simulate the exposure
	while ( (mState.remaining_exposure_length > 0) && (mAbort == false))
	{
		// sleep for 1 second
		std::this_thread::sleep_for(std::chrono::seconds(1));
		// reduce remaining exposure length by 1000 ms
		mState.remaining_exposure_length -= 1000;
		// increased elapsed exposure length by 1000 ms
		mState.elapsed_exposure_length  += 1000;
	}
	if(mAbort)
	{
		mState.exposure_in_progress = FALSE;
		mState.exposure_state = ExposureState::IDLE;
		return;
	}
	// Simulate the readout
	cout << "Starting readout" << endl;
	LOG4CXX_INFO(logger,"Starting readout");
	mState.exposure_state = ExposureState::READOUT;
	std::this_thread::sleep_for(std::chrono::seconds(1));   
	mImageBuf.resize(total_pixels);
	for (int i = 0; i < reg_height; i++)
	{
		for (int j = 0; j < reg_width; j++)
		{
			mImageBuf[i*reg_width+j] = (i*j) * pow(2, 14) / total_pixels;
		}
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	// We're done
	cout << "Exposure complete." << endl;
	LOG4CXX_INFO(logger,"Exposure complete.");
	if(mAbort)
	{
		mState.exposure_in_progress = FALSE;
		mState.exposure_state = ExposureState::IDLE;
		return;
	}
	mState.exposure_in_progress = FALSE;
	mState.exposure_state = ExposureState::IDLE;
	cout << "Expose complete" << endl;
	LOG4CXX_INFO(logger,"Expose complete");
}

/**
 * Thread to emulate the taking of a bias image.
 * <ul>
 * <li>We use mState.use_window to determine whether to are reading out a window or full frame,
 *     and based on that use either mState.window or mCameraConfig config value "ccd.ncols" /"ccd.nrows" 
 *     to calculate the image dimensions.
 * <li>We compute the total number of pixels in the readout image using the data calculated.
 * <li>We intialise mState's exposure_length and exposure_index to 0.
 * <li>We initialise mAbort to false.
 * <li>We set mState's exposure_state to exposing, elapsed_exposure_length and remaining_exposure_length to 0.
 * <li>We check whether mAbort is set true, and if so reset mState's exposure_state to idle and exit the thread.
 * <li>We set mState's exposure_state to readout, and sleep for a second to emulate the readout.
 * <li>We resize mImageBuf to the computed total number of pixels in the readout image.
 * <li>We loop over the image dimensions setting the pixel value in mImageBuf.
 * <li>We sleep for another second.
 * <li>We check whether mAbort is set true, and if so reset mState's exposure_state to idle and exit the thread.
 * <li>We reset mState's exposure_state to idle.
 * </ul>
 * @see EmulatedCamera::mState
 * @see EmulatedCamera::mCameraConfig
 * @see EmulatedCamera::mAbort
 * @see EmulatedCamera::mImageBuf
 * @see EmulatedCamera::mImageBufNCols
 * @see EmulatedCamera::mImageBufNRows
 */
void EmulatedCamera::bias_thread()
{
	int reg_width;
	int reg_height;
	int total_pixels;

	mState.exposure_in_progress = TRUE;
	// setup image dimensions
	if(mState.use_window)
	{
		reg_width = (mState.window.x_end - mState.window.x_start)+1;
		reg_height = (mState.window.y_end - mState.window.y_start)+1; 	
	}
	else
	{
		mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.ncols",&reg_width);
		mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.nrows",&reg_height);
	}
	mImageBufNCols = reg_width;
	mImageBufNRows = reg_height;
	total_pixels = reg_width * reg_height;
 	cout << "Starting bias thread." << endl;
	LOG4CXX_INFO(logger,"Starting bias thread.");
	mState.exposure_length = 0;
	mAbort = false;
	mState.exposure_state = ExposureState::EXPOSING;
	mState.elapsed_exposure_length = 0;
	mState.remaining_exposure_length = 0;
	if(mAbort)
	{
		mState.exposure_in_progress = FALSE;
		mState.exposure_state = ExposureState::IDLE;
		return;
	}
	// Simulate the readout
	cout << "Starting readout" << endl;
	LOG4CXX_INFO(logger,"Starting readout");
	mState.exposure_state = ExposureState::READOUT;
	std::this_thread::sleep_for(std::chrono::seconds(1));   
	mImageBuf.resize(total_pixels);
	for (int i = 0; i < reg_height; i++)
	{
		for (int j = 0; j < reg_width; j++)
		{
			mImageBuf[i*reg_width+j] = (i*j) * pow(2, 14) / total_pixels;
		}
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	// We're done
	cout << "Bias complete." << endl;
	LOG4CXX_INFO(logger,"Bias complete.");
	if(mAbort)
	{
		mState.exposure_in_progress = FALSE;
		mState.exposure_state = ExposureState::IDLE;
		return;
	}
	mState.exposure_in_progress = FALSE;
	mState.exposure_state = ExposureState::IDLE;
	cout << "bias complete" << endl;
	LOG4CXX_INFO(logger,"bias complete");
}

/**
 * Thread to emulate the taking of a dark image.
 * <ul>
 * <li>We use mState.use_window to determine whether to are reading out a window or full frame,
 *     and based on that use either mState.window or mCameraConfig config values "ccd.ncols"/"ccd.nrows" 
 *     to calculate the image dimensions.
 * <li>We compute the total number of pixels in the readout image using the data calculated.
 * <li>We intialise mState's exposure_length to the dark_thread exposure_length parameter.
 * <li>We initialise mAbort to false.
 * <li>We set mState's exposure_state to exposing, exposure_index to the for loops index, 
 *     elapsed_exposure_length to 0 and remaining_exposure_length to the exposure_length parameter.
 * <li>We enter a while loop, while mState's remaining_exposure_length is greater than 0 and mAbort is false:
 *     <ul>
 *     <li>We sleep for 1 second.
 *     <li>We decrement mState's remaining_exposure_length by 1000 ms.
 *     <li>We increment mState's elapsed_exposure_length by 1000 ms.
 *     </ul>
 * <li>We check whether mAbort is set true, and if so reset mState's exposure_state to idle and exit the thread.
 * <li>We set mState's exposure_state to readout, and sleep for a second to emulate the readout.
 * <li>We resize mImageBuf to the computed total number of pixels in the readout image.
 * <li>We loop over the image dimensions setting the pixel value in mImageBuf.
 * <li>We sleep for another second.
 * <li>We check whether mAbort is set true, and if so reset mState's exposure_state to idle and exit the thread.
 * <li>We reset mState's exposure_state to idle.
 * </ul>
 * @param exposure_length The length of one exposure in milliseconds. Should be at least 1.
 * @see EmulatedCamera::mState
 * @see EmulatedCamera::mCameraConfig
 * @see EmulatedCamera::mAbort
 * @see EmulatedCamera::mImageBuf
 * @see EmulatedCamera::mImageBufNCols
 * @see EmulatedCamera::mImageBufNRows
 */
void EmulatedCamera::dark_thread(int32_t exposure_length)
{
	int reg_width;
	int reg_height;
	int total_pixels;

	mState.exposure_in_progress = TRUE;
	// setup image dimensions
	if(mState.use_window)
	{
		reg_width = (mState.window.x_end - mState.window.x_start)+1;
		reg_height = (mState.window.y_end - mState.window.y_start)+1; 	
	}
	else
	{
		mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.ncols",&reg_width);
		mCameraConfig.get_config_int(CONFIG_CAMERA_SECTION,"ccd.nrows",&reg_height);
	}
	mImageBufNCols = reg_width;
	mImageBufNRows = reg_height;
	total_pixels = reg_width * reg_height;
 	cout << "dark thread with exposure length " << exposure_length << "ms." << endl;
	LOG4CXX_INFO(logger,"dark thread with  exposure length " << exposure_length << "ms.");
	mState.exposure_length = exposure_length;
	mAbort = false;
	mState.exposure_state = ExposureState::EXPOSING;
	mState.elapsed_exposure_length = 0;
	mState.remaining_exposure_length = exposure_length;
	cout << "Starting dark exposure of length " << exposure_length << " ms." << endl;
	LOG4CXX_INFO(logger,"Starting dark exposure of length " << exposure_length << " ms.");
	// Simulate the exposure
	while ( (mState.remaining_exposure_length > 0) && (mAbort == false))
	{
		// sleep for 1 second
		std::this_thread::sleep_for(std::chrono::seconds(1));
		// reduce remaining exposure length by 1000 ms
		mState.remaining_exposure_length -= 1000;
		// increased elapsed exposure length by 1000 ms
		mState.elapsed_exposure_length  += 1000;
	}
	if(mAbort)
	{
		mState.exposure_in_progress = FALSE;
		mState.exposure_state = ExposureState::IDLE;
		return;
	}
	// Simulate the readout
	cout << "Starting readout" << endl;
	LOG4CXX_INFO(logger,"Starting readout");
	mState.exposure_state = ExposureState::READOUT;
	std::this_thread::sleep_for(std::chrono::seconds(1));   
	mImageBuf.resize(total_pixels);
	for (int i = 0; i < reg_height; i++)
	{
		for (int j = 0; j < reg_width; j++)
		{
			mImageBuf[i*reg_width+j] = (i*j) * pow(2, 14) / total_pixels;
		}
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	// We're done
	cout << "Dark exposure complete." << endl;
	LOG4CXX_INFO(logger,"Dark exposure complete.");
	if(mAbort)
	{
		mState.exposure_in_progress = FALSE;
		mState.exposure_state = ExposureState::IDLE;
		return;
	}
	mState.exposure_in_progress = FALSE;
	mState.exposure_state = ExposureState::IDLE;
	cout << "dark complete" << endl;
	LOG4CXX_INFO(logger,"dark complete");
}

