/* ccd_exposure.h */
#ifndef CCD_EXPOSURE_H
#define CCD_EXPOSURE_H
/**
 * @file
 * @brief ccd_exposure.h contains the externally declared API for performing an exposure with the CCD Controller.
 * @author Chris Mottram
 * @version $Id$
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _POSIX_SOURCE
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_C_SOURCE 199309L
#endif

#include <time.h>
#include "ccd_fits_header.h"
#include "ccd_general.h"

/* enums */
/**
 * Return value from CCD_Exposure_Get_Status. 
 * <ul>
 * <li>CCD_EXPOSURE_STATUS_NONE means the library is not currently performing an exposure.
 * <li>CCD_EXPOSURE_STATUS_WAIT_START means the library is waiting for the correct moment to open the shutter.
 * <li>CCD_EXPOSURE_STATUS_EXPOSE means the library is currently performing an exposure.
 * <li>CCD_EXPOSURE_STATUS_READOUT means the library is currently reading out data from the ccd.
 * </ul>
 * @see CCD_Exposure_Get_Status
 */
enum CCD_EXPOSURE_STATUS
{
	CCD_EXPOSURE_STATUS_NONE,CCD_EXPOSURE_STATUS_WAIT_START,
	CCD_EXPOSURE_STATUS_EXPOSE,CCD_EXPOSURE_STATUS_READOUT
};

/**
 * Macro to check whether the exposure status is a legal value.
 * @see #CCD_EXPOSURE_STATUS
 */
#define CCD_EXPOSURE_IS_STATUS(status)	(((status) == CCD_EXPOSURE_STATUS_NONE)|| \
        ((status) == CCD_EXPOSURE_STATUS_WAIT_START)||((status) == CCD_EXPOSURE_STATUS_EXPOSE)|| \
        ((status) == CCD_EXPOSURE_STATUS_READOUT)

extern void CCD_Exposure_Initialise(void);
extern int CCD_Exposure_Expose(int open_shutter,struct timespec start_time,int exposure_length,
			       void *buffer,size_t buffer_length);
extern int CCD_Exposure_Bias(void *buffer,size_t buffer_length);
extern int CCD_Exposure_Abort(void);
extern enum CCD_EXPOSURE_STATUS CCD_Exposure_Status_Get(void);
extern int CCD_Exposure_Start_Time_Get(struct timespec *start_time);
extern int CCD_Exposure_Accumulation_Get(void);
extern int CCD_Exposure_Series_Get(void);
extern int CCD_Exposure_Index_Get(void);
extern int CCD_Exposure_Count_Get(void);
extern int CCD_Exposure_Length_Get(void);
extern int CCD_Exposure_Save(char *filename,void *buffer,size_t buffer_length,int ncols,int nrows,
			     struct Fits_Header_Struct header);
extern int CCD_Exposure_Get_Error_Number(void);
extern void CCD_Exposure_Error(void);
extern void CCD_Exposure_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
