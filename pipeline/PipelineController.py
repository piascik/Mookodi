import configparser
import logging as log
from astropy.io import fits

class ReductionController(object):

    def __init__(self):
        '''Read the config file and load the calibrations into memory'''

        self.config = configparser.ConfigParser()
        self.config.read('config/mkd.cfg')

        #for key in self.config['Reduction']:
        #  print(key," is ",self.config['Reduction'][key])

	# Read bias, dark, flat, filenames.  Hold in memory for immediate use
        # For Bias and Flat we should never need the header. For Dark we do need the header.
	# We do not care what detector and instrument configs were used. That is the operator's 
        # responsibility. We use whatever they give us.

        hdu = fits.open(self.config['Reduction']['reduction.image.bias'])
        self.bias_data = hdu[0].data
        hdu.close()

        hdu = fits.open(self.config['Reduction']['reduction.image.dark'])
        # Read any one of XPOSURE, EXPOSURE, EXPTIME
        self.dark_exposure = hdu[0].header.['EXPOSURE']
        self.dark_data = hdu[0].data
        hdu.close()

        hdu = fits.open(self.config['Reduction']['reduction.image.flat'])
        self.flat_data = hdu[0].data
        hdu.close()



    def reduce_image(raw_filename=None, reduced_filename=None, error=0):
        '''Basic CCD reductions for acquisition images.

        Applies a simple bias, dark, flat using calibration files that the user has to provide 
        on disk. We are not creating an entire infrastructure to generate and maintain calibration 
        reference frames like we have in the LT DpRT.  

        It is not a generic reduction package. It will only handle one binning, one amp config, 
        one filter, all matched to how the acquisition images are obtained. It would be possible to 
        change (for example) the binning the acquisition uses by installing new bias, dark and flat 
        for use, but that set then gets used on all future observations. It is not configurable on a 
        target-by-target basis. It is the user’s responsibility to ensure the bias, dark, flat they 
        have installed are appropriate to the observing mode used by the MKD.

        The only dynamically configurable option in the reduction pipeline is the acquisition image 
        integration time which is read from the FITS header. Note this grants some flexibility, but 
        not nearly as much as you might imagine since the depth of the acquisition image must be 
        matched to the astrometric catalogue.

        User provided prerequisites
        i) FITS of zero integration bias frame in same detector config as acquisition images. 
        ii) FITS of dark integration in same detector config as acquisition images with bias already 
        subtracted. Dark integration to be specified in FITS header (keyword EXPOSURE in seconds.)
        iii) FITS of flatfield image in same detector and instrument (i.e., filter) config as acquisition 
        images, already normalised to unity counts.
        All three filenames specified in config/mkd.cfg.
        Since detector configuration may be different foe acquisition images and spectra, these
        calibrations are generally different form the bias, dark, flat for spectral reductions.
        '''

        error = 0

        # Read the raw FITS
        hdu = fits.open( raw_filename )
        raw_data = hdu[0].data
        headdata = hdu[0].header
        raw_exposure = hdu[0].header.['EXPOSURE']
        hdu.close()

        # Subtract bias image as is from science image. No scaling or configurable options.
        # Subtract dark image scaled by ratio of hte EXPOSURE times
        # Divide science image by the flatfield image as is. No scaling or configurable options.
        reduced_data = ( raw_data - XXX.bias_data - (raw_exposure/XXX.dark_exposure)*XXX.dark_data ) / XXX.flat_data

        # Write reduced image to disk. Output filename - TBD
        #
        # return error state to calling process.
        headdata['L1'] = 'True'
        headdata['L1BIAS'] = XXX.config['Reduction']['reduction.image.bias']
        headdata['L1DARK'] = XXX.config['Reduction']['reduction.image.dark']
        headdata['L1FLAT'] = XXX.config['Reduction']['reduction.image.flat']
        fits.writeto(reduced_filename, reduced_data, header=headdata, overwrite=True)

        return error





class AcquisitionController(object):
    def __init__(self):
        '''Read the config file and set variables'''
        self.config = configparser.ConfigParser()
        self.config.read('config/mkd.cfg')
        for key in self.config['Acquisition']:
          print(key)


    def aquire_wcs(filename=None, target_ra, target_dec, target_skypa, magic_pix_x, magic_pix_y, offset_ra=None, offset_dec=None, error=0):
        '''Performs a WCS fit and returns to MKD a telescope delRA, delDEC in seconds of arc 
        required to place the specified RA,Dec on the specified pixel.
        Coordinates are all specified in a sky reference frame (RA, Dec, arcsec etc)
        '''
        return error

    def aquire_brightest(filename=None, target_ra, target_dec, radius=100, magic_pix_x, magic_pix_y, offset_x=None, offset_y=None, error=0):
        '''Runs source detection on image. The target is assumed to be the brightest source detected 
        within a specified radius of the specified magic pixel. Service returns to MKD the offset_x, offset_y
        in pixels required to place the assumed target on the specified pixel. 
        Coordinates are all specified in the instrument focal plane (X,Y, pixels etc).
        '''
        return error

    def aquire_wcs():




