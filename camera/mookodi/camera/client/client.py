#!/usr/bin/env python

import socket

from time import sleep
from requests.exceptions import ConnectionError

from thrift.transport import TSocket, TTransport
from thrift.protocol import TBinaryProtocol
from mookodi.camera.client.camera_interface import CameraService, constants, ttypes

class Client(CameraService.Client):

    def __init__(self, host="localhost", port=9020):
        socket = TSocket.TSocket(host, port)
        socket.setTimeout(5000)

        self.transport = TTransport.TFramedTransport(socket)
        self.client = CameraService.Client(
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

if __name__ == '__main__':

    c = Client()

    print("Set binning")
    c.set_binning(1,1)
    fi = []
    print("Set FITS headers...")
    c.set_fits_headers(fi)
    print("Before exposure:")
    print(c.get_state())
    c.start_multrun(ExposureType.EXPOSURE,1,10000)
    print("Started exposure")
    for i in range(16):
        print("Got state:")
        print(c.get_state())
        sleep(0.5)
