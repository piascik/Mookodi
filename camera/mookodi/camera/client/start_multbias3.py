#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to start taking a series of bias frames.
The previously configured readout speed, gain, window and binning are used. The command returns after
MookodiCameraServer has started to acquire biases, use ./get_state3.py to query MookodiCameraServer to see
when it has finished.

./start_multbias3.py <exposure count>

Parameters:
<exposure count> specifies the number of bias frames to acquire.
"""
import argparse
from mookodi.camera.client.client import Client


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of bias frames to take")
args = parser.parse_args()

# Create client and start multbias
c= Client()
c.start_multbias(args.exposure_count)
