#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to take a series of exposures.
The previously configured readout speed, gain, window and binning are used. 
The command sets the exposure length to use, loops over the exposure count,
calls start_expose() to start the camera taking each frame, 
and then uses get_state() to determine when the frame has been taken,
and uses get_last_image_filename() to retrieve the FITS image filename generated. 
The command returns after MookodiCameraServer has finished taking the images.

./multrun3.py <exposure count> <exposure length>

Parameters:
<exposure count> specifies the number of exposures to acquire.
<exposure length> specifies the length of each exposure in milliseconds.
"""
import argparse
import time
from mookodi.camera.client.client import Client

# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of frames to take")
parser.add_argument("exposure_length", type=int,help="The length of each frame in milliseconds")
args = parser.parse_args()

# Create client and start multrun
c= Client()
c.set_exposure_length(args.exposure_length)
for i in range(args.exposure_count):
    c.start_expose(True)
    done = False
    while done == False:
        time.sleep(1)
        state = c.get_state()
        done = state.exposure_in_progress == False
    filename = c.get_last_image_filename()
    print ("Image "+repr(i)+": "+filename)
