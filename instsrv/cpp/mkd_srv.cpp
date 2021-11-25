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
#include <iostream>

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


/* @brief Is LAC within tolerance of a position
 *
 * @param[in] lac   = LAC ID 
 * @param[in] state = Filter position
 *
 * @return  ERR=Failed, BAD=Not in position, POS0-5=Position  
 */
FilterState::type WhereIsFilter( int lac )
{
    int pos;  // Target position
    int now;  // Actual position
    int state; 

    if ( mkd_simulate )
    {
//      Use simulated position
        now = mkd_sim_pos[lac];
    }
    else
    {   
//      Get actual position
        if ( MKD_FAIL == ( now = lac_xfer( lac, LAC_GET_FEEDBACK, 0 )))
            return FilterState::ERR;
    }

//  If LAC near predefined position then return it
    state = lac_State[lac];
    if ( state == -1 ) // State has never been set so try and find it 
    { 
        for( state = 0; state < LAC_POSITIONS; state++ )
        {
            pos = lac_Actuator[lac].pos[state];   
            if ( abs( now - pos ) <= lac_Accuracy )
            {
                lac_State[lac] = state;
                return (FilterState::type)state;
            }
        }
    }
    else // Does requested state match actual position
    { 
        pos = lac_Actuator[lac].pos[state];
        if ( abs( now - pos ) <= lac_Accuracy )
            return (FilterState::type)state;
    }

    return FilterState::BAD;
}



/* @brief Check if LAC position is within tolerance
 *
 * @param[in] lac   = LAC ID 
 * @param[in] state = Filter position
 *
 * @return  ERR=Failed, BAD=Not in position, POS0-5=Position  
 */
FilterState::type CheckFilter( int lac, FilterState::type state )
{
    int pos;  // Target position
    int now;  // Actual position

//  Desired target position
    pos = lac_Actuator[lac].pos[state]; 

    if ( mkd_simulate )
    {
        return state;
    }
    else
    {   
//      Get actual position
        if ( MKD_FAIL == ( now = lac_xfer( lac, LAC_GET_FEEDBACK, 0 )))
            return FilterState::ERR;
    }

//  Check if position is within tolerance
    if ( abs( now - pos ) <= lac_Accuracy ) 
        return state;
    else
        return FilterState::BAD;
}


/* @brief Wait for a single mechanism to reach deployment state
 *
 * @param[inp] bit = output bit for controlling state
 * @param[inp] ena = input  bit for deployed state
 * @param[inp] dis = input  bit for stowed state
 * @param[inp] tmo = timeout in ms 
 *
 * @return ERR=Fail, ENA=Enabled (deployed), DIS=Disabled (stowed), UNK=Unknown
 */
DeployState::type WaitDeploy( unsigned char bit, unsigned char ena, unsigned char dis, int tmo ) {
    int tick  = TIM_TICK;                     // Timer ticks [ms]
    int count = TIM_MICROSECOND * tmo / tick; // Timer count [ms]

    unsigned char out;
    unsigned char inp;

//  Get the PIO output demand bit and input state bits
    if ( MKD_OK != pio_get_output( &out ))
        return DeployState::ERR;

    mkd_log( MKD_OK, LOG_DBG, FAC, "WaitDeploy: bit=0x%02X out=0x%02X ena=0x%02X dis=0x%02X", bit, out, ena, dis );

//  Wait for input state 
    do {
        if ( MKD_OK != pio_get_input( &inp ))                   // Get current state
            return DeployState::ERR;
        else if (  (out & bit) && (inp & ena) && !(inp & dis) ) // Enable state reached 
            return DeployState::ENA;        
        else if ( !(out & bit) && (inp & dis) && !(inp & ena) ) // Disable state reached 
            return DeployState::DIS;        

    } while( count-- );

//  Time-out
    return DeployState::UNK;
}


/* @brief Wait for PIO state 
 *
 * @param[inp] msk = output mask to set mechanism
 * @param[inp] sta = input bit mask for final desired state
 * @param[inp] ret = final state ENA or DIS
 * @param[inp] tmo = timeout in ms
 *
 * @return ERR=Fail, ENA=Enabled (deployed)
 */
DeployState::type WaitPIO( unsigned char msk, unsigned char sta, DeployState::type ret , int tmo ) {
    int tick  = TIM_TICK;                     // Timer ticks 
    int count = TIM_MICROSECOND * tmo / tick; // Timer count
    char msg[256];

    unsigned char out;
    unsigned char inp;

//  Get the PIO output demand 
    if ( (MKD_OK != pio_get_output( &out ))||
         (msk    != out                  )  )
        return DeployState::ERR;

//  Wait for input mask to reach expected state
    do {
        if ( MKD_OK != pio_get_input( &inp )) // Get current state
            return DeployState::ERR;
        else if ( inp == sta )                // State achieved 
            return ret;

        sprintf( msg, "WaitPIO: Count=%i output=0x%2.2x input=0x%2.2x", count, out, inp );
        LOG4CXX_DEBUG(logger, msg );
    } while( count-- );

//  Time-out
    sprintf( msg, "WaitPIO: Timeout for input=0x%2.2x to become 0x%2.2x", inp, sta );
    LOG4CXX_ERROR( logger, msg );
    return DeployState::ERR;
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

    mkd_log( MKD_OK, LOG_DBG, FAC, "CheckDeploy: bit=0x%02X inp=0x%02X out=0x%02X ena=0x%02X dis=0x%02X",bit, inp, out, ena, dis );

//  Check if the mechanism is deployed, stowed or intermediate 
    if      ( (inp & ena) && !(inp & dis) )  // Deploy=set and Stow=clear
        return DeployState::ENA;
    else if ( (inp & dis) && !(inp & ena ) ) // Stow=set and Deploy=clear
        return DeployState::DIS;
    else if (!(inp & ena) && !(inp & dis ) ) // Stow=clear and Deploy=clear. Moving?
       return DeployState::UNK;

//  Invalid input bit states
    return DeployState::ERR;
}


//Set/get slit deployment output state
  DeployState::type CtrlSlit(const DeployState::type state, const int32_t tmo ) {
    unsigned char     out; 
    DeployState::type ret = DeployState::ERR;

    LOG4CXX_INFO(logger, "CtrlSlit");

//  Get the current output state  
    if ( MKD_OK != pio_get_output( &out ) ) {
      LOG4CXX_ERROR(logger, "CtrlSlit");
      return DeployState::ERR;
    }

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = CheckDeploy( PIO_OUT_SLIT_DEPLOY, PIO_INP_SLIT_DEPLOY, PIO_INP_SLIT_STOW);  
    } 
    else if (( state  == DeployState::ENA                            )&& 
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_SLIT_DEPLOY))&&   
             ( MKD_OK == pio_get_output(&out                        ))&&
             ( out & PIO_OUT_SLIT_DEPLOY                             )  ) {   
       if ( tmo )
           ret = WaitDeploy ( PIO_OUT_SLIT_DEPLOY, PIO_INP_SLIT_DEPLOY, PIO_INP_SLIT_STOW, tmo );  
       else        
           ret = CheckDeploy( PIO_OUT_SLIT_DEPLOY, PIO_INP_SLIT_DEPLOY, PIO_INP_SLIT_STOW);  
    }
    else if (( state  == DeployState::DIS                            )&& 
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_SLIT_DEPLOY))&&   
             ( MKD_OK == pio_get_output(&out                        ))&&   
             ( ~out & PIO_OUT_SLIT_DEPLOY                            )  ) {   
       if ( tmo )
           ret = WaitDeploy ( PIO_OUT_SLIT_DEPLOY, PIO_INP_SLIT_DEPLOY, PIO_INP_SLIT_STOW, tmo );  
       else        
           ret = CheckDeploy( PIO_OUT_SLIT_DEPLOY, PIO_INP_SLIT_DEPLOY, PIO_INP_SLIT_STOW);  
    }
    else {
      ret = DeployState::INV;
    }

    return ret;
  }

//Set/get grism deployment output state
  DeployState::type CtrlGrism(const DeployState::type state, const int32_t tmo ) {
    unsigned char     out;
    DeployState::type ret = DeployState::ERR;

    LOG4CXX_INFO(logger, "CtrlGrism");

//  Get the current output state
    if ( MKD_FAIL == pio_get_output( &out ) )
      return DeployState::ERR;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = CheckDeploy( PIO_OUT_GRISM_DEPLOY, PIO_INP_GRISM_DEPLOY, PIO_INP_GRISM_STOW);  
    }
    else if (( state  == DeployState::ENA                             )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_GRISM_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                         ))&&
             ( out & PIO_OUT_GRISM_DEPLOY                             )  ) {   
       if ( tmo ) {
           ret = WaitDeploy(  PIO_OUT_GRISM_DEPLOY, PIO_INP_GRISM_DEPLOY, PIO_INP_GRISM_STOW, tmo);  
       }
       else {
           ret = CheckDeploy( PIO_OUT_GRISM_DEPLOY, PIO_INP_GRISM_DEPLOY, PIO_INP_GRISM_STOW);  
       }
    }
    else if (( state  == DeployState::DIS                             )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_GRISM_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                         ))&&
             ( ~out & PIO_OUT_GRISM_DEPLOY                            )  ) {   
       if ( tmo )
           ret = WaitDeploy(  PIO_OUT_GRISM_DEPLOY, PIO_INP_GRISM_DEPLOY, PIO_INP_GRISM_STOW, tmo);  
       else
           ret = CheckDeploy( PIO_OUT_GRISM_DEPLOY, PIO_INP_GRISM_DEPLOY, PIO_INP_GRISM_STOW);  
    }
    else {
      ret = DeployState::INV;
    }

    return ret; 
  }


//Set/get mirror deployment output state
  DeployState::type CtrlMirror(const DeployState::type state, const int32_t tmo ) {
    unsigned char     out;
    DeployState::type ret = DeployState::ERR;

    LOG4CXX_INFO(logger, "CtrlMirror");

//  Get the current output state
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = CheckDeploy( PIO_OUT_MIRROR_DEPLOY, PIO_INP_MIRROR_DEPLOY, PIO_INP_MIRROR_STOW);  
    }
    else if (( state  == DeployState::ENA                              )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_MIRROR_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                          ))&&
             ( out & PIO_OUT_MIRROR_DEPLOY                             )  ) {   
       if ( tmo )
           ret = WaitDeploy(  PIO_OUT_MIRROR_DEPLOY, PIO_INP_MIRROR_DEPLOY, PIO_INP_MIRROR_STOW, tmo);  
       else
           ret = CheckDeploy( PIO_OUT_MIRROR_DEPLOY, PIO_INP_MIRROR_DEPLOY, PIO_INP_MIRROR_STOW);  
    }
    else if (( state  == DeployState::DIS                              )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_MIRROR_DEPLOY))&&
             ( MKD_OK == pio_get_output(&out                          ))&&
             ( ~out & PIO_OUT_MIRROR_DEPLOY                            )  ) {   
       if ( tmo )
           ret = WaitDeploy(  PIO_OUT_MIRROR_DEPLOY, PIO_INP_MIRROR_DEPLOY, PIO_INP_MIRROR_STOW, tmo);  
       else
           ret = CheckDeploy( PIO_OUT_MIRROR_DEPLOY, PIO_INP_MIRROR_DEPLOY, PIO_INP_MIRROR_STOW);  
    }
    else {
      ret = DeployState::INV;
    }

    return ret;
  }


//Set/get tungsten lamp output state
  DeployState::type CtrlLamp(const DeployState::type state ) {
    unsigned char     out;
    DeployState::type ret = DeployState::ERR;

    LOG4CXX_INFO(logger, "CtrlLamp");

//  Get the current output state
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = (out & PIO_OUT_WLAMP_ON) ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::ENA                         )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_WLAMP_ON))&&
             ( MKD_OK == pio_get_output(&out                     ))&&
             ( out & PIO_OUT_WLAMP_ON                             )  ) {   
      ret = out & PIO_OUT_WLAMP_ON ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::DIS                         )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_WLAMP_ON))&&
             ( MKD_OK == pio_get_output(&out                     ))&&
             ( ~out & PIO_OUT_WLAMP_ON                            )  ) {   
      ret = out & PIO_OUT_WLAMP_ON ? DeployState::ENA : DeployState::DIS;
    }
    else {
      ret = DeployState::INV;
    }

    return ret;
  }


//Set/get arc output state
  DeployState::type CtrlArc(const DeployState::type state) {
    unsigned char     out;
    DeployState::type ret = DeployState::ERR;

    LOG4CXX_INFO(logger, "CtrlArc");

//  Get the current output state
    if ( MKD_OK != pio_get_output( &out ) )
      return ret;

//  If GET query, return the output state
    if ( state == DeployState::GET ) {
      ret = out & PIO_OUT_ARC_ON ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::ENA                       )&&
             ( MKD_OK == pio_set_output( out |=  PIO_OUT_ARC_ON))&&
             ( MKD_OK == pio_get_output(&out                   ))&&
             ( out & PIO_OUT_ARC_ON                             )  ) {   
      ret = out & PIO_OUT_ARC_ON ? DeployState::ENA : DeployState::DIS;
    }
    else if (( state  == DeployState::DIS                       )&&
             ( MKD_OK == pio_set_output( out &= ~PIO_OUT_ARC_ON))&&
             ( MKD_OK == pio_get_output(&out                   ))&&
             ( ~out & PIO_OUT_ARC_ON                            )  ) {   
      ret = out & PIO_OUT_ARC_ON ? DeployState::ENA : DeployState::DIS;
    }
    else {
      ret = DeployState::INV;
    }

    return ret;
  }


//Set output state
  DeployState::type CtrlPIO(const int8_t msk, const int8_t sts, DeployState::type ret,  const int32_t tmo) {

    LOG4CXX_INFO(logger, "CtrlPIO");

    if ( MKD_OK == pio_set_output( msk ) )
      return WaitPIO( msk, sts, ret, tmo );  
    else
      return DeployState::ERR;
  }


  FilterState::type CtrlFilter( FilterID::type filter, const FilterState::type state, int tmo ) {

    LOG4CXX_INFO(logger, "CtrlFilter");

//  Check filter position
    if ( state == FilterState::GET )
      return WhereIsFilter( filter );

//  Check position is within valid range
    if ( state < FilterState::POS0 || state > FilterState::POS5 )
      return FilterState::INV;

//  Save target state for use by WhereIsFilter()
    lac_State[filter] = (int)state;

//  Set position of single filter slide
    if ( MKD_OK == lac_set_pos( filter, lac_Actuator[filter].pos[state], tmo ) )  
      return WhereIsFilter( FilterID::FILTER0 );
    else 
      return FilterState::ERR;
  }


  void CtrlFilters( FilterConfig& _return, const FilterState::type state0, const FilterState::type state1, const int32_t timeout_ms) {

    LOG4CXX_INFO(logger, "CtrlFilters");

//  If GET state, ensure state requests are same before checking filter position
    if ( state0 == FilterState::GET || state1 == FilterState::GET ) {
      if ( state0 != state1 ) {
         _return.Filter0 = _return.Filter1 = FilterState::INV;
         return; 
       }
       else {
         _return.Filter0 = WhereIsFilter( FilterID::FILTER0 );
         _return.Filter1 = WhereIsFilter( FilterID::FILTER1 );
         return; 
      }
    }

//  Check requested state is within range
    if ( state0 < FilterState::POS0 || state0 > FilterState::POS5 ||
         state1 < FilterState::POS0 || state1 > FilterState::POS5   ) {
      _return.Filter0 = _return.Filter1 = FilterState::INV;
    }

//  Save target state for use by WhereIsFilter()
    lac_State[LAC_0] = (int)state0;
    lac_State[LAC_1] = (int)state1;

    if ( MKD_OK == lac_set_both( lac_Actuator[LAC_0].pos[state0], 
                                 lac_Actuator[LAC_1].pos[state1],
                                 timeout_ms )) {
       _return.Filter0 = WhereIsFilter( FilterID::FILTER0 );
       _return.Filter1 = WhereIsFilter( FilterID::FILTER1 );
    }
    else {
      _return.Filter0 = _return.Filter1 = FilterState::ERR;
    }
  }
};


int main(int argc, char **argv) {
// Read command line options
  mkd_opts( argc, argv );

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
