#!/usr/bin/env python3
import argparse
from mookodi.camera.client.client import Client


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of dark frames to take")
parser.add_argument("exposure_length", type=int,help="The length of each dark frame in milliseconds")
args = parser.parse_args()

# Create client and start multdark
c= Client()
c.start_multdark(args.exposure_count,args.exposure_length)
