!/usr/bin/env python3

import sys
import random
import time
import logging
from astropy.io import fits

sys.path.append("../gen-py")
sys.path.append("/home/dev/src/Mookodi/camera")

# Camera
from mookodi.camera.client.client import Client
from mookodi.camera.client.camera_interface.ttypes import ExposureState
from mookodi.camera.client.camera_interface.ttypes import ReadoutSpeed
from mookodi.camera.client.camera_interface.ttypes import Gain
from mookodi.camera.client.camera_interface.ttypes import FitsCardType

# Instrument
from instsrv          import InstSrv
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol  import TBinaryProtocol
from instsrv.ttypes   import DeployState
from instsrv.ttypes   import DetectorConfig
from instsrv.ttypes   import FilterID
from instsrv.ttypes   import FilterState

trans = TSocket.TSocket("localhost", 9090)
trans = TTransport.TBufferedTransport(trans)
proto = TBinaryProtocol.TBinaryProtocol(trans)

inst = InstSrv.Client(proto)

trans.open()

#Use default logging for now
#logging.basicConfig(filename='/tmp/mookodi.log', encoding='utf-8', level=logging.DEBUG)

# INSTRUMENT
# Bitmask controlling instrument state via PIO
PIO_OUT_SPECTRUM = 0x60
PIO_INP_SPECTRUM = 0x1A
PIO_INP_SPECTRUM = 0x1A
PIO_OUT_IMAGING  = 0x00
PIO_INP_IMAGING  = 0x15
PIO_OUT_SPEC_ARC = -16
PIO_INP_SPEC_ARC = 0x2A

# Default timeout in seconds
PIO_TMO = 10

# Connect to camera
Cam = Client()

# Ensure CCD is at target temp.
def cam_cool( Cam, Temp, Tmo ):
  Cam.cool_down()

  while True:
    CamState = Cam.get_state()
    if CamState.exposure_state == ExposureState.IDLE:
      return

    if CamState.ccd_temperature < Temp:
      return
    logging.warning("Cooling CCD. T=" + refr(CamState.ccd_temperature) + " C")

    Tmo += 1
    if Tmo <= 0:
       logging.error("Cooling CCD. Timeout at T==" + refr(CamState.ccd_temperature) + " C")
       return
    time.sleep(1)

def obs_acquire():
# Acquisition defaults
  CamCoolTemp = -50.0
  CamCoolTime = 40
  CamBinX     = int(1)
  CamBinY     = int(1)
  CamExpCount = 1
  CamExpTime  = 1
  CamGain     = Gain.ONE
  CamReadout  = ReadoutSpeed.FAST
  CamXMin     = int(1)
  CamXMax     = int(1024)
  CamYMin     = int(1)
  CamYMax     = int(1024)

# Cool camera
  cam_cool( Cam, CamCoolTemp, CamCoolTime )

# Configure image settings
  Cam.set_binning( CamBinX, CamBinY )
  Cam.set_gain   ( CamGain )
  Cam.set_window ( int(CamXMin), int(CamYMin), int(CamXMax), int(CamYMax) )

  StatePIO     = inst.CtrlPIO( PIO_OUT_IMAGING, PIO_INP_IMAGING, DeployState.ENA, PIO_TMO )
  StateFilters = inst.CtrlFilters( FilterState.POS0, FilterState.POS0, PIO_TMO )

#  DEBUG
#  print('IMAGING Mode=' + DeployState._VALUES_TO_NAMES[StatePIO] )
#  print('FILTER State='+repr(StateFilters))

# Set FITS header values
  Cam.clear_fits_headers()
  Cam.add_fits_header( 'OBSTYPE', FitsCardType.STRING, 'ACQUIRE', 'Image type' ) 

# Start exposure and wait for completion
  Cam.start_expose( CamExpCount, CamExpTime )
  while True:
    CamState = Cam.get_state()
    logging.warning("Cam State=" + str(CamState.exposure_state))
    logging.warning("Cam State=" + str(ExposureState._VALUES_TO_NAMES[CamState.exposure_state]))
    if CamState.exposure_state == ExposureState.IDLE:
      break;
    time.sleep(1)

  FitsFile = Cam.get_last_image_filename()
  CamXMin     = int(1)
  CamXMax     = int(1024)
  CamYMin     = int(1)
  CamYMax     = int(1024)

# Cool camera
  cam_cool( Cam, CamCoolTemp, CamCoolTime )

# Configure image settings
  Cam.set_binning( CamBinX, CamBinY )
  Cam.set_gain   ( CamGain )
  Cam.set_window ( int(CamXMin), int(CamYMin), int(CamXMax), int(CamYMax) )

  StatePIO     = inst.CtrlPIO( PIO_OUT_IMAGING, PIO_INP_IMAGING, DeployState.ENA, PIO_TMO )
  StateFilters = inst.CtrlFilters( FilterState.POS0, FilterState.POS0, PIO_TMO )

#  DEBUG
#  print('IMAGING Mode=' + DeployState._VALUES_TO_NAMES[StatePIO] )
#  print('FILTER State='+repr(StateFilters))

# Set FITS header values
  Cam.clear_fits_headers()
  Cam.add_fits_header( 'OBSTYPE', FitsCardType.STRING, 'ACQUIRE', 'Image type' ) 

# Start exposure and wait for completion
  Cam.start_expose( CamExpCount, CamExpTime )
  while True:
    CamState = Cam.get_state()
    logging.warning("Cam State=" + str(CamState.exposure_state))
    logging.warning("Cam State=" + str(ExposureState._VALUES_TO_NAMES[CamState.exposure_state]))
    if CamState.exposure_state == ExposureState.IDLE:
      break;
    time.sleep(1)

  FitsFile = Cam.get_last_image_filename()

  CamXMin     = int(1)
  CamXMax     = int(1024)
  CamYMin     = int(1)
  CamYMax     = int(1024)

# Cool camera
  cam_cool( Cam, CamCoolTemp, CamCoolTime )

# Configure image settings
  Cam.set_binning( CamBinX, CamBinY )
  Cam.set_gain   ( CamGain )
  Cam.set_window ( int(CamXMin), int(CamYMin), int(CamXMax), int(CamYMax) )

  StatePIO     = inst.CtrlPIO( PIO_OUT_IMAGING, PIO_INP_IMAGING, DeployState.ENA, PIO_TMO )
  StateFilters = inst.CtrlFilters( FilterState.POS0, FilterState.POS0, PIO_TMO )

#  DEBUG
#  print('IMAGING Mode=' + DeployState._VALUES_TO_NAMES[StatePIO] )
#  print('FILTER State='+repr(StateFilters))


# Set FITS header values
  Cam.clear_fits_headers()
  Cam.add_fits_header( 'OBSTYPE', FitsCardType.STRING, 'ACQUIRE', 'Image type' ) 

# Start exposure and wait for completion
  Cam.start_expose( CamExpCount, CamExpTime )
  while True:
    CamState = Cam.get_state()
    logging.warning("Cam State=" + str(CamState.exposure_state))
    logging.warning("Cam State=" + str(ExposureState._VALUES_TO_NAMES[CamState.exposure_state]))
    if CamState.exposure_state == ExposureState.IDLE:
      break;
    time.sleep(1)

  FitsFile = Cam.get_last_image_filename()

# DEBUG
  print ( FitsFile )

  
# Main call
obs_acquire()

# Tidy up
trans.close()
