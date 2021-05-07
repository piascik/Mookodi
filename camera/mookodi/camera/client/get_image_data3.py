#!/usr/bin/env python3
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import ImageData

# Create client
c = Client()
image_data = c.get_image_data()
x_size = image_data.x_size
y_size = image_data.y_size
print ("Image data has size: X = " + repr(image_data.x_size) + ", Y = "+ repr(image_data.y_size) + ".")
if image_data.data is not None:
    print ("Image data is not None.")
    print ("Minimum pixel value is " + repr(min(image_data.data)))
    print ("Maximum pixel value is " + repr(max(image_data.data)))
    print ("Mean pixel value is " + repr(sum(image_data.data)/len(image_data.data)))
else:
    print ("There is no image data.")
