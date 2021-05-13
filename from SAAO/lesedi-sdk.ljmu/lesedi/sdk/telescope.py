#!/usr/bin/env python

import logging as log
from astropy.io.fits.hdu.hdulist import HDUList

from lesedi.sdk.client import Connection
from lesedi.sdk import Service, constants, ttypes
from lesedi.sdk.ttypes import LesediException

import time

class Telescope(object):
    
    def __init__(self,
                 host=constants.DEFAULT_HOST,
                 port=constants.DEFAULT_PORT,
                 bridge=None):
        
        self.bridge = bridge
        self.host = host
        self.port = port


    @property
    def tcsclient(self):
        if not getattr(self, '_tcsclient', False):
            try:
                self._tcsclient = Connection(host = self.host,
                                             port = self.port)
            except:
                self._tcsclient = {}
        return self._tcsclient

    def send_message(self, message):
        # For now, just print the message to screen
        print("Received from LCU: {}".format(message))
        return "ok"
    
    def do_shutdown(self, message):
        # For now, just print the message to screen
        print("Received shutdown from LCU: {}".format(message))
    
    def get_focus_score(self, image):
        """Determine how well the image is focussed, and award a score"""

        hdul = HDUList(image)
        hdr = hdul[0].header
        data = hdul[0].data
        nrows = hdr['NAXIS1']
        ncols = hdr['NAXIS2']

        ns = 1
        back = 400
        imx = 512
        jmx = 512
        spix = 50
        background = 360
        from moments import moments
        fwhm = moments(data, imx, jmx, spix, nrows, ncols, background)
        print("FWHM = {}".format(fwhm))
        return fwhm
        
    def autofocus(self, start, stepsize, nsteps):
        """Automatically focus the telescope.
        
        Acquires images from the currently selected instrument, analyses these,
        and moves the telescope focus until best focus is achieved.
        """
        print("Attempting autofocs from position {} using {} steps of size {}".format(start, nsteps, stepsize))
        # Can we communicate with an instrument?
        if not self.bridge:
            raise LesediException("Unable to connect to any instruments")
        
        try:
            telstate = self.tcsclient.get_status()
        except Exception as e:
            raise Exception("Couldn't get telescope state: {}".format(e))
            # Determine the currently selected instrument

        print("Current telescope focus: {}".format(telstate.focus))

        try:
            instrument_num  = telstate.instrument
        except:
            raise LesediException("Could not find current instrument")
        if instrument_num == 1:
            instrument = 'shoc'
        elif instrument_num == 2:
            instrument = 'sibonise'
        else:
            raise Exception("Unknown instrument code")
        print("Current instrument is {}".format(instrument))

        
        # A very basic demo algorithm which will likely be replaced.
        
        # The idea: iterate over a range of focus positions. At each
        # position, measure the image focus quality and assign a
        # score. Afterwards, move the focus to the position which
        # resulted in the best score.

        original_focus_pos = telstate.focus
        print("Original focus = {}".format(original_focus_pos))


        best_score = float("inf")
        min_focus_pos = start

        exposure_time = 5

        # Initialize the camera if it isn't yet

        instrument = self.bridge.instrument
        print("Found instrument")

        needed_initialization = False
        try:
            if not instrument.camera.is_initialized():
                try:
                    print("Initializing camera")
                    instrument.camera.initialize()
                    needed_initialization = True
                except Exception as e:
                    raise Exception("Unable to initialize telescope")
        except:
            raise Exception("Could determine instrument's initialization state")
        print("The camera is initialized")

        position = best_position = min_focus_pos
        for i in range(nsteps):

            print("==== Iteration {} ======".format(i+1))
            # Set the telescope position
            print("Setting focus position = {}".format(position))

            self.tcsclient.set_focus(position)
            time.sleep(4)
            # Get the image from the instrument
            try:
                print("Requesting image from instrument")
                image = instrument.get_basic_image(exposure_time)
            except Exception as e:
                raise LesediException("Unable to get an image for autofocus: {}".format(e))
            print("Got image {}".format(i+1))

            # Get a focus score for the image
            score = self.get_focus_score(image)
            print("Autofocus: found score {} at {}".format(score, position))
            if score < best_score:
                best_score = score
                best_position = position

            time.sleep(1)
            # Set the telescope position for the next iteration
            position = position + stepsize
            

        # Set the focus position to the best found
        position = best_position
        print("Setting best focus position = {}".format(position))
        self.tcsclient.set_focus(position)

        if needed_initialization:
            instrument.camera.switch_off()

if __name__ == '__main__':
    import sys
    try:
        tel = Telescope(host="10.2.2.31")
    except Exception as e:
        print("Caught exception creating Telescope object: ", e)
        sys.exit(1)
    try:
        tel_status = tel.tcsclient.get_status()
        print("{}".format(tel_status))
        print()
        print("Focus = ", tel_status.focus)
    except Exception as  e:
        print("Caught exception getting Telescope state: ", e)
        sys.exit(1)
    
