import argparse
import ctypes
import logging
import socket
import sys

from lesedi.sdk import ttypes
from lesedi.lib import driver


logger = logging.getLogger(__name__)


class Status(driver.Bitfield):
    """Represents the current focuser status"""

    _anonymous_ = ('fields',)

    class Fields(ctypes.LittleEndianStructure):
        _fields_ = [
            ('secondary_mirror_moving', ctypes.c_uint8, 1),
            ('tertiary_mirror_moving', ctypes.c_uint8, 1),
            ('secondary_mirror_auto', ctypes.c_uint8, 1),
            ('tertiary_mirror_auto', ctypes.c_uint8, 1),
            ('secondary_mirror_at_limit', ctypes.c_uint8, 1),
            ('autofocus', ctypes.c_uint8, 1),
            ('tertiary_mirror_left_fork_position', ctypes.c_uint8, 1),
            ('tertiary_mirror_right_fork_position', ctypes.c_uint8, 1),
            ('comms_fault_servocommunicator', ctypes.c_uint8, 1),
            ('comms_fault_servocontroller', ctypes.c_uint8, 1),
        ]
    _fields_ = [
        ('fields', Fields),
        ('value', ctypes.c_uint16),
    ]


class Response(driver.BaseResponse):
    """A focuser response message

    The response contains the following:

        - Status bit field
        - Position (inches)
        - Tertiary mirror angle (degrees)

    """

    def __init__(self, response):
        status, position, tertiary_mirror_angle = self.parse(response)

        self.status = Status(status)
        self.position = float(position)
        self.tertiary_mirror_angle = float(tertiary_mirror_angle)


class Driver(driver.BaseDriver):

    PORT = 1961

    def get_status(self):
        response = Response(self.execute('GetFocuserTertiaryData'))

        return ttypes.FocuserStatus(
            autofocus=bool(response.status.autofocus),
            comms_fault_servocommunicator=bool(response.status.comms_fault_servocommunicator),
            comms_fault_servocontroller=bool(response.status.comms_fault_servocontroller),
            secondary_mirror_at_limit=bool(response.status.secondary_mirror_at_limit),
            secondary_mirror_auto=bool(response.status.secondary_mirror_auto),
            secondary_mirror_moving=bool(response.status.secondary_mirror_moving),
            secondary_mirror_position=response.position,
            tertiary_mirror_angle=response.tertiary_mirror_angle,
            tertiary_mirror_auto=bool(response.status.tertiary_mirror_auto),
            tertiary_mirror_moving=bool(response.status.tertiary_mirror_moving),
            tertiary_mirror_left_fork_position=bool(response.status.tertiary_mirror_left_fork_position),
            tertiary_mirror_right_fork_position=bool(response.status.tertiary_mirror_right_fork_position),
        )

    def select_instrument(self, instrument):
        """Tertiary mirror is is position 1 (i.e. directing the beam to the
        SHOC port) or position 2 (for WiNCam).
        """

        if instrument == ttypes.Instrument.WINCAM:
            response = Response(
                self.execute('GoLeftTertiary'))
        elif instrument == ttypes.Instrument.SHOC:
            response = Response(
                self.execute('GoRightTertiary'))
        else:
            raise ttypes.LesediException('Unknown instrument')

        if response.message in ('GoLeftTertiaryAccepted', 'GoRightTertiaryAccepted'):
            return response

        raise ttypes.LesediException('Select instrument command failed.')

    def open_mirror_covers(self):
        """Open the mirror covers."""

        response = Response(self.execute('MirrorCoverOpen'))

        if response.message == 'MirrorCoverOpenAccepted':
            return response

        raise ttypes.LesediException('Mirror covers open command failed.')

    def close_mirror_covers(self):
        """Close the mirror covers."""

        response = Response(self.execute('MirrorCoverClose'))

        if response.message == 'MirrorCoverCloseAccepted':
            return response

        raise ttypes.LesediException('Mirror covers close command failed.')

    def open_baffle_covers(self):
        """Open the baffle covers."""

        response = Response(self.execute('BaffleOpen'))

        if response.message == 'BaffleOpenAccepted':
            return response

        raise ttypes.LesediException('Baffle open command failed.')

    def close_baffle_covers(self):
        """Close the baffle covers."""

        response = Response(self.execute('BaffleClose'))

        if response.message == 'CloseBaffleAccepted':
            return response

        raise ttypes.LesediException('Baffle close command failed.')

    def secondary_move_to(self, inches):
        """Move the focuser to nn.nn inches (or will move to the limit)."""

        response = Response(
            self.execute('MoveFocuserTo {}'.format(inches)))

        if response.message == 'MoveFocuserAccepted':
            return response

        raise ttypes.LesediException('Move command failed.')

    def secondary_stop(self):
        """Stop the focuser movement."""

        response = Response(self.execute('StopFocuser'))

        if response.message == 'StopFocuserAccepted':
            return response

        raise ttypes.LesediException('Stop command failed.')

    def do_autofocus(self):

        response = Response(self.execute('DoAutoFocus'))

        if response.message == 'FocuserToAutoAccepted':
            return response

        raise ttypes.LesediException('Autofocus command failed.')

    def cancel_autofocus(self):

        response = Response(self.execute('CancelAutoFocus'))

        if response.message == 'CancelAutoFocusAccepted':
            return response

        raise ttypes.LesediException('Cancel command failed.')

    def secondary_to_auto(self):

        response = Response(self.execute('ToAutoFocuser'))

        if response.message == 'ToAutoFocuserAccepted':
            return response

        raise ttypes.LesediException('Auto command failed.')

    def secondary_to_manual(self):

        response = Response(self.execute('ToBlinkyFocuser'))

        if response.message == 'ToBlinkyFocuserAccepted':
            return response

        raise ttypes.LesediException('Manual command failed.')

    def restart_comms(self):
        """Try to reconnect to ServoCommunicator if there is no communications."""

        response = Response(self.execute('RestartCommunicatorComms'))

        if response.message == 'RestartCommunicatorCommsAccepted':
            return response

        raise ttypes.LesediException('Restart command failed.')

    def tertiary_to_auto(self):
        """Puts the tertiary servo motor into the "Auto" mode."""

        response = Response(self.execute('ToAutoTertiary'))

        if response.message == 'ToAutoTertiaryAccepted':
            return response

        raise ttypes.LesediException('Tertiary auto command failed.')

    def tertiary_to_manual(self):
        """Puts the tertiary servo motor into the "Manual" (or blinky) mode."""

        response = Response(self.execute('ToBlinkyTertiary'))

        if response.message == 'ToBlinkyTertiaryAccepted':
            return response

        raise ttypes.LesediException('Tertiary manual command failed.')

    def tertiary_go_left(self):

        response = Response(self.execute('GoLeftTertiary'))

        if response.message == 'GoLeftTertiaryAccepted':
            return response

        raise ttypes.LesediException('Tertiary go command failed.')

    def tertiary_go_right(self):

        response = Response(self.execute('GoRightTertiary'))

        if response.message == 'GoRightTertiaryAccepted':
            return response

        raise ttypes.LesediException('Tertiary go command failed.')

    def tertiary_move_to(self, angle):
        """Move the Tertiary mirror to nn.nn angle (or will move to the limit)."""

        response = Response(
            self.execute('MoveTertiaryTo {:.2f}'.format(angle)))

        if response.message == 'MoveTertiaryAccepted':
            return response

        raise ttypes.LesediException('Tertiary move command failed.')

    def tertiary_jog_by(self, angle):
        """Jog the Tertiary mirror by nn.nn angle (or will move to the limit)."""

        response = Response(
            self.execute('JogTertiary {:.2f}'.format(angle)))

        if response.message == '':
            return response

        raise ttypes.LesediException('Tertiary jog command failed.')


class MockDriver(object):

    STATE = {
        'status': Status(tertiary_mirror_left_fork_position=0),
        'position': 0,  # inches
        'tertiary_mirror_angle': 0,  # degrees
    }

    def execute(self, command):
        message = ''

        # Apply state changes based on the command.
        if command == 'GoLeftTertiary':
            self.STATE['status'].tertiary_mirror_left_fork_position = 1
            self.STATE['status'].tertiary_mirror_right_fork_position = 0
            message = 'GoLeftTertiaryAccepted'
        elif command == 'GoRightTertiary':
            self.STATE['status'].tertiary_mirror_left_fork_position = 0
            self.STATE['status'].tertiary_mirror_right_fork_position = 1
            message = 'GoRightTertiaryAccepted'
        elif command.startswith('MoveFocuserTo'):
            command, position = command.split(' ')

            self.STATE['position'] = float(position)
            message = 'MoveFocuserAccepted'
        elif command == 'DoAutoFocus':
            self.STATE['status'].autofocus = 1
            message = 'FocuserToAutoAccepted'
        elif command == 'CancelAutoFocus':
            self.STATE['status'].autofocus = 0
            message = 'CancelAutoFocusAccepted'
        elif command == 'ToAutoFocuser':
            self.STATE['status'].secondary_mirror_auto = 1
            message = 'ToAutoFocuserAccepted'
        elif command == 'ToBlinkyFocuser':
            self.STATE['status'].secondary_mirror_auto = 0
            message = 'ToBlinkyFocuserAccepted'
        elif command == 'ToAutoTertiary':
            self.STATE['status'].tertiary_mirror_auto = 1
            message = 'ToAutoTertiaryAccepted'
        elif command == 'ToBlinkyTertiary':
            self.STATE['status'].tertiary_mirror_auto = 0
            message = 'ToBlinkyTertiaryAccepted'
        elif command == 'RestartCommunicatorComms':
            message = 'RestartCommunicatorCommsAccepted'
        elif command == 'StopFocuser':
            self.STATE['status'].secondary_mirror_moving = 0
            message = 'StopFocuserAccepted'

        # The cover and baffle commands are essentially no-ops at the moment
        # because we can't get the current state.
        elif command == 'MirrorCoverOpen':
            message = 'MirrorCoverOpenAccepted'
        elif command == 'MirrorCoverClose':
            message = 'MirrorCoverCloseAccepted'
        elif command == 'BaffleOpen':
            message = 'BaffleOpenAccepted'
        elif command == 'BaffleClose':
            message = 'CloseBaffleAccepted'

        response = '{};{};{};_{}'.format(
            self.STATE['status'].value,
            self.STATE['position'],
            self.STATE['tertiary_mirror_angle'],
            message,
        )

        return response + Driver.TERMINATOR


def main():
    """Launches a TCP server that responds to focuser commands."""

    parser = argparse.ArgumentParser(prog='lesedi-focuser')
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
