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

/**
 * Valid gain factors supported by the camera.
 * <ul> 
 * <li><b>ONE</b> Gain factor of 1.
 * <li><b>TWO</b> Gain factor of 2.
 * <li><b>FOUR</b> Gain factor of 4.
 * </ul>
 */
enum Gain
{
	ONE = 1,
	TWO = 2,
	FOUR = 4
}

/**
 * Structure containing read out data from the camera.
 * <ul> 
 * <li><b>data</b> The actual read out data, 16-bit unsigned data in an i32 list.
 * <li><b>x_size</b> The X (horizontal) dimension of the image data.
 * <li><b>y_size</b> The Y (vertical) dimension of the image data.
 * </ul>
 */
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
 * <li><b>readout_speed</b> The currently configured readout speed of the camera.
 * <li><b>gain</b> The currently configured gain of the camera.
 * </ul>
 * @see CameraWindow
 * @see ExposureState
 * @see ReadoutSpeed
 * @see Gain
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
	12: ReadoutSpeed readout_speed;
	13: Gain gain;
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
 * <ul>
 * <li><b>set_binning</b> Set the binning used by the camera for the next readout (in pixels).
 * <li><b>set_window</b> Set a region of interest/sub-window of the detector for the next readout (in pixelss).
 * <li><b>clear_window</b> Remove a previously configured sub-window.
 * <li><b>set_readout_speed</b> Configure whether to read out the image data slow or fast.
 * <li><b>set_gain</b> Set the gain to be used for the next readout.
 * <li><b>set_fits_headers</b> Add a list of FITS headers, to the CameraService's internal list, which will be
 *                             be added to the next FITS image genreated by a readout.
 * <li><b>add_fits_header</b> Add an individual FITS header to the CameraService's internal list, which will be
 *                             be added to the next FITS image genreated by a readout.
 * <li><b>clear_fits_headers</b> Remove all the current FITS headers from the CameraService's internal list.
 * <li><b>start_multbias</b> Start a thread to take a series of bias frames.
 * <li><b>start_multdark</b> Start a thread to take a series of dark frames of a specified exposure length (in ms).
 * <li><b>start_multrun</b> Start a thread to take a series of exposures of the specified exposure type,
 *                          of a specified exposure length (in ms)
 * <li><b>abort_exposure</b> Abort (stop) a currently running multbias / multdark / multrun
 * <li><b>get_state</b> Get the current state of the camera / configuration / multbias / multdark / multrun.
 * <li><b>get_image_data</b> Get a copy of the last image read out by the camera. 
 * <li><b>get_last_image_filename</b> Get the filename of the last FITS image written to disk.
 * <li><b>get_image_filenames</b> Get a list of filename of the last set of FITS images written to disk
 *                               as a result of the last multbias / multdark / multrun.
 * <li><b>cool_down</b> Cool down the camera to it's operating temperature.
 * <li><b>warm_up</b> Warm up the camera to ambient temperature.
 * </ul>
 * @see CameraException
 * @see ReadoutSpeed
 * @see Gain
 * @see FitsHeaderCard
 * @see ExposureType
 * @see CameraState
 */
service CameraService
{
        void set_binning(1: i8 xbin, 2: i8 ybin) throws (1: CameraException e);
        void set_window(1: i32 x_start, 2: i32 y_start, 3: i32 x_end, 4: i32 y_end) throws (1: CameraException e);
	void clear_window() throws (1: CameraException e);
	void set_readout_speed(1: ReadoutSpeed speed) throws (1: CameraException e);
	void set_gain(1: Gain gain_number) throws (1: CameraException e);
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
