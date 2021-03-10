#!/usr/bin/env python
import argparse
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import ExposureType

# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_type", help="One of: EXPOSURE | ACQUIRE | ARC | SKYFLAT | STANDARD | LAMPFLAT")
parser.add_argument("exposure_count", type=int,help="The number of frames to take")
parser.add_argument("exposure_length", type=int,help="The length of each frame in milliseconds")
args = parser.parse_args()

# Create client and start multrun
c= Client()
exptype = ExposureType.EXPOSURE
if args.exposure_type == 'EXPOSURE':
    exptype = ExposureType.EXPOSURE
elif args.exposure_type == 'ACQUIRE':
    exptype = ExposureType.ACQUIRE
elif args.exposure_type == 'ARC':
    exptype = ExposureType.ARC
elif args.exposure_type == 'SKYFLAT':
    exptype = ExposureType.SKYFLAT
elif args.exposure_type == 'STANDARD':
    exptype = ExposureType.STANDARD
elif args.exposure_type == 'LAMPFLAT':
    exptype = ExposureType.LAMPFLAT
else:
    raise Exception('Illegal type ' + args.exposure_type + ' specified.')
c.start_multrun(exptype,args.exposure_count,args.exposure_length)
