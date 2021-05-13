"""Lesedi server."""

import argparse
import concurrent.futures
import configparser
import logging
import socket
import sys
import threading

from thrift.protocol import TBinaryProtocol
from thrift.server import TServer
from thrift.transport import TSocket, TTransport
from lesedi.sdk import Service, constants, ttypes
from lesedi.lib import __version__, aux, dome, focuser, rotator, telescope


def safety_check(f):
    """Prevents commands from being issued when the lockout is engaged in the
    dome or when the emergency stop has been triggered.

    NOTE: This is an important safety feature. All commands that aren't simply
    reading information from a subsystem should use this decorator to make sure
    nothing moves while the lockout is engaged or in the case of an emergency
    stop.

    This does not guarantee safety for technicians within the dome since
    commands can still be issued directly to the services below the Thrift
    layer where there are no safety checks besides the physical dome lockout
    which only affects the dome and none of the other subsystems.

    """

    def wrapper(self, *args, **kwargs):
        if self._stop.is_set():
            raise ttypes.LesediException(
                'Emergency stop has been triggered. Please reset to continue.')

        dome = self.dome.get_status()

        if dome.tcs_locked_out:
            raise ttypes.LesediException('Lockout engaged. Command not allowed.')

        return f(self, *args, **kwargs)
    return wrapper


class Handler(object):

    state = ttypes.State.OFF

    def __init__(self, config):
        self.config = config

        # The stop event signals to the startup and shutdown threads that the
        # emergency stop has been triggered.
        self._stop = threading.Event()

    @property
    def aux(self):
        if not hasattr(self, '_aux'):
            try:
                self._aux = aux.driver.Driver(host=self.config['aux']['host'])
            except Exception as e:
                # TODO: Handle more specific types of exceptions.
                raise ttypes.LesediException(str(e))
        return self._aux

    @property
    def dome(self):
        if not hasattr(self, '_dome'):
            try:
                self._dome = dome.driver.Driver(
                    host=self.config['dome']['host'])
            except socket.timeout as e:
                raise ttypes.LesediException('Connection timed out.')
            except socket.error as e:
                raise ttypes.LesediException(str(e))
        return self._dome

    @property
    def focuser(self):
        if not hasattr(self, '_focuser'):
            try:
                self._focuser = focuser.driver.Driver(
                    host=self.config['focuser']['host'])
            except socket.timeout as e:
                raise ttypes.LesediException('Connection timed out.')
            except socket.error as e:
                raise ttypes.LesediException(str(e))
        return self._focuser

    @property
    def rotator_left(self):
        if not hasattr(self, '_rotator_left'):
            try:
                self._rotator_left = rotator.driver.LeftRotatorDriver(
                    host=self.config['rotator']['host'])
            except socket.timeout as e:
                raise ttypes.LesediException('Connection timed out.')
            except socket.error as e:
                raise ttypes.LesediException(str(e))
        return self._rotator_left

    @property
    def rotator_right(self):
        if not hasattr(self, '_rotator_right'):
            try:
                self._rotator_right = rotator.driver.RightRotatorDriver(
                    host=self.config['rotator']['host'])
            except socket.timeout as e:
                raise ttypes.LesediException('Connection timed out.')
            except socket.error as e:
                raise ttypes.LesediException(str(e))
        return self._rotator_right

    @property
    def telescope(self):
        if not hasattr(self, '_telescope'):
            try:
                self._telescope = telescope.driver.Driver(
                    host=self.config['telescope']['host'])
            except socket.timeout as e:
                raise ttypes.LesediException('Connection timed out.')
            except socket.error as e:
                raise ttypes.LesediException(str(e))
        return self._telescope

    def get_status(self):
        dome = self.dome.get_status()

        telescope = self.telescope.get_status()

        if telescope.non_sidereal_tracking:
            telescope_tracking_type = ttypes.TrackType.NONSIDEREAL
        else:
            telescope_tracking_type = ttypes.TrackType.SIDEREAL

        focuser = self.focuser.get_status()

        if focuser.tertiary_mirror_left_fork_position:
            instrument = ttypes.Instrument.WINCAM
        elif focuser.tertiary_mirror_right_fork_position:
            instrument = ttypes.Instrument.SHOC
        else:
            # The tertiary mirror can be between one of the two positions in
            # which case no instrument is technically active/selected.
            instrument = None

        rotator_left = self.rotator_left.get_status()
        rotator_right = self.rotator_right.get_status()

        aux = self.aux.get_status()

        return ttypes.Status(
            stopped=self._stop.is_set(),
            state=self.state,
            airmass=telescope.airmass,
            julian_date=telescope.julian_date,
            focus=focuser.secondary_mirror_position,

            covers_moving=aux.moving,
            covers_open=(aux.mirror_cover1_open and aux.mirror_cover2_open and
                aux.baffle_open),

            dome_angle=dome.position,
            dome_remote=dome.remote,
            dome_tracking=dome.following,
            dome_shutter_moving=dome.shutter_moving,
            dome_shutter_open=(
                dome.shutter_opened and not dome.shutter_closed),
            dome_moving=dome.moving,
            dome_tcs_lockout=dome.tcs_locked_out,
            dome_lights_on=dome.dome_lights_on,
            slew_lights_on=dome.slew_lights_on,

            secondary_mirror_auto=focuser.secondary_mirror_auto,
            secondary_mirror_moving=focuser.secondary_mirror_moving,
            tertiary_mirror_auto=focuser.tertiary_mirror_auto,
            tertiary_mirror_moving=focuser.tertiary_mirror_moving,

            rotators={
                ttypes.Instrument.WINCAM: rotator_left,
                ttypes.Instrument.SHOC: rotator_right,
            },
            instrument=instrument,
            telescope_auto=(not telescope.manual),
            telescope_parked=telescope.parked,
            telescope_alt=telescope.alt,
            telescope_az=telescope.az,
            telescope_ra=telescope.ra,
            telescope_dec=telescope.dec,
            telescope_slewing=telescope.slewing,
            telescope_tracking=telescope.tracking,
            telescope_tracking_type=telescope_tracking_type,
            location=telescope.location,
        )

    def stop(self):
        self._stop.set()

        # Revert the state if we were in the middle of starting up or shutting
        # down.
        if self.state == ttypes.State.STARTUP:
            self.state = ttypes.State.OFF
        elif self.state == ttypes.State.SHUTDOWN:
            self.state = ttypes.State.READY

        # Stop all systems by calling relevant commands directly without
        # checking if the dome lock is on or checking if the stop event is set.
        self.telescope.abort()
        self.dome.emergency_stop()
        self.focuser.secondary_stop()

    def reset(self):
        self._stop.clear()

    def _startup(self):
        """Carry out the following steps in order:

        1. Set the dome to remote control
        2. Switch off fluorescent lights
        3. Switch on slew lights
        4. Open the dome shutters, then wait 55 seconds
        5. Open the baffle covers, then wait 15 seconds
        6. Open the mirror covers
        7. Set the dome to track the telescope (FollowScopeOn)
        8. Select instrument port 1
        9. Switch off the slew lights

        """

        if self.state not in (ttypes.State.OFF, ttypes.State.READY):
            return

        self.state = ttypes.State.STARTUP

        self.dome_remote_enable()
        self.dome_lights_off()

        # Turn on the slew lights so that we can see what's happening.
        self.slew_lights_on()

        # Open the dome and wait before opening the covers.
        self.open_dome()

        # Wait for a stop event or until the timeout occurs while the dome is
        # busy opening.
        self._stop.wait(55)

        self.open_covers()

        self.unpark()

        self.dome_follow_telescope_start()

        # Select default instrument.
        self.tertiary_mirror_auto_on()
        self.select_instrument(ttypes.Instrument.SHOC)

        # Turn off the slew lights when we're done.
        self.slew_lights_off()

        self.state = ttypes.State.READY

    @safety_check
    def startup(self):
        """Run the startup procedure."""

        def done(f: concurrent.futures.Future):
            # Revert the state if the future didn't complete successfully.
            if not f.done():
                self.state = ttypes.State.OFF

                error = f.exception()
                if error:
                    pass  # TODO: Do something with the exception.

        executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        executor.submit(self._startup).add_done_callback(done)

    def _shutdown(self):
        """"One button to carry out the following steps in order:

        1. Set the dome to remote control (it should already be in
        remote, but just as a precaution)
        2. Switch on slew lights
        3. Close the mirror covers, then wait 5 seconds
        4. Close the baffle covers, then wait 20 seconds
        5. Close the dome shutters, then wait 55 seconds
        6. Set the dome to stop tracking the telescope (FollowScopeOff)
        7. Park dome at az=270 degrees
        8. Park telescope at az=90, alt=50 degrees
        9. Park rotators
        10. Switch off the slew lights

        """

        if self.state not in (ttypes.State.OFF, ttypes.State.READY):
            return

        self.state = ttypes.State.SHUTDOWN

        self.dome_remote_enable()
        self.dome_lights_off()

        # Turn on the slew lights so that we can see what's happening.
        self.slew_lights_on()

        self.close_covers()

        self._stop.wait(20)

        # Close the dome and wait.
        self.close_dome()

        # Wait for a stop event or until the timeout occurs while the dome is
        # busy closing.
        self._stop.wait(55)

        self.dome_follow_telescope_stop()

        # Park the dome and then park the telescope.
        self.park_dome()
        self.park()

        self.rotator_left.tracking_off()
        self.rotator_left.park()

        self.rotator_right.tracking_off()
        self.rotator_right.park()

        # Turn off the slew lights when we're done.
        self.slew_lights_off()
        self.dome_lights_off()

        self.state = ttypes.State.OFF

    @safety_check
    def shutdown(self):
        """Run the shutdown procedure."""

        def done(f: concurrent.futures.Future):
            # Revert the state if the future didn't complete successfully.
            if not f.done():
                self.state = ttypes.State.READY

                error = f.exception()
                if error:
                    pass  # TODO: Do something with the exception.

        executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        executor.submit(self._shutdown).add_done_callback(done)

    @safety_check
    def open_covers(self):
        """Open the baffle then mirror covers. The order is important."""

        self.aux.open_covers()

    @safety_check
    def close_covers(self):
        """Close the mirror then baffle covers. The order is important."""

        self.aux.close_covers()

    @safety_check
    def dome_remote_enable(self):
        self.dome.remote_control_on()

    @safety_check
    def park_dome(self):
        dome = self.dome.get_status()

        if not dome.remote:
            raise ttypes.LesediException(
                'Dome cannot be parked while not in remote mode.')

        self.dome.park()

    @safety_check
    def open_dome(self):
        dome = self.dome.get_status()

        if not dome.remote:
            raise ttypes.LesediException(
                'Dome cannot be opened while not in remote mode.')

        self.dome.open()

    @safety_check
    def close_dome(self):
        dome = self.dome.get_status()

        if not dome.remote:
            raise ttypes.LesediException(
                'Dome cannot be closed while not in remote mode.')

        self.dome.close()

    # We don't need a safety check here because it just stops all dome movement.
    def stop_dome(self):
        self.dome.emergency_stop()

    @safety_check
    def dome_follow_telescope_start(self):
        self.dome.follow_telescope_start()

    @safety_check
    def dome_follow_telescope_stop(self):
        self.dome.follow_telescope_stop()

    @safety_check
    def rotate_dome(self, az):
        dome = self.dome.get_status()

        if not dome.remote:
            raise ttypes.LesediException(
                'Dome cannot be rotated while not in remote mode.')

        self.dome.rotate(az)

    @safety_check
    def dome_lights_on(self):
        self.dome.lights_on()

    @safety_check
    def dome_lights_off(self):
        self.dome.lights_off()

    @safety_check
    def slew_lights_on(self):
        self.dome.slew_lights_on()

    @safety_check
    def slew_lights_off(self):
        self.dome.slew_lights_off()

    @safety_check
    def rotator_tracking_on(self):
        """Enable tracking on the currently selected instrument."""

        status = self.get_status()

        if status.instrument:
            if status.instrument == ttypes.Instrument.WINCAM:
                self.rotator_left.tracking_on()
            elif status.instrument == ttypes.Instrument.SHOC:
                self.rotator_right.tracking_on()

    @safety_check
    def rotator_tracking_off(self):
        """Disable tracking on the currently selected instrument."""

        status = self.get_status()

        if status.instrument:
            if status.instrument == ttypes.Instrument.WINCAM:
                self.rotator_left.tracking_off()
            elif status.instrument == ttypes.Instrument.SHOC:
                self.rotator_right.tracking_off()

    @safety_check
    def rotator_auto_on(self):
        """Put the currently selected instrument rotator in auto mode."""

        status = self.get_status()

        if status.instrument:
            if status.instrument == ttypes.Instrument.WINCAM:
                self.rotator_left.to_auto()
            elif status.instrument == ttypes.Instrument.SHOC:
                self.rotator_right.to_auto()

    @safety_check
    def rotator_auto_off(self):
        """Put the currently selected instrument rotator in manual mode."""

        status = self.get_status()

        if status.instrument:
            if status.instrument == ttypes.Instrument.WINCAM:
                self.rotator_left.to_manual()
            elif status.instrument == ttypes.Instrument.SHOC:
                self.rotator_right.to_manual()

    @safety_check
    def secondary_mirror_stop(self):
        self.focuser.secondary_stop()

    @safety_check
    def secondary_mirror_auto_on(self):
        self.focuser.secondary_to_auto()

    @safety_check
    def secondary_mirror_auto_off(self):
        self.focuser.secondary_to_manual()

    @safety_check
    def tertiary_mirror_auto_on(self):
        self.focuser.tertiary_to_auto()

    @safety_check
    def tertiary_mirror_auto_off(self):
        self.focuser.tertiary_to_manual()

    @safety_check
    def set_focus(self, inches):
        focuser = self.focuser.get_status()

        if not focuser.secondary_mirror_auto:
            raise ttypes.LesediException(
                'Cannot set focus. Secondary mirror is in manual mode.')

        self.focuser.secondary_move_to(inches)

    @safety_check
    def select_instrument(self, instrument):
        focuser = self.focuser.get_status()

        if not focuser.tertiary_mirror_auto:
            raise ttypes.LesediException(
                'Cannot select instrument. Tertiary mirror is in manual mode.')

        # Disable the rotator for the currently selected instrument before
        # changing to the newly selected instrument.
        if instrument == ttypes.Instrument.WINCAM:
            self.rotator_right.tracking_off()
            self.rotator_left.tracking_on()
        elif instrument == ttypes.Instrument.SHOC:
            self.rotator_left.tracking_off()
            self.rotator_right.tracking_on()

        self.focuser.select_instrument(instrument)

    @safety_check
    def auto_on(self):
        self.telescope.set_motors_to_auto()

    @safety_check
    def auto_off(self):
        self.telescope.set_motors_to_manual()

    @safety_check
    def park(self):
        self.telescope.park()

    @safety_check
    def unpark(self):
        self.telescope.unpark()

        # The TCS enables tracking after unpark so make sure to disable it.
        self.telescope.set_tracking_mode(0, ttypes.TrackType.SIDEREAL, 0, 0)

    @safety_check
    def goto_altaz(self, alt, az):
        telescope = self.telescope.get_status()

        if telescope.parked:
            raise ttypes.LesediException(
                'Telescope cannot be slewed while parked.')

        if telescope.manual:
            raise ttypes.LesediException(
                'Cannot move telescope. Motors are in manual mode.')

        self.telescope.goto_altaz(alt, az)

    @safety_check
    def goto_radec(self, ra, dec):
        telescope = self.telescope.get_status()

        if telescope.parked:
            raise ttypes.LesediException(
                'Telescope cannot be slewed while parked.')

        if telescope.manual:
            raise ttypes.LesediException(
                'Cannot move telescope. Motors are in manual mode.')

        self.telescope.goto_radec(ra, dec)

    @safety_check
    def set_tracking_mode(self, tracking, track_type, ra_rate, dec_rate):
        self.telescope.set_tracking_mode(
            tracking, track_type, ra_rate, dec_rate)

    @safety_check
    def abort(self):
        self.telescope.abort()

    @safety_check
    def move(self, direction, arcsec):
        telescope = self.telescope.get_status()

        if telescope.parked:
            raise ttypes.LesediException(
                'Telescope cannot be slewed while parked.')

        if telescope.manual:
            raise ttypes.LesediException(
                'Cannot move telescope. Motors are in manual mode.')

        if direction == ttypes.Direction.NORTH:
            direction = 'N'
        elif direction == ttypes.Direction.SOUTH:
            direction = 'S'
        elif direction == ttypes.Direction.EAST:
            direction = 'E'
        elif direction == ttypes.Direction.WEST:
            direction = 'W'
        else:
            raise ttypes.LesediException('Unknown direction: {}'.format(direction))

        self.telescope.jog(direction, arcsec)


def run():
    parser = argparse.ArgumentParser(description=__doc__, prog='lesedi')
    parser.add_argument('--version', action='version',
        version='%(prog)s {}'.format(__version__))
    parser.add_argument('--log-file', help='the log file to write to')
    parser.add_argument('--log-level', default='INFO', type=str.upper,
        choices=['DEBUG', 'INFO', 'WARN', 'ERROR', 'CRITICAL'],
        help='the level of logging to use.')

    parser.add_argument('--config', help='path to the configuration file')

    parser.add_argument('--host', default=constants.DEFAULT_HOST,
        help='the TCP interface to listen on')
    parser.add_argument('--port', default=constants.DEFAULT_PORT, type=int,
        help='the TCP port to listen on')

    arguments = parser.parse_args()

    if not arguments.log_file:
        logging.basicConfig(stream=sys.stdout, level=arguments.log_level)
    else:
       logging.basicConfig(filename=arguments.log_file,
            format='%(asctime)s [%(levelname)5s]: %(message)s',
            level=arguments.log_level)
    logger = logging.getLogger(__name__)

    if arguments.config:
        try:
            config = configparser.ConfigParser()

            with open(arguments.config, 'r') as f:
                config.readfp(f)
        except IOError as e:
            sys.exit('Error while reading config file "{}"'.format(arguments.config))
    else:
        # If no config is given, assume that all services are running locally.
        config = {
            'aux': {'host': '127.0.0.1'},
            'dome': {'host': '127.0.0.1'},
            'focuser': {'host': '127.0.0.1'},
            'rotator': {'host': '127.0.0.1'},
            'telescope': {'host': '127.0.0.1'},
        }

    handler = Handler(config)

    processor = Service.Processor(handler)
    transport = TSocket.TServerSocket(host=arguments.host, port=arguments.port)
    tfactory = TTransport.TBufferedTransportFactory()
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()

    server = TServer.TThreadedServer(processor, transport, tfactory, pfactory)

    logger.info('Listening on {}:{}'.format(arguments.host, arguments.port))

    try:
        server.serve()
    except (KeyboardInterrupt, SystemExit):
        sys.exit(0)
    finally:
        transport.close()


if __name__ == '__main__':
    run()
