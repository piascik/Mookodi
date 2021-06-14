#!/usr/bin/env python3
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import ExposureState
from mookodi.camera.client.camera_interface.ttypes import ReadoutSpeed
from mookodi.camera.client.camera_interface.ttypes import Gain

# Create client
c= Client()
s = c.get_state()
print ("X Bin:" + repr(s.xbin))
print ("Y Bin:" + repr(s.ybin))
print ("Use Window:" + repr(s.use_window))
if(s.use_window):
    print ("Window:"+ repr(s.window))
print ("Exposure Length:" + repr(s.exposure_length)+ " ms.")
print ("Elapsed Exposure Length:" + repr(s.elapsed_exposure_length)+ " ms.")
print ("Remaining Exposure Length:" + repr(s.remaining_exposure_length)+ " ms.")
print ("Exposure In Progress:" +repr(s.exposure_in_progress)+".")
print ("Exposure State:"+ ExposureState._VALUES_TO_NAMES[s.exposure_state])
print ("Exposure State:"+ repr(s.exposure_state))
print ("Exposure Count:" + repr(s.exposure_count)+ ".")
print ("Exposure Index:" + repr(s.exposure_index)+ ".")
print ("CCD Temperature:" + repr(s.ccd_temperature)+ " C.")
print ("Readout Speed:"+ ReadoutSpeed._VALUES_TO_NAMES[s.readout_speed])
print ("Gain:"+ Gain._VALUES_TO_NAMES[s.gain])
