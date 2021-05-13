"""Base driver primitives for Lesedi telescope control system."""

import ctypes
import logging
import socket
import threading


logger = logging.getLogger(__name__)


class Bitfield(ctypes.Union):

    def __init__(self, value=0, *args, **kwargs):
        # Use the value argument as the initial status value.
        self.value = int(value)

        # Keyword arguments are assumed to be the fields that need to be set.
        for k, v in kwargs.items():
            setattr(self, k, v)

    def __repr__(self):
        return '<{}: {}>'.format(self.__class__.__name__, self.value)

    def __eq__(self, other):
        """Use the value property for comparisons."""

        if isinstance(other, type(self)):
            # Support comparisons against other instances.
            return self.value == other.value
        elif isinstance(other, (int, long)):
            # Support comparisons against integer instances.
            return self.value == other

        return False


class BaseResponse(object):
    """Represents a response returned by one of the subsystems.

    Subsystems return a response with a list of parameters separated by a
    semicolon, followed by a final semicolon, an underscore and an optional
    message. The one exception is the dome, which uses spaces as parameter
    separator.

    Example response from the telescope to the "SiteLocations" command:

        32.3798;20.8107;1822;_SiteLocations

    Where latitude = 32.3798,
          longitude = 20.8107,
          elevation = 1822
    """

    # Use semicolons as parameter separator when parsing responses. This can be
    # overridden in subclasses.
    SEPARATOR = ';'

    def parse(self, response):
        """Parses the response and returns a list of parameters."""

        params, message = response.strip().split('_')
        self.message = message
        return params.strip(';').split(self.SEPARATOR)


class SocketFile(object):

    def __init__(self, sock, mode):
        self._file = sock.makefile(mode)

    def __enter__(self):
        return self._file

    def __exit__(self, type, value, traceback):
        self._file.close()


class BaseDriver(object):
    """Base driver that is meant to be implemented by subclasses.

    It is assumed that all subsystems implement a request/response pattern and
    that messages are terminated with a single newline character.
    """

    TERMINATOR = '\n'

    def __init__(self, host='127.0.0.1', port=None):
        self.socket = socket.socket()
        self.socket.connect((host, self.PORT or port))
        self.socket.settimeout(1)

        # We use the lock so that we don't issue out of order reads or writes
        # from different threads within the same application.
        self._lock = threading.Lock()

    def write(self, data):
        with SocketFile(self.socket, 'w') as f:
            f.write(data + self.TERMINATOR)

    def read(self):
        with SocketFile(self.socket, 'r') as f:
            return f.readline()

    def execute(self, command):
        """Execute the given command and immediately read back the response."""

        with self._lock:
            self.write(command)
            response = self.read().strip()
        return response
