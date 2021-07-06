#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to start warming up the CCD to ambient. 
The command returns after the warm up has been started, use ./get_state3.py to query MookodiCameraServer to see
the current temperature.
"""
from mookodi.camera.client.client import Client
# Create client
c= Client()
c.warm_up()
