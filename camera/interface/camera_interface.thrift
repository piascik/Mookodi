/* Camera thrift interface */
/**
 * Enumeration to specify what kind of data a FITS header card (an individual element of a FITS header) contains.
 * <ul>
 * <li><b>INTEGER</b>
 * <li><b>FLOAT</b>
 * <li><b>STRING</b>
 * </ul>
 */
enum FitsCardType
{
     INTEGER = 0,
     FLOAT = 1,
     STRING = 2
}

/**
 * Structure containging the data for a FITS header card (an individual element of a FITS header) contains.
 * <ul>
 * <li><b>string key</b> The keyword.
 * <li><b>FitsCardType valtype</b> What kind of FIRS header card this is.
 * <li><b>string val</b> The value, as a string.
 * <li><b>string comment</b> A comment string.
 * </ul>
 * @see #FitsCardType
 */
struct FitsHeaderCard
{
       1: string key;
       2: FitsCardType valtype;
       3: string val;
       4: string comment;
}

/**
 * Enumeration to return the current state of an exposure.
 * <ul>
 * <li><b>IDLE</b>
 * <li><b>SETUP</b>
 * <li><b>EXPOSING</b>
 * <li><b>READOUT</b>
 * </ul>
 */
enum ExposureState
{
     IDLE = 0,
     SETUP = 1,
     EXPOSING = 2,
     READOUT = 3
}

/**
 * Enumeration to configure the type of exposure being performed.
 * <ul>
 * <li><b>BIAS</b>
 * <li><b>DARK</b>
 * <li><b>EXPOSURE</b>
 * <li><b>ACQUIRE</b>
 * <li><b>ARC</b>
 * <li><b>SKYFLAT</b>
 * <li><b>STANDARD</b>
 * <li><b>LAMPFLAT</b>
 * </ul>
 */
enum ExposureType
{
	BIAS = 0,
	DARK = 1,
	EXPOSURE = 2,
	ACQUIRE = 3,
	ARC = 4,
	SKYFLAT = 5,
	STANDARD = 6,
	LAMPFLAT = 7
}

/**
 * Enumeration to configure the readout speed of the detector.
 * <ul>
 * <li><b>SLOW</b>
 * <li><b>FAST</b>
 * </ul>
 */
enum ReadoutSpeed
{
	SLOW = 0,
	FAST = 1
}

struct ImageData
{
       1: list<i32> data;
       2: i32 x_size;
       3: i32 y_size;
}

/**
 * Structure containing the pixel positions of a sub-window to read out.
 * <ul>
 * <li><b>x_start</b>
 * <li><b>y_start</b>
 * <li><b>x_end</b>
 * <li><b>y_end</b>
 * </ul>
 */
struct CameraWindow
{
        1: i32 x_start;
        2: i32 y_start;
        3: i32 x_end;
        4: i32 y_end;
}

/**
 * Structure returning the current camera state.
 * <ul>
 * <li><b>xbin</b> 
 * <li><b>ybin</b>
 * <li><b>use_window</b> A boolean, if true read out just the specified sub-window, otherwise read out the full CCD.
 * <li><b>window</b> A CameraWindow structure containing the sub-window to read out.
 * <li><b>exposure_length</b> In milliseconds.
 * <li><b>elapsed_exposure_length</b> In milliseconds.
 * <li><b>remaining_exposure_length</b> In milliseconds.
 * <li><b>exposure_state</b>
 * <li><b>exposure_count</b>
 * <li><b>exposure_index</b>
 * <li><b>ccd_temperature</b> In degrees centigrade.
 * </ul>
 */
struct CameraState
{
        1: i8 xbin;
        2: i8 ybin;
	3: bool use_window;
	4: CameraWindow window;
	5: i32 exposure_length;
	6: i32 elapsed_exposure_length;
	7: i32 remaining_exposure_length;
	8: ExposureState exposure_state;
	9: i32 exposure_count;
	10: i32 exposure_index;
	11: double ccd_temperature;
}

/**
 * An exception thrown when a CameraService operation fails. Contains a string message with details of the problem.	
 */
exception CameraException
{
	1: string message;
}

/**
 * The thrift API support by the MookodiCameraService.
 */
service CameraService
{
        void set_binning(1: i8 xbin, 2: i8 ybin) throws (1: CameraException e);
        void set_window(1: i32 x_start, 2: i32 y_start, 3: i32 x_end, 4: i32 y_end) throws (1: CameraException e);
	void clear_window() throws (1: CameraException e);
	void set_readout_speed(1: ReadoutSpeed speed) throws (1: CameraException e);
	void set_gain(1: int gain_number) throws (1: CameraException e);
	void set_fits_headers(1: list<FitsHeaderCard> fits_header) throws (1: CameraException e);
	void add_fits_header(1: string keyword, 2: FitsCardType valtype,
	     3: string value,4: string comment) throws (1: CameraException e);
	void clear_fits_headers() throws (1: CameraException e);
	void start_multbias(1: i32 exposure_count) throws (1: CameraException e);
	void start_multdark(1: i32 exposure_count, 2: i32 exposure_length) throws (1: CameraException e);
	void start_multrun(1: ExposureType exptype, 2: i32 exposure_count, 3: i32 exposure_length) throws (1: CameraException e);
	void abort_exposure() throws (1: CameraException e);
	CameraState get_state() throws (1: CameraException e);
        ImageData get_image_data() throws (1: CameraException e);
	string get_last_image_filename() throws (1: CameraException e);
	list<string> get_image_filenames() throws (1: CameraException e);
	void cool_down() throws (1: CameraException e);
	void warm_up() throws (1: CameraException e);
}
