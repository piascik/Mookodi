/* ccd_fits_filename.h
** $Id$
*/
#ifndef CCD_FITS_FILENAME_H
#define CCD_FITS_FILENAME_H
/**
 * @file
 * @brief ccd_fits_filename.h contains the externally declared API the FITS filename generation routines.
 * @author Chris Mottram
 * @version $Id$
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Default instrument code, used as first part of an SAAO FITS image filename.
 */
#define CCD_FITS_FILENAME_DEFAULT_INSTRUMENT_CODE ("MKD")
/**
 * Default data directory root, used as first part of an SAAO FITS image data directory.
 */
#define CCD_FITS_FILENAME_DEFAULT_DATA_DIR_ROOT ("/data")
/**
 * Default telescope name, used as part of an SAAO FITS image data directory.
 */
#define CCD_FITS_FILENAME_DEFAULT_DATA_DIR_TELESCOPE ("lesedi")
/**
 * Default instrument namee, used as part of an SAAO FITS image data directory.
 */
#define CCD_FITS_FILENAME_DEFAULT_DATA_DIR_INSTRUMENT ("mkd")

extern int CCD_Fits_Filename_Initialise(char *instrument_code,char *data_dir_root,char *data_dir_telescope,
					char *data_dir_instrument);
extern int CCD_Fits_Filename_Next_Run(void);
extern int CCD_Fits_Filename_Get_Filename(char *filename,int filename_length);
extern int CCD_Fits_Filename_Run_Get(void);
extern int CCD_Fits_Filename_Lock(char *filename);
extern int CCD_Fits_Filename_UnLock(char *filename);
extern int CCD_Fits_Filename_Get_Error_Number(void);
extern void CCD_Fits_Filename_Error(void);
extern void CCD_Fits_Filename_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
