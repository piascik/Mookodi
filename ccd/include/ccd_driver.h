/* ccd_driver.h */
#ifndef CCD_DRIVER_H
#define CCD_DRIVER_H

#include "mookodi_ccd_driver.h"

extern int CCD_Driver_Register(struct Mookodi_CCD_Driver_Function_Struct *functions);
extern int CCD_Driver_Get_Error_Number(void);
extern void CCD_Driver_Error(void);
extern void CCD_Driver_Error_String(char *error_string);

#endif
