#!/usr/bin/env python

import time
import logging as log
logger = log.getLogger(__name__)


from TopLevelController import TopLevelController

tlc = TopLevelController()

raw_filename = "testdata/image_random.fits"
print(f"Take an exposure called {raw_filename}")

reduced_filename = "testdata/image_reduced.fits"
print(f"Reduce the image from {raw_filename} to {reduced_filename} using reduce_image()")
tlc.mookodi.reduction.reduce_image(raw_filename,reduced_filename)
print(f"After image CCD reduction, erstat = {tlc.mookodi.reduction.erstat}")

# Run acquisition 
# The coordinating layer handles all the iteration. All this is demonstrated here
# is make a call to aquire_wcs() or aquire_brightest() to get the offsets that are
# needed by the TCS. Maybe those method names should be changed?

# Target is brightest object found within 50pix if pixel 400,300
#tlc.mookodi.acquisition.acquire_brightest(reduced_filename, 400, 300, 50)
#print(f"X,Y offsets for TCS are {tlc.mookodi.acquisition.offset_x},{tlc.mookodi.acquisition.offset_y}")
# Target at RA,dec = 12.123, -34.012 to be moved to pixel 400,300
tlc.mookodi.acquisition.acquire_wcs(reduced_filename, 12.123, -34.012, 400, 300)
print(f"RA,dec offsets for TCS are {tlc.mookodi.acquisition.offset_ra},{tlc.mookodi.acquisition.offset_dec}")
print(f"After acquire_???(), erstat = {tlc.mookodi.acquisition.erstat}")

#tlc.mookodi.acquisition.__init__
#print(f"X,Y offsets for TCS are {tlc.mookodi.acquisition.offset_x},{tlc.mookodi.acquisition.offset_y}")


# Observe spectrum

raw_filename = "testdata/spec_random.fits"
print(f"Take an exposure called {raw_filename}")

reduced_filename = "testdata/spec_reduced.fits"
print(f"Reduce the spectrum from {raw_filename} to {reduced_filename} using reduce_spectrum()")
erstat = tlc.mookodi.reduction.reduce_spectrum(raw_filename,reduced_filename)
print(f"After spectral CCD reduction, erstat = {erstat}")
print(f"After spectral CCD reduction, erstat = {tlc.mookodi.reduction.erstat}")



# Position the star on the slit (blocks till telescope is done moving)
#print("Positioning star on slit")
#tlc.mookodi.position_star_on_slit()
#print("Done positioning star on slit")

# Gather fits info and pass it to the camera
#fits_info = tlc.mookodi.gather_fits_info()
#tlc.mookodi.camera.set_fits_info(fits_info)

# Take an exposure
#print("Taking an exposure")
#tlc.mookodi.camera.start_exposure()

#while tlc.mookodi.camera.state.exposure_state != 0:
#    time.sleep(1)

#print("Retrieving exposure")
#print(f"Last image was {tlc.mookodi.camera.get_last_image()}")
print("Done")

