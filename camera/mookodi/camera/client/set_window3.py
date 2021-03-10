#!/usr/bin/env python3
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

