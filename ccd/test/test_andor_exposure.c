/* test_andor_exposure.c
 * $Header$
 * Test exposure code.
 */
/**
 * @file
 * @brief Low level test of the Andor library, using direct andor calls to do an exposure.
 * @author $Author$
 * @version $Id$
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "atmcdLXd.h"
#include "fitsio.h"

/* hash definitions */
/**
 * Truth value.
 */
#define TRUE 1
/**
 * Falsity value.
 */
#define FALSE 0
/**
 * Default temperature to set the CCD to.
 */
#define DEFAULT_TEMPERATURE	(-20.0)
/**
 * Default number of columns in the CCD.
 */
#define DEFAULT_SIZE_X		(1024)
/**
 * Default number of rows in the CCD.
 */
#define DEFAULT_SIZE_Y		(1024)
/**
 * Default number of pixels to readout.
 * @see #DEFAULT_SIZE_X
 * @see #DEFAULT_SIZE_Y
 */
#define DEFAULT_READOUT_PIXEL_COUNT  (DEFAULT_SIZE_X*DEFAULT_SIZE_Y)

/* enums */
/**
 * Enumeration determining which command this program executes. One of:
 * <ul>
 * <li>COMMAND_ID_NONE
 * <li>COMMAND_ID_DARK
 * <li>COMMAND_ID_EXPOSURE
 * </ul>
 */
enum COMMAND_ID
{
	COMMAND_ID_NONE=0,COMMAND_ID_DARK,COMMAND_ID_EXPOSURE
};

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id$";
/**
 * How many Andor cameras are detected.
 */
static int Number_Of_Cameras = 1;
/**
 * Which camera to use.
 */
static int Selected_Camera = -1;
/**
 * Set temperature or not.
 */
static int Set_Temperature = FALSE;
/**
 * Temperature to set the CCD to.
 * @see #DEFAULT_TEMPERATURE
 */
static double Temperature = DEFAULT_TEMPERATURE;
/**
 * Window flags specifying which window to use.
 */
static int Window_Flags = 0;

/**
 * Which type ef exposure command to call.
 * @see #COMMAND_ID
 */
static enum COMMAND_ID Command = COMMAND_ID_NONE;
/**
 * If doing a dark or exposure, the exposure length in milliseconds.
 */
static int Exposure_Length = 0;
/**
 * Filename to store resultant fits image in. 
 */
static char *Fits_Filename = NULL;
/**
 * Filename for configuration directory. 
 */
static char *Config_Dir = "/usr/local/etc/andor";
/**
 * Binning parameter for SetImage. Defaults to 1.
 */
static int Bin_X = 1;
/**
 * Binning parameter for SetImage. Defaults to 1.
 */
static int Bin_Y = 1;
/**
 * Window parameter for SetImage. Defaults to 1.
 */
static int Horizontal_Start = 1;
/**
 * Window parameter for SetImage. Defaults to 1024.
 */
static int Horizontal_End = 1024;
/**
 * Window parameter for SetImage. Defaults to 1.
 */
static int Vertical_Start = 1;
/**
 * Window parameter for SetImage. Defaults to 1024.
 */
static int Vertical_End = 1024;
/**
 * Number of pixels in readout - defaults to 1024 x 1024.
 * @see #DEFAULT_READOUT_PIXEL_COUNT
 */
static int Readout_Pixel_Count = DEFAULT_READOUT_PIXEL_COUNT;
/**
 * Width of image - used for saving FITS images.
 */
static int Width = 1024;
/**
 * Height of image - used for saving FITS images.
 */
static int Height = 1024;

/* internal routines */
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);
static int Test_Save_Fits_Headers(int exposure_time,int ncols,int nrows,enum COMMAND_ID command,
				  float current_temperature,char *filename);
static void Test_Fits_Header_Error(int status);
static int Exposure_Save(char *filename,void *buffer,size_t buffer_length,int ncols,int nrows);
static void Exposure_Debug_Buffer(char *description,unsigned short *buffer,size_t buffer_length);
static int fexist(char *filename);

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive integer if it fails.
 */
int main(int argc, char *argv[])
{
	unsigned long andor_retval;
	unsigned short *image_buffer = NULL;
	size_t image_buffer_length = 0;
	char command_buffer[256];
	int i,retval,value,width,height,status,min_gain,max_gain;
	at_32 Camera_Handle;
	float current_temperature_f;

	GetAvailableCameras(&Number_Of_Cameras);
	fprintf(stdout,"Found %d cameras.\n",Number_Of_Cameras);
/* parse arguments */
	fprintf(stdout,"Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	if((Selected_Camera >= 0)&&(Selected_Camera <= Number_Of_Cameras))
	{
		fprintf(stdout,"GetCameraHandle(Selected_Camera=%d)\n",Selected_Camera);
		GetCameraHandle(Selected_Camera, &Camera_Handle);
		fprintf(stdout,"SetCurrentCamera(Camera_Handle=%d)\n",Camera_Handle);
		SetCurrentCamera(Camera_Handle);
	}
	fprintf(stdout,"Initialize(%s)\n",Config_Dir);
	andor_retval = Initialize(Config_Dir);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"Initialize failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"sleep(2)\n");
	sleep(2);
	fprintf(stdout,"SetReadMode(4)\n");
	andor_retval = SetReadMode(4);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"SetReadMode failed %lu.\n",andor_retval);
		return 3;
	}
	fprintf(stdout,"SetAcquisitionMode(1)\n");
	andor_retval = SetAcquisitionMode(1);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"SetAcquisitionMode failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"SetBaselineClamp(1)\n");
	andor_retval = SetBaselineClamp(1);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"SetBaselineClamp(1) failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"SetExposureTime(%.3f)\n",((float)Exposure_Length)/1000.0f);
	andor_retval = SetExposureTime(((float)Exposure_Length)/1000.0f);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"SetExposureTime failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"GetDetector()\n");
	GetDetector(&width, &height);
	fprintf(stdout,"GetDetector returned width %d height %d.\n",width,height);
	if(Command == COMMAND_ID_EXPOSURE)
	{
		fprintf(stdout,"SetShutter(1,0,50,50)\n");
		andor_retval = SetShutter(1,0,50,50);
		if(andor_retval!=DRV_SUCCESS)
		{
			fprintf(stderr,"SetShutter failed %lu.\n",andor_retval);
			return 2;
		}
	}
	else if(Command == COMMAND_ID_DARK)
	{
		fprintf(stdout,"SetShutter(0,0,50,50)\n");
		andor_retval = SetShutter(0,0,50,50);
		if(andor_retval!=DRV_SUCCESS)
		{
			fprintf(stderr,"SetShutter failed %lu.\n",andor_retval);
			return 2;
		}
	}
	if(Set_Temperature)
	{
		fprintf(stdout,"SetTemperature(%.2f)\n",Temperature);
		andor_retval = SetTemperature(Temperature);
		if(andor_retval != DRV_SUCCESS)
		{
			fprintf(stderr,"SetTemperature() failed %lu.\n",andor_retval);
			return 2;
		}
		fprintf(stdout,"CoolerON()\n");
		andor_retval = CoolerON();
		if(andor_retval != DRV_SUCCESS)
		{
			fprintf(stderr,"CoolerON() failed %lu.\n",andor_retval);
			return 2;
		}
		fprintf(stdout,"SetCoolerMode(1) (maintain temperature on shutdown)\n");
		andor_retval = SetCoolerMode(1);
		if(andor_retval != DRV_SUCCESS)
		{
			fprintf(stderr,"SetCoolerMode(1) failed %lu.\n",andor_retval);
			return 2;
		}
	}
	fprintf(stdout,"SetImage(bin x=%d,bin y=%d,hstart=%d,hend=%d,vstart=%d,vend=%d)\n",Bin_X,Bin_Y,
		Horizontal_Start,Horizontal_End,Vertical_Start,Vertical_End );
	andor_retval = SetImage(Bin_X,Bin_Y,Horizontal_Start,Horizontal_End,Vertical_Start,Vertical_End);
 	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"SetImage failed %lu.\n",andor_retval);
		return 2;
	}
	/* check CCD temperature */
	fprintf(stdout,"GetTemperatureF().\n");
 	andor_retval = GetTemperatureF(&current_temperature_f);
	fprintf(stdout,"Current CCD Temperature = %.2f.\n",current_temperature_f);
	switch(andor_retval)
	{
		case DRV_NOT_INITIALIZED:
			fprintf(stdout,"Andor library not initialised.\n");
			break;
		case DRV_ACQUIRING:
			fprintf(stdout,"Acquiring data.\n");
			break;
		case DRV_ERROR_ACK:
			fprintf(stdout,"ACK error.\n");
			break;
		case DRV_TEMP_OFF:
			fprintf(stdout,"Temperature is OFF.\n");
			break;
		case DRV_TEMP_STABILIZED:
			fprintf(stdout,"Temperature is STABILIZED.\n");
			break;
		case DRV_TEMP_NOT_STABILIZED:
			fprintf(stdout,"Temperature is NOT STABILIZED.\n");
			break;
		case DRV_TEMP_NOT_REACHED:
			fprintf(stdout,"Temperature is NOT REACHED.\n");
			break;
		case DRV_TEMP_DRIFT:
			fprintf(stdout,"Temperature is DRIFTing.\n");
			break;
		default:
			fprintf(stdout,"GetTemperatureF returned unknown temperature status %lu.\n",andor_retval);
	}

	/* if no exposure command, quit */
	if(Command == COMMAND_ID_NONE)
	{
		fprintf(stdout,"No exposure or dark command specified - stopping here.\n");
		fprintf(stdout,"ShutDown()\n");
		ShutDown();
		return 0;
	}
	/* allocate buffer for image */
	image_buffer = (unsigned short*)malloc(Readout_Pixel_Count*sizeof(unsigned short));
	if(image_buffer == NULL)
	{
		fprintf(stderr,"Failed to allocate image buffer of length %d.\n",Readout_Pixel_Count);
		return 4;
	}
	fprintf(stdout,"StartAcquisition()\n");
	andor_retval = StartAcquisition();
 	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"StartAcquisition failed %lu.\n",andor_retval);
		return 2;
	}
	GetStatus(&status);
	while(status==DRV_ACQUIRING)
	{
		GetStatus(&status);
	}
	fprintf(stdout,"GetAcquiredData16(%p,%d)\n",image_buffer,Readout_Pixel_Count);
	andor_retval = GetAcquiredData16(image_buffer,Readout_Pixel_Count);
 	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"GetAcquiredData16 failed %lu.\n",andor_retval);
		return 2;
	}
	if(Fits_Filename == NULL)
	{
		fprintf(stderr,"FITS filename not specified.\n");
		return 5;
	}
	if(!Test_Save_Fits_Headers(Exposure_Length,Width,Height,Command,current_temperature_f,Fits_Filename))
	{
		fprintf(stdout,"Saving FITS headers failed.\n");
		return 4;
	}
	retval = Exposure_Save(Fits_Filename,image_buffer,image_buffer_length,
			       Width,Height);
	if(retval == FALSE)
	{
		return 7;
	}
/* close  */
	fprintf(stdout,"ShutDown()\n");
	ShutDown();
	return 0;
}

/* -----------------------------------------------------------------------------
**      Internal routines
** ----------------------------------------------------------------------------- */
/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Test Andor Exposure:Help.\n");
	fprintf(stdout,"This program does an exposure using direct Andor library calls.\n");
	fprintf(stdout,"test_exposure \n");
	fprintf(stdout,"\t[-camera <n>]\n");
	fprintf(stdout,"\t[-co[nfig_dir] <directory>]\n");
	fprintf(stdout,"\t[-h[elp]]\n");
	fprintf(stdout,"\t[-temperature <temperature>]\n");
	fprintf(stdout,"\t[-w[idth] <no. of pixels>][-h[eight] <no. of pixels>]\n");
	fprintf(stdout,"\t[-xb[in] <binning factor>][-yb[in] <binning factor>]\n");
	fprintf(stdout,"\t[-xs|-xstart <number>][-ys|-ystart <number>]\n");
	fprintf(stdout,"\t[-xe|-xend <number>][-ye|-yend <number>]\n");
	fprintf(stdout,"\t[-rpc|-readout_pixel_count <number>]\n");
	fprintf(stdout,"\t[-f[its_filename] <filename>]\n");
	fprintf(stdout,"\t[-d[ark] <exposure length>][-e[xpose] <exposure length>]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t<filename> should be a valid filename.\n");
	fprintf(stdout,"\t<temperature> should be a valid double, a temperature in degrees Celcius.\n");
	fprintf(stdout,"\t<exposure length> is a positive integer in milliseconds.\n");
	fprintf(stdout,"\t<no. of pixels> and <binning factor> is a positive integer.\n");
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Help
 * @see #Selected_Camera
 * @see #Set_Temperature
 * @see #Temperature
 * @see #Width
 * @see #Height
 * @see #Bin_X
 * @see #Bin_Y
 * @see #Horizontal_Start
 * @see #Horizontal_End
 * @see #Vertical_Start
 * @see #Vertical_End
 * @see #Readout_Pixel_Count
 * @see #Command
 * @see #Exposure_Length
 * @see #Fits_Filename
 * @see #Config_Dir
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval,log_level;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-camera")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Selected_Camera);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing selected camera %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:camera requires a camera number.\n");
				return FALSE;
			}

		}
		else if((strcmp(argv[i],"-config_dir")==0)||(strcmp(argv[i],"-co")==0))
		{
			if((i+1)<argc)
			{
				Config_Dir = argv[i+1];
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:config dir name required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-dark")==0)||(strcmp(argv[i],"-d")==0))
		{
			Command = COMMAND_ID_DARK;
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Exposure_Length);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing exposure length %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:dark requires exposure length.\n");
				return FALSE;
			}

		}
		else if((strcmp(argv[i],"-expose")==0)||(strcmp(argv[i],"-e")==0))
		{
			Command = COMMAND_ID_EXPOSURE;
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Exposure_Length);
				if(retval != 1)
				{
					fprintf(stderr,
						"Parse_Arguments:Parsing exposure length %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:exposure requires exposure length.\n");
				return FALSE;
			}

		}
		else if((strcmp(argv[i],"-fits_filename")==0)||(strcmp(argv[i],"-f")==0))
		{
			if((i+1)<argc)
			{
				Fits_Filename = argv[i+1];
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:filename required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-height")==0)||(strcmp(argv[i],"-h")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Height);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Height %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:size required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-help")==0)||(strcmp(argv[i],"-h")==0))
		{
			Help();
			exit(0);
		}
		else if((strcmp(argv[i],"-readout_pixel_count")==0)||(strcmp(argv[i],"-rpc")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Readout_Pixel_Count);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing readout pixel count %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:number required for readout pixel count.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-temperature")==0)
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%lf",&Temperature);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing temperature %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				Set_Temperature = TRUE;
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:temperature required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-width")==0)||(strcmp(argv[i],"-w")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Width);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Width %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:size required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-xbin")==0)||(strcmp(argv[i],"-xb")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Bin_X);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing X Bin %s failed.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:bin required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-ybin")==0)||(strcmp(argv[i],"-yb")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Bin_Y);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Y Bin %s failed.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:bin required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-xstart")==0)||(strcmp(argv[i],"-xs")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Horizontal_Start);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing X start %s failed.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:number required for xstart.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-xend")==0)||(strcmp(argv[i],"-xe")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Horizontal_End);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing X end %s failed.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:number required for xend.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-ystart")==0)||(strcmp(argv[i],"-ys")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Vertical_Start);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Y start %s failed.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:number required for ystart.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-yend")==0)||(strcmp(argv[i],"-ye")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Vertical_End);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Y end %s failed.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:number required for yend.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}
	return TRUE;
}


/**
 * Internal routine that saves some basic FITS headers to the relevant filename.
 * This is needed as Exposure_Save routine need saved FITS headers to
 * not give an error.
 * @param exposure_time The amount of time, in milliseconds, of the exposure.
 * @param ncols The number of columns in the FITS file.
 * @param nrows The number of rows in the FITS file.
 * @param command Which command was used (exposure or dark).
 * @param current_temperature Current CCD temperature (degrees C).
 * @param filename The filename to save the FITS headers in.
 */
static int Test_Save_Fits_Headers(int exposure_time,int ncols,int nrows,enum COMMAND_ID command,
				  float current_temperature,char *filename)
{
	static fitsfile *fits_fp = NULL;
	int status = 0,retval,ivalue;
	double dvalue;

/* open file */
	if(fits_create_file(&fits_fp,filename,&status))
	{
		Test_Fits_Header_Error(status);
		return FALSE;
	}
/* SIMPLE keyword */
	ivalue = TRUE;
	retval = fits_update_key(fits_fp,TLOGICAL,(char*)"SIMPLE",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* BITPIX keyword */
	ivalue = 16;
	retval = fits_update_key(fits_fp,TINT,(char*)"BITPIX",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* NAXIS keyword */
	ivalue = 2;
	retval = fits_update_key(fits_fp,TINT,(char*)"NAXIS",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* NAXIS1 keyword */
	ivalue = ncols;
	retval = fits_update_key(fits_fp,TINT,(char*)"NAXIS1",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* NAXIS2 keyword */
	ivalue = nrows;
	retval = fits_update_key(fits_fp,TINT,(char*)"NAXIS2",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* BZERO keyword */
	dvalue = 32768.0;
	retval = fits_update_key_fixdbl(fits_fp,(char*)"BZERO",dvalue,6,
		(char*)"Number to offset data values by",&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* BSCALE keyword */
	dvalue = 1.0;
	retval = fits_update_key_fixdbl(fits_fp,(char*)"BSCALE",dvalue,6,
		(char*)"Number to multiply data values by",&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
	/* exposure type */
	if(command == COMMAND_ID_DARK)
		retval = fits_update_key(fits_fp,TSTRING,"OBSTYPE","DARK",NULL,&status);
	else if(command == COMMAND_ID_EXPOSURE)
		retval = fits_update_key(fits_fp,TSTRING,"OBSTYPE","EXPOSE",NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
	/* exposure length */
	dvalue = ((double)Exposure_Length)/1000.0;
	retval = fits_update_key_fixdbl(fits_fp,(char*)"EXPTIME",dvalue,6,
					(char*)"[Seconds] Exposure length",&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
	/* current temp */
	dvalue = ((double)current_temperature)+273.15;
	retval = fits_update_key_fixdbl(fits_fp,(char*)"CCDATEMP",dvalue,6,
					(char*)"[K] Current CCD Temperature",&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* close file */
	if(fits_close_file(fits_fp,&status))
	{
		Test_Fits_Header_Error(status);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal routine to write the complete CFITSIO error stack to stderr.
 * @param status The status returned by CFITSIO.
 */
static void Test_Fits_Header_Error(int status)
{
	/* report the whole CFITSIO error message stack to stderr. */
	fits_report_error(stderr, status);
}

/**
 * Save the exposure to disk.
 * @param filename The name of the file to save the image into. If it does not exist, it is created.
 * @param buffer Pointer to a previously allocated array of unsigned shorts containing the image pixel values.
 * @param buffer_length The length of the buffer in bytes.
 * @param ncols The number of binned image columns (the X size/width of the image).
 * @param nrows The number of binned image rows (the Y size/height of the image).
 * @return Returns TRUE on success, and FALSE if an error occurs.
 * @see #Exposure_Debug_Buffer
 * @see #fexist
 */
static int Exposure_Save(char *filename,void *buffer,size_t buffer_length,int ncols,int nrows)
{
	static fitsfile *fits_fp = NULL;
	char buff[32]; /* fits_get_errstatus returns 30 chars max */
	long axes[2];
	int status = 0,retval,ivalue;
	double dvalue;

	/* check existence of FITS image and create or append as appropriate? */
	if(fexist(filename))
	{
		retval = fits_open_file(&fits_fp,filename,READWRITE,&status);
		if(retval)
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			fprintf(stderr,"Exposure_Save: File open failed(%s,%d,%s).",
				filename,status,buff);
			return FALSE;
		}
	}
	else
	{
		/* open file */
		if(fits_create_file(&fits_fp,filename,&status))
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			fprintf(stderr,"Exposure_Save: File create failed(%s,%d,%s).",
				filename,status,buff);
			return FALSE;
		}
		/* create image block */
		axes[0] = nrows;
		axes[1] = ncols;
		retval = fits_create_img(fits_fp,USHORT_IMG,2,axes,&status);
		if(retval)
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			fits_close_file(fits_fp,&status);
			fprintf(stderr,"Exposure_Save: Create image failed(%s,%d,%s).",
				filename,status,buff);
			return FALSE;
		}
	}
	/* debug whats in the buffer */
	Exposure_Debug_Buffer("Exposure_Save",(unsigned short*)buffer,buffer_length);
	/* write the data */
	retval = fits_write_img(fits_fp,TUSHORT,1,ncols*nrows,buffer,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		fprintf(stderr,"Exposure_Save: File write image failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
	/* diddly time stamp etc*/
	/* ensure data we have written is in the actual data buffer, not CFITSIO's internal buffers */
	/* closing the file ensures this. */ 
	retval = fits_close_file(fits_fp,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		fprintf(stderr,"CCD_Exposure_Save: File close file failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
	return TRUE;
}

/**
 * Debug routine to print out part of the readout buffer.
 * @param description A string containing a description of the buffer.
 * @param buffer The readout buffer.
 * @param buffer_length The length of the readout buffer in pixels.
 * @see ccd_general.html#CCD_General_Log
 */
static void Exposure_Debug_Buffer(char *description,unsigned short *buffer,size_t buffer_length)
{
	char buff[256];
	int i;

	strcpy(buff,description);
	strcat(buff," : ");
	for(i=0; i < 10; i++)
	{
		sprintf(buff+strlen(buff),"[%d] = %hu,",i,buffer[i]);
	}
	strcat(buff," ... ");
	for(i=(buffer_length-10); i < buffer_length; i++)
	{
		sprintf(buff+strlen(buff),"[%d] = %hu,",i,buffer[i]);
	}
	fprintf(stdout,"Buffer:%s.\n",buff);
}

/**
 * Return whether the specified filename exists or not.
 * @param filename A string representing the filename to test.
 * @return The routine returns TRUE if the filename exists, and FALSE if it does not exist. 
 */
static int fexist(char *filename)
{
	FILE *fptr = NULL;

	fptr = fopen(filename,"r");
	if(fptr == NULL )
		return FALSE;
	fclose(fptr);
	return TRUE;
}

