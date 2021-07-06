#!/usr/bin/env python3
"""
Command line tool to take a series of exposures to test the MookodiCameraServer.
"""
import argparse
import time
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import Gain
from mookodi.camera.client.camera_interface.ttypes import ReadoutSpeed
from mookodi.camera.client.camera_interface.ttypes import ExposureState

def do_multrun(exposure_count = 1, exposure_length=1000):
    """
    Function to perform an exposure.

    Parameters:
    exposure_count The number of exposures to perform.
    exposure_length The length of each exposure in milliseconds.
    """
    print ("Doing a multrun of " + repr(exposure_count) + " of length " + repr(exposure_length) + "ms.")
    c.start_multrun(exposure_count,exposure_length)
    done = False
    while done == False:
        state = c.get_state()
        print ("Exposure In Progress:" + repr(state.exposure_in_progress)+ ".")
        print ("Exposure State:" + repr(state.exposure_state)+ ".")
        print ("Exposure Count:" + repr(state.exposure_count)+ ".")
        print ("Exposure Index:" + repr(state.exposure_index)+ ".")
        print ("Elapsed Exposure Length:" + repr(state.elapsed_exposure_length)+ " ms.")
        print ("Remaining Exposure Length:" + repr(state.remaining_exposure_length)+ " ms.")
        time.sleep(1)
        last_exposure = ( state.exposure_index == ( state.exposure_count - 1 ) )
        finished_exposure = ( state.remaining_exposure_length <= 0 )
        is_idle = ( state.exposure_state == ExposureState.IDLE )
        done = state.exposure_in_progress == False
    filename_list = c.get_image_filenames()
    filename_count = len(filename_list)
    print ("There are " + repr(filename_count) + " Image Filenames:")
    for s in filename_list:
        print ("Image Filename:" + s)
    print ("Multrun finished.")

# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("main_loop_count", type=int,help="The number of times round the main loop")
args = parser.parse_args()

c= Client()
# Cool down
print ("Cooling Down the camera.")
c.cool_down()
c.clear_fits_headers()
c.clear_window()
bin_list = [1, 2, 4]
bin_x_list = [1, 2, 4]
bin_y_list = [1, 2, 4]
readout_speed_list = [ ReadoutSpeed.SLOW, ReadoutSpeed.FAST ]
gain_list = [ Gain.ONE, Gain.TWO, Gain.FOUR ]

#exposure_length_list = [ 10, 100, 1000, 10000, 60000, 300000 ]
exposure_length_list = [ 10, 100, 1000, 10000 ]
for main_loop_index in range(0,args.main_loop_count):
    print ("Main loop:" + repr(main_loop_index) + " of " + repr(args.main_loop_count) +".")
    c.set_readout_speed(ReadoutSpeed.FAST)
    c.set_gain(Gain.ONE)
    for bin in bin_list:
        print ("Set binning to :(" + repr(bin) + " , " + repr(bin) + ").")
        c.set_binning(bin,bin)
        for exposure_length in exposure_length_list:
            print ("Doing a multrun of 1 of length " + repr(exposure_length) + "ms.")
            do_multrun(1,exposure_length)

