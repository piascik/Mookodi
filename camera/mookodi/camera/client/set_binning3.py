#!/usr/bin/env python3
"""
Command line tool to set the camera x and y binning in the MookodiCameraServer.

set_binning3.py <xbin> <ybin>
"""
import argparse
from mookodi.camera.client.client import Client


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("xbin", type=int,help="The chip binning in the x dimension to use")
parser.add_argument("ybin", type=int,help="The chip binning in the y dimension to use")
args = parser.parse_args()

# Create client and set binning
c= Client()
c.set_binning(args.xbin,args.ybin)

