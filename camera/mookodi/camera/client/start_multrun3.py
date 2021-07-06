#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to start taking a series of exposures.
The previously configured readout speed, gain, window and binning are used. The command returns after
MookodiCameraServer has started to acquire the exposures, use ./get_state3.py to query MookodiCameraServer to see
when it has finished.

./start_multrun3.py <exposure count> <exposure length>

Parameters:
<exposure count> specifies the number of exposures to acquire.
<exposure length> specifies the length of each exposure in milliseconds.
"""
import argparse
from mookodi.camera.client.client import Client

# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of frames to take")
parser.add_argument("exposure_length", type=int,help="The length of each frame in milliseconds")
args = parser.parse_args()

# Create client and start multrun
c= Client()
c.start_multrun(args.exposure_count,args.exposure_length)
