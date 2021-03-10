/**
 * @file
 * @brief Camera.h provides the API declaration of the camera detector thrift interface. 
 * @author Chris Mottram
 * @version $Id$
 */
#include "CameraService.h"
#include <log4cxx/logger.h>
#include <boost/program_options.hpp>
#include "ccd_fits_header.h"
#include "ccd_setup.h"

using std::string;
using std::vector;

class Camera : virtual public CameraServiceIf
{
  public:
    // Set up system
    Camera();
    virtual ~Camera();
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
    void start_multdark(const int32_t exposure_count,const int32_t exposure_time);
    void start_multrun(const ExposureType::type exptype,const int32_t exposure_count,const int32_t exposure_time);
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
    void multdark_thread(int32_t exposure_count,int32_t exposure_length);
    void multrun_thread(const ExposureType::type exptype,int32_t exposure_count,int32_t exposure_length);
    void add_camera_fits_headers(const ExposureType::type,
				 int image_index,int32_t exposure_count,int32_t exposure_length);
    CameraException create_ccd_library_exception();
    /**
     * The configuration filename to load configuration data from.
     */
    std::string mConfigFilename;
    /**
     * The loaded config from the configuration filename.
     */
    boost::program_options::variables_map mConfigFileVM;
    /**
     * The variable used to hold FITS header data, used by the CCD library to write FITS headers into read out images.
     * @see Fits_Header_Struct
     * @see CCD_Fits_Header_Initialise
     */
    struct Fits_Header_Struct mFitsHeader;
    /**
     * A cached copy of the number of imaging columns on the detector.
     */
    int mCachedNCols;
    /**
     * A cached copy of the number of imaging rows on the detector.
     */
    int mCachedNRows;
    /**
     * A cached copy of the current horizontal binning of the detector.
     */
    int mCachedHBin;
    /**
     * A cached copy of the current vertical binning of the detector.
     */
    int mCachedVBin;
    /**
     * A cached copy of the window flags. This is really a boolean, if true we use the mCachedWindow to
     * configure the detector readout, otherwise we use mCachedNCols/mCachedNRows (i.e. full frame).
     * @see Camera::mCachedWindow
     * @see Camera::mCachedNCols
     * @see Camera::mCachedNRows
     */
    int mCachedWindowFlags;
    /**
     * A cached copy of the sub-window on the detector to read out. This is only used if mCachedWindowFlags
     * is true.
     * @see CCD_Setup_Window_Struct
     * @see Camera::mCachedWindowFlags
     */
    struct CCD_Setup_Window_Struct mCachedWindow;
    /**
     * Which exposure we are currently taking in the current multrun.
     */
    int mExposureIndex;
    /**
     * The number of exposure to take in the current multrun.
     */
    int mExposureCount;
    /**
     * The image buffer, used to hold read-out images.
     */
    std::vector<int16_t> mImageBuf;
};    
