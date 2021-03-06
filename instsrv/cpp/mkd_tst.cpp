// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#define MAIN
#define FAC FAC_MOK

#include "InstSrv.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "mkd.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

//using namespace std;

using namespace log4cxx;
using namespace log4cxx::helpers;
LoggerPtr logger(Logger::getLogger("mookodi.instrument.server"));

class InstSrvHandler : virtual public InstSrvIf {
 public:
  InstSrvHandler() {
// Initialization 

//  Init. plib system 
    p_libsys_init();

//  Read configuration file
    ini_read( gen_FileInit );

//  Open PIO serial device interface 
    pio_open( pio_device );
    pio_set_attrib( __MAX_BAUD, 0 );
    pio_set_blocking( false ); 
  }

//Set/get slit deployment state
  void CtrlSlit(std::string& _return, const DeployState::type state) {
    unsigned char out; 
    int           ret;
    pio_get_output( &out );
    if ( state == DeployState::ENA ) {
      ret = pio_set_output( out |=  PIO_OUT_SLIT_DEPLOY, state );   
    }
    else if ( state == DeployState::DIS ) {
      ret = pio_set_output( out &= ~PIO_OUT_SLIT_DEPLOY, state );   
    }
    else  {
    } 

    pio_get_output( &out );
    LOG4CXX_INFO(logger, "CtrlSlit");
  }

//Set/get grism deployment state
  void CtrlGrism(std::string& _return, const DeployState::type state) {
    // Your implementation goes here
    LOG4CXX_INFO(logger, "CtrlGrism");
  }

//Set/get mirror deployment state
  void CtrlMirror(std::string& _return, const DeployState::type state) {
    // Your implementation goes here
    LOG4CXX_INFO(logger, "CtrlMirror");
  }

//Set/get lamp deployment state
  void CtrlLamp(std::string& _return, const DeployState::type state) {
    // Your implementation goes here
    LOG4CXX_INFO(logger, "CtrlLamp");
  }

  void CtrlArc(std::string& _return, const DeployState::type state) {
    // Your implementation goes here
    LOG4CXX_INFO(logger, "CtrlArc");
  }

  void CtrlDetector(DetectorConfig& _return, const DetectorConfig& config) {
    // Your implementation goes here
    LOG4CXX_INFO(logger, "CtrlDetector");
  }

  void CtrlDetectOW(const DetectorConfig& config) {
    // Your implementation goes here
    LOG4CXX_INFO(logger, "CtrlDetectOW");
  }
};

int main(int argc, char **argv) {
  int port = 9090;
  ::std::shared_ptr<InstSrvHandler> handler(new InstSrvHandler());
  ::std::shared_ptr<TProcessor> processor(new InstSrvProcessor(handler));
  ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

//Use basic log4cxx 
  BasicConfigurator::configure();

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

