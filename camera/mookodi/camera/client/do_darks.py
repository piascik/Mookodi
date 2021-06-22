#!/usr/bin/env python3
import argparse
import time
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import Gain
from mookodi.camera.client.camera_interface.ttypes import ReadoutSpeed
from mookodi.camera.client.camera_interface.ttypes import ExposureState

def do_darks(exposure_count = 1, exposure_length=1000):
    print ("Doing " + repr(exposure_count) + " darks of exposure length " + repr(exposure_length) + "ms.")
    c.start_multdark(exposure_count,exposure_length)
    done = False
    loop_count = 0
    while done == False:
        state = c.get_state()
        if( (loop_count % 10) == 0):
            print ("Exposure In Progress:" + repr(state.exposure_in_progress)+ ".")
            print ("Exposure State:" + ExposureState._VALUES_TO_NAMES[state.exposure_state] + ".")
            print ("Exposure Count:" + repr(state.exposure_count)+ ".")
            print ("Exposure Index:" + repr(state.exposure_index)+ ".")
            print ("Elapsed Exposure Length:" + repr(state.elapsed_exposure_length)+ " ms.")
            print ("Remaining Exposure Length:" + repr(state.remaining_exposure_length)+ " ms.")
        time.sleep(1)
        done = state.exposure_in_progress == False
        loop_count ++
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
exposure_length_list = [ 10, 100, 1000, 10000, 60000, 300000, 600000 ]
for readout_speed in readout_speed_list:
    print ("Set readout speed to :" + ReadoutSpeed._VALUES_TO_NAMES[readout_speed] + ".")
    c.set_readout_speed(readout_speed)
    for gain in gain_list:
        c.set_gain(gain)
        print ("Set gain to :" + Gain._VALUES_TO_NAMES[gain] + ".")
        for bin in bin_list:
            print ("Set binning to :(" + repr(bin) + " , " + repr(bin) + ").")
            c.set_binning(bin,bin)
            for exposure_length in exposure_length_list:
                print ("Doing " + repr(exposure_count) + " darks of length " + repr(exposure_length) + " ms with readout speed: " + ReadoutSpeed._VALUES_TO_NAMES[readout_speed] + ", gain: " + Gain._VALUES_TO_NAMES[gain] + ", binning: " + repr(bin) + ".")
                do_darks(exposure_count,exposure_length)
                print ("Finished " + repr(exposure_count) + " darks of length " + repr(exposure_length) + " ms with readout speed: " + ReadoutSpeed._VALUES_TO_NAMES[readout_speed] + ", gain: " + Gain._VALUES_TO_NAMES[gain] + ", binning: " + repr(bin) + ".")
                filename_list = c.get_image_filenames()
                filename_count = len(filename_list)
                print ("There are " + repr(filename_count) + " Image Filenames:")
                for s in filename_list:
                    print ("Image Filename:" + s)
