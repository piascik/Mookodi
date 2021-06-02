"""Top level controller for the Mookodi project"""

from MookodiController import MookodiController
#from lesedi.sdk.telescope import Telescope
#from weather.sdk.client import WeatherClient

class TopLevelController(object):
    def __init__(self):
        self.mookodi = MookodiController(top_level_controller=self)
        self.telescope = "fred" 
        #self.telescope = Telescope(bridge=self, host="10.2.2.31") # Use for the actual telescope
        #self.weather = WeatherClient(tcp_host = "10.2.2.31") 

