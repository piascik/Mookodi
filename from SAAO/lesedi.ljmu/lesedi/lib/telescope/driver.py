import argparse
import ctypes
import logging
import socket
import sys

from astropy import units as u
from astropy.coordinates import Angle, EarthLocation, SkyCoord, AltAz
from astropy.time import Time
from lesedi.sdk import ttypes
from lesedi.lib import driver


logger = logging.getLogger(__name__)


class Status(driver.Bitfield):
    """Represents the current telescope status"""

    _anonymous_ = ('fields',)

    class Fields(ctypes.LittleEndianStructure):
        _fields_ = [
            ('initialized', ctypes.c_uint8, 1),
            ('tracking', ctypes.c_uint8, 1),
            ('slewing', ctypes.c_uint8, 1),
            ('parking', ctypes.c_uint8, 1),
            ('parked', ctypes.c_uint8, 1),
            ('pointing_east', ctypes.c_uint8, 1),  # GEM mount
            ('manual', ctypes.c_uint8, 1),
            ('communication_fault', ctypes.c_uint8, 1),
            ('limit_switch_primary_plus', ctypes.c_uint8, 1),
            ('limit_switch_primary_minus', ctypes.c_uint8, 1),
            ('limit_switch_secondary_plus', ctypes.c_uint8, 1),
            ('limit_switch_secondary_minus', ctypes.c_uint8, 1),
            ('homing_switch_primary_axis', ctypes.c_uint8, 1),
            ('homing_switch_secondary_axis', ctypes.c_uint8, 1),
            ('goto_commanded_rotator_position', ctypes.c_uint8, 1),
            ('non_sidereal_tracking', ctypes.c_uint8, 1),
        ]
    _fields_ = [
        ('fields', Fields),
        ('value', ctypes.c_uint16),
    ]


class StatusResponse(driver.BaseResponse):
    """A telescope status response message

    The response contains the following:

        - Status bit field
        - Right ascension (hours, JNow)
        - Declination (degrees, JNow)
        - Alititude (degrees)
        - Azimuth (degrees)
        - Secondary axis angle (degrees)
        - Primary axis angle (degrees)
        - Sidereal time (hours)
        - Julian date
        - Time (hours)
        - Airmass

    """

    def __init__(self, response):
        (status, right_ascension, declination, altitude, azimuth,
            secondary_axis_angle, primary_axis_angle, sidereal_time,
            julian_date, time, airmass) = self.parse(response)

        self.status = Status(status)
        self.ra = float(right_ascension)
        self.dec = float(declination)
        self.alt = float(altitude)
        self.az = float(azimuth)
        self.secondary_axis_angle = float(secondary_axis_angle)
        self.primary_axis_angle = float(primary_axis_angle)
        self.sidereal_time = float(sidereal_time)
        self.julian_date = float(julian_date)
        self.time = float(time)
        self.airmass = float(airmass)


class RotatorCommsResponse(driver.BaseResponse):
    """A telescope rotator comms response message

    The response contains the following:

        - Status bit field
        - Right ascension (hours, JNow)
        - Declination (degrees, JNow)
        - Alititude (degrees)
        - Azimuth (degrees)
        - Parallactic angle (degrees)
        - Parallactic rate
        - Camera solved angle (degrees)
        - Goto position
        - Time (hours)

    """

    def __init__(self, response):
        (status, right_ascension, declination, altitude, azimuth,
            parallactic_angle, parallactic_rate, camera_solved_angle,
            goto_position, time) = self.parse(response)

        self.status = TelescopeStatus(status)
        self.right_ascension = float(right_ascension)
        self.declination = float(declination)
        self.altitude = float(altitude)
        self.azimuth = float(azimuth)
        self.parallactic_angle = float(parallactic_angle)
        self.parallactic_rate = float(parallactic_rate)
        self.camera_solved_angle = float(camera_solved_angle)
        self.goto_position = float(goto_position)
        self.time = float(time)


class LocationResponse(driver.BaseResponse):
    """A telescope location response message

    The response contains the following:

        - Latitude (degrees)
        - Longitude (degrees)
        - Elevation (meters)

    """

    def __init__(self, response):
        latitude, longitude, elevation = self.parse(response)

        self.latitude = float(latitude)
        self.longitude = float(longitude)
        self.elevation = float(elevation)


class Driver(driver.BaseDriver):
    """Driver that communicates with the telescope subsystem

    Docstrings for each method have been copied verbatim from documentation
    provided by the developer at SiderealTechnology.
    """

    PORT = 1956

    def get_status(self):
        response = StatusResponse(
            self.execute('ReadScopeStatus'))

        location = self.get_location()

        return ttypes.TelescopeStatus(
            initialized=bool(response.status.initialized),
            tracking=bool(response.status.tracking),
            slewing=bool(response.status.slewing),
            parking=bool(response.status.parking),
            parked=bool(response.status.parked),
            pointing_east=bool(response.status.pointing_east),
            manual=bool(response.status.manual),
            communication_fault=bool(response.status.communication_fault),
            limit_switch_primary_plus=bool(response.status.limit_switch_primary_plus),
            limit_switch_primary_minus=bool(response.status.limit_switch_primary_minus),
            limit_switch_secondary_plus=bool(response.status.limit_switch_secondary_plus),
            limit_switch_secondary_minus=bool(response.status.limit_switch_secondary_minus),
            homing_switch_primary_axis=bool(response.status.homing_switch_primary_axis),
            homing_switch_secondary_axis=bool(response.status.homing_switch_secondary_axis),
            goto_commanded_rotator_position=bool(response.status.goto_commanded_rotator_position),
            non_sidereal_tracking=bool(response.status.non_sidereal_tracking),
            ra=response.ra,
            dec=response.dec,
            alt=response.alt,
            az=response.az,
            primary_axis_angle=response.primary_axis_angle,
            secondary_axis_angle=response.secondary_axis_angle,
            sidereal_time=response.sidereal_time,
            julian_date=response.julian_date,
            time=response.time,
            airmass=response.airmass,
            location=location,
        )

    def get_destination(self):
        return self.execute('ReadScopeDestination')

    def park(self):
        """If all is well (no blinky, and a park position has been specified),
        the mount will move to the park position, and then will officially be
        in the "parked" status.
        """

        return self.execute('Park')

    def unpark(self):
        """If the mount is officially parked, this will "unpark" the mount. It
        will then be ready for goto's.
        """

        return self.execute('UnPark')

    def abort(self):
        """This will abort any slews, and stop the scope from tracking as well."""

        return self.execute('Abort')

    def goto_altaz(self, alt, az):
        """Move telescope to given altitude and azimuth.

        Example:

        GoToAltAz 180.0 80.0<newline>

        This will move the scope azimuth to the south, and the altitude to 80
        degs 0 azimuth is north, and 90 azimuth is east. If scope is parked, if
        in blinky mode, if alt/az is out of parameters, or if below horizon
        limit, an error will be returned after the "_".
        """

        return self.execute('GoToAltAz {} {}'.format(az, alt))

    def goto_radec(self, ra, dec):
        """Move telescope to given right ascension and declination.

        Example:

        GoTo 12.0 45.0 J2K<newline>

        This will move the scope to 12 hours of right ascension and +45 deg's
        in declination, J2000. If the coordinates are in J2000, then put an
        optional "J2K" before the newline character. SiTech will then perform
        precession, Nutation, and Aberration corrections. If scope is parked,
        if in blinky mode, if RA/Dec is out of parameters, or if below horizon
        limit, an error will be returned after the "_".
        """

        return self.execute('GoTo {} {} J2K'.format(ra, dec))

    def set_tracking_mode(self, tracking, track_type, ra_rate, dec_rate):
        """This will start tracking at the sidereal rate, or any other rate, or
        stop tracking altogether.

        There are 4 parameters.

        1. bool 1 equals track. Any other value will stop tracking.
        2. 1 = use the following rates. 0 means track at the sidereal rate.
        3. Right Ascension Rate (arc seconds per second). A 0.0 will track at
        the sidereal rate.
        4. Declination rate (arc seconds per second) A 0.0 will track the
        declination according to the telescope model and refraction.

        Example:

        SetTrackMode 1 0 0.0 0.0<newline> start the mount tracking at the sidereal rate.
        SetTrackMode 0 0 0.0 0.0<newline> stop the mount from tracking.
        """

        return self.execute('SetTrackMode {} {} {} {}'.format(
            tracking, track_type, ra_rate, dec_rate))

    def set_motors_to_manual(self):
        """This will force the motors to blinky mode (Manual Mode), in effect
        removing all power from the motors. They will coast to a stop.
        """

        return self.execute('MotorsToBlinky')

    def set_motors_to_auto(self):
        """This will put the motors back into Auto Mode."""

        return self.execute('MotorsToAuto')

    def sync_to_altaz(self, alt, az):
        """Example:

        SyncToAltAz 180.0 80.0<newline>

        This will tell sitechExe that the scope is pointed in azimuth to the
        south, and the altitude to 80 deg's. 0 azimuth is north, and 90 azimuth
        is east.

        If scope is parked, if in blinky mode, if alt/az is out of parameters,
        or if below horizon limit, an error will be returned after the "_".

        If successful, SiTechExe will believe that the scope is pointed to 180
        az and 80 elevation.
        """

        return self.execute('SyncToAltAz {} {}'.format(alt, az))

    def sync_to_radec(self, ra, dec, type_=0, equinox='J2000'):
        """The <n> is optional, and is the type of sync.

        0 = use SiTechExe standard Init Window.
        1 = perform an instant Offset init (no init window comes up)
        2 = perform an instant "load calibration" init (no init window comes up).

        Example:

        Sync 12.0 45.0<newline>

        If successful, this will tell sitechExe that the scope is pointed in RA
        to 12 hours, and the declination to plus 45 deg's, JNow. SiTechExe will
        also use the default SiTechExe Init window.
        """

        return self.execute('Sync {} {} {} {}'.format(ra, dec, type_, equinox))

    def get_location(self):
        """Returns the site latitude, longitude and elevation."""

        location = LocationResponse(
            self.execute('SiteLocations'))

        return ttypes.Location(
            location.latitude,
            location.longitude,
            location.elevation
        )

    def cook_coordinates(self, ra, dec):
        """Returns the JNow coordinates after the "_" in the return string.
        They will be adjusted for Precession, Nutation, and Aberration.
        """

        return self.execute('CookCoordinates {} {}'.format(ra, dec))

    def uncook_coordinates(self, ra, dec):
        """Returns the J2000 coordinates after the "_" in the return string.
        They will be adjusted for Precession, Nutation, and Aberration, from
        JNow to J2000.
        """

        return self.execute('UnCookCoordinates {} {}'.format(ra, dec))

    def pulse_guide(self, direction, milliseconds):
        """This will nudge the mount in the direction specified, for the number
        of Milliseconds specified, at the Guide Rate stored in the servo
        controller configuration.

        Direction:

            0=North, 1=South, 2=East, 3=West

        So the way that SiTechExe performs this, is it does some math on the
        current guide rate (stored in the controller configuration) and figures
        how far the mount will move if guided at that rate for the time
        specified.

        It then changes the RA and Dec setpoint by that amount.
        """

        return self.execute('PulseGuide {} {}'.format(direction, milliseconds))

    def jog(self, direction, arcseconds):
        """Self explanatory, N, S, E, or W,"""

        return self.execute('JogArcSeconds {} {}'.format(direction, arcseconds))


class MockDriver(object):
    """Driver implementation that presents a mocked interface."""

    STATE = {
        'status': Status(),
        'ra': 0.0,
        'dec': 0.0,
        'equinox': 'J2000',
        'alt': 45.0,
        'az': 0.0,
        'destination': {
            'ra': 0.0,
            'dec': 0.0,
            'equinox': 'J2000',
            'alt': 4.0,
            'az': 0.0,
        },
        'primary_axis_angle': 0,
        'secondary_axis_angle': 0,
        'time': 0,
        'tracking': {
            'enabled': 0,
            'ra_rate': 0,  # arcsec/sec
            'dec_rate': 0,  # arcsec/sec
        },
        'instrument': ttypes.Instrument.WINCAM,
    }

    def __init__(self):
        self.location = EarthLocation(lat=-32.3798, lon=20.8107, height=1822)

    def execute(self, command):
        t = Time(Time.now(), location=self.location)

        altaz = SkyCoord(self.STATE['az'], self.STATE['alt'], unit=(u.hour, u.deg),
            frame=AltAz(obstime=t, location=self.location))

        # Apply state changes based on the command.
        if command == 'Abort':
            self.STATE['status'].slewing = 0
            self.STATE['status'].tracking = 0
        elif command == 'Park':
            self.STATE['alt'] = 50
            self.STATE['az'] = 90
            self.STATE['status'].parked = 1
        elif command == 'UnPark':
            self.STATE['status'].parked = 0
        elif command.startswith('GoToAltAz'):
            command, az, alt = command.split(' ')

            altaz = SkyCoord(az, alt, unit=u.deg, frame=AltAz(
                obstime=t, location=self.location))

            self.STATE['destination']['alt'] = altaz.alt.deg
            self.STATE['destination']['az'] = altaz.az.deg

            # Slew to the given alt/az, updating current alt and az as we go.
            # TODO: threading.Thread(target=self._slew_altaz).start()
            self.STATE['alt'] = altaz.alt.deg
            self.STATE['az'] = altaz.az.deg
        elif command.startswith('GoTo'):
            command, ra, dec, equinox = command.split(' ')

            radec = SkyCoord(ra, dec, unit=(u.hour, u.deg), equinox='J2000', obstime=t,
                location=self.location)

            self.STATE['destination']['ra'] = radec.ra.hour
            self.STATE['destination']['dec'] = radec.dec.deg
            self.STATE['destination']['equinox'] = str(equinox)
            self.STATE['destination']['alt'] = radec.altaz.alt.deg
            self.STATE['destination']['az'] = radec.altaz.az.deg

            # Slew to the given ra/dec, updating current ra and dec as we go.
            # TODO: threading.Thread(target=self._slew_radec).start()
            self.STATE['ra'] = radec.ra.hour
            self.STATE['dec'] = radec.dec.deg

            self.STATE['status'].tracking = 1
        elif command.startswith('SetTrackMode'):
            command, tracking, track_type, ra_rate, dec_rate = command.split(' ')

            self.STATE['status'].tracking = int(tracking)
            self.STATE['status'].non_sidereal_tracking = int(track_type)

            self.STATE.update({
                'tracking': {
                    'enabled': int(tracking),
                    'ra_rate': float(ra_rate),
                    'dec_rate': float(dec_rate),
                },
            })
        elif command.startswith('SyncToAltAz'):
            command, az, alt = command.split(' ')

            self.STATE['alt'] = float(alt)
            self.STATE['az'] = float(az)
        elif command.startswith('Sync'):
            command, ra, dec, type_, equinox = command.split(' ')

            self.STATE['ra'] = float(ra)
            self.STATE['dec'] = float(dec)
            self.STATE['equinox'] = equinox
        elif command.startswith('PulseGuide'):
            pass  # TODO
        elif command == 'MotorsToBlinky':
            self.STATE['status'].manual = 1
        elif command == 'MotorsToAuto':
            self.STATE['status'].manual = 0
        elif command.startswith('CookCoordinates'):
            pass  # TODO
        elif command.startswith('UnCookCoordinates'):
            pass  # TODO
        elif command.startswith('JogArcSeconds'):
            command, direction, arcsec = command.split(' ')

            arcsec = float(arcsec) * u.arcsec

            if direction == 'N':
                self.STATE['dec'] = (Angle(self.STATE['dec'] * u.deg) + arcsec).value
            elif direction == 'S':
                self.STATE['dec'] = (Angle(self.STATE['dec'] * u.deg) - arcsec).value
            elif direction == 'E':
                self.STATE['ra'] = (Angle(self.STATE['ra'] * u.hour) + arcsec).value
            elif direction == 'W':
                self.STATE['ra'] = (Angle(self.STATE['ra'] * u.hour) - arcsec).value

        # Construct a response based on the issued command.
        if command == 'ReadScopeDestination':
            response = '{};{};{};{};{};{};{};{};{};{};{};_'.format(
                self.STATE['status'].value,
                self.STATE['destination']['ra'],
                self.STATE['destination']['dec'],
                self.STATE['destination']['alt'],
                self.STATE['destination']['az'],
                self.STATE['secondary_axis_angle'],
                self.STATE['primary_axis_angle'],
                t.sidereal_time(kind='apparent').hour,
                t.jd,
                self.STATE['time'],
                altaz.secz,
            )
        elif command == 'RotatorComms':
            response = '{};{};{};{};{};{};{};{};{};{};{};_'.format(
                self.STATE['status'].value,
                self.STATE['ra'],
                self.STATE['dec'],
                self.STATE['alt'],
                self.STATE['az'],
                self.STATE['derotator'][self.STATE['instrument']]['parallactic_angle'],
                self.STATE['derotator'][self.STATE['instrument']]['parallactic_rate'],
                self.STATE['derotator'][self.STATE['instrument']]['camera_solved_angle'],
                self.STATE['derotator'][self.STATE['instrument']]['destination_position'],
                self.STATE['time'],
                altaz.secz,
            )
        elif command == 'SiteLocations':
            response = '{};{};{};_SiteLocations'.format(
                self.location.lat.value,
                self.location.lon.value,
                self.location.height.value
            )
        else:
            response = '{};{};{};{};{};{};{};{};{};{};{};_'.format(
                self.STATE['status'].value,
                self.STATE['ra'],
                self.STATE['dec'],
                self.STATE['alt'],
                self.STATE['az'],
                self.STATE['secondary_axis_angle'],
                self.STATE['primary_axis_angle'],
                t.sidereal_time(kind='apparent').hour,
                t.jd,
                self.STATE['time'],
                altaz.secz,
            )

        return response + Driver.TERMINATOR


def main():
    """Launches a TCP server that responds to telescope commands."""

    parser = argparse.ArgumentParser(prog='lesedi-telescope')
    parser.add_argument('--port', type=int, default=Driver.PORT,
        help='the TCP port to listen on')

    arguments = parser.parse_args()

    logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('127.0.0.1', arguments.port))
    s.listen(1)

    driver = MockDriver()

    try:
        # Wait for client connections.
        while 1:
            conn, addr = s.accept()

            logger.info(addr)

            # Execute incoming commands from client.
            while 1:
                command = conn.recv(1024).decode().strip()

                if not command: break

                logger.info(repr(command))

                response = driver.execute(command)

                logger.info(repr(response))

                if response is not None:
                    conn.sendall(response.encode())

            conn.close()

    except KeyboardInterrupt:
        s.close()


if __name__ == '__main__':
    main()
