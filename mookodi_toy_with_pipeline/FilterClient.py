from FitsInfoType import FitsInfoType

class State:
    def __init__(self):
        self.filter = -1

class FilterClient:

    def __init__(self):
        self.state = State()
        self.state.filter = 1
        
    def select_filter(self, filter):
        self.state.filter = filter
    
    def state(self):
        return self.state

    def get_fits_info(self):
        fits_info = []
        fits_info.append(FitsInfoType("FILTER", "string", self.state.filter, "The filter in use"))
        return fits_info
