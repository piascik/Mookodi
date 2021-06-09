/* ccd_setup.h */

#ifndef CCD_SETUP_H
#define CCD_SETUP_H
/**
 * @file
 * @brief ccd_setup.h contains the externally declared API the routines that setup/configure the CCD and readout
 *        binning/sub-windows.
 * @author Chris Mottram
 * @version $Id$
 */

#ifdef __cplusplus
extern "C" {
#endif

/* structures */
/**
 * @struct CCD_Setup_Window_Struct
 * Structure holding position information for one window on the CCD.
 * @see #CCD_Setup_Dimensions
 */
struct CCD_Setup_Window_Struct
{
	/** The pixel number of the X start position of the window (upper left corner). */
	int X_Start;
	/** Y_Start The pixel number of the Y start position of the window (upper left corner). */
	int Y_Start;
	/** The pixel number of the X end position of the window (lower right corner). */
	int X_End;
	/** The pixel number of the Y end position of the window (lower right corner). */
	int Y_End;
};

extern int CCD_Setup_Startup(void);
extern int CCD_Setup_Shutdown(void);
extern int CCD_Setup_Config_Directory_Set(char *directory);
extern int CCD_Setup_Dimensions(int ncols,int nrows,int hbin,int vbin,int window_flags,
				struct CCD_Setup_Window_Struct window);
extern void CCD_Setup_Abort(void);
extern int CCD_Setup_Set_Flip_X(int flip_x);
extern int CCD_Setup_Set_Flip_Y(int flip_y);
extern int CCD_Setup_Get_Flip_X(void);
extern int CCD_Setup_Get_Flip_Y(void);
extern int CCD_Setup_Get_NCols(void);
extern int CCD_Setup_Get_NRows(void);
extern int CCD_Setup_Get_Bin_X(void);
extern int CCD_Setup_Get_Bin_Y(void);
extern int CCD_Setup_Is_Window(void);
extern int CCD_Setup_Set_HS_Speed(int hs_speed_index);
extern int CCD_Setup_Set_VS_Speed(int vs_speed_index);
extern int CCD_Setup_Set_VS_Amplitude(int vs_amplitude);
extern int CCD_Setup_Set_Pre_Amp_Gain(int pre_amp_gain_index);
extern int CCD_Setup_Get_Horizontal_Start(void);
extern int CCD_Setup_Get_Horizontal_End(void);
extern int CCD_Setup_Get_Vertical_Start(void);
extern int CCD_Setup_Get_Vertical_End(void);
extern int CCD_Setup_Get_Detector_Pixel_Count_X(void);
extern int CCD_Setup_Get_Detector_Pixel_Count_Y(void);
extern int CCD_Setup_Get_Camera_Head_Model_Name(char *name,int name_length);
extern int CCD_Setup_Get_Camera_Serial_Number(void);
extern float CCD_Setup_Get_HS_Speed(void);
extern int CCD_Setup_Get_HS_Speed_Index(void);
extern float CCD_Setup_Get_VS_Speed(void);
extern int CCD_Setup_Get_VS_Speed_Index(void);
extern int CCD_Setup_Get_VS_Amplitude(void);
extern float CCD_Setup_Get_Pre_Amp_Gain(void);
extern int CCD_Setup_Get_Pre_Amp_Gain_Index(void);
extern int CCD_Setup_Get_Buffer_Length(size_t *buffer_length);
extern int CCD_Setup_Allocate_Image_Buffer(void **buffer,size_t *buffer_length);
extern int CCD_Setup_Get_Error_Number(void);
extern void CCD_Setup_Error(void);
extern void CCD_Setup_Error_String(char *error_string);
#ifdef __cplusplus
}
#endif

#endif
