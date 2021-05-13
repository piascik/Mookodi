Starting with version 0.92e of SiTechExe, there is a TCP/IP interface to
SiTechExe.
First you set up the port number in the
SiTechExe/Config/ChangeConfig/Misc.
It's labeled "Indi Port Number"
Note, if something like INDI is connected when you change this, there will
likely be an exception in the external app.
Best to disconnect all TCP processes before changing this SiTechExe config
item.

You need to be running SiTechExe before connecting with your software.
Basically, you send an ASCII string as a command, SiTechExe possibly does
something, and returns a string.
Nothing blocks in SiTechExe, the string is returned right away, even if the
command will take a while to complete.

Unless otherwise stated, every command returns the standard return string.
The command "ReadScopeStatus" returns "_" as the message.
Other commands return a message after the "_";

Parameters are separated by a ';' semi-colon.
As of version 0.92e, here is the standard return string description.
The type (int, double, or string) is a literal string, your software must
convert these strings to an int or double if necessary.
int boolParms (Slewing, Tracking, Initialized, etc.)
double RightAsc (Hours, JNow)
double Declination (Degs, JNow)
double ScopeAlititude (Degs)
double ScopeAzimuth (Degs)
double Secondary Axis Angle (Degs) (if a RotatorComms command, this will be
the ParallacticAngle) (if this is a ReadScopeDestination command, it is the
destination RA)
double Primary Axis Angle (Degs) (if a RotatorComms command, this will be
the ParallacticRate) (if this is a ReadScopeDestination command, it is the
destination Dec)
double ScopeSidereal Time (Hours) (if a RotatorComms command, this will be
the CameraSolvedAngle) (if this is a ReadScopeDestination command, it is
the destination ScopeAltitude)
double Scope Julian Day  (if a RotatorComms command, this will be the
Commanded Rotator GoTo position) (if this is a ReadScopeDestination
command, it is the destination ScopeAzimuth)
double Scope Time( Hours)
double AirMass
String "_" followed by message.

The boolParms have bits in it that mean certain things as follows:
Bit 00 (AND with    1) Scope Is Initialized
Bit 01 (AND with    2) Scope Is Tracking (remains true when slewing)
Bit 02 (AND with    4) Scope is Slewing
Bit 03 (AND with    8) Scope is Parking
Bit 04 (AND with   16) Scope is Parked
Bit 05 (AND with   32) Scope is "Looking East" (GEM mount);
Bit 06 (AND with   64) ServoController is in "Blinky" (Manual) mode, one or
both axis's
Bit 07 (AND with  128) There is a communication fault between SiTechExe and
the ServoController
Bit 08 (AND with  256) Limit Swith is activated (Primary Plus) (ServoII and
Brushless)
Bit 09 (AND with  512) Limit Swith is activated (Primary Minus) (ServoII
and Brushless)
Bit 10 (AND with 1024) Limit Swith is activated (Secondary Plus) (ServoII
and Brushless)
Bit 11 (AND with 2048) Limit Swith is activated (Secondary Minus) (ServoII
and Brushless)
Bit 12 (AND with 4096) Homing Switch Primary Axis is activated
Bit 13 (AND with 8192) Homing Switch Secondary Axis is activated
Bit 14 (AND with 16384) GoTo Commanded Rotator Position (if this is a
rotator response)
Bit 15 (AND with 32768) Tracking at Offset Rate of some kind (non-sidereal)

COMMANDS:
"ReadScopeStatus\n"
Returns the above string, no other action taken

"ReadScopeDestination\n"
Returns the above string, but the axis angles secondary, primary, sidereal
time,
julian day are RA, Dec, Alt, Az, are the final destination
of the telescope. No other action taken. This is for the dome control
program.
Secondary Axis Angle (Degs) (if a ReadScopeDestination command, this will
be the RA)
Primary Axis Angle (Degs) (if a ReadScopeDestination command, this will be
the Dec)
ScopeSidereal Time (Hours) (if a ReadScopeDestination command, this will be
the Altitude)
Scope Julian Day  (if a ReadScopeDestination command, this will be the
Azimuth)


"RotatorComms IsMoving RotatorAngle\n"
IsMoving is 0 or 1.  RotatorAngle is the current rotator angle in degs.
Following is the difference from the standard response.
Returns the above string in RotatorComms mode, The string on the end is
"RotatorComms"
Secondary Axis Angle (Degs) (if a RotatorComms command, this will be the
ParallacticAngle)
Primary Axis Angle (Degs) (if a RotatorComms command, this will be the
ParallacticRate)
ScopeSidereal Time (Hours) (if a RotatorComms command, this will be the
CameraSolvedAngle)
Scope Julian Day  (if a RotatorComms command, this will be the Commanded
Rotator GoTo position)


"SiteLocations\n"
returns a string separated by ';'
siteLatitude (deg's);
siteLongitude (deg's);
siteElevation (meters);
_SiteLocations

"GoToAltAz Az Alt\n"
Az and Alt are a double number, converted to a string.  Example:
"GoToAltAz 180.0 80.0\n"
This will move the scope azimuth to the south, and the altitude to 80
deg's.  0 azimuth is north, and 90 azimuth is east.
If scope is parked, if in blinky mode, if alt/az is out of parameters, or
if below horizon limit, an error will be returned after the "_".

"GoTo RA Dec <J2K>\n"
RA and Dec is a double number, converted to a string. Example:
"GoTo 12.0 45.0\n"
This will move the scope to 12 hours of right ascension and +45 deg's in
declination, JNow.

"GoTo 12.0 45.0 J2K\n"
This will move the scope to 12 hours of right ascension and +45 deg's in
declination, J2000.
If the coordinates are in J2000, then put an optional " J2K" before the
\n.  SiTech will then perform precession, Nutation, and Aberration
corrections.
If scope is parked, if in blinky mode, if RA/Dec is out of parameters, or
if below horizon limit, an error will be returned after the "_".

"CookCoordinates 12.0 45.0"
The RA and Dec should be in J2000 epoch.
This will return the standard return string, and after the "_" will return
the JNow coordinates, with Precession, Nutation, and Aberration applied.

"UnCookCoordinates 12.0 45.0"
The RA and Dec should be in JNow coordinates, with Precession, Nutation,
and Aberration applied.
This will return the standard return string, and after the "_" will return
the J2000 coordinates.

"SyncToAltAz Az Alt\n"
Az and Alt are a double number, converted to a string.  Example:
"SyncToAltAz 180.0 80.0\n"
This will tell sitechExe that the scope is pointed in azimuth to the south,
and the altitude to 80 deg's.  0 azimuth is north, and 90 azimuth is east.
If scope is parked, if in blinky mode, if alt/az is out of parameters, or
if below horizon limit, an error will be returned after the "_".
If successful, SiTechExe will believe that the scope is pointed to 180 az
and 80 elevation.

"Sync 12.0 45.0 <n> <J2K>\n"
RA and Dec is a double number, converted to a string.
The <n> is optional, and is the type of sync.
0 = use SiTechExe standard Init Window.
1 = perform an instant Offset init (no init window comes up)
2 = perform an instant "load calibration" init (no init window comes up).
Example:
"Sync 12.0 45.0\n"
If successfu, this will tell sitechExe that the scope is pointed in RA to
12 hours, and the declination to plus 45 deg's, JNow.
SiTechExe will also use the default SiTechExe Init window.

"Sync 12.0 45.0 J2K\n"
This will tell sitechExe that the scope is pointed in RA to 12 hours, and
the declination to plus 45 deg's, J2000.
SiTechExe will also use the default SiTechExe Init window.

"Sync 12.0 45.0 2 J2K\n"
This will tell sitechExe that the scope is pointed in RA to 12 hours, and
the declination to plus 45 deg's, J2000.
SiTechExe will perform an add calibration point.

If scope is parked, if in blinky mode, if alt/az is out of parameters, or
if below horizon limit, an error will be returned after the "_".


"Park\n"
If all is well (no blinky, and a park position has been specified), the
mount will move to the park position, and then will officially be in the
"parked" status.

"UnPark\n"
If the mount is officially parked, this will "unpark" the mount.  It will
then be ready for goto's.

"Abort\n"
This will abort any slews, and stop the scope from tracking as well.

"SetTrackMode 1 1 0.0 0.0\n"
This will start tracking at the sidereal rate, or any other rate, or stop
tracking altogether.
There are 4 parameters.
1. bool 1 equals track.  Any other value will  stop tracking.
2. 1 = use the following rates.  0 means track at the sidereal rate.
3. Right Ascension Rate (arc seconds per second).  A 0.0 will  track at the
sidereal rate.
4. Declination rate (arc seconds per second) A 0.0 will track the
declination according to the telescope model and refraction.
Example:
"SetTrackMode 1 0 0.0 0.0\n" will start the mount tracking at the sidereal
rate.
"SetTrackMode 0 0 0.0 0.0\n" will stop the mount from tracking.


"PulseGuide Direction, Milliseconds\n"
This will nudge the mount in the direction specified, for the number of
Milliseconds specified, at the Guide Rate stored in the servo controller
configuration.
Direction:
 0=North, 1=South, 2=East, 3 = West
So the way that SiTechExe performs this, is it does some math on the
current guide rate (stored in the controller configuration)
and figures how far the mount will move if guided at that rate for the time
specified.
It then changes the RA and Dec setpoint by that amount.

Example:
"PulseGuide 0 1000\n"
This will nudge the mount north by 5 arc seconds if the guide rate is set
for 5 arc seconds per second.

"MotorsToBlinky\n"
This will force the motors to blinky mode (Manual Mode), in effect removing
all power from the motors.  They will coast to a stop.

"MotorsToAuto\n"
This will put the motors back into Auto Mode.

"CookCoordinates 12 45"
This command will return the JNow coordinates after the "_" in the return
string.
They will be adjusted for Precession, Nutation, and Aberration.

"UnCookCoordinates 12 45"
This command will return the J2000 coordinates after the "_" in the return
string.
They will be adjusted for Precession, Nutation, and Aberration, from JNow
to J2000.

"JogArcSeconds N 5.0"
Self explanatory, N, S, E, or W,