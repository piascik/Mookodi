#!/usr/bin/env python3
"""
Command line tool to abort an ongoing exposure running on the  MookodiCameraServer.
"""
from mookodi.camera.client.client import Client
# Create client
c= Client()
c.abort_exposure()
