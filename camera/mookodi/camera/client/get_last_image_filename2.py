#!/usr/bin/env python
from mookodi.camera.client.client import Client

# Create client
c= Client()
s = c.get_last_image_filename()
print ("Last Image Filename:" + s)
