#!/usr/bin/env python3
"""
Command line tool to clear any set sub-window readouts in the MookodiCameraServer.
"""
from mookodi.camera.client.client import Client
# Create client
c= Client()
c.clear_window()
