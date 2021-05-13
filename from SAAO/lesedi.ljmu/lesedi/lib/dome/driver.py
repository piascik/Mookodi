import argparse
import ctypes
import logging
import socket
import sys

from lesedi.sdk import ttypes
from lesedi.lib import driver


logger = logging.getLogger(__name__)


class Status(driver.Bitfield):
    """Represents the current dome status

    This combines the three (BitsFromDome0, BitsFromDome1, BitsFromDomeDriver)
    separate 16 bit statuses into one.
    """

    _anonymous_ = ('fields',)

    class Fields(ctypes.LittleEndianStructure):
        _fields_ = [
            # BitsFromDome0
            ('shutter_closed', ctypes.c_uint8, 1),
            ('shutter_opened', ctypes.c_uint8, 1),
            ('shutter_motor_on', ctypes.c_uint8, 1),
            ('shutter_fault', ctypes.c_uint8, 1),
            ('moving', ctypes.c_uint8, 1),
            ('rotation_fault', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('lights_on', ctypes.c_uint8, 1),
            ('lights_manual', ctypes.c_uint8, 1),
            ('shutter_opening_manually', ctypes.c_uint8, 1),
            ('shutter_closing_manually', ctypes.c_uint8, 1),
            ('moving_right_manually', ctypes.c_uint8, 1),
            ('moving_left_manually', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('shutter_power', ctypes.c_uint8, 1),
            ('power', ctypes.c_uint8, 1),

            # BitsFromDome1
            ('remote', ctypes.c_uint8, 1),
            ('is_raining', ctypes.c_uint8, 1),
            ('tcs_locked_out', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('emergency_stop', ctypes.c_uint8, 1),
            ('_reserved', ctypes.c_uint8, 1),
            ('shutter_closed_rain', ctypes.c_uint8, 1),
            ('power_failure', ctypes.c_uint8, 1),
            ('shutter_closed_power_failure', ctypes.c_uint8, 1),
            ('watchdog_tripped', ctypes.c_uint8, 1),

            # BitsFromDomeDriver
            ('no_comms_with_servocontroller', ctypes.c_uint8, 1),
            ('no_comms_with_sitechexe', ctypes.c_uint8, 1),
            ('no_comms_with_plc', ctypes.c_uint8, 1),
            ('at_position_setpoint', ctypes.c_uint8, 1),
            ('following', ctypes.c_uint8, 1),
            ('driver_moving', ctypes.c_uint8, 1),
            ('shutter_moving', ctypes.c_uint8, 1),
            ('slew_lights_on', ctypes.c_uint8, 1),
            ('dome_lights_on', ctypes.c_uint8, 1),
        ]
    _fields_ = [
        ('fields', Fields),
        ('value', ctypes.c_uint64),
    ]


class Response(driver.BaseResponse):
    """A dome response message

    The response contains the following:

        - Position (degrees * 10)
        - Status0 and Status1 are two 16 bit bit fields
        - Driver status bit field

    """

    SEPARATOR = ' '

    def __init__(self, response):
        position, status0, status1, driver_status = self.parse(response)

        # Combine all three separate statuses into one.
        self.status = Status(
            (int(driver_status) << 32) | (int(status1) << 16) | int(status0))

        self.position = float(position) / 10


class Driver(driver.BaseDriver):

    PORT = 1963

    def get_status(self):
        response = Response(self.execute('GetDomeData'))

        return ttypes.DomeStatus(
            # BitsFromDome0
            shutter_closed=bool(response.status.shutter_closed),
            shutter_opened=bool(response.status.shutter_opened),
            shutter_motor_on=bool(response.status.shutter_motor_on),
            shutter_fault=bool(response.status.shutter_fault),
            moving=bool(response.status.moving),
            rotation_fault=bool(response.status.rotation_fault),
            lights_on=bool(response.status.lights_on),
            lights_manual=bool(response.status.lights_manual),
            shutter_opening_manually=bool(response.status.shutter_opening_manually),
            shutter_closing_manually=bool(response.status.shutter_closing_manually),
            moving_right_manually=bool(response.status.moving_right_manually),
            moving_left_manually=bool(response.status.moving_left_manually),
            shutter_power=bool(response.status.shutter_power),
            power=bool(response.status.power),

            # BitsFromDome1
            remote=bool(response.status.remote),
            is_raining=bool(response.status.is_raining),
            tcs_locked_out=bool(response.status.tcs_locked_out),
            emergency_stop=bool(response.status.emergency_stop),
            shutter_closed_rain=bool(response.status.shutter_closed_rain),
            power_failure=bool(response.status.power_failure),
            shutter_closed_power_failure=bool(response.status.shutter_closed_power_failure),
            watchdog_tripped=bool(response.status.watchdog_tripped),

            # BitsFromDomeDriver
            no_comms_with_servocontroller=bool(response.status.no_comms_with_servocontroller),
            no_comms_with_sitechexe=bool(response.status.no_comms_with_sitechexe),
            no_comms_with_plc=bool(response.status.no_comms_with_plc),
            at_position_setpoint=bool(response.status.at_position_setpoint),
            following=bool(response.status.following),
            driver_moving=bool(response.status.driver_moving),
            shutter_moving=bool(response.status.shutter_moving),
            slew_lights_on=bool(response.status.slew_lights_on),
            dome_lights_on=bool(response.status.dome_lights_on),

            position=response.position,
        )

    def emergency_stop(self):
        """Stop all motion if in remote."""

        return self.execute('EmergencyStop')

    def remote_control_on(self):
        """Put the PLC in remote control if not locked out."""

        return self.execute('ToRemoteControl')

    def follow_telescope_start(self):
        """The dome will follow the telescope."""

        return self.execute('FollowTelescopeStart')

    def follow_telescope_stop(self):
        """The dome won't follow the telescope."""

        return self.execute('FollowTelescopeStop')

    def motors_on(self):
        """Turn on both the shutter power and the rotation power."""

        return self.execute('PowerMotorsOn')

    def motors_off(self):
        """Turn off both the shutter power and the rotation power."""

        return self.execute('PowerMotorsOff')

    def park(self):
        """Park the dome by rotating to Az=270 (West)."""

        return self.execute('GoParkAndClose')

    def open(self):
        """Open the dome shutters."""

        return self.execute('OpenShutters')

    def close(self):
        """Close the dome shutters."""

        return self.execute('CloseShutters')

    def rotate(self, az):
        """Rotate dome to the provided azimuth."""

        return self.execute('MoveDomeTo {:.1f}'.format(az))

    def lights_on(self):
        """Turn the dome lights on."""

        return self.execute('DomeLightsOn')

    def lights_off(self):
        """Turn the dome lights off."""

        return self.execute('DomeLightsOff')

    def slew_lights_on(self):
        """Turn the slew lights on."""

        return self.execute('SlewLightsOn')

    def slew_lights_off(self):
        """Turn the slew lights off."""

        return self.execute('SlewLightsOff')


class MockDriver(object):

    STATE = {
        'status': Status(),
        'is_open': False,
        'angle': 0,
    }

    def execute(self, command):

        # Apply state changes based on the command.
        if command == 'ToRemoteControl':
            self.STATE['status'].remote = 1
        elif command.startswith('MoveDomeTo'):
            command, angle = command.split(' ')

            self.STATE['angle'] = float(angle) * 10
        elif command == 'FollowTelescopeStart':
            self.STATE['status'].following = 1
        elif command == 'FollowTelescopeStop':
            self.STATE['status'].following = 0
        elif command == 'OpenShutters':
            self.STATE['status'].shutter_closed = 0
            self.STATE['status'].shutter_opened = 1
        elif command == 'CloseShutters':
            self.STATE['status'].shutter_closed = 1
            self.STATE['status'].shutter_opened = 0
        elif command == 'DomeLightsOn':
            self.STATE['status'].dome_lights_on = 1
        elif command == 'DomeLightsOff':
            self.STATE['status'].dome_lights_on = 0
        elif command == 'SlewLightsOn':
            self.STATE['status'].slew_lights_on = 1
        elif command == 'SlewLightsOff':
            self.STATE['status'].slew_lights_on = 0
        elif command == 'GoParkAndClose':
            self.STATE['status'].shutter_closed = 1
            self.STATE['status'].shutter_opened = 0
            self.STATE['angle'] = 270 * 10
        elif command == 'EmergencyStop':
            self.STATE['status'].following = 0
            self.STATE['status'].moving = 0
            self.STATE['status'].shutter_moving = 0

        response = '{} {} {} {};_'.format(
            self.STATE['angle'],
            self.STATE['status'].value & 0xffff,  # last 16 bits (BitsFromDomeDriver)
            self.STATE['status'].value >> 16 & 0xffff,  # next 16 bits (BitsFromDome1)
            self.STATE['status'].value >> 32,  # first 16 of 48 bits (BitsFromDome0)
        )

        return response + Driver.TERMINATOR


def main():
    """Launches a TCP server that responds to dome commands."""

    parser = argparse.ArgumentParser(prog='lesedi-dome')
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
