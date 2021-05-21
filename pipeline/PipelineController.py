import configparser
import logging as log

class ReductionController(object):

    def __init__(self):
        '''Read the config file and load the calibrations into memory'''
	# Read bias, dark, flat, filenames
	# Hold in memory for immedaite use

    def reduce_image(raw_filename="undefined", reduced_filename="undefined", error=0):
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
        # Tasks to perform
        #
        # Read bias, dark, flat, filenames
        #
        # Subtract bias image as is from science image. No scaling or configurable options.
        #
        # Bias subtracted dark image, scaled by integration time from science image. 
        # Keyword EXPOSURE is read from both dark and science frame to define the scaling.
        #
        # Divide science image by the flatfield image as is. No scaling or configurable options.
        #
        # Write reduced image to disk. Output filename - TBD
        #
        # return error state to calling process.

        return error


