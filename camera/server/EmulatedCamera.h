/**
 * @file
 * @brief Camera.h provides the API declaration of the emulated camera detector thrift interface. 
 * @author Chris Mottram
 * @version $Id$
 */
#include "CameraService.h"
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
    void load_config();
    void initialize();
    void set_config_filename(const std::string & config_filename);

    // Configure camera
    void set_binning(const int8_t xbin, const int8_t ybin);
    void set_window(const int32_t x_start,const int32_t y_start,const int32_t x_end,const int32_t y_end);
    void clear_window();
    
    // FITS header processing routines
    void set_fits_headers(const std::vector<FitsHeaderCard> & fits_info);
    void add_fits_header(const std::string & keyword, const FitsCardType::type valtype,const std::string & value,
			 const std::string & comment);
    void clear_fits_headers();
    
    // Take and process exposures
    void start_multbias(const int32_t exposure_count);
    void start_multdark(const int32_t exposure_count,const int32_t exposure_length);
    void start_multrun(const ExposureType::type exptype, const int32_t exposure_count,const int32_t  exposure_length);
    void abort_exposure();
    
    // Return state and data
    void get_state(CameraState &state);
    void get_image_data(ImageData& img_data);

    //Camera temperature control
    void cool_down();
    void warm_up();
    
  private:
    // Private methods
    void multbias_thread(int32_t exposure_count);
    void multdark_thread(int32_t exposure_count,int32_t exposure_time);
    void multrun_thread(int32_t exposure_count,int32_t exposure_time);

    // Private member vars
    /**
     * The configuration filename to load configuration data from.
     */
    std::string mConfigFilename;
    /**
     * The loaded config from the configuration filename.
     */
    boost::program_options::variables_map mConfigFileVM;
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