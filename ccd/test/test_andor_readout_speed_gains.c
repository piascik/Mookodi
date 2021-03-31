/* test_andor_readout_speed_gains.c
 * Test what readout speeds and gains the Andor camera supports.
 */
/**
 * @file
 * @brief Test what readout speeds and gains the Andor camera supports. 
 *        Low level test of the Andor library.
 * @author $Author$
 * @version $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "atmcdLXd.h"
/* hash definitions */
/**
 * Truth value.
 */
#define TRUE 1
/**
 * Falsity value.
 */
#define FALSE 0
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
 * Filename for configuration directory. 
 */
static char *Config_Dir = "/usr/local/etc/andor";

/* internal routines */
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive integer if it fails.
 */
int main(int argc, char *argv[])
{
	unsigned long andor_retval;
	int retval,value;
	int channel_count,amplifier_count,pre_amp_gain_count,bit_depth,vs_speed_count,hs_speed_count;
	int channel_index,amplifier_index,pre_amp_gain_index,vs_speed_index,hs_speed_index,is_pre_amp_gain_available;
	float vs_speed,hs_speed,pre_amp_gain;
	at_32 Camera_Handle;

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
	/* get number of A/D channels */
	andor_retval = GetNumberADChannels(&channel_count);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"GetNumberADChannels failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"GetNumberADChannels returned %d A/D channels.\n",channel_count);
	/* get number of amplifiers*/
	andor_retval = GetNumberAmp(&amplifier_count);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"GetNumberAmp failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"GetNumberAmp returned %d amplifiers.\n",amplifier_count);
	/* get number of pre amp gains */
	andor_retval = GetNumberPreAmpGains(&pre_amp_gain_count);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"GetNumberPreAmpGains failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"GetNumberPreAmpGains returned %d pre amplifier gains.\n",pre_amp_gain_count);
	/* print out vertical readout speeds */
	andor_retval = GetNumberVSSpeeds(&vs_speed_count);
	if(andor_retval!=DRV_SUCCESS)
	{
		fprintf(stderr,"GetNumberVSSpeeds failed %lu.\n",andor_retval);
		return 2;
	}
	fprintf(stdout,"GetNumberVSSpeeds %d vertical speeds.\n",vs_speed_count);
	for(vs_speed_index = 0; vs_speed_index < vs_speed_count; vs_speed_index++)
	{
		andor_retval = GetVSSpeed(vs_speed_index,&vs_speed);
		if(andor_retval!=DRV_SUCCESS)
		{
			fprintf(stderr,"GetVSSpeed failed %lu.\n",andor_retval);
			return 2;
		}
		fprintf(stdout,"GetVSSpeed returned %.6f microseconds/pixel shift VS Speed index %d.\n",vs_speed,vs_speed_index);
	}/* end for on vs_speed_index */
	/* loop over possible a/d channel, amplifier, hsspeed, pre-amp settings */
	for(channel_index=0;channel_index<channel_count;channel_index++)
	{
		/* get bit depth */
		andor_retval = GetBitDepth(channel_index,&bit_depth);
		if(andor_retval!=DRV_SUCCESS)
		{
			fprintf(stderr,"GetBitDepth failed %lu.\n",andor_retval);
			return 2;
		}
		fprintf(stdout,"GetBitDepth returned %d for channel %d.\n",bit_depth,channel_index);
		/* loop over amplifiers */
		for(amplifier_index=0;amplifier_index < amplifier_count; amplifier_index++)
		{
			/* get number of horizontal speeds */
			andor_retval = GetNumberHSSpeeds(channel_index,amplifier_index,&hs_speed_count);
			if(andor_retval!=DRV_SUCCESS)
			{
				fprintf(stderr,"GetNumberHSSpeeds failed %lu.\n",andor_retval);
				return 2;
			}
			fprintf(stdout,"GetNumberHSSpeeds returned %d speeds for channel %d and amplifier %d.\n",
				hs_speed_count,channel_index,amplifier_index);
			for(hs_speed_index = 0; hs_speed_index < hs_speed_count; hs_speed_index++)
			{
				/* get horizontal speed */
				andor_retval = GetHSSpeed(channel_index,amplifier_index,hs_speed_index,&hs_speed);
				if(andor_retval!=DRV_SUCCESS)
				{
					fprintf(stderr,"GetHSSpeed failed %lu.\n",andor_retval);
					return 2;
				}
				fprintf(stdout,"GetHSSpeed returned %.6f MHz for channel %d, amplifier %d "
					"and HS Speed index %d.\n",
					hs_speed,channel_index,amplifier_index,hs_speed_index);
				for(pre_amp_gain_index = 0; pre_amp_gain_index < pre_amp_gain_count;
				    pre_amp_gain_index++)
				{
					/* get pre-amp gain */
					andor_retval = GetPreAmpGain(pre_amp_gain_index,&pre_amp_gain);
					if(andor_retval!=DRV_SUCCESS)
					{
						fprintf(stderr,"GetPreAmpGain failed %lu.\n",andor_retval);
						return 2;
					}
					/* is this pre-amp gain availablev */
					andor_retval = IsPreAmpGainAvailable(channel_index,amplifier_index,
									     hs_speed_index,pre_amp_gain_index,
									     &is_pre_amp_gain_available);
					if(andor_retval!=DRV_SUCCESS)
					{
						fprintf(stderr,"IsPreAmpGainAvailable failed %lu.\n",andor_retval);
						return 2;
					}
					fprintf(stdout,"IsPreAmpGainAvailable: channel %d, amplifier %d, "
						"HS Speed index %d (%.6f Hz), Pre-amp gain index %d (gain %.6f)"
						" is avilable = %d.\n",
						channel_index,amplifier_index,
						hs_speed_index,hs_speed,pre_amp_gain_index,pre_amp_gain,
						is_pre_amp_gain_available);
				}/* end for on pre_amp_gain_index */
			}/* end for on hs_speed_index */
		}/* end for on amplifier_index */
	}/* end for on channel_index */
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
	fprintf(stdout,"Test Andor Readout Speed and Gain:Help.\n");
	fprintf(stdout,"Find out what readout speeds and gains the Andor camera supports using direct Andor library calls.\n");
	fprintf(stdout,"test_andor_readout_speed_gains \n");
	fprintf(stdout,"\t[-camera <n>]\n");
	fprintf(stdout,"\t[-co[nfig_dir] <directory>]\n");
	fprintf(stdout,"\t[-h[elp]]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
	fprintf(stdout,"\n");
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Help
 * @see #Selected_Camera
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
		else if((strcmp(argv[i],"-help")==0)||(strcmp(argv[i],"-h")==0))
		{
			Help();
			exit(0);
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}
	return TRUE;
}
