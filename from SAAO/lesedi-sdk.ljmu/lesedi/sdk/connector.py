import threading
import sys
import time

from lesedi.sdk.telescope import Telescope
from shoc.instrument import Coordinator as InstrumentCoordinator


class Connector(object):

    def __init__(self,
                 telescopehost='localhost',
                 instrumenthost='localhost',
                 connections=None):
        self.instrumenthost = instrumenthost
        self.telescopehost = telescopehost
        self._connections = connections or threading.local()

    @property
    def instrument(self):
        print("Finding instrument")
        if not hasattr(self._connections, '_instrument'):
            self._connections._instrument = InstrumentCoordinator(
                connector=self,
                host=self.instrumenthost,
            )
        return self._connections._instrument

    @property
    def telescope(self):
        if not hasattr(self._connections, 'telescope'):
            self._connections.telescope = Telescope(
                connector = self,
                host=self.telescopehost
            )
        return self._connections.telescope

if __name__ == '__main__':
    print("Instantiating  parent controller")
    try:
        tlc = Connector(instrumenthost='localhost',
                        telescopehost='localhost')
    except:
        print("Exception while instantiating controller")
        sys.exit()
    try:
        print("Getting statuses")
        print("")
        print("Filter status: {}".format(tlc.instrument.filter.get_state()))
        print("")
        print("GPS status: {}".format(tlc.instrument.gps.get_state()))
        print("")
    except:
        print("Caught an exception getting statuses")
    try:
        print("Initializing camera")
        tlc.instrument.camera.initialize()
        print("Camera initialised: ")
        print("Setting exposure parameters")
        tlc.instrument.camera.set_acquisition_mode(3)
        tlc.instrument.camera.set_trigger_mode(0)
        tlc.instrument.camera.set_readout_mode(4)
        tlc.instrument.camera.set_exposure_time(0.4)
        tlc.instrument.camera.set_kinetic_series_length(5)
        print("Taking exposure")
        tlc.instrument.start_exposure()
        print("Exposure started")
        time.sleep(15)
    except Exception as e:
        print("Uh oh. Something went wrong")
        print(str(e))
    while(True):
        try:
            tlc.instrument.camera.shutdown()
            break
        except Exception as e:
            print("Exception caught while shutting down: {}".format(str(e)))
            time.sleep(10)
    print("...and we're done")
