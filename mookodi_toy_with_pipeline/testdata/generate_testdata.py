import numpy as np
from astropy.io import fits

# No good reason for these to be BITPIX -64. 
# I can make them way smaller using astype=

# Random noise, 100 ± 10 
nrand = np.random.normal(100, 10, size=(1024, 1024))       
hdu = fits.PrimaryHDU(nrand)
hdu.header['EXPOSURE'] = 10
hdu.writeto('image_random.fits',overwrite='True')
hdu.writeto('spec_random.fits',overwrite='True')

# normal 10 ± 1
nbias = np.random.normal(0, 1, size=(1024, 1024)) + 10     
hdu = fits.PrimaryHDU(nbias)
hdu.writeto('image_zero.fits',overwrite='True')
hdu.writeto('spec_zero.fits',overwrite='True')

# random between 0 and 10. Never -ve
ndark = np.random.uniform(0, 10, size=(1024, 1024))         
hdu = fits.PrimaryHDU(ndark)
hdu.header['EXPOSURE'] = 100
hdu.writeto('image_dark.fits',overwrite='True')
hdu.writeto('spec_dark.fits',overwrite='True')

# normal 1 ± 0.01
nflat = np.random.normal(0, 0.01, size=(1024, 1024)) + 1   
hdu = fits.PrimaryHDU(nflat)
hdu.writeto('image_flat.fits',overwrite='True')
hdu.writeto('spec_flat.fits',overwrite='True')


#import matplotlib.pyplot as plt
#mean = [0, 0]
#cov = [[1, 0], [0, 1]]
#x, y = np.random.multivariate_normal(mean, cov, 5000).T
#plt.plot(x, y, 'x')
#plt.axis('equal')
#plt.show()

# from astropy.modeling import models
# models.Gaussian2D.evaluate(0,0,10,0,0,3,3,0)
