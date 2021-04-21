/**
 * @file
 * @brief CameraConfig.h provides a C++ wrapper around plibsys's ini file parser. This provides
 *                 configuration file support for the camera server.
 * @author Chris Mottram
 * @version $Id$
 */
#ifndef CAMERACONFIG_H
#define CAMERACONFIG_H
#include <log4cxx/logger.h>
#include <plibsys/plibsys.h>
#include "CameraService.h"
using std::string;
using std::vector;

class CameraConfig
{
 public:
	CameraConfig();
	~CameraConfig();
	void initialise();
	void set_config_filename(const std::string & config_filename);
	void load_config();
	void get_config_string(const char* section,const char* keyword, char* value, int value_length);
	void get_config_int(const char* section,const char* keyword,int* value);
	void get_config_double(const char* section,const char* keyword,double* value);
	void get_config_boolean(const char* section,const char* keyword,int* value);
  private:
	/**
	 * This method wraps a plibsys PError generated by an ini file problem in a CameraException
	 */
	CameraException create_config_exception(PError *error);
	/**
	 * This method creates a CameraException with the specified string message.
	 */
	CameraException create_config_exception_string(std::string string);
	/**
	 * The configuration filename to load configuration data from.
	 */
	std::string mCameraConfigFilename;
	/**
	 * The plibsys config file pointer.
	 */
	PIniFile *mCameraConfigFile;
};    
#endif
