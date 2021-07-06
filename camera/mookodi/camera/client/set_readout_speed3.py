#!/usr/bin/env python3
"""
Command line tool to set the camera readout speed in MookodiCameraServer.

set_readout_speed3.py <speed>

where <speed> is one of "SLOW" or "FAST".
"""
import argparse
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import ReadoutSpeed


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("readout_speed", help="How quickly to read out the CCD, one of SLOW or FAST")
args = parser.parse_args()

# Create client and set parameters
c= Client()
if args.readout_speed == 'SLOW':
    readout_speed = ReadoutSpeed.SLOW
elif args.readout_speed == 'FAST':
    readout_speed = ReadoutSpeed.FAST
else:
    raise Exception('Illegal readout speed ' + args.readout_speed + ' specified.')
c.set_readout_speed(readout_speed)

