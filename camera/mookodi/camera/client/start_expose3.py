#!/usr/bin/env python3
import argparse
from mookodi.camera.client.client import Client

# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_length", type=int,help="The length of the exposure in milliseconds")
parser.add_argument("--save_image", default=False, action='store_true',help="Save the acquired image in a FITS file.")
args = parser.parse_args()

# Create client and start expose
c= Client()
c.start_expose(args.exposure_length,args.save_image)
