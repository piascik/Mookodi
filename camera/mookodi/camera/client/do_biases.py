#!/usr/bin/env python3
import argparse
import sys
import time
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import Gain
from mookodi.camera.client.camera_interface.ttypes import ReadoutSpeed
from mookodi.camera.client.camera_interface.ttypes import ExposureState

def do_biases(exposure_count = 1):
    print ("Doing " + repr(exposure_count) + " biases.")
    c.start_multbias(exposure_count)
    done = False
    loop_count = 0
    while done == False:
        state = c.get_state()
        if( (loop_count % 10) == 0):
            print ("Exposure In Progress:" + repr(state.exposure_in_progress)+ ".")
            print ("Exposure State:" + ExposureState._VALUES_TO_NAMES[state.exposure_state] + ".")
            print ("Exposure Count:" + repr(state.exposure_count)+ ".")
            print ("Exposure Index:" + repr(state.exposure_index)+ ".")
        time.sleep(1)
        done = state.exposure_in_progress == False
        loop_count += 1
    filename_list = c.get_image_filenames()
    filename_count = len(filename_list)
    print ("There are " + repr(filename_count) + " Image Filenames:")
#    for s in filename_list:
#        print ("Image Filename:" + s)

# parse command line arguments
#parser = argparse.ArgumentParser()
#parser.add_argument("main_loop_count", type=int,help="The number of times round the main loop")
#args = parser.parse_args()

c= Client()
# Cool down
print ("Cooling Down the camera.")
c.cool_down()
c.clear_fits_headers()
c.clear_window()
bin_list = [1, 2, 4]
readout_speed_list = [ ReadoutSpeed.SLOW, ReadoutSpeed.FAST ]
gain_list = [ Gain.ONE, Gain.TWO, Gain.FOUR ]
exposure_count = 10
for readout_speed in readout_speed_list:
    print ("Set readout speed to :" + ReadoutSpeed._VALUES_TO_NAMES[readout_speed] + ".")
    c.set_readout_speed(readout_speed)
    for gain in gain_list:
        c.set_gain(gain)
        print ("Set gain to :" + Gain._VALUES_TO_NAMES[gain] + ".")
        for bin in bin_list:
            print ("Set binning to :(" + repr(bin) + " , " + repr(bin) + ").")
            c.set_binning(bin,bin)
            print ("Doing " + repr(exposure_count) + " biases with readout speed: " + ReadoutSpeed._VALUES_TO_NAMES[readout_speed] + ", gain: " + Gain._VALUES_TO_NAMES[gain] + ", binning: " + repr(bin) + ".")
            do_biases(exposure_count)
            print ("Finished " + repr(exposure_count) + " biases with readout speed: " + ReadoutSpeed._VALUES_TO_NAMES[readout_speed] + ", gain: " + Gain._VALUES_TO_NAMES[gain] + ", binning: " + repr(bin) + ".")
            filename_list = c.get_image_filenames()
            filename_count = len(filename_list)
            print ("There are " + repr(filename_count) + " Image Filenames:")
            for s in filename_list:
                print ("Image Filename:" + s)
            sys.stdout.flush()
            
