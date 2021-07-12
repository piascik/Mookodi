#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to take a bias frame.
The previously configured readout speed, gain, window and binning are used. The command returns after
MookodiCameraServer has started to acquire the bias, use ./get_state3.py to query MookodiCameraServer to see
when it has finished.


./start_bias3.py

Parameters:
"""
import argparse
from mookodi.camera.client.client import Client

# Create client and start multbias
c= Client()
c.start_bias()
#done = False
#while done == False:
#    time.sleep(1)
#    state = c.get_state()
#    done = state.exposure_in_progress == False
#filename = c.get_last_image_filename()
#print ("Image: "+filename)
