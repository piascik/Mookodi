/**
 * @file
 * @brief Config.h provides a C++ wrapper around plibsys's ini file parser. This provides
 * configuration file support for the camera server.
 * @author Chris Mottram
 * @version $Id$
 */
#include "Config.h"
#include <vector>
#include <iostream>
#include "log4cxx/logger.h"
#include <plibsys/plibsys.h>
using std::string;
using std::vector;

using std::cout;
using std::cerr;
using std::endl;
using std::exception;
using namespace log4cxx;

/**
 * Logger instance for the configuration file (Config.cpp).
 */
static LoggerPtr logger(Logger::getLogger("mookodi.camera.server.Config"));

/**
 * Constructor for the Config object. 
 */
Config::Config()
{
}


/**
 * Destructor for the Config object. Does nothing.
 */
Config::~Config()
{
}

/**
 * Method to set the configuration filename to load config from in load_config.
 * @param config_filename The configuration filename as a string.
 * @see Config::mConfigFilename
 * @see Config::load_config
 */
void Config::set_config_filename(const std::string & config_filename)
{
	mConfigFilename = config_filename;
}

/**
 * Method to load configuration data from the previously specified configuration filename (mConfigFilename),
 * into the config object.
 * @see Config::mConfigFilename
 * @see Config::mConfigFile
 */
void Config::load_config()
{
	CameraException ce;
	PError *error;
	
	LOG4CXX_INFO(logger,"load_config using configuration filename " << mConfigFilename);
	mConfigFile = p_ini_file_new(mConfigFilename.c_str());
	if(!p_ini_file_parse(mConfigFile,&error))
	{
		ce = create_config_exception(error);
		throw ce;
	}
	
}

/**
 * Retrieve a string value from the parsed config file, from the specified section, with the specified keyword.
 * If the keyword does not exist in the section, a CameraException is thrown. If the keyword's value length is
 * longer than value_length, a  CameraException is thrown.
 * @param section The section of the config file to search for the keyword in.
 * @param  keyword The keyword of the config to search for.
 * @param value An allocated number of characters of length value_length, to store the retrieved value into.
 * @param value_length The length of the allocated memory (in characters) for value. 
 * @see #mConfigFile
 * @see CameraException
 */
void Config::get_config_string(const char* section,const char* keyword, char* value, int value_length)
{
	CameraException ce;
	char * value_p_string = NULL;
	
	if(p_ini_file_is_key_exists(mConfigFile,section,keyword) == FALSE)
	{
		ce = create_config_exception_string("get_config_string:Keyword " + std::string(keyword) +
						    " does not exist in section " + std::string(section) + ".");
		throw ce;
	}
	value_p_string =  p_ini_file_parameter_string(mConfigFile,section,keyword,"");
	if((int)strlen(value_p_string) >=  value_length)
	{
		ce = create_config_exception_string("get_config_string:Keyword "+std::string(keyword)+
						    " value "+std::string(value_p_string)+
						    " is too long ( "+std::to_string(strlen(value_p_string))+
						    " vs "+std::to_string(value_length)+") characters.");
		if(value_p_string != NULL)
			p_free(value_p_string);
		throw ce;		
	}
	strcpy(value,value_p_string);
	if(value_p_string != NULL)
		p_free(value_p_string);
}

/**
 * Retrieve an integer value from the parsed config file, from the specified section, with the specified keyword.
 * If the keyword does not exist in the section, a CameraException is thrown. 
 * @param section The section of the config file to search for the keyword in.
 * @param  keyword The keyword of the config to search for.
 * @param value The address of an integer, to store the retrieved value into.
 * @see #mConfigFile
 * @see CameraException
 */
void Config::get_config_int(const char* section,const char* keyword, int* value)
{
	CameraException ce;

	if(value == NULL)
	{
		ce = create_config_exception_string("get_config_int:value is NULL.");
		throw ce;
	}
	if(p_ini_file_is_key_exists(mConfigFile,section,keyword) == FALSE)
	{
		ce = create_config_exception_string("get_config_int:Keyword "+std::string(keyword)+
						    " does not exist in section "+std::string(section)+".");
		throw ce;
	}
	(*value) =  p_ini_file_parameter_int(mConfigFile,section,keyword,0);
}

/**
 * Retrieve an double value from the parsed config file, from the specified section, with the specified keyword.
 * If the keyword does not exist in the section, a CameraException is thrown. 
 * @param section The section of the config file to search for the keyword in.
 * @param  keyword The keyword of the config to search for.
 * @param value The address of a double, to store the retrieved value into.
 * @see #mConfigFile
 * @see CameraException
 */
void Config::get_config_double(const char* section,const char* keyword, double* value)
{
	CameraException ce;

	if(value == NULL)
	{
		ce = create_config_exception_string("get_config_double:value is NULL.");
		throw ce;
	}
	if(p_ini_file_is_key_exists(mConfigFile,section,keyword) == FALSE)
	{
		ce = create_config_exception_string("get_config_double:Keyword "+std::string(keyword)+
						    " does not exist in section "+std::string(section)+".");
		throw ce;
	}
	(*value) =  p_ini_file_parameter_double(mConfigFile,section,keyword,0.0);
}

/**
 * Retrieve a boolean value from the parsed config file, from the specified section, with the specified keyword.
 * If the keyword does not exist in the section, a CameraException is thrown. 
 * @param section The section of the config file to search for the keyword in.
 * @param  keyword The keyword of the config to search for.
 * @param value The address of a integer, to store the retrieved value (as a boolean) into.
 * @see #mConfigFile
 * @see CameraException
 */
void Config::get_config_boolean(const char* section,const char* keyword, int* value)
{
	CameraException ce;

	if(value == NULL)
	{
		ce = create_config_exception_string("get_config_boolean:value is NULL.");
		throw ce;
	}
	if(p_ini_file_is_key_exists(mConfigFile,section,keyword) == FALSE)
	{
		ce = create_config_exception_string("get_config_boolean:Keyword "+std::string(keyword)+
						    " does not exist in section "+std::string(section)+".");
		throw ce;
	}
	(*value) =  p_ini_file_parameter_boolean(mConfigFile,section,keyword,FALSE);
}

/**
 * This method wraps a plibsys PError generated by an ini file problem in a CameraException.
 * @param error The plibsys error to turn into a CameraException, of type PError.
 * @return An instance of CameraException with the string returned by p_error_get_message as a message.
 * @see p_error_get_message
 * @see CameraException
 */
CameraException Config::create_config_exception(PError *error)
{
	CameraException ce;
	
	std::string p_error_str(p_error_get_message(error));
	std::string error_string("Error reading config file:"+p_error_str);
	ce.message = error_string;
	return ce;
}

/**
 * This method creates a CameraException with the specified string message.
 * @param string The error string to turn into a CameraException.
 * @return An instance of CameraException with this string is returned.
 * @see CameraException
 */
CameraException Config::create_config_exception_string(std::string string)
{
	CameraException ce;
	
	ce.message = string;
	return ce;
}
