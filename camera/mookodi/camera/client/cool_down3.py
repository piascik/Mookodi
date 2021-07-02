#!/usr/bin/env python3
"""
Command line tool to tell MookodiCameraServer to start cooling down the camera.
"""
from mookodi.camera.client.client import Client
# Create client
c= Client()
c.cool_down()
