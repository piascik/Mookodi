#!/usr/bin/env python3
import argparse
import time
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import Gain
from mookodi.camera.client.camera_interface.ttypes import ReadoutSpeed
from mookodi.camera.client.camera_interface.ttypes import ExposureState

def do_biases(exposure_count = 1):
    print ("Doing " + repr(exposure_count) + " biases.")
    c.start_multbias(exposure_count)
    done = False
    while done == False:
        state = c.get_state()
        print ("Exposure In Progress:" + repr(state.exposure_in_progress)+ ".")
        print ("Exposure State:" + repr(state.exposure_state)+ ".")
        print ("Exposure Count:" + repr(state.exposure_count)+ ".")
        print ("Exposure Index:" + repr(state.exposure_index)+ ".")
        time.sleep(1)
        done = state.exposure_in_progress == False
    filename_list = c.get_image_filenames()
    filename_count = len(filename_list)
    print ("There are " + repr(filename_count) + " Image Filenames:")
    for s in filename_list:
        print ("Image Filename:" + s)

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
    print ("Set readout_speed to :" + repr(readout_speed) + ".")
    c.set_readout_speed(readout_speed)
    for gain in gain_list:
        c.set_gain(gain)
        print ("Set gain to :" + repr(gain) + ".")
        for bin in bin_list:
            print ("Set binning to :(" + repr(bin) + " , " + repr(bin) + ").")
            c.set_binning(bin,bin)
            print ("Doing " + repr(exposure_count) + " biases with readout speed: " + repr(readout_speed) + ", gain: " + repr(gain) + ", binning: " + repr(bin) + ".")
            do_multbias(exposure_count)
