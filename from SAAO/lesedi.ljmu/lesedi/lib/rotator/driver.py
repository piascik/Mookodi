import argparse
import ctypes
import logging
import socket
import sys

from lesedi.sdk import ttypes
from lesedi.lib import driver


logger = logging.getLogger(__name__)


class Status(driver.Bitfield):
    """Represents a rotator status"""

    _anonymous_ = ('fields',)

    class Fields(ctypes.LittleEndianStructure):
        _fields_ = [
            ('moving', ctypes.c_uint8, 1),
            ('tracking', ctypes.c_uint8, 1),
            ('auto', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('at_limit', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('comms_fault_servocommunicator', ctypes.c_uint8, 1),
            ('comms_fault_servocontroller', ctypes.c_uint8, 1),
            ('moving_by_buttons', ctypes.c_uint8, 1),
        ]
    _fields_ = [
        ('fields', Fields),
        ('value', ctypes.c_uint16),
    ]


class Response(driver.BaseResponse):
    """A rotator response message

    The response contains the following:

        - Status bit field
        - Angle (degrees)
        - Parallactic angle (degrees)
        - Parallactic rate (revolutions per sidereal day)

    """

    def __init__(self, response):
        status, angle, parallactic_angle, parallactic_rate = self.parse(response)

        self.status = Status(int(status))
        self.angle = float(angle)
        self.parallactic_angle = float(parallactic_angle)
        self.parallactic_rate = float(parallactic_rate)


class Driver(driver.BaseDriver):

    @property
    def config(self):
        if not hasattr(self, '_config'):
            try:
                with open(self.CONFIG, 'r') as f:
                    config = f.read()
            except IOError:
                logger.error('Error while reading configuration at: {}'.format(
                    self.CONFIG))

                # Default to an empty dict when we're unable to read the
                # configuration file.
                self._config = {}
            else:
                # Parse the configuration file, which is a list of key/value
                # pairs in `key=value` format.
                self._config = dict(
                    map(str.strip, l.split('=')) for l in config.split())
        return self._config

    def get_status(self):
        response = Response(
            self.execute('GetRotatorData'))

        return ttypes.RotatorStatus(
            limit_min=float(self.config.get('CCWLimit', 0)),
            limit_max=float(self.config.get('CWLimit', 100)),
            moving=bool(response.status.moving),
            tracking=bool(response.status.tracking),
            auto=bool(response.status.auto),
            at_limit=bool(response.status.at_limit),
            comms_fault_servocommunicator=bool(response.status.comms_fault_servocommunicator),
            comms_fault_servocontroller=bool(response.status.comms_fault_servocontroller),
            moving_by_buttons=bool(response.status.moving_by_buttons),
            angle=response.angle,
            parallactic_angle=response.parallactic_angle,
            parallactic_rate=response.parallactic_rate,
        )

    def to_auto(self):
        """Put it in "Auto" mode."""

        response = Response(self.execute('RotatorToAuto'))

        if response.message == 'RotatorToAutoAccepted':
            return response

        raise ttypes.LesediException('Auto command failed.')

    def to_manual(self):
        """Put it in "Manual" (blinky) mode."""

        response = Response(self.execute('RotatorToBlinky'))

        if response.message == 'RotatorToBlinkyAccepted':
            return response

        raise ttypes.LesediException('Manual command failed.')

    def jog_by(self, degrees):
        """Jog the rotator nn.nn degrees.

        If it's negative, it will decrease the angle, otherwise, it will
        increase the angle by nn.nn.

        """

        response = Response(
            self.execute('JogRotatorDegs {:.2f}'.format(degrees)))

        if response.message == 'JogRotatorDegs':
            return response

        raise ttypes.LesediException('Jog command failed.')

    def move_to(self, angle):
        """Move the rotator to nn.nn deg's (or will move to the limit)."""

        response = Response(
            self.execute('MoveRotatorToAngle {:.2f}'.format(angle)))

        if response.message == 'MoveRotatorToAngleAccepted':
            return response

        raise ttypes.LesediException('Move command failed.')

    def move_to_parallactic_angle(self):
        """Move the rotator to the parallactic angle (North Up) (or will move to the limit)."""

        response = Response(self.execute('MoveRotatorToParallacticAngle'))

        if response.message == '':
            return response

        raise ttypes.LesediException('Move to parallactic angle command failed.')

    def move_to_limit(self, degrees_per_second):
        """Move the rotator to the limit at speed nn.nn degs per second."""

        response = Response(self.execute(
            'SpinRotator {:.2f}'.format(degrees_per_second)))

        if response.message == 'SpinRotatorAccepted':
            return response

        raise ttypes.LesediException('Move command failed.')

    def tracking_on(self):

        response = Response(self.execute('RotatorTrackingOn'))

        if response.message == 'TrackingOnAccepted':
            return response

        raise ttypes.LesediException('Tracking on command failed.')

    def tracking_off(self):

        response = Response(self.execute('RotatorTrackingOff'))

        if response.message == 'TrackingOffAccepted':
            return response

        raise ttypes.LesediException('Tracking off command failed.')

    def park(self):
        response = Response(self.execute('ParkRotator'))

        if response.message == 'ParkRotatorAccepted':
            return response

        raise ttypes.LesediException('Park command failed.')

    def unpark(self):
        response = Response(self.execute('UnParkRotator'))

        if response.message == 'UnParkRotatorAccepted':
            return response

        raise ttypes.LesediException('Unpark command failed.')


class LeftRotatorDriver(Driver):

    PORT = 1958
    CONFIG = '/usr/share/SiTech/ServoSCommunicator/RotatorCfgFileLeft.txt'


class RightRotatorDriver(Driver):

    PORT = 1959
    CONFIG = '/usr/share/SiTech/ServoSCommunicator/RotatorCfgFileRight.txt'


class MockDriver(object):

    STATE = {
        'status': Status(tracking=0),
        'angle': 0,  # degrees
        'parallactic_angle': 0,
        'parallactic_rate': 0,
    }

    def execute(self, command):
        message = ''

        # Apply state changes based on the command.
        if command == 'RotatorTrackingOn':
            message = 'TrackingOnAccepted'
            self.STATE['status'].tracking = 1
        elif command == 'RotatorTrackingOff':
            message = 'TrackingOffAccepted'
            self.STATE['status'].tracking = 0
        elif command == 'RotatorToAuto':
            message = 'RotatorToAutoAccepted'
            self.STATE['status'].auto = 1
        elif command == 'RotatorToBlinky':
            message = 'RotatorToBlinkyAccepted'
            self.STATE['status'].auto = 0
        elif command == 'ParkRotator':
            message = 'ParkRotatorAccepted'
            self.STATE['angle'] = 90
        elif command == 'UnParkRotator':
            message = 'UnParkRotatorAccepted'
            self.STATE['angle'] = 90

        response = '{};{};{};{};_{}'.format(
            self.STATE['status'].value,
            self.STATE['angle'],
            self.STATE['parallactic_angle'],
            self.STATE['parallactic_rate'],
            message,
        )

        return response + Driver.TERMINATOR


def main():
    """Launches a TCP server that responds to rotator commands."""

    parser = argparse.ArgumentParser(prog='lesedi-rotator')
    parser.add_argument('--port', type=int, required=True,
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
