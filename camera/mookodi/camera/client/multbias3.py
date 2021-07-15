#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to take a series of bias frames.
The previously configured readout speed, gain, window and binning are used. The command loops over the exposure count,
calls start_bias() to start the camera taking each bias frame, 
and then uses get_state() to determine when the bias has been taken,
and uses get_last_image_filename() to retrieve the FITS image filename generated.
The command returns after MookodiCameraServer has finished taking the bias frames.

./multbias3.py <exposure count>

Parameters:
<exposure count> specifies the number of bias frames to acquire.
"""
import argparse
from mookodi.camera.client.client import Client


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of bias frames to take")
args = parser.parse_args()

# Create client and loop over start_bias, waiting for each to complete before starting the next
c= Client()
for(i=0; i<args.exposure_count; i++):
    c.start_bias()
    done = False
    while done == False:
        time.sleep(1)
        state = c.get_state()
        done = state.exposure_in_progress == False
    filename = c.get_last_image_filename()
    print ("Bias Image "+repr(i)+": "+filename)
