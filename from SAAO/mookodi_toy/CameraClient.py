# For now, this is just a dummy camera. Replace with a client
# communicating with the real camera or camera server

from FitsInfoType import FitsInfoType

class State:
    def __init__(self):
        self.exp_time = -1
        self.num_exposures = 1
        self.binx = 1
        self.biny = 1
        self.x1 = 0
        self.y1 = 0
        self.x2 = 1023
        self.y2 = 1023
        self.temperature = -50
        self.cooler_on = False
        self.new_image = False
        self.exposure_state = 0

class CameraClient:
    def __init__(self):
        self.state = State()
        
    def state(self):
        return self.state
        
    def start_exposure(self):
        self.state.new_image = True
    
    def start_continuous(self):
        self.state.new_image = True
    
    def stop(self):
        self.state.new_image = True
    
    def abort(self):
        self.state.new_image = False
    
    def get_last_image(self):
        self.state.new_image = False
        return []
    
    def exposure_time(self, exp_time):
        self.state.exp_time = exp_time
        
    def num_exposures(self, nexp):
        self.state.num_exposures = 1
    
    def set_binning(self, binx, biny):
        self.state.binx = 1
        self.state.biny = 1
        
    def set_region(self, x1, y1, x2, y2):
        self.state.x1 = x1
        self.state.y1 = y1
        self.state.x2 = x2
        self.state.y2 = y2
    
    def get_fits_info(self):
        """Called as part of the instrument's gather_fits_info call"""
        fits_info = []
        fits_info.append(FitsInfoType("BINX", "int", self.state.binx, "The X binning factor"))
        fits_info.append(FitsInfoType("BINY", "int", self.state.biny, "The Y binning factor"))
        fits_info.append(FitsInfoType("EXPTIME", "double", self.state.exp_time, "The exposure time"))
        fits_info.append(FitsInfoType("REGION_X1", "int", self.state.x1, "The ROI left coordinate"))
        fits_info.append(FitsInfoType("REGION_Y1", "int", self.state.y1, "The ROI bottom coordinate"))
        fits_info.append(FitsInfoType("REGION_X2", "int", self.state.x2, "The ROI right coordinate"))
        fits_info.append(FitsInfoType("REGION_Y2", "int", self.state.y2, "The ROI top coordinate"))
        return fits_info

    def set_fits_info(self, fits_info):
        """Set the FITS info.

        Called before start exposure, so that the FITS info can be
        incoporated into the resulting FITS file header.
        """
        self.fits_info = fits_info
    
    def set_temperature(self, temperature):
        self.state.temperature = temperature

    def cooler_on(self, cooler_on):
        self.state.cooler_on = cooler_on
        
