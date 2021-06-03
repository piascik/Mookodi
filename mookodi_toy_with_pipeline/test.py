#!/usr/bin/env python

import time
import logging as log
logger = log.getLogger(__name__)

from TopLevelController import TopLevelController

tlc = TopLevelController()

# Assume that magic pixels come from an MKD config
magicX = 400
magicY = 300

print(f"MKD will take an exposure.")
print(f"MKD will define the names and pass both to reduce_ccd_image().")
raw_filename = "testdata/image_random.fits"
reduced_filename = "testdata/image_reduced.fits"

print(f"Reduce the image from {raw_filename} to {reduced_filename} using reduce_ccd_image()")
erstat = tlc.mookodi.reduction.reduce_ccd_image(raw_filename,reduced_filename)
print(f"After image CCD reduction, erstat = {erstat}")
print(f"After image CCD reduction, erstat = {tlc.mookodi.reduction.erstat}")


# The coordinating layer handles all the iteration. All that is demonstrated here
# is make a call to aquire_wcs() or aquire_brightest() to get the offsets that are
# needed by the TCS. Maybe those method names should be changed? Calling these functions
# 'acquire' seems like an overstatement.
print(f"MKD will iterate, taking images and calling acquire_brightest() or acquire_wcs()")

print(f"Call acquire_brightest(). Target is brightest object found within 50pix if pixel {magicX},{magicY}")
tlc.mookodi.acquisition.acquire_brightest(reduced_filename, magicX, magicY, 50)
print(f"X,Y offsets for TCS are {tlc.mookodi.acquisition.offset_x},{tlc.mookodi.acquisition.offset_y}")

print(f"Call acquire_wcs(). Target at RA,dec = 12.123, -34.012 to be moved to pixel {magicX},{magicY}. SkyPA is 0.")
tlc.mookodi.acquisition.acquire_wcs(reduced_filename, 12.123, -34.012, 0, magicX, magicY)
print(f"RA,dec offsets for TCS are {tlc.mookodi.acquisition.offset_ra},{tlc.mookodi.acquisition.offset_dec}")

print(f"After acquire_XXX() call, erstat = {tlc.mookodi.acquisition.erstat}")


# Observe spectrum
print(f"MKD will take the spectral science exposure.")
print(f"MKD will define the names and pass both to reduce_ccd_spectrum().")
raw_filename = "testdata/spec_random.fits"
reduced_filename = "testdata/spec_reduced.fits"


print(f"Reduce the spectrum from {raw_filename} to {reduced_filename} using reduce_ccd_spectrum()")
tlc.mookodi.reduction.reduce_ccd_spectrum(raw_filename,reduced_filename)
print(f"After spectral CCD reduction, erstat = {tlc.mookodi.reduction.erstat}")

last_acq_filename = "acq_image_3.fits"
print(f"Run ASPIRED on {reduced_filename} using star at {magicX},{magicY} in {last_acq_filename} for the flux calibration")
tlc.mookodi.reduction.extract_spectrum(reduced_filename,last_acq_filename, magicX, magicY)


print("Done")

