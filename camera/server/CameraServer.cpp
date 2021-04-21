/**
 * @file
 * @brief CameraServer.cpp is the entry point, and provides the main() for, the MookodiCameraServer, which
 * drives the Andor CCD camera via a thrift interface.
 * @author Chris Mottram
 * @version $Id$
 */
#include "CameraConfig.h"
#include "Camera.h"
#include "EmulatedCamera.h"

#include <sys/prctl.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TNonblockingServerSocket.h>
#include <thrift/transport/TNonblockingServerTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransport.h>

#include <boost/program_options.hpp>
#include <iostream>

#include "log4cxx/logger.h"
#include "log4cxx/basicconfigurator.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/helpers/exception.h"


using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace log4cxx;
using namespace log4cxx::helpers;

using std::shared_ptr;
using std::cout;
using std::cerr;
using std::endl;
using std::thread;
using std::exception;
namespace po = boost::program_options;

/**
 * The default port number to run the thrift interface on.
 */
const int DEFAULT_PORT=9020;
/**
 * The default configuration file.
 */
const std::string DEFAULT_CONFIG_FILE = "/mookodi/conf/mkd.cfg";
/**
 * Default logging configuration file.
 */
const std::string DEFAULT_LOGGING_CONFIG_FILE = "log4cxx.properties";
/**
 * Logger instance for the main program (CameraServer.cpp).
 */
static LoggerPtr logger(Logger::getLogger("mookodi.camera.server.CameraServer"));

/**
 * A routine to parse the command line options.
 * @param argc The number of command line options in argv.
 * @param argv An array of strings, each one containing a command line option.
 * @param vm A variable map containing a map from command line option to the value given in the command line
 *         (if applicable).
 * @return The routine returns 0 if the cmmand line is parsed successfully, and 1 if there is an error (or
 *         if "help" is specified and the help text is printed).
 * @see DEFAULT_PORT
 * @see DEFAULT_CONFIG_FILE
 * @see DEFAULT_LOGGING_CONFIG_FILE
 */
int process_options(int argc, char **argv, po::variables_map &vm)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
	("config_file,co", po::value<string>()->default_value(DEFAULT_CONFIG_FILE), "Set configuration file")
	("logging_config_file,lc", po::value<string>()->default_value(DEFAULT_LOGGING_CONFIG_FILE),
	 "Set logging configuration file")
        ("emulate_camera,ec", "Talk to an emulated software camera rather than the real camera head.")
        ("port", po::value<int>()->default_value(DEFAULT_PORT), "Set listening port");

    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << desc << endl;
            return 1;
        }
        cout << "Port set to " << vm["port"].as<int>() << endl;
    }
    catch(po::error &e)
    {
        cerr << "Error: " << e.what() << endl;
        cerr << desc << endl;
        return 1;
    }
    return 0;
}

/**
 * The main entry point for the MookodiCameraServer.
 * @param argc The number of command line options in argv.
 * @param argv An array of strings, each one containing a command line option.
 * @return The return error code from the program, usually 0 if the program is finishing successfully, and 
 *         non-zero if a fatal error occurs.
 * @see CameraServiceIf
 * @see EmulatedCamera
 * @see Camera
 */
int main(int argc, char **argv) 
{
	CameraConfig config;
	
	try
	{
		shared_ptr<CameraServiceIf> handler = NULL;
		// Parse the command line options
		po::variables_map vm;
		
		if (process_options(argc, argv, vm) > 0)
		{
			std::cerr << "Error processing options" << std::endl;
			return 1;
		}
		
		std::cout << "Assigning options" << std::endl;
		
		int port = vm["port"].as<int>();

		// logging initialisation/configuration
		PropertyConfigurator::configure(vm["logging_config_file"].as<std::string>());

		// Setup the CameraConfig file object
		std::cout << "Initialising the CameraConfig." << std::endl;
		LOG4CXX_INFO(logger,"Initialising the CameraConfig.");
		config.initialise();
		
		std::string config_filename =  vm["config_file"].as<std::string>();
		std::cout << "Setting config_filename to " << config_filename << "." << std::endl;
		LOG4CXX_INFO(logger,"Setting config_filename to " << config_filename << ".");
		config.set_config_filename(config_filename);
		
		std::cout << "Loading configuration..." << std::endl;
		LOG4CXX_INFO(logger,"Loading configuration ...");
		config.load_config();
		
		// Set up the server
		if (vm.count("emulate_camera"))
		{
			std::cout << "Emulating CCD camera..." << std::endl;
			LOG4CXX_INFO(logger,"Emulating CCD camera...");
			shared_ptr<EmulatedCamera> emulated_camera = shared_ptr<EmulatedCamera>(new EmulatedCamera());
			//handler = shared_ptr<CameraServiceIf>(new EmulatedCamera());
			handler = shared_ptr<CameraServiceIf>(emulated_camera);
			emulated_camera->set_config(config);
			emulated_camera->initialize();
		}
		else
		{
			std::cout << "Using real camera..." << std::endl;
			LOG4CXX_INFO(logger,"Using real camera...");
			shared_ptr<Camera> camera = shared_ptr<Camera>(new Camera());
			//handler = shared_ptr<CameraServiceIf>(new Camera());
			handler = shared_ptr<CameraServiceIf>(camera);
			camera->set_config(config);
			camera->initialize();
		}
		shared_ptr<TProcessor> processor(new CameraServiceProcessor(handler));
		shared_ptr<TNonblockingServerTransport> serverTransport(new TNonblockingServerSocket(port));
		TNonblockingServer server(processor, serverTransport);

		//Serve
		server.serve();
		return 0;
	}
	catch(TException&e)
	{
		cerr << "Caught TException: " << e.what() << ". Rethrowing..." << endl;
		LOG4CXX_ERROR(logger,"Caught TException: " << e.what() << ". Rethrowing...");
		throw e;
	}
	catch(exception& e)
	{
		cerr << "Unhandled exception caught in CameraServer: " << e.what() << endl;
		cerr << "Application will exit" << endl;
		LOG4CXX_FATAL(logger,"Unhandled exception caught in CameraServer: " << e.what());
		LOG4CXX_FATAL(logger,"Application will exit");
		return 1;
	}
}
