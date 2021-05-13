import logging
import socket

from thrift.transport import TSocket, TTransport
from thrift.protocol import TBinaryProtocol
from lesedi.sdk import Service, constants, ttypes

from datetime import datetime

logger = logging.getLogger(__name__)

class ConnectionError(ttypes.LesediException):

    def __str__(self):
        return self.message


class Connection(object):

    def __init__(self, host=constants.DEFAULT_HOST, port=constants.DEFAULT_PORT):
        socket = TSocket.TSocket(host, port)
        socket.setTimeout(5000)

        self.transport = TTransport.TBufferedTransport(socket)
        self.client = Service.Client(
            TBinaryProtocol.TBinaryProtocol(self.transport))

        self.open()

    def open(self, close_first=False):
        if close_first:
            self.close()
        elif self.transport.isOpen():
            return

        try:
            self.transport.open()
        except TTransport.TTransportException as e:
            raise ConnectionError(str(e))

    def close(self):
        if not self.transport.isOpen():
            return

        self.transport.close()

    def __del__(self):
        self.close()

    def __getattr__(self, name):
        """Delegate attribute access to the client instance."""

        def handle_errors(f):
            """Handles connection errors on callable client attributes."""

            def wrapper(*args, **kwargs):
                try:
                    return f(*args, **kwargs)
                except socket.timeout as e:
                    raise ConnectionError(str(e))
                except (socket.error, TTransport.TTransportException) as e:
                    # Try to reconnect when we get any kind of network error
                    # and raise a ConnectionError if unsuccessful.

                    self.open(close_first=True)

                    try:
                        return f(*args, **kwargs)
                    except (socket.error, TTransport.TTransportException) as e:
                        raise ConnectionError(str(e))
            return wrapper

        attr = getattr(self.client, name)

        if callable(attr):
            return handle_errors(attr)

        return attr

    def gather_fits_info(self):
        logger.debug("Gathering TCS fits info now")
        status = self.get_status()

        tcs_fits_info = {
            'TELRA': {
                str(status.telescope_ra): 'The telescope right ascension'
            },
            'TELDEC': {
                str(status.telescope_dec): 'The telescope declination'
             },
            'AIRMASS': {
                str(status.airmass): 'The airmass (sec(z))'
            },
            'ZD': {
                str(status.zenith_distance): 'The telescope zenith distance'
            },
            'TELFOCUS': {
                str(status.focus): 'The telescope focus'
            },
            'DOMEPOS': {
                str(status.dome_angle): 'The dome position'
            }
        }
        return tcs_fits_info

    
    def get_state_for_web(self):
        self.connection_timestamp = datetime.now()
        print("Recorded timestamp: ", self.connection_timestamp)
        return self.get_state()

    def get_state(self):
        return self.get_status()

    def get_connection_timestamp(self):
        return self.connection_timestamp()

    """Here just for testing purposes. Something similar should eventually
be implemented in the back-end"""
    def nudge(self, x, y):
        return

    """Here for testing. Should be part of status"""
    def is_ready(self):
        return True
