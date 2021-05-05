/* ccd_fits_header.h
** $Id$
*/
#ifndef CCD_FITS_HEADER_H
#define CCD_FITS_HEADER_H
/**
 * @file
 * @brief ccd_fits_header.h contains the externally declared API the FITS header handling routines.
 * @author Chris Mottram
 * @version $Id$
 */

#ifdef __cplusplus
extern "C" {
#endif

/* for struct timespec declaration */
#include <time.h>
/* for fitsfile declaration */
#include "fitsio.h"

/**
 * Structure defining the contents of a FITS header. Note the common basic FITS cards may not
 * be defined in this list.
 * @see #Fits_Header_Card_Struct
 */
struct Fits_Header_Struct
{
	/** The list of Fits_Header_Card_Struct, each element representing one keyword/value pair in the header. */
	struct Fits_Header_Card_Struct *Card_List;
	/** The number of cards in the (reallocatable) list. */
	int Card_Count;
	/** The amount of memory allocated in the Card_List pointer, in terms of number of cards. */
	int Allocated_Card_Count;
};

extern int CCD_Fits_Header_Initialise(struct Fits_Header_Struct *header);
extern int CCD_Fits_Header_Clear(struct Fits_Header_Struct *header);
extern int CCD_Fits_Header_Delete(struct Fits_Header_Struct *header,const char *keyword);
extern int CCD_Fits_Header_Add_String(struct Fits_Header_Struct *header,const char *keyword,const char *value,
				      const char *comment);
extern int CCD_Fits_Header_Add_Int(struct Fits_Header_Struct *header,const char *keyword,int value,
				   const char *comment);
extern int CCD_Fits_Header_Add_Float(struct Fits_Header_Struct *header,const char *keyword,double value,
				     const char *comment);
extern int CCD_Fits_Header_Add_Logical(struct Fits_Header_Struct *header,const char *keyword,int value,
				       const char *comment);
extern int CCD_Fits_Header_Add_Comment(struct Fits_Header_Struct *header,const char *keyword,const char *comment);
extern int CCD_Fits_Header_Add_Units(struct Fits_Header_Struct *header,const char *keyword,const char *units);
extern int CCD_Fits_Header_Free(struct Fits_Header_Struct *header);

extern int CCD_Fits_Header_Write_To_Fits(struct Fits_Header_Struct header,fitsfile *fits_fp);

extern void CCD_Fits_Header_TimeSpec_To_Date_String(struct timespec time,char *time_string);
extern void CCD_Fits_Header_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string);
extern void CCD_Fits_Header_TimeSpec_To_UtStart_String(struct timespec time,char *time_string);

extern int CCD_Fits_Header_Get_Error_Number(void);
extern void CCD_Fits_Header_Error(void);
extern void CCD_Fits_Header_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
