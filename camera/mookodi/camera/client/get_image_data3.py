#!/usr/bin/env python3
from PIL import Image
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
    # Mode: 'L' 8 bit, 'I;16' 16-bit unsigned, 'I' 32 bit signed
    output_image = Image.new(mode='I', size=(x_size, y_size)) 
    for x in range(x_size):
        for y in range(y_size):
            pixel_value = image_data.data[(y*x_size)+x]
            #print ( "x " + repr(x) + "y " + repr(y) + " pixel value " + repr(pixel_value))
            # pixel_value is 0..65535
            # rescale to 0..255?
            pixel_value = int(pixel_value * 255 / 65536)
            #print ( "x " + repr(x) + "y " + repr(y) + " pixel value " + repr(pixel_value))
            output_image.putpixel((x,y), pixel_value)
    output_image.show()
else:
    print ("There is no image data.")
