/** @file   mkd_srv.cpp
  *
  * @brief  Mookodi instrument mechanism service 
  *
  * @author asp
  *
  * @date   2021-05-21
  *
  * @version $Id$
  */

#define MAIN
#define FAC FAC_MKD

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

using namespace log4cxx;
using namespace log4cxx::helpers;
LoggerPtr logger(Logger::getLogger("mookodi.instrument.server"));

class InstSrvHandler : virtual public InstSrvIf {
 public:
  InstSrvHandler() {
//  Initialization 

//  Init. plib system 
    p_libsys_init();

//  Read configuration file
    ini_read( gen_FileInit );
    
//  Open PIO serial device interface 
    pio_open( pio_device );
    pio_set_attrib( __MAX_BAUD, 0 );
    pio_set_blocking( false ); 

//  Linear actuator initialisation
    lac_init(); // Initialise USB LAC data
    lac_open(); // Open USB comms. to LAC board
    lac_conf(); // Configure LAC board

    LOG4CXX_INFO(logger, "Init. complete");
  }


/* @brief Check if LAC is within tolerance
 *
 * @param[in] lac   = LAC ID 
 * @param[in] state = Filter position
 *
 * @return  ERR=Failed, BAD=Not inposition, POS1-6=Position  
 */
FilterState::type CheckFilter( int lac, FilterState::type state )
{
    int pos;  // Target position
    int now;  // Actual position

//  Desired target position
    pos = lac_Actuator[lac].pos[state]; 

//  Get actual position
    if ( MKD_FAIL == ( now = lac_xfer( lac, LAC_GET_FEEDBACK, 0 )))
        return FilterState::ERR;

    if ( abs( now - pos ) <= lac_Accuracy )  // Position within tolerance
        return state;
    else
        return FilterState::BAD;
}


/* @brief Check mechanism deployment state
 *
 * @param[inp] bit = output bit for controlling state
 * @param[inp] ena = input bit for deployed state
 * @param[inp] dis = input bit for stowed state
 *
 * @return ERR=Fail, ENA=Enabled (deployed), DIS=Disabled (stowed), UNK=Unknown
 */
DeployState::type CheckDeploy( unsigned char bit, unsigned char ena, unsigned char dis ) {
    unsigned char out;
    unsigned char inp;

//  Get the PIO output demand bit and input state bits
    if (( MKD_OK != pio_get_output( &out ))||
        ( MKD_OK != pio_get_input ( &inp ))  )
        return DeployState::ERR;

//  Check if the mechanism is deployed, stowed, intermediate or error
    if      ( (inp & ena) && !(inp & dis) )  // Deploy=set and Stow=clear
        return out & bit ? DeployState::ENA : DeployState::DIS;
    else if ( (inp & dis) && !(inp & ena ) ) // Stow=set and Deploy=clear
        return out & bit ? DeployState::DIS : DeployState::ENA;
    else if (!(inp & ena) && !(inp & dis ) ) // Stow=clear and Deploy=clear. Moving
       return DeployState::UNK;

//  Invalid input bit states
    return DeployState::ERR;
}


//Set/get slit deployment output state
  DeployState::type CtrlSlit(const DeployState::type state) {
    unsigned char     out; 
    DeployState::type ret;

//  Get the current output state  
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = CheckDeploy( PIO_OUT_SLIT_DEPLOY, PIO_INP_SLIT_DEPLOY, PIO_INP_SLIT_STOW);  
    } 
    else if (( state  == DeployState::ENA                            )&& 
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_SLIT_DEPLOY))&&   
             ( MKD_OK == pio_get_output(&out                        ))  ) {   
      ret = out & PIO_OUT_SLIT_DEPLOY ? DeployState::ENA : DeployState::DIS;            
    }
    else if (( state  == DeployState::DIS                            )&& 
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_SLIT_DEPLOY))&&   
             ( MKD_OK == pio_get_output(&out                        ))  ) {   
      ret = out & PIO_OUT_SLIT_DEPLOY ? DeployState::ENA : DeployState::DIS;            
    }
    else {
      ret = DeployState::INV;
    }

    LOG4CXX_INFO(logger, "CtrlSlit");
    return DeployState::ERR;
  }


//Set/get grism deployment output state
  DeployState::type CtrlGrism(const DeployState::type state) {
    unsigned char     out;
    DeployState::type ret = DeployState::ERR;

//  Get the current output state
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = CheckDeploy( PIO_OUT_GRISM_DEPLOY, PIO_INP_GRISM_DEPLOY, PIO_INP_GRISM_STOW);  
    }
    else if (( state  == DeployState::ENA                             )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_GRISM_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                         ))  ) {
      ret = out & PIO_OUT_GRISM_DEPLOY ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::DIS                             )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_GRISM_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                         ))  ) {
      ret = out & PIO_OUT_GRISM_DEPLOY ? DeployState::ENA : DeployState::DIS;
    }
    else {
      ret = DeployState::INV;
    }

    LOG4CXX_INFO(logger, "CtrlGrism");
    return DeployState::ERR; 
  }


//Set/get mirror deployment output state
  DeployState::type CtrlMirror(const DeployState::type state) {
    unsigned char     out;
    DeployState::type ret;

//  Get the current output state
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = CheckDeploy( PIO_OUT_MIRROR_DEPLOY, PIO_INP_MIRROR_DEPLOY, PIO_INP_MIRROR_STOW);  
    }
    else if (( state  == DeployState::ENA                              )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_MIRROR_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                          ))  ) {
      ret = out & PIO_OUT_MIRROR_DEPLOY ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::DIS                              )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_MIRROR_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                          ))  ) {
      ret = out & PIO_OUT_MIRROR_DEPLOY ? DeployState::ENA : DeployState::DIS;
    }
    else {
      ret = DeployState::INV;
    }

    LOG4CXX_INFO(logger, "CtrlMirror");
    return DeployState::ERR;
  }


//Set/get tungsten lamp output state
  DeployState::type CtrlLamp(const DeployState::type state) {
    unsigned char     out;
    DeployState::type ret;

//  Get the current output state
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = (out & PIO_OUT_WLAMP_ON) ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::ENA                         )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_WLAMP_ON))&&
             ( MKD_OK == pio_get_output(&out                     ))  ) {
      ret = out & PIO_OUT_WLAMP_ON ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::DIS                         )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_WLAMP_ON))&&
             ( MKD_OK == pio_get_output(&out                     ))  ) {
      ret = out & PIO_OUT_WLAMP_ON ? DeployState::ENA : DeployState::DIS;
    }
    else {
      ret = DeployState::INV;
    }

    LOG4CXX_INFO(logger, "CtrlLamp");
    return DeployState::ERR;
  }


//Set/get arc output state
  DeployState::type CtrlArc(const DeployState::type state) {
    unsigned char     out;
    DeployState::type ret;

//  Get the current output state
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = out & PIO_OUT_ARC_ON ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::ENA                       )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_ARC_ON))&&
             ( MKD_OK == pio_get_output(&out                   ))  ) {
      ret = out & PIO_OUT_ARC_ON ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::DIS                       )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_ARC_ON))&&
             ( MKD_OK == pio_get_output(&out                   ))  ) {
      ret = out & PIO_OUT_ARC_ON ? DeployState::ENA : DeployState::DIS;
    }
    else {
      ret = DeployState::INV;
    }

    LOG4CXX_INFO(logger, "CtrlArc");
    return DeployState::ERR;
  }


  FilterState::type CtrlFilter( FilterID::type filter, const FilterState::type state ) {

//  Check if filter is in  position
    if ( state == FilterState::GET )
      return CheckFilter( filter, state );

//  Check position is within valid range
    if ( state < FilterState::POS0 || state > FilterState::POS5 )
      return FilterState::INV;

    LOG4CXX_INFO(logger, "CtrlFilter");
    return CtrlFilter( filter, state );
  }


  void CtrlFilters(FilterConfig& _return, const FilterConfig& config, const int32_t timeout_ms) {

//  If get state, ensure state requests are same before checking filter position is within limits
    if ( config.Filter0 == FilterState::GET || config.Filter1 == FilterState::GET ) {
      if ( config.Filter0 != config.Filter1 ) {
         _return.Filter0 = _return.Filter1 = FilterState::INV;
         return; 
       }
       else {
         _return.Filter0 = CheckFilter( FilterID::Filter0, config.Filter0 );
         _return.Filter1 = CheckFilter( FilterID::Filter1, config.Filter1 );
         return; 
      }
    }

    if ( config.Filter0 < FilterState::POS0 || config.Filter0 > FilterState::POS5 ||
         config.Filter1 < FilterState::POS0 || config.Filter1 > FilterState::POS5   ) {
      _return.Filter0 = _return.Filter1 = FilterState::INV;
    }

    if ( MKD_OK == lac_set_pos( lac_Actuator[LAC_0].pos[config.Filter0], 
                                lac_Actuator[LAC_1].pos[config.Filter1],
                                timeout_ms )) {
      _return.Filter0 = config.Filter0;
      _return.Filter1 = config.Filter1;
    }
    else {
      _return.Filter0 = _return.Filter1 = FilterState::ERR;
    }
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
