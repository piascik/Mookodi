from FitsInfoType import FitsInfoType

class State:
    def __init__(self):
        self.slit_width = 0

class SpectrographClient:

    def __init__(self):
        self.state = State()
        self.state.grism_pos = True
        self.state.slit_pos = True
        self.state.mirror_pos = False
        self.state.arc_state = False
        self.state.slit_width

    def state(self):
        return self.state
    
    def slit_width(self, slit_width):
        self.state.slit_width = slit_width

    def grism_pos(self, pos):
        """Set the grism position (in or out)"""
        self.state.grism_pos = pos
    
    def slit_pos(self, pos):
        """Set the slit position (in or out)"""
        self.state.slit_pos = pos
    
    def mirror_pos(self, pos):
        """Set the mirror position (in or out)"""
        self.state.mirror_pos = pos
        
    def arc_state(self, state):
        """Set the arc state (on or off)"""
        self.state.arc_state = state

    def get_fits_info(self):
        fits_info = []
        fits_info.append(FitsInfoType("SLIT_WID",
                                      "float",
                                      self.state.slit_width,
                                      "The slit width"))
        return fits_info
