namespace py lesedi.sdk

const string DEFAULT_HOST = "localhost";
const i32    DEFAULT_PORT = 9050;

exception LesediException {
    1: string message;
}

enum State {
    OFF = 0,
    STARTUP = 1,
    READY = 2,
    SHUTDOWN = 3
}

enum Instrument {
    SHOC = 1,
    WINCAM = 2
}

enum TrackType {
    SIDEREAL = 0,
    NONSIDEREAL = 1
}

enum Direction {
    NORTH = 0,
    SOUTH = 1,
    EAST = 2,
    WEST = 3
}

struct FitsInfoType {
       1: string key;
       2: string val;
       3: string comment;
}

struct Location {
    1: double latitude;
    2: double longitude;
    3: double elevation;
}

struct TelescopeStatus {
    1: bool initialized;
    2: bool tracking;
    3: bool slewing;
    4: bool parking;
    5: bool parked;
    6: bool pointing_east;
    7: bool manual;
    8: bool communication_fault;
    9: bool limit_switch_primary_plus;
    10: bool limit_switch_primary_minus;
    11: bool limit_switch_secondary_plus;
    12: bool limit_switch_secondary_minus;
    13: bool homing_switch_primary_axis;
    14: bool homing_switch_secondary_axis;
    15: bool goto_commanded_rotator_position;
    16: bool non_sidereal_tracking;
    17: double ra;
    18: double dec;
    19: double alt;
    20: double az;
    21: double secondary_axis_angle;
    22: double primary_axis_angle;
    23: double sidereal_time;
    24: double julian_date;
    25: double time;
    26: double airmass;
    27: Location location;
}

struct DomeStatus {
    1: bool shutter_closed;
    2: bool shutter_opened;
    3: bool shutter_motor_on;
    4: bool shutter_fault;
    5: bool moving;
    6: bool rotation_fault;
    7: bool lights_on;
    8: bool lights_manual;
    9: bool shutter_opening_manually;
    10: bool shutter_closing_manually;
    11: bool moving_right_manually;
    12: bool moving_left_manually;
    13: bool shutter_power;
    14: bool power;

    15: bool remote;
    16: bool is_raining;
    17: bool tcs_locked_out;
    18: bool emergency_stop;
    19: bool shutter_closed_rain;
    20: bool power_failure;
    21: bool shutter_closed_power_failure;
    22: bool watchdog_tripped;

    23: bool no_comms_with_servocontroller;
    24: bool no_comms_with_sitechexe;
    25: bool no_comms_with_plc;
    26: bool at_position_setpoint;
    27: bool following;
    28: bool driver_moving;
    29: bool shutter_moving;
    30: bool slew_lights_on;
    31: bool dome_lights_on;

    32: double position;
}

struct FocuserStatus {
    1: bool autofocus;
    2: bool comms_fault_servocommunicator;
    3: bool comms_fault_servocontroller;
    4: bool secondary_mirror_at_limit;
    5: bool secondary_mirror_auto;
    6: bool secondary_mirror_moving;
    7: double secondary_mirror_position;
    8: double tertiary_mirror_angle;
    9: bool tertiary_mirror_auto;
    10: bool tertiary_mirror_moving;
    11: bool tertiary_mirror_left_fork_position;
    12: bool tertiary_mirror_right_fork_position;
}

struct RotatorStatus {
    1: bool moving;
    2: bool tracking;
    3: bool auto;
    4: bool at_limit;
    5: bool comms_fault_servocommunicator;
    6: bool comms_fault_servocontroller;
    7: bool moving_by_buttons;

    8: double limit_min;
    9: double limit_max;

    10: double angle;
    11: double parallactic_angle
    12: double parallactic_rate;
}

struct Status {
    1: State state;
    2: bool stopped;

    3: double julian_date;
    4: double airmass;
    5: double zenith_distance;

    6: map<Instrument,RotatorStatus> rotators;

    7: bool covers_moving;
    8: bool covers_open;

    9: double dome_angle;
    10: bool dome_remote;
    11: bool dome_shutter_moving;
    12: bool dome_shutter_open;
    13: bool dome_tracking;
    14: bool dome_moving;
    15: bool dome_tcs_lockout;

    16: double focus;
    17: bool secondary_mirror_auto;
    18: bool secondary_mirror_moving;

    19: double tertiary_mirror_angle;
    20: bool tertiary_mirror_auto;
    21: bool tertiary_mirror_moving;
    22: Instrument instrument;

    23: bool slew_lights_on;
    24: bool dome_lights_on;

    25: bool telescope_auto;
    26: bool telescope_parked;
    27: double telescope_alt;
    28: double telescope_az;
    29: double telescope_ra;
    30: double telescope_dec;
    31: bool telescope_slewing;
    32: bool telescope_tracking;
    33: TrackType telescope_tracking_type;
    34: Location location;
}

service Service {

    /**
    * Get the current status.
    */
    Status get_status() throws (1: LesediException e);

    /**
    * Emergency stop.
    */
    void stop();

    /**
    * Reset the stopped state.
    */
    void reset();

    /**
    * Execute the startup procedure.
    */
    void startup() throws (1: LesediException e);

    /**
    * Execute the shutdown procedure.
    */
    void shutdown() throws (1: LesediException e);

    /**
    * Put the telescope motors in auto or manual mode.
    */
    void auto_on() throws(1: LesediException e);
    void auto_off() throws(1: LesediException e);

    /**
    * Park or unpark the telescope.
    */
    void park() throws (1: LesediException e);
    void unpark() throws (1: LesediException e);

    /**
    * Go to the given altitude and azimuth.
    */
    void goto_altaz(1: double alt, 2: double az) throws (1: LesediException e);

    /**
    * Go to the given right ascension and declination.
    */
    void goto_radec(1: double ra, 2: double dec) throws (1: LesediException e);

    /**
    * Turn tracking on or off and enable or disable non-sidereal tracking.
    */
    void set_tracking_mode(1: i32 tracking, 2: i32 track_type, 3: double ra_rate, 4: double dec_rate) throws (1: LesediException e);

    /**
    * Abort any slews, and stop the telescope from tracking.
    */
    void abort() throws (1: LesediException e);

    /**
    * Move the telescope by the given arc seconds in the given direction.
    */
    void move(1: Direction direction, 2: double arcsec) throws (1: LesediException e);

    /**
    * Change to the given instrument port.
    */
    void select_instrument(1: Instrument instrument) throws (1: LesediException e);

    /**
    * Enable or disable dome remote mode.
    */
    void dome_remote_enable() throws (1: LesediException e);
    void dome_remote_disable() throws (1: LesediException e);

    /**
    * Rotate the dome to the park position.
    */
    void park_dome() throws (1: LesediException e);

    /**
    * Stop dome shutters and rotation.
    */
    void stop_dome() throws(1: LesediException e);

    /**
    * Make the dome follow or unfollow the telescope.
    */
    void dome_follow_telescope_start() throws (1: LesediException e);
    void dome_follow_telescope_stop() throws (1: LesediException e);

    /**
    * Manually rotate the dome to the given azimuth.
    */
    void rotate_dome(1: double az) throws (1: LesediException e);

    /**
    * Open or close the dome shutter.
    */
    void open_dome() throws (1: LesediException e);
    void close_dome() throws (1: LesediException e);

    /**
    * Turn the dome's fluorescent lights on or off.
    */
    void dome_lights_on() throws (1: LesediException e);
    void dome_lights_off() throws (1: LesediException e);

    /**
    * Turn the slew lights on or off.
    */
    void slew_lights_on() throws (1: LesediException e);
    void slew_lights_off() throws (1: LesediException e);

    /**
    * Open the baffle then mirror covers.
    */
    void open_covers() throws (1: LesediException e);

    /**
    * Close the mirror then baffle covers.
    */
    void close_covers() throws (1: LesediException e);

    /**
    * Turn rotator tracking on or off.
    */
    void rotator_tracking_on() throws (1: LesediException e);
    void rotator_tracking_off() throws (1: LesediException e);

    /**
    * Put rotator motors in auto or manual mode.
    */
    void rotator_auto_on() throws (1: LesediException e);
    void rotator_auto_off() throws (1: LesediException e);

    /*
    * Stop secondary mirror movement.
    */
    void secondary_mirror_stop() throws (1: LesediException e);

    /**
    * Put the secondary mirror motors in auto or manual mode.
    */
    void secondary_mirror_auto_on() throws (1: LesediException e);
    void secondary_mirror_auto_off() throws (1: LesediException e);

    /**
    * Put the tertiary mirror motors in auto or manual mode.
    */
    void tertiary_mirror_auto_on() throws (1: LesediException e);
    void tertiary_mirror_auto_off() throws (1: LesediException e);

    /**
    * Set the focus by moving the secondary mirror to the given position.
    */
    void set_focus(1: double inches) throws(1: LesediException e);
}
