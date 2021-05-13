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
    
