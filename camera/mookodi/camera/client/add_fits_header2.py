#!/usr/bin/env python
import argparse
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import FitsCardType


# parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("keyword", help="The FITS header card keyword.")
parser.add_argument("type", help="What type of FITS header this is: one of: STRING INTEGER DOUBLE")
parser.add_argument("value", help="The value of the FITS header.")
parser.add_argument("comment", help="A commment to put against this FITS header.")
args = parser.parse_args()

# Create client and start multrun
c= Client()
valtype = FitsCardType.INTEGER
if args.type == 'STRING':
    valtype = FitsCardType.STRING
elif args.type == 'DOUBLE':
    valtype = FitsCardType.FLOAT
elif args.type == 'INTEGER':
    valtype = FitsCardType.INTEGER
else:
    raise Exception('Illegal type ' + args.type + ' specified.')
c.add_fits_header(args.keyword,valtype,args.value,args.comment)
