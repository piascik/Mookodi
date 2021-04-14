#!/usr/bin/env python
import argparse
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import Gain


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("gain", help="The gain factor to use when reading out the detector, one of: ONE, TWO or FOUR.")
args = parser.parse_args()

# Create client and set parameters
c= Client()
if args.gain == 'ONE':
    gain = Gain.ONE
elif args.gain == 'TWO':
    gain = Gain.TWO
elif args.gain == 'FOUR':
    gain = Gain.FOUR
else:
    raise Exception('Illegal gain ' + args.gain + ' specified.')
c.set_gain(gain)

