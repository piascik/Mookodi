#!/usr/bin/env python3
"""
Command line tool to tell the camera controlled by MookodiCameraServer to read out a sub-image of the full frame.

set_window3.py <x start> <y start> <x end> <y end>

Where <x start> <y start> <x end> <y end> are all inclusive pixel positions between 0 and the size of the sensor (1024).
"""
import argparse
from mookodi.camera.client.client import Client


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("x_start", type=int,help="The start of the window in x")
parser.add_argument("y_start", type=int,help="The start of the window in y")
parser.add_argument("x_end", type=int,help="The end of the window in x")
parser.add_argument("y_end", type=int,help="The end of the window in y")
args = parser.parse_args()

# Create client and set binning
c= Client()
c.set_window(args.x_start,args.y_start,args.x_end,args.y_end)

