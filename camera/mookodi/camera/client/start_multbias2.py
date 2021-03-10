#!/usr/bin/env python
import argparse
from mookodi.camera.client.client import Client


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("exposure_count", type=int,help="The number of bias frames to take")
args = parser.parse_args()

# Create client and start multbias
c= Client()
c.start_multbias(args.exposure_count)
