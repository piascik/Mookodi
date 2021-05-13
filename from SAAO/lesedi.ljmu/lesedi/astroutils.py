from astropy.coordinates import Angle

def validate_alt(alt):
    try:
        float(alt)
    except ValueError as e:
        raise ValueError("Altitude {} is not valid: {}".format(alt, e))
    if alt < 0:
        raise ValueError("Altitude {} is not valid. Must be greater than 0".format(alt))
    if alt > 90:
        raise ValueError("Altitude {} is not valid. Must be less than 90".format(alt))
    
def validate_az(az):
    try:
        float(az)
    except ValueError as e:
        raise ValueError("Azimuth {} is not valid: {}".format(az, e))
    if az < 0:
        raise ValueError("Azimuth {} is not valid. Must be greater than 0".format(az))
    if az > 360:
        raise ValueError("Azimuth {} is not valid. Must be less than 360".format(az))
    
def hms2dd(hms):
    (h, m, s) = hms.split()
    print("RA values are {} {} {}".format(h, m, s))
    d  = Angle('{} {} {} hours'.format(h, m, s)).degree
    return d

def dms2dd(dms):
    (d,m,s) = dms.split()
    return Angle('{} {} {} degrees'.format(d,m,s)).degree

def dd2hms(dd):
    return Angle('{}d'.format(dd)).hms
        
def dd2dms(dd):
    return Angle('{}d'.format(dd)).dms
        
def is_set(x, n):
    """Returns true if the nth bit of x is set, else false"""
    return x & 2**n != 0
