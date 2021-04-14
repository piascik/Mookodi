#!/usr/bin/env python3
from mookodi.camera.client.client import Client

# Create client
c= Client()
l = c.get_image_filenames()
filename_count = len(l)
print ("There are " + repr(filename_count) + " Image Filenames:")
for s in l:
    print ("Image Filename:" + s)
