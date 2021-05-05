/**
 * @file
 * @brief Camera.h provides the API declaration of the emulated camera detector thrift interface. 
 * @author Chris Mottram
 * @version $Id$
 */
#ifndef EMULATED_CAMERA_H
#define EMULATED_CAMERA_H
#include "CameraService.h"
#include "CameraConfig.h"
#include <boost/program_options.hpp>
#include <log4cxx/logger.h>

using std::string;
using std::vector;

class EmulatedCamera : virtual public CameraServiceIf
{
  public:
    // Set up system
    EmulatedCamera();
    virtual ~EmulatedCamera();
    void initialize();
    void set_config(CameraConfig & config);

    // Configure camera
    void set_binning(const int8_t xbin, const int8_t ybin);
    void set_window(const int32_t x_start,const int32_t y_start,const int32_t x_end,const int32_t y_end);
    void clear_window();
    void set_readout_speed(const ReadoutSpeed::type speed);
    void set_gain(const Gain::type gain_number);
    
    // FITS header processing routines
    void set_fits_headers(const std::vector<FitsHeaderCard> & fits_info);
    void add_fits_header(const std::string & keyword, const FitsCardType::type valtype,const std::string & value,
			 const std::string & comment);
    void clear_fits_headers();
    
    // Take and process exposures
    void start_expose(const int32_t exposure_length, const bool save_image);
    void start_multbias(const int32_t exposure_count);
    void start_multdark(const int32_t exposure_count,const int32_t exposure_length);
    void start_multrun(const int32_t exposure_count,const int32_t  exposure_length);
    void abort_exposure();
    
    // Return state and data
    void get_state(CameraState &state);
    void get_image_data(ImageData& img_data);
    void get_last_image_filename(std::string &filename);
    void get_image_filenames(std::vector<std::string> &filename_list);
    
    //Camera temperature control
    void cool_down();
    void warm_up();
    
  private:
    // Private methods
    void expose_thread(int32_t exposure_length, bool save_image);
    void multbias_thread(int32_t exposure_count);
    void multdark_thread(int32_t exposure_count,int32_t exposure_time);
    void multrun_thread(int32_t exposure_count,int32_t exposure_time);

    // Private member vars
    /**
     * The configuration object to load configuration data from.
     * @see CameraConfig
     */
    CameraConfig mCameraConfig;
    /**
     * A local copy of a CameraState instance, use to emulate exposures and state changes and returned
     * as status when get_state is called.
     * @see EmulatedCamera::get_state
     * @see CameraState
     */
    CameraState mState;
    /**
     * A set of FITS headers used for emulation.
     */
    std::vector<FitsHeaderCard> mFitsHeader;
    /**
     * An emulated image buffer, used to simulate CCD readouts.
     */
    std::vector<int32_t> mImageBuf;
    /**
     * A cached copy of the number of columns (x dimension) of data in the image buffer.
     */
    int mImageBufNCols;
    /**
     * A cached copy of the number of rows (y dimension) of data in the image buffer.
     */
    int mImageBufNRows;
    /**
     * This is used to simulate aborting exposures. It is set to false at the start of a 
     * multbias/multdark/multrun thread, and can be set using abort_exposure, 
     * which causes the multbias/multdark/multrun to terminate early.
     * @see EmulatedCamera::abort_exposure
     * @see EmulatedCamera::multbias_thread
     * @see EmulatedCamera::multdark_thread
     * @see EmulatedCamera::multrun_thread
     */
    bool mAbort;
};    
#endif
