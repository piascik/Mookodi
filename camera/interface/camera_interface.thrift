/* Camera thrift interface */

enum FitsCardType
{
     INTEGER = 0,
     FLOAT = 1,
     STRING = 2
}

struct FitsHeaderCard
{
       1: string key;
       2: FitsCardType valtype;
       3: string val;
       4: string comment;
}

enum ExposureState
{
     IDLE = 0,
     SETUP = 1,
     EXPOSING = 2,
     READOUT = 3
}

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

struct ImageData
{
       1: list<i32> data,
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

exception CameraException
{
	1: string message;
}

/* TODO shutdown? */
service CameraService
{
        void set_binning(1: i8 xbin, 2: i8 ybin) throws (1: CameraException e);
        void set_window(1: i32 x_start, 2: i32 y_start, 3: i32 x_end, 4: i32 y_end) throws (1: CameraException e);
	void clear_window() throws (1: CameraException e);
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
	void cool_down() throws (1: CameraException e);
	void warm_up() throws (1: CameraException e);
}
