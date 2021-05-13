#!/usr/bin/env python

import argparse
import sys

from lesedi.sdk.connector import Connector

# BEfore test: original focus = 0.7748
START_FOCUS = 0.07748
STEPSIZE = 0.0005
NSTEPS = 1

INSTRUMENT_HOST = "10.2.2.88"
TCS_HOST = "10.2.2.31"

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description=__doc__, prog='test_autofocus')

    parser.add_argument('--tcs_host', default=TCS_HOST,
                        help='The remote TCS host name')
    parser.add_argument('--instrument_host', default=INSTRUMENT_HOST,
                        help='The remote instrument host name')
    parser.add_argument('--start', default=START_FOCUS,
                        type=float,
                        help='The starting focus setting')
    parser.add_argument('--stepsize', default=STEPSIZE,
                        type=float,
                        help='The focus stepsize')
    parser.add_argument('--nsteps', default=NSTEPS,
                        type=int,                        
                        help='The number of focus steps')

    arguments = parser.parse_args()
    print("Testing autofocus")
    try:
        connector = Connector(instrumenthost=arguments.instrument_host,
                              telescopehost=arguments.tcs_host)
    except Exception as e:
        print("Exception getting connector: {}".format(e))
        sys.exit()

    print("Calling autofocus")
    try:
        connector.telescope.autofocus(start=arguments.start,
                                      stepsize=arguments.stepsize,
                                      nsteps = arguments.nsteps)
    except Exception as e:
        print("Problem calling autofocus: {}".format(e))
        sys.exit(1)
