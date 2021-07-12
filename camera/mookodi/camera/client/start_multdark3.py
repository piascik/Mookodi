#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to start taking a series of dark frames.
The previously configured readout speed, gain, window and binning are used. The command returns after
MookodiCameraServer has started to acquire dark frames, use ./get_state3.py to query MookodiCameraServer to see
when it has finished.

./start_multdark3.py <exposure count> <exposure length>

Parameters:
<exposure count> specifies the number of dark frames to acquire.
<exposure length> specifies the length of each dark frame in milliseconds.
"""
import argparse
from mookodi.camera.client.client import Client


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of dark frames to take")
parser.add_argument("exposure_length", type=int,help="The length of each dark frame in milliseconds")
args = parser.parse_args()

# Create client and start multdark
c= Client()
c.set_exposure_length(args.exposure_length)
for(i=0; i<args.exposure_count; i++):
    c.start_dark()
    done = False
    while done == False:
        time.sleep(1)
        state = c.get_state()
        done = state.exposure_in_progress == False
    filename = c.get_last_image_filename()
    print ("Dark Image "+repr(i)+": "+filename)
