#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to start taking a series of dark frames.
The previously configured readout speed, gain, window and binning are used. 
The command sets the exposure length to use, loops over the exposure count,
calls start_dark() to start the camera taking each dark frame, 
and then uses get_state() to determine when the dark has been taken,
and uses get_last_image_filename() to retrieve the FITS image filename generated. 
The command returns after MookodiCameraServer has finished taking the dark frames.

./multdark3.py <exposure count> <exposure length>

Parameters:
<exposure count> specifies the number of dark frames to acquire.
<exposure length> specifies the length of each dark frame in milliseconds.
"""
import argparse
import time
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import ExposureState


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of dark frames to take")
parser.add_argument("exposure_length", type=int,help="The length of each dark frame in milliseconds")
args = parser.parse_args()

# Create client and start multdark
c= Client()
c.set_exposure_length(args.exposure_length)
for i in range(args.exposure_count):
    print ("Starting Dark "+repr(i)+" with exposure length "+repr(args.exposure_length))
    c.start_dark()
    done = False
    loop_count = 0
    while done == False:
        time.sleep(1)
        state = c.get_state()
        if( (loop_count % 10) == 0):
            print ("Exposure In Progress:" + repr(state.exposure_in_progress)+ ".")
            print ("Exposure State:" + ExposureState._VALUES_TO_NAMES[state.exposure_state] + ".")
            print ("Elapsed Exposure Length:" + repr(state.elapsed_exposure_length)+ " ms.")
            print ("Remaining Exposure Length:" + repr(state.remaining_exposure_length)+ " ms.")
        done = state.exposure_in_progress == False
    filename = c.get_last_image_filename()
    print ("Dark Image "+repr(i)+": "+filename)
