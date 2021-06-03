import configparser
import logging as log

class AcquisitionController(object):

    def __init__(self):
        '''Read the config file and set variables'''
        self.clear()
        # Read config file. 
        self.config = configparser.ConfigParser()
        self.config.read('../config/mkd.cfg')
        #for key in self.config['Acquisition']:
        #  print(key)

    def clear(self):
        '''Clear the error state and offset variables.
        Optionally allows caller to discard offsets after use if they wish.
        Call to clear() is never required by AcquisitionController().'''
        self.erstat = None
        self.offset_ra = None
        self.offset_dec = None
        self.offset_x = None
        self.offset_y = None


    def acquire_wcs(self, filename, target_ra, target_dec, target_skypa, magic_pix_x, magic_pix_y):
        '''Performs a WCS fit.
        After calling this method, the offsets in seconds of arc on the sky required to place the specified RA,Dec 
        on the specified pixel are available in AcquisitionController.offset_ra and AcquisitionController.offset_dec.
        Coordinates are all specified in a sky reference frame (RA, Dec, arcsec etc)
        '''
        self.erstat = 0
        self.offset_ra = 1.2
        self.offset_dec = -3.1
        return self.erstat

    def acquire_brightest(self, filename, magic_pix_x, magic_pix_y, radius):
        '''Runs source detection on image. The target is assumed to be the brightest source detected 
        within the specified radius of the specified magic pixel. 
        After calling this method, the offsets in pixels required to place the assumed target 
        on the specified pixel are available from AcquisitionController.offset_x and AcquisitionController.offset_y. 
        Coordinates are all specified in the instrument focal plane (X,Y, pixels etc).
        '''
        self.erstat = 0
        self.offset_x = 10.1
        self.offset_y = 15.4
        return self.erstat




