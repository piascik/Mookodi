#!/usr/bin/env python3
"""
Command line tool to clear the list of FITS headers maintained in the MookodiCameraServer.
"""
from mookodi.camera.client.client import Client
# Create client
c= Client()
c.clear_fits_headers()
