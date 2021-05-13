#!/usr/bin/env python

import time
import logging as log
logger = log.getLogger(__name__)


from TopLevelController import TopLevelController

tlc = TopLevelController()

# Get the telescope state
tel = tlc.telescope
telclient = tel.tcsclient
telstate = tel.tcsclient.get_state()
print("Telescope state:")
for k,v in telstate.__dict__.items():
    print("{}: {}".format(k,v))

# Move the telescope to some star
tlc.telescope.tcsclient.move(0, 20.0)

# Initialise instrument
tlc.mookodi.set_hot_pixel(10, 20)
print(f"Hot pixel is now {tlc.mookodi.hot_x}, {tlc.mookodi.hot_y}")

# Position the star on the slit (blocks till telescope is done moving)
print("Positioning star on slit")
tlc.mookodi.position_star_on_slit()
print("Done positioning star on slit")

# Gather fits info and pass it to the camera
fits_info = tlc.mookodi.gather_fits_info()
tlc.mookodi.camera.set_fits_info(fits_info)

# Take an exposure
print("Taking an exposure")
tlc.mookodi.camera.start_exposure()

while tlc.mookodi.camera.state.exposure_state != 0:
    time.sleep(1)

print("Retrieving exposure")
print(f"Last image was {tlc.mookodi.camera.get_last_image()}")
print("Done")

