/* ccd_fits_filename.c
** CCD fits filename routines
** $Id$
*/
/**
 * @file
 * @brief Routines to look after fits filename generation.
 * @author Chris Mottram
 * @version $Id$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L
/**
 * Define this to enable scandir and alphasort in 'dirent.h', which are BSD 4.3 prototypes.
 */
#define _BSD_SOURCE 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _POSIX_TIMERS
#include <sys/time.h>
#endif
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ccd_fits_filename.h"
#include "ccd_general.h"

/* hash defines */
/**
 * Maximum length of the data directory string.
 */
#define DATA_DIR_STRING_LENGTH    (256)
/**
 * Length of string to reserve  for each component of the directory structure string.
 */
#define COMPONENT_STRING_LENGTH   (64)

/* structure declarations */
/**
 * Data type holding local data about fits filenames associated with one camera. 
 * This consists of the following:
 * <dl>
 * <dt>Data_Dir</dt> <dd>Directory containing FITS images.</dd>
 * <dt>Instrument_Code</dt> <dd>String at the start of FITS filenames representing instrument.</dd>
 * <dt>Data_Dir_Root</dt> <dd>String containing the start of the data path used to construct the Data_Dir.</dd>
 * <dt>Data_Dir_Telescope</dt> <dd>String containing the name of the telescope, used to construct the Data_Dir.</dd>
 * <dt>Data_Dir_Instrument</dt> <dd>String containing the name of the instrument, used to construct the Data_Dir.</dd>
 * <dt>Current_Date_Number</dt> <dd>Current date number of the last file produced, of the form yyyymmdd.</dd>
 * <dt>Current_Run_Number</dt> <dd>Current Run number.</dd>
 * </dl>
 * @see #DATA_DIR_STRING_LENGTH
 * @see #COMPONENT_STRING_LENGTH
 */
struct Fits_Filename_Struct
{
	char Data_Dir[DATA_DIR_STRING_LENGTH];
	char Instrument_Code[COMPONENT_STRING_LENGTH];
	char Data_Dir_Root[COMPONENT_STRING_LENGTH];
	char Data_Dir_Telescope[COMPONENT_STRING_LENGTH];
	char Data_Dir_Instrument[COMPONENT_STRING_LENGTH];
	int Current_Date_Number;
	int Current_Run_Number;
};

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Variable holding error code of last operation performed by the fits filename routines.
 */
static int Fits_Filename_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char Fits_Filename_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";
/**
 * Fits filename data.
 * @see #Fits_Filename_Struct
 * @see #CCD_FITS_FILENAME_DEFAULT_INSTRUMENT_CODE
 */
static struct Fits_Filename_Struct Fits_Filename_Data = 
{
	"",CCD_FITS_FILENAME_DEFAULT_INSTRUMENT_CODE,CCD_FITS_FILENAME_DEFAULT_DATA_DIR_ROOT,
	CCD_FITS_FILENAME_DEFAULT_DATA_DIR_TELESCOPE,CCD_FITS_FILENAME_DEFAULT_DATA_DIR_INSTRUMENT,0,0
};

/* internal functions */
static int Fits_Filename_Setup_Data_Directory(void);
static int Fits_Filename_Create_Directory(char *dir,int *directory_created);
static int Fits_Filename_Get_Year_Number(int *year_number);
static int Fits_Filename_Get_Month_Day_String(char *month_day_string);
static int Fits_Filename_Get_Date_Number(int *date_number);
static int Fits_Filename_File_Select(const struct dirent *entry);
static int Fits_Filename_Lock_Filename_Get(char *filename,char *lock_filename);
static int fexist(char *filename);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Initialise FITS filename data, using the given data directory and the current (astronomical) day of year.
 * Retrieves current FITS images in directory, find the one with the highest multrun number, and sets
 * the current multrun number to it.
 * @param instrument_code A string describing which instrument code to associate with this camera, which appears in
 *        the resulting FITS filenames.
 * @param data_dir_root A string containing the first part of the directory name containing FITS images.
 * @param data_dir_telescope A string containing the telescope name, used to construct part of the directory path.
 * @param data_dir_instrument A string containing the instrument name/code (lower case), 
 *        used to construct part of the directory path.
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 * @see #Fits_Filename_Data
 * @see #Fits_Filename_Setup_Data_Directory
 * @see #Fits_Filename_File_Select
 * @see #COMPONENT_STRING_LENGTH
 */
int CCD_Fits_Filename_Initialise(char *instrument_code,char *data_dir_root,char *data_dir_telescope,
				 char *data_dir_instrument)
{
	struct dirent **name_list = NULL;
	int name_list_count,i,retval,date_number,run_number,fully_parsed;
	char *chptr = NULL;
	char filename[256];
	char inst_code[5] = "";
	char date_string[17] = "";
	char run_string[9] = "";
	int debug;
	
#if LOGGING > 1
	CCD_General_Log("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			       LOG_VERBOSITY_INTERMEDIATE,"FITS","Started.");
#endif
	/* instrument code */
	if(instrument_code == NULL)
	{
		Fits_Filename_Error_Number = 6;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Initialise:instrument_code was NULL.");
		return FALSE;
	}
	if(strlen(instrument_code) > (COMPONENT_STRING_LENGTH-1))
	{
		Fits_Filename_Error_Number = 7;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Initialise:instrument_code was too long(%ld).",
			strlen(data_dir_root));
		return FALSE;
	}
	strcpy(Fits_Filename_Data.Instrument_Code,instrument_code);
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			       LOG_VERBOSITY_VERBOSE,"FITS","Instrument Code is '%s'.",Fits_Filename_Data.Instrument_Code);
#endif
	/* data_dir_root */
	if(data_dir_root == NULL)
	{
		Fits_Filename_Error_Number = 1;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Initialise:data_dir_root was NULL.");
		return FALSE;
	}
	if(strlen(data_dir_root) > (COMPONENT_STRING_LENGTH-1))
	{
		Fits_Filename_Error_Number = 2;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Initialise:data_dir_root was too long(%ld).",
			strlen(data_dir_root));
		return FALSE;
	}
	strcpy(Fits_Filename_Data.Data_Dir_Root,data_dir_root);
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			       LOG_VERBOSITY_VERBOSE,"FITS","Root data directory is '%s'.",
			       Fits_Filename_Data.Data_Dir_Root);
#endif
	/* data_dir_telescope */
	if(data_dir_telescope == NULL)
	{
		Fits_Filename_Error_Number = 9;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Initialise:data_dir_telescope was NULL.");
		return FALSE;
	}
	if(strlen(data_dir_telescope) > (COMPONENT_STRING_LENGTH-1))
	{
		Fits_Filename_Error_Number = 10;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Initialise:data_dir_telescope was too long(%ld).",
			strlen(data_dir_telescope));
		return FALSE;
	}
	strcpy(Fits_Filename_Data.Data_Dir_Telescope,data_dir_telescope);
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			       LOG_VERBOSITY_VERBOSE,"FITS","Telescope component of the data directory is '%s'.",
			       Fits_Filename_Data.Data_Dir_Telescope);
#endif
	/* data_dir_instrument */
	if(data_dir_instrument == NULL)
	{
		Fits_Filename_Error_Number = 11;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Initialise:data_dir_instrument was NULL.");
		return FALSE;
	}
	if(strlen(data_dir_instrument) > (COMPONENT_STRING_LENGTH-1))
	{
		Fits_Filename_Error_Number = 12;
		sprintf(Fits_Filename_Error_String,
			"CCD_Fits_Filename_Initialise:data_dir_instrument was too long(%ld).",
			strlen(data_dir_instrument));
		return FALSE;
	}
	strcpy(Fits_Filename_Data.Data_Dir_Instrument,data_dir_instrument);
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			       LOG_VERBOSITY_VERBOSE,"FITS","Instrument component of the data directory is '%s'.",
			       Fits_Filename_Data.Data_Dir_Instrument);
#endif
	/* setup data_dir */
	if(!Fits_Filename_Setup_Data_Directory())
		return FALSE;
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Data Dir set to %s.",
			      Fits_Filename_Data.Data_Dir);
#endif
	if(!Fits_Filename_Get_Date_Number(&(Fits_Filename_Data.Current_Date_Number)))
		return FALSE;
#if LOGGING > 5
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Current Date Number is %d.",
			      Fits_Filename_Data.Current_Date_Number);
#endif
	Fits_Filename_Data.Current_Run_Number = 0;
	name_list_count = scandir(Fits_Filename_Data.Data_Dir,&name_list,
				  Fits_Filename_File_Select,alphasort);
	for(i=0; i< name_list_count;i++)
	{
#if LOGGING > 9
		CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
				      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Filename %d is %s.",i,name_list[i]->d_name);
#endif
		fully_parsed = FALSE;
		if(strlen(name_list[i]->d_name) > 255)
		{
			Fits_Filename_Error_Number = 26;
			sprintf(Fits_Filename_Error_String,
				"CCD_Fits_Filename_Initialise:filename '%s' was too long (%ld).",
				name_list[i]->d_name,strlen(name_list[i]->d_name));
			return FALSE;
		}
		strcpy(filename,name_list[i]->d_name);
		chptr = strtok(filename,"_");
		if(chptr != NULL)
		{
			strncpy(inst_code,chptr,4);
			inst_code[4] = '\0';
			chptr = strtok(NULL,".");
			if(chptr != NULL)
			{
				strncpy(date_string,chptr,8);
				date_string[8] = '\0';
				for(debug = 0; debug < 8; debug++)
				{
#if LOGGING > 9
					CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
							       LOG_VERBOSITY_VERY_VERBOSE,"FITS",
							       "date_string[%d] = '%c' (%d).",
							       debug,date_string[debug],(int)(date_string[debug]));
#endif
				}
				chptr = strtok(NULL,".");
				if(chptr != NULL)
				{
					strncpy(run_string,chptr,16);
					run_string[16] = '\0';
					fully_parsed = TRUE;
				}
			}
		}
		if(fully_parsed)
		{
#if LOGGING > 9
			CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
					       LOG_VERBOSITY_VERY_VERBOSE,"FITS",
					       "Filename %s parsed OK: inst_code = %s,date_string = %s,run_string = %s.",
					       name_list[i]->d_name,inst_code,date_string,run_string);
#endif
			/* check filename is for the right instrument (camera_index) */
			if(strcmp(inst_code,Fits_Filename_Data.Instrument_Code) == 0)
			{
				retval = sscanf(date_string,"%d",&date_number);
#if LOGGING > 9
				CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
						      LOG_VERBOSITY_VERY_VERBOSE,"FITS",
						      "Filename %s has date number %d.",
						      name_list[i]->d_name,date_number);
#endif
				/* check filename has right date number */
				if((retval == 1)&&
				   (date_number == Fits_Filename_Data.Current_Date_Number))
				{
					retval = sscanf(run_string,"%d",&run_number);
#if LOGGING > 9
					CCD_General_Log_Format("ccd","ccd_fits_filename.c",
							      "CCD_Fits_Filename_Initialise",
							      LOG_VERBOSITY_VERY_VERBOSE,"FITS",
							      "Filename %s has run number %d.",
							      name_list[i]->d_name,run_number);
#endif
					/* check if multrun number is highest yet found */
					if((retval == 1)&&(run_number > Fits_Filename_Data.Current_Run_Number))
					{
						Fits_Filename_Data.Current_Run_Number = run_number;
#if LOGGING > 9
						CCD_General_Log_Format("ccd","ccd_fits_filename.c",
								      "CCD_Fits_Filename_Initialise",
								      LOG_VERBOSITY_VERY_VERBOSE,"FITS",
								      "Current run number now %d.",
								      Fits_Filename_Data.Current_Run_Number);
#endif
					}/* end if run_number > Current_Run_Number */
				}/* end if filename has right date number */
			}/* end if instrument has right instrument code */
		}/* end if fully_parsed */
		else
		{
#if LOGGING > 9
			CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
					      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Failed to parse filename %s: "
					      "inst_code = %s,date_string = %s,run_string = %s.",
					       name_list[i]->d_name,inst_code,date_string,run_string);
#endif
		}
		free(name_list[i]);
	}/* end for on name_list_count */
	free(name_list);
#if LOGGING > 1
	CCD_General_Log("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Initialise",
			LOG_VERBOSITY_INTERMEDIATE,"FITS","Finished.");
#endif
	return TRUE;
}

/**
 * Increments the Run number. Fits_Filename_Setup_Data_Directory is called first, to make sure the data directory
 * exists, and has not changed since the last run (in which case it is created and the run number reset to 0).
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 * @see #Fits_Filename_Data
 * @see #Fits_Filename_Setup_Data_Directory
 */
int CCD_Fits_Filename_Next_Run(void)
{
	if(!Fits_Filename_Setup_Data_Directory())
		return FALSE;
	Fits_Filename_Data.Current_Run_Number++;
	return TRUE;
}

/**
 * Returns a filename based on the current filename data.
 * @param filename Pointer to an array of characters filename_length long to store the filename.
 * @param filename_length The length of the filename array.
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 * @see #Fits_Filename_Data
 */
int CCD_Fits_Filename_Get_Filename(char *filename,int filename_length)
{
	char tmp_buff[512];

	if(filename == NULL)
	{
		Fits_Filename_Error_Number = 3;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Get_Filename:filename was NULL.");
		return FALSE;
	}
	/* check data dir is not too long : 256 is length of tmp_buff, 26 is approx length of filename itself 
	** (with a very long run number) */
	if(strlen(Fits_Filename_Data.Data_Dir) > (256-26))
	{
		Fits_Filename_Error_Number = 8;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Get_Filename:Data Dir too long (%ld).",
			strlen(Fits_Filename_Data.Data_Dir));
		return FALSE;
	}
	sprintf(tmp_buff,"%s/%s_%d.%04d.fits",Fits_Filename_Data.Data_Dir,
		Fits_Filename_Data.Instrument_Code,Fits_Filename_Data.Current_Date_Number,
		Fits_Filename_Data.Current_Run_Number);
	if(strlen(tmp_buff) > (filename_length-1))
	{
		Fits_Filename_Error_Number = 4;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Get_Filename:"
			"Generated filename was too long(%ld).",strlen(tmp_buff));
		return FALSE;
	}
	strcpy(filename,tmp_buff);
	return TRUE;
}

/**
 * Get the current run number.
 * @return The current run number. 
 * @see #Fits_Filename_Data
 */
int CCD_Fits_Filename_Run_Get(void)
{
	return Fits_Filename_Data.Current_Run_Number;
}

/**
 * Lock the FITS file specified, by creating a '.lock' file based on it's filename.
 * This allows interaction with the data transfer processes, so the FITS image will not be
 * data transferred until the lock file is removed.
 * @param filename The filename of the '.fits' FITS filename.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Fits_Filename_Lock_Filename_Get
 * @see #CCD_GENERAL_ERROR_STRING_LENGTH
 */
int CCD_Fits_Filename_Lock(char *filename)
{
	char lock_filename[CCD_GENERAL_ERROR_STRING_LENGTH];
	int fd,open_errno;

	/* check arguments */
	if(filename == NULL)
	{
		Fits_Filename_Error_Number = 14;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Lock:filename was NULL.");
		return FALSE;
	}
	if(strlen(filename) >= CCD_GENERAL_ERROR_STRING_LENGTH)
	{
		Fits_Filename_Error_Number = 17;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_Lock:FITS filename was too long(%ld).",
			strlen(filename));
		return FALSE;
	}
	/* get lock filename */
	if(!Fits_Filename_Lock_Filename_Get(filename,lock_filename))
		return FALSE;
#if LOGGING > 9
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Lock",LOG_VERBOSITY_VERY_VERBOSE,
			      "FILELOCK","Creating lock file %s.",lock_filename);
#endif
	/* try to open lock file. */
	/* O_CREAT|O_WRONLY|O_EXCL : create file, O_EXCL means the call will fail if the file already exists. 
	** Note atomic creation probably fails on NFS systems. */
	fd = open((const char*)lock_filename,O_CREAT|O_WRONLY|O_EXCL);
	if(fd == -1)
	{
		open_errno = errno;
		Fits_Filename_Error_Number = 18;
		sprintf(Fits_Filename_Error_String,
			"CCD_Fits_Filename_Lock:Failed to create lock filename(%s):error %d.",
			lock_filename,open_errno);
		return FALSE;
	}
	/* close created file */
	close(fd);
#if LOGGING > 9
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_Lock",LOG_VERBOSITY_VERY_VERBOSE,
			      "FILELOCK","Lock file %s created.",lock_filename);
#endif
	return TRUE;
}

/**
 * UnLock the FITS file specified, by removing the previously created '.lock' file based on it's filename.
 * This allows interaction with the data transfer processes, so the FITS image will not be
 * data transferred until the lock file is removed.
 * @param filename The filename of the '.fits' FITS filename.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Fits_Filename_Lock_Filename_Get
 * @see #fexist
 */
int CCD_Fits_Filename_UnLock(char *filename)
{
	char lock_filename[CCD_GENERAL_ERROR_STRING_LENGTH];
	int retval,remove_errno;

	/* check arguments */
	if(filename == NULL)
	{
		Fits_Filename_Error_Number = 19;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_UnLock:filename was NULL.");
		return FALSE;
	}
	if(strlen(filename) >= CCD_GENERAL_ERROR_STRING_LENGTH)
	{
		Fits_Filename_Error_Number = 20;
		sprintf(Fits_Filename_Error_String,"CCD_Fits_Filename_UnLock:FITS filename was too long(%ld).",
			strlen(filename));
		return FALSE;
	}
	/* get lock filename */
	if(!Fits_Filename_Lock_Filename_Get(filename,lock_filename))
		return FALSE;
	/* check existence */
	if(fexist(lock_filename))
	{
#if LOGGING > 9
		CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_UnLock",
				      LOG_VERBOSITY_VERY_VERBOSE,"FILELOCK","Removing lock file %s.",lock_filename);
#endif
		/* remove lock file */
		retval = remove(lock_filename);
		if(retval == -1)
		{
			remove_errno = errno;
			Fits_Filename_Error_Number = 21;
			sprintf(Fits_Filename_Error_String,
				"CCD_Fits_Filename_UnLock:Failed to unlock filename '%s':(%d,%d).",
				lock_filename,retval,remove_errno);
			return FALSE;
		}
#if LOGGING > 9
		CCD_General_Log_Format("ccd","ccd_fits_filename.c","CCD_Fits_Filename_UnLock",
				      LOG_VERBOSITY_VERY_VERBOSE,"FILELOCK","Lock file %s removed.",lock_filename);
#endif
	}
	return TRUE;
}

/**
 * Get the current value of ccd_fits_filename's error number.
 * @return The current value of ccd_fits_filename's error number.
 * @see #Fits_Filename_Error_Number
 */
int CCD_Fits_Filename_Get_Error_Number(void)
{
	return Fits_Filename_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_fits_filename in a standard way.
 * @see CCD_General_Get_Current_Time_String
 * @see #Fits_Filename_Error_Number
 * @see #Fits_Filename_Error_String
 */
void CCD_Fits_Filename_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Fits_Filename_Error_Number == 0)
		sprintf(Fits_Filename_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Fits_Filename:Error(%d) : %s\n",time_string,Fits_Filename_Error_Number,
		Fits_Filename_Error_String);
}

/**
 * The error routine that reports any errors occuring in ccd_fits_filename in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see CCD_General_Get_Current_Time_String
 * @see #Fits_Filename_Error_Number
 * @see #Fits_Filename_Error_String
 */
void CCD_Fits_Filename_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Fits_Filename_Error_Number == 0)
		sprintf(Fits_Filename_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Fits_Filename:Error(%d) : %s\n",time_string,
		Fits_Filename_Error_Number,Fits_Filename_Error_String);
}
/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Construct a suitable directory name to store FITS images in, and if the directory does not exist create it.
 * The SAAO FITS directory structure is as follows: /data/lesedi/mkd/2021/0311, and the name is constructed 
 * from the Data_Dir_Root,Data_Dir_Telescope,and Data_Dir_Instrument part of the Fits_Filename_Data data, and
 * placed in the Data_Dir part of Fits_Filename_Data.
 * The year and month/day directories should be checked to see if they exist, and if they do not they should be
 * created. 
 * @see #Fits_Filename_Data
 * @see #Fits_Filename_Get_Year_Number
 * @see #Fits_Filename_Get_Month_Day_String
 * @see #Fits_Filename_Create_Directory
 */
static int Fits_Filename_Setup_Data_Directory(void)
{
	char year_string[5];
	char month_day_string[5];
	int year,new_directory;
	
	new_directory = FALSE;
	/* create base of directory structure. We assume this bit already exists on disk */
	sprintf(Fits_Filename_Data.Data_Dir,"/%s/%s/%s/",Fits_Filename_Data.Data_Dir_Root,
		Fits_Filename_Data.Data_Dir_Telescope,Fits_Filename_Data.Data_Dir_Instrument);
#if LOGGING > 7
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","Fits_Filename_Setup_Data_Directory",
			      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Base Data Dir set to %s.",
			      Fits_Filename_Data.Data_Dir);
#endif
	/* find year number and append to the Data_Dir */
	if(!Fits_Filename_Get_Year_Number(&year))
		return FALSE;
	sprintf(Fits_Filename_Data.Data_Dir+strlen(Fits_Filename_Data.Data_Dir),"%d",year);
	/* If the year directory does not exist, create it */
#if LOGGING > 7
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","Fits_Filename_Setup_Data_Directory",
			      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Check year Data Dir '%s' exists.",
			      Fits_Filename_Data.Data_Dir);
#endif
	if(!Fits_Filename_Create_Directory(Fits_Filename_Data.Data_Dir,&new_directory))
		return FALSE;
	/* Add the month/day string to the directory name */
	strcat(Fits_Filename_Data.Data_Dir,"/");
	if(!Fits_Filename_Get_Month_Day_String(month_day_string))
		return FALSE;
	strcat(Fits_Filename_Data.Data_Dir,month_day_string);
	/* If the month/day string does not exist, create it */ 
#if LOGGING > 7
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","Fits_Filename_Setup_Data_Directory",
			      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Check month/day Data Dir '%s' exists.",
			      Fits_Filename_Data.Data_Dir);
#endif
	if(!Fits_Filename_Create_Directory(Fits_Filename_Data.Data_Dir,&new_directory))
		return FALSE;
	/* if we had to create a new directory, reset run number to 0 */
	if(new_directory)
		Fits_Filename_Data.Current_Run_Number = 0;
	/* Add terminating seperator */
	strcat(Fits_Filename_Data.Data_Dir,"/");
#if LOGGING > 7
	CCD_General_Log_Format("ccd","ccd_fits_filename.c","Fits_Filename_Setup_Data_Directory",
			      LOG_VERBOSITY_VERY_VERBOSE,"FITS","Data Dir set to '%s'.",
			      Fits_Filename_Data.Data_Dir);
#endif
	return TRUE;
}

/**
 * Check whether the specified directory exists. If it does not, create it.
 * @param dir A string representing the directory to create, if it does already exist.
 * @param directory_created The address of an integer. If this function creates the specified directory, the 
 *        variable is set to TRUE. If the function does NOT create the specified directory, 
 *        this variable is NOT modified.
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 */
static int Fits_Filename_Create_Directory(char *dir,int *directory_created)
{
	struct stat s;
	int retval,mkdir_errno;

	retval = stat(dir,&s);
	if(retval == 0) /* dir exists */
	{
		if(!S_ISDIR(s.st_mode)) /* dir is NOT a directory */
		{
			Fits_Filename_Error_Number = 13;
			sprintf(Fits_Filename_Error_String,
				"Fits_Filename_Create_Directory:File '%s' is NOT a directory.",dir);
			return FALSE;
		}
		/* We do NOT set (*directory_created) = FALSE; here.
		** This is so we can call Fits_Filename_Create_Directory multiple times with the same directory_created
		** variable, and tell if any one invocation caused a directory to be created. */
	}
	else /* dir does not exist, create it */
	{
#if LOGGING > 7
		CCD_General_Log_Format("ccd","ccd_fits_filename.c","Fits_Filename_Create_Directory",
				       LOG_VERBOSITY_VERY_VERBOSE,"FITS","Creating directory '%s'.",dir);
#endif
		retval = mkdir(dir,0777);
		if(retval != 0)
		{
			mkdir_errno = errno;
			Fits_Filename_Error_Number = 15;
			sprintf(Fits_Filename_Error_String,
				"Fits_Filename_Create_Directory:Failed to create directory '%s' (%d).",dir,mkdir_errno);
			return FALSE;
		}
		(*directory_created) = TRUE;
	}
	return TRUE;
}
			       
/**
 * Get a year number. This is an integer (year) of the form yyyy, used as the year directory
 * in a SAAO FITS directory. The year is for the start of night, 
 * i.e. between mignight and 12 noon on 1st January the year before is used.
 * @param year_number The date number, a pointer to an integer. On successful return, an integer of the form yyyy.
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 */
static int Fits_Filename_Get_Year_Number(int *year_number)
{
	struct timespec current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	time_t seconds_since_epoch;
	const struct tm *broken_down_time = NULL;

	if(year_number == NULL)
	{
		Fits_Filename_Error_Number = 16;
		sprintf(Fits_Filename_Error_String,"Fits_Filename_Get_Year_Number:year_number was NULL.");
		return FALSE;
	}
#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&current_time);
#else
	gettimeofday(&gtod_current_time,NULL);
	current_time.tv_sec = gtod_current_time.tv_sec;
	current_time.tv_nsec = gtod_current_time.tv_usec*CCD_GENERAL_ONE_MICROSECOND_NS;
#endif
	seconds_since_epoch = (time_t)(current_time.tv_sec);
	broken_down_time = gmtime(&(seconds_since_epoch));
	/* should we be looking at yesterdays date? If hour of day 0..12, yes */
	if(broken_down_time->tm_hour < 12)
	{
		seconds_since_epoch -= (12*60*60); /* subtract 12 hours from seconds since epoch */
		broken_down_time = gmtime(&(seconds_since_epoch));
	}
	/* compute year number */
	(*year_number) = (broken_down_time->tm_year+1900);
	return TRUE;
}

/**
 * Get a string representing the month/day in the form mmdd. This is used as the month/day directory as part
 * of the directory structure used for storing SAAO FITS images.
 * @param month_day_string A string to store the computed month/day string. This should be at least 5 characters long.
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 */
static int Fits_Filename_Get_Month_Day_String(char *month_day_string)
{
	struct timespec current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	time_t seconds_since_epoch;
	const struct tm *broken_down_time = NULL;
	int month,day;
	
	if(month_day_string == NULL)
	{
		Fits_Filename_Error_Number = 25;
		sprintf(Fits_Filename_Error_String,"Fits_Filename_Get_Date_Number:month_day_string was NULL.");
		return FALSE;
	}
#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&current_time);
#else
	gettimeofday(&gtod_current_time,NULL);
	current_time.tv_sec = gtod_current_time.tv_sec;
	current_time.tv_nsec = gtod_current_time.tv_usec*CCD_GENERAL_ONE_MICROSECOND_NS;
#endif
	seconds_since_epoch = (time_t)(current_time.tv_sec);
	broken_down_time = gmtime(&(seconds_since_epoch));
	/* should we be looking at yesterdays date? If hour of day 0..12, yes */
	if(broken_down_time->tm_hour < 12)
	{
		seconds_since_epoch -= (12*60*60); /* subtract 12 hours from seconds since epoch */
		broken_down_time = gmtime(&(seconds_since_epoch));
	}
	/* tm_mon is months since January, 0..11, so add one */
	month = (broken_down_time->tm_mon+1);
	/* tm_mday is day of month 1..31. */
	day = broken_down_time->tm_mday;
	sprintf(month_day_string,"%02d%02d",month,day);
	return TRUE;
}

/**
 * Get a date number. This is an integer of the form yyyymmdd, used as the date indicator
 * in a SAAO FITS filename. The date is for the start of night, 
 * i.e. between mignight and 12 noon the day before is used.
 * @param date_number The date number, a pointer to an integer. On successful return, an integer of the form yyyymmdd.
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 */
static int Fits_Filename_Get_Date_Number(int *date_number)
{
	struct timespec current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	time_t seconds_since_epoch;
	const struct tm *broken_down_time = NULL;

	if(date_number == NULL)
	{
		Fits_Filename_Error_Number = 5;
		sprintf(Fits_Filename_Error_String,"Fits_Filename_Get_Date_Number:date_number was NULL.");
		return FALSE;
	}
#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&current_time);
#else
	gettimeofday(&gtod_current_time,NULL);
	current_time.tv_sec = gtod_current_time.tv_sec;
	current_time.tv_nsec = gtod_current_time.tv_usec*CCD_GENERAL_ONE_MICROSECOND_NS;
#endif
	seconds_since_epoch = (time_t)(current_time.tv_sec);
	broken_down_time = gmtime(&(seconds_since_epoch));
	/* should we be looking at yesterdays date? If hour of day 0..12, yes */
	if(broken_down_time->tm_hour < 12)
	{
		seconds_since_epoch -= (12*60*60); /* subtract 12 hours from seconds since epoch */
		broken_down_time = gmtime(&(seconds_since_epoch));
	}
	/* compute date number */
	(*date_number) = 0;
	/* tm_year is years since 1900, add 1900 and multiply by 10000 to leave room for month/day */
	(*date_number) += (broken_down_time->tm_year+1900)*10000;
	/* tm_mon is months since January, 0..11, so add one and multiply by 100 to leave room for day */
	(*date_number) += (broken_down_time->tm_mon+1)*100;
	/* tm_mday is day of month 1..31, so just add */
	(*date_number) += broken_down_time->tm_mday;
	return TRUE;
}

/**
 * Select routine for scandir. Selects files starting with the instrument code (Fits_Filename_Data.Instrument_Code) 
 * and ending in '.fits'.
 * @param entry The directory entry.
 * @return The routine returns TRUE if the file starts with the instrument code and ends in '.fits',
 *         and so is added to the list of files scandir returns. Otherwise, FALSE it returned.
 * @see #Fits_Filename_Data
 */
static int Fits_Filename_File_Select(const struct dirent *entry)
{
	if(strncmp(entry->d_name,Fits_Filename_Data.Instrument_Code,strlen(Fits_Filename_Data.Instrument_Code)) == 0)
	{
		if(strstr(entry->d_name,".fits")!=NULL)
			return (TRUE);
	}
	return (FALSE);
}

/**
 * Given a FITS filename derive a suitable '.lock' filename.
 * @param filename The FITS filename.
 * @param lock_filename A buffer. On return, this is filled with a suitable lock filename for the FITS image.
 * @return Returns TRUE if the routine succeeds and returns FALSE if an error occurs.
 * @see #CCD_GENERAL_ERROR_STRING_LENGTH
 */
static int Fits_Filename_Lock_Filename_Get(char *filename,char *lock_filename)
{
	char *ch_ptr = NULL;

	if(filename == NULL)
	{
		Fits_Filename_Error_Number = 22;
		sprintf(Fits_Filename_Error_String,"Fits_Filename_Lock_Filename_Get:filename was NULL.");
		return FALSE;
	}
	if(strlen(filename) >= CCD_GENERAL_ERROR_STRING_LENGTH)
	{
		Fits_Filename_Error_Number = 23;
		sprintf(Fits_Filename_Error_String,"Fits_Filename_Lock_Filename_Get:FITS filename was too long(%ld).",
			strlen(filename));
		return FALSE;
	}
	/* lock filename starts the same as FITS filename */
	strcpy(lock_filename,filename);
	/* Find FITS filename '.fits' extension in lock_filename buffer */
	/* This does allow for multiple '.fits' to be present in the FITS filename.
	** This should never occur. */
	ch_ptr = strstr(lock_filename,".fits");
	if(ch_ptr == NULL)
	{
		Fits_Filename_Error_Number = 24;
		sprintf(Fits_Filename_Error_String,"Fits_Filename_Lock_Filename_Get:'.fits' not found in filename %s.",
			filename);
		return FALSE;
	}
	/* terminate lock filename at start of '.fits' (removing '.fits') */
	(*ch_ptr) = '\0';
	/* add '.lock' to lock filename */
	strcat(lock_filename,".lock");
	return TRUE;
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
	if(fptr == NULL)
		return FALSE;
	fclose(fptr);
	return TRUE;
}
