import configparser
import logging as log
from astropy.io import fits

class ReductionController(object):

    def __init__(self):
        '''Read the config file and load the calibrations into memory.
        Reads in bias,dark,flat for both the imaging anD spectral modes and holds them separately.'''

        self.erstat = 0

        self.config = configparser.ConfigParser()
        self.config.read('../config/mkd.cfg')

        #for key in self.config['Reduction']:
        #  print(key," is ",self.config['Reduction'][key])

	# Read bias, dark, flat, filenames.  Hold in memory for immediate use
        # For Bias and Flat we should never need the header. For Dark we do need the header.
	# We do not care what detector and instrument configs were used. That is the operator's 
        # responsibility. We use whatever they give us in mkd.cfg.

        # image relates to reducing acquisition images
        print(f"Initialise image pipeline. Read bias frame {self.config['Reduction']['reduction.image.bias']}")
        hdu = fits.open(self.config['Reduction']['reduction.image.bias'])
        self.image_bias_data = hdu[0].data
        hdu.close()

        print(f"Initialise image pipeline. Read dark frame {self.config['Reduction']['reduction.image.dark']}")
        hdu = fits.open(self.config['Reduction']['reduction.image.dark'])
        # Read any one of XPOSURE, EXPOSURE, EXPTIME
        self.image_dark_exposure = hdu[0].header['EXPOSURE']
        self.image_dark_data = hdu[0].data
        hdu.close()

        print(f"Initialise image pipeline. Read flat frame {self.config['Reduction']['reduction.image.flat']}")
        hdu = fits.open(self.config['Reduction']['reduction.image.flat'])
        self.image_flat_data = hdu[0].data
        hdu.close()

        # spectrum relates to reducing spectra
        print(f"Initialise spectral pipeline. Read bias frame {self.config['Reduction']['reduction.spectrum.bias']}")
        hdu = fits.open(self.config['Reduction']['reduction.spectrum.bias'])
        self.spectrum_bias_data = hdu[0].data
        hdu.close()

        print(f"Initialise spectral pipeline. Read dark frame {self.config['Reduction']['reduction.spectrum.dark']}")
        hdu = fits.open(self.config['Reduction']['reduction.spectrum.dark'])
        # Read any one of XPOSURE, EXPOSURE, EXPTIME
        self.spectrum_dark_exposure = hdu[0].header['EXPOSURE']
        self.spectrum_dark_data = hdu[0].data
        hdu.close()

        print(f"Initialise spectral pipeline. Read flat frame {self.config['Reduction']['reduction.spectrum.flat']}")
        hdu = fits.open(self.config['Reduction']['reduction.spectrum.flat'])
        self.spectrum_flat_data = hdu[0].data
        hdu.close()



    def reduce_ccd_spectrum(self, raw_filename, reduced_filename):
        '''Basic CCD reductions for spectral images.

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
        images, already normalised to unity counts. This flat maps pixel-to-pixel senstivity variations
        only. It does not contain correction for slit width irregularities, optical vignetting or the
        instrumental spectral response function. i.e., it will normally be a flat, largely featureless
        array with all values near unity. All those other effects are corrected in the spectral 
        analysis pipeline, not here, which is purely CCD reduction.

        All three filenames specified in config/mkd.cfg.
        Since detector configuration may be different foe acquisition images and spectra, these
        calibrations are generally different form the bias, dark, flat for spectral reductions.
        '''

        self.erstat = 0

        # Read the raw FITS
        hdu = fits.open( raw_filename )
        raw_data = hdu[0].data
        headdata = hdu[0].header
        raw_exposure = hdu[0].header['EXPOSURE']
        hdu.close()

        # Subtract bias image as is from science image. No scaling or configurable options.
        # Subtract dark image scaled by ratio of hte EXPOSURE times
        # Divide science image by the flatfield image as is. No scaling or configurable options.
        reduced_data = ( raw_data - self.spectrum_bias_data - (raw_exposure/self.spectrum_dark_exposure)*self.spectrum_dark_data ) / self.spectrum_flat_data

        # Write reduced image to disk. Output filename - TBD
        #
        # return error state to calling process.
        headdata['L1'] = 'True'
        headdata['L1BIAS'] = self.config['Reduction']['reduction.image.bias']
        headdata['L1DARK'] = self.config['Reduction']['reduction.image.dark']
        headdata['L1FLAT'] = self.config['Reduction']['reduction.image.flat']
        fits.writeto(reduced_filename, reduced_data, header=headdata, overwrite=True)

        return self.erstat



    def reduce_ccd_image(self, raw_filename, reduced_filename):
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

        self.erstat = 0

        # Read the raw FITS
        hdu = fits.open( raw_filename )
        raw_data = hdu[0].data
        headdata = hdu[0].header
        raw_exposure = hdu[0].header['EXPOSURE']
        hdu.close()

        # Subtract bias image as is from science image. No scaling or configurable options.
        # Subtract dark image scaled by ratio of hte EXPOSURE times
        # Divide science image by the flatfield image as is. No scaling or configurable options.
        reduced_data = ( raw_data - self.image_bias_data - (raw_exposure/self.image_dark_exposure)*self.image_dark_data ) / self.image_flat_data

        # Write reduced image to disk. Output filename - TBD
        #
        # return error state to calling process.
        headdata['L1'] = 'True'
        headdata['L1BIAS'] = self.config['Reduction']['reduction.image.bias']
        headdata['L1DARK'] = self.config['Reduction']['reduction.image.dark']
        headdata['L1FLAT'] = self.config['Reduction']['reduction.image.flat']
        fits.writeto(reduced_filename, reduced_data, header=headdata, overwrite=True)

        return self.erstat



    def extract_spectrum(self, spec_filename, acq_filename, magic_pix_x, magic_pix_y):
        '''Uses ASPIRED to create an extracted, calibrated spectrum.
        I anticipate this can be designed so that nearly everything it needs comes from the FITS header, not passed
        in a function call. It will need all the normal things that you would expect to be in a FITS header.

        Parameters
          spec_filename: The spectrum.
          acq_filename: The acquisition image to be used for flux calibration.
          magic_pix_x, magic_pix_y: X,Y pixel coordinates of target in ac_filename.
        [Alternatively acq_filename could be in the FITS header of spec_filename and magic_pix_x, magic_pix_y could
        be in the FITS header of acq_filename.]          
        '''
        self.erstat = 0
        return self.erstat


