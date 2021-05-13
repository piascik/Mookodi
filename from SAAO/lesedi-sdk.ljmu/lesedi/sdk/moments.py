from math import sqrt

def moments(image_raw, imx, jmx, spix, nrows, ncols, background=50):
    """Finds the centroid and FWHM of star

    background is a measure of the background - unless supplied just use a value of e.g. 50
    imx and jmx are the pixel coordinates of the center of the selection box containing the star
    spix is the side size/2 of the box - needs to be big enough to have some sky around the star
    image_raw is the array containing the image, flattened to 1d, with nrows rows and ncols columns

fwhm is the calculated fwhm
    xc and yc is the precise position of the star --- sub pixel level

all other variables - zx, zxx etc are z(intensity) and x,y, histograms from which the fwhm gaussian parameters are calculated

    """

    n = 0
    zt = 0.0
    zx = 0.0
    zy = 0.0
    zxx = 0.0
    zyy = 0.0

    # Calculate zmax, the brightest pixle in the box
    zmax = -1
    for j in range(jmx-spix, jmx+spix + 1):
       y = j - jmx ;
       for i in range(imx-spix,  imx+spix+1):
           z = image_raw[j][i]
           if z > zmax:
               zmax = z

    zm = 0.3*(zmax - background)

    for j  in range(jmx-spix, jmx+spix + 1):
       y = j - jmx ;
       for i in range(imx-spix,  imx+spix+1):
           x = i - imx
           z = image_raw[j][i] - background
           if ( z > zm):
               n+=1
               zt += z
               zx += z*x
               zy += z*y
               zxx += z*x*x
               zyy += z*y*y

    zx = zx/zt
    zxx = zxx/zt
    zy = zy/zt
    zyy = zyy/zt
    zxx = zxx - zx*zx
    zyy = zyy - zy*zy

    if ( (zxx>0) and (zyy>0) ):
        dy = sqrt(zxx)
        dx = sqrt(zyy)
        fwhm = 1.175*(dx + dy)
    else:
        fwhm = -1.0

    return fwhm
