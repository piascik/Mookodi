from FilterClient import FilterClient
from SpectrographClient import SpectrographClient
from CameraClient import CameraClient
from ReductionController import ReductionController
from AcquisitionController import AcquisitionController

from time import sleep
import logging as log

class MookodiController(object):

    def __init__(self, top_level_controller=None):
        self.top_level_controller = top_level_controller
        self.camera = CameraClient()
        self.filter = FilterClient()
        self.spectrograph = SpectrographClient()
        self.reduction = ReductionController()
        self.acquisition = AcquisitionController()

    def set_hot_pixel(self, x, y):
        '''The coordinates of the "hot pixel"'''
        self.hot_x = x
        self.hot_y = y
    
    def position_star_on_slit(self):
        '''Find the offset required, and nudge the telescope to that position'''
        # Acquire an image
        self.camera.start_continuous()
        while not self.camera.state.new_image:
            print("Waiting for new image")
            sleep(5) # Wait until there is a new image
        image = self.camera.get_last_image()
        # Analyse the image and get the offset required
        print("Analysing image")
        # For now, just move the telescope a bit
        offset = (-17.53, 18.23)
        print("Moving telescope by offset {}".format(offset))
        if offset[0] < 0:
            # Direction = 'W' = 0
            self.top_level_controller.telescope.tcsclient.move(0, offset[0])
        elif offset[0] > 0:
            # Direction = 'E' = 1
            self.top_level_controller.telescope.tcsclient.move(1, offset[0])
        if offset[1] < 0:
            # Direction = 'N' = 2
            self.top_level_controller.telescope.tcsclient.move(2, offset[1])
        elif offset[1] > 0:
            # Direction = 'E' = 1
            self.top_level_controller.telescope.tcsclient.move(3, offset[1])
            
        # Wait till the telescope is done moving
        sleep(1)
        print("Waiiting for telescope to be done moving")
        while not self.top_level_controller.telescope.is_done():
            sleep(5)
        
    
    def gather_fits_info(self):
        fits_info = []
        # Get the fits info for the components directly under the
        # control of the instrument
        fits_info.extend(self.filter.get_fits_info())
        fits_info.extend(self.spectrograph.get_fits_info())
        fits_info.extend(self.camera.get_fits_info())
        # Also the telescope fits info
        fits_info.extend(self.top_level_controller.telescope.get_fits_info())
        # And the environmental conditions
        # fits_info.extend(self.top_level_controller.weather.get_fits_info())
        return fits_info
