/** @file   mkd_lac.cpp
  *
  * @brief  Linear actuator functions  
  *
  * @author asp
  *
  * @date   2021-11-15
  *
  * @version $Id$
  */

#define FAC FAC_LAC
#include "mkd.h"
#include "InstSrv.h"

#include <iostream>

using namespace log4cxx;
using namespace log4cxx::helpers;
extern LoggerPtr logger;

// Local static data
static int                    usb_if;             // USB interface
static struct libusb_context *usb_ctx;            // USB context
static libusb_device_handle  *lac_hnd[LAC_COUNT]; // LAC USB device handles

/* @brief Initialise USB and static LAC variables  
 */
void lac_init( void )
{
    usb_if  = 0;
    usb_ctx = NULL;

    for ( int i=0; i < LAC_COUNT; i++ )
        lac_hnd[i] = NULL;

//  Only init USB if not simulated 
    if ( !mkd_simulate )
        libusb_init( &usb_ctx );
}


/* @brief Release LAC USB handles and exit USB context
 */
void lac_close( void )
{
    int ret;

    if ( mkd_simulate )
        return;

    for ( int i=0; i < LAC_COUNT; i++ )
    { 
        ret = libusb_release_interface( lac_hnd[i], usb_if );
        assert( ret == 0 );

        if ( lac_hnd[i] )
            libusb_close( lac_hnd[i] );
    }

    libusb_exit( usb_ctx );
}


/* @brief Set USB debug level 
 *
 * @param[in]   level = Debug level 
 */
void lac_debug( int level )
{
    libusb_set_option( usb_ctx, LIBUSB_OPTION_LOG_LEVEL, level );
}


/* @brief    Configure LAC boards 
 *
 * @return   MKD_OK = Success, MKD_FAIL = Failure 
 */
int lac_conf( void )
{
    if ( mkd_simulate )
        return MKD_OK;

    for( int i=0; i < LAC_COUNT; i++ )
    {
//      Check that each parameter is successfully written to LAC board
        if (( lac_Speed              == lac_xfer(i, LAC_SET_SPEED               ,lac_Speed              ))&& 
            ( lac_Accuracy           == lac_xfer(i, LAC_SET_ACCURACY            ,lac_Accuracy           ))&&
            ( lac_RetractLimt        == lac_xfer(i, LAC_SET_RETRACT_LIMIT       ,lac_RetractLimt        ))&&
            ( lac_ExtendLimit        == lac_xfer(i, LAC_SET_EXTEND_LIMIT        ,lac_ExtendLimit        ))&&
            ( lac_MovementThreshold  == lac_xfer(i, LAC_SET_MOVEMENT_THRESHOLD  ,lac_MovementThreshold  ))&&
            ( lac_StallTime          == lac_xfer(i, LAC_SET_STALL_TIME          ,lac_StallTime          ))&&
            ( lac_PWMThreshold       == lac_xfer(i, LAC_SET_PWM_THRESHOLD       ,lac_PWMThreshold       ))&&
            ( lac_DerivativeThreshold== lac_xfer(i, LAC_SET_DERIVATIVE_THRESHOLD,lac_DerivativeThreshold))&&
            ( lac_DerivativeMaximum  == lac_xfer(i, LAC_SET_DERIVATIVE_MAXIMUM  ,lac_DerivativeMaximum  ))&&
            ( lac_DerivativeMinimum  == lac_xfer(i, LAC_SET_DERIVATIVE_MINIMUM  ,lac_DerivativeMinimum  ))&&
            ( lac_PWMMaximum         == lac_xfer(i, LAC_SET_PWM_MAXIMUM         ,lac_PWMMaximum         ))&&
            ( lac_PWMMinimum         == lac_xfer(i, LAC_SET_PWM_MINIMUM         ,lac_PWMMinimum         ))&&
            ( lac_ProportionalGain   == lac_xfer(i, LAC_SET_PROPORTIONAL_GAIN   ,lac_ProportionalGain   ))&&
            ( lac_DerivativeGain     == lac_xfer(i, LAC_SET_DERIVATIVE_GAIN     ,lac_DerivativeGain     ))&&
            ( lac_AverageRC          == lac_xfer(i, LAC_SET_AVERAGE_RC          ,lac_AverageRC          ))&&
            ( lac_AverageADC         == lac_xfer(i, LAC_SET_AVERAGE_ADC         ,lac_AverageADC         ))  )
        {
            return MKD_OK;
        }
    }

    return MKD_FAIL;
}


/* @brief    Search for LAC USB devices, open handles and claim  
 *
 * @return   Number of LAC devices found,  MKD_FAIL = Failure
 */
int lac_open( void )
{
    int          ret;  // Integer return
    libusb_error err;  // USB error return
    ssize_t     count; // Number of USB devices found
    int lac = 0;       // Number of LAC devices found

    if ( mkd_simulate )
        return mkd_log(LAC_COUNT, LOG_DBG, FAC, "Simulating %i LAC devices", LAC_COUNT );

    libusb_device **usb_dev;
    libusb_device  *dev = NULL;
    struct libusb_device_descriptor ddesc;

    count = libusb_get_device_list( usb_ctx, &usb_dev);
    if ( 0 >= count )
        return mkd_log(MKD_FAIL, LOG_ERR, FAC, "libusb_get_device()=%li", count);
    mkd_log(MKD_OK, LOG_DBG, FAC, "Found %i USB devices", count);

//  Search all USB devices for LACs 
    for( int i=0; i < count; i++)
    {
        dev = usb_dev[i];
        ret = libusb_get_device_descriptor(dev, &ddesc);
        if ( ret < 0 )
            return mkd_log(MKD_FAIL, LOG_ERR, FAC, "libsub_get_device_descriptor()=%li", ret);

        if ((ddesc.idVendor == LAC_VID) && (ddesc.idProduct == LAC_PID))
        {
            mkd_log( lac, LOG_DBG, FAC, "Found LAC device vid=0x%4.4X pid=0x%4.4X index=%i", LAC_VID, LAC_PID, lac);
            if (ret = libusb_open(dev, &lac_hnd[lac]))
                return mkd_log (MKD_FAIL, LOG_SYS, FAC, "libusb_open()=%i", ret);

            if (ret = libusb_claim_interface(lac_hnd[lac], usb_if))
                return mkd_log( MKD_FAIL, LOG_SYS, FAC, "libusb_claim_interface()=%li", ret);
            lac++;
        }
    }

//  Must find correct number of devices 
    if ( lac == LAC_COUNT )
        return mkd_log(lac, LOG_DBG, FAC, "Found %i LAC devices", lac);
    else
        return mkd_log(MKD_FAIL, LOG_ERR, FAC, "Found %i LAC devices, must be %i", lac, LAC_COUNT );
}

/* @brief Set a LAC destinations positions and optionally wait to be within tolerance
 *
 * @param[in] lac = LAC to be controlled 
 * @param[in] pos = Destination position
 * @param[in] tmo = Timeout. 0 = No wait
 *
 * @return    MKD_OK = Success, MKD_FAIL = Failure
 */
int lac_set_pos( int lac, int pos, int tmo )
{
    int tick  = TIM_TICK;                     // Timer ticks [ms]
    int count = TIM_MICROSECOND * tmo / tick; // Timer count [ms]
    int now;                                  // Current LAC position

    if ( mkd_simulate )
    {
        mkd_sim_pos[lac]=pos;
        return mkd_log( MKD_OK, LOG_DBG, FAC, "Simulated lac_set_pos(%i, %i, %i)", lac, pos, tmo );
    }

//  Set position
    if ( MKD_FAIL == lac_xfer( lac, LAC_SET_POSITION, pos ))
        return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_xfer() fail" );

//  If timeout supplied, wait for requested positions to be reached
    if ( tmo )
    {
        do
        {
            if ( MKD_FAIL == ( now = lac_xfer( lac, LAC_GET_FEEDBACK, 0 )))
                return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_xfer() fail" );
            usleep(tick);
            if ( abs( now - pos ) <= lac_Accuracy )  // Position is within tolerance
                return mkd_log( MKD_OK, LOG_DBG, FAC, "LAC Position Request=%i, Actual=%i", pos, now );
        } while( count-- );
        return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_set_pos() timeout" );
    }

    return mkd_log( MKD_OK, LOG_DBG, FAC, "No Wait LAC Position Request=%i", pos );
}


/* @brief Simultaneous set two LAC positions and optionally wait to be within tolerance
 * 
 * @param[in] pos0 = LAC0 position 
 * @param[in] pos1 = LAC1 position 
 * @param[in] tmo  = Timeout. 0 = No wait
 *
 * @return    MKD_OK = Success, MKD_FAIL = Failure
 */
int lac_set_both( int pos0, int pos1, int tmo )
{
    int tick  = TIM_TICK;                     // Timer ticks [s] 
    int count = TIM_MICROSECOND * tmo / tick; // Timer count [s] 
    int now0;  // Current LAC #0 position
    int now1;  // Current LAC #1 position 

    if ( mkd_simulate )
    {
	mkd_sim_pos[0]=(int)pos0;
	mkd_sim_pos[1]=(int)pos1;
        if ( tmo )
            return mkd_log( MKD_OK, LOG_DBG, FAC, "Simulated lac_set_both(%i, %i, %i)=%i", pos0, pos1, tmo, sleep(1) );
        else
            return mkd_log( MKD_OK, LOG_DBG, FAC, "Simulated lac_set_both(%i, %i, %i)=%i", pos0, pos1, tmo, 0        );
    }

//  Set positions
    if ( MKD_FAIL == lac_xfer( LAC_0, LAC_SET_POSITION, pos0 ))
        return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_xfer(LAC_0) fail" );
    if ( MKD_FAIL == lac_xfer( LAC_1, LAC_SET_POSITION, pos1 ))
        return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_xfer(LAC_1) fail" );

//  If timeout supplied, wait for requested positions to be reached
    if ( tmo ) 
    {
        do
        {
            if ( MKD_FAIL == ( now0 = lac_xfer( LAC_0, LAC_GET_FEEDBACK, 0 )))
                return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_xfer(LAC_1) fail" );
            usleep(tick);
            if ( MKD_FAIL == ( now1 = lac_xfer( LAC_1, LAC_GET_FEEDBACK, 0 )))
                return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_xfer(LAC_0) fail" );

            if (( abs( now0 - pos0 ) <= lac_Accuracy )&&  // Position is within tolerance
                ( abs( now1 - pos1 ) <= lac_Accuracy )  )
                return mkd_log( MKD_OK, LOG_DBG, FAC, "LAC Position Request: 0=%i 1=%i. Actual: 0=%i 1=%i", pos0, now0, pos1, now1 );

            mkd_log( MKD_OK, LOG_DBG, FAC, "lac_set_both() count=%i 0=%/i%i 1=%i/%i", count, pos0, now0, pos1, now1 );
        } while( count-- );
        return mkd_log( MKD_FAIL, LOG_ERR, FAC, "lac_set_both(%i, %i, %i) timeout", pos0, pos1, tmo );
    }

    return mkd_log( MKD_OK, LOG_DBG, FAC, "lac_set_both(%i/%i, %i/%i, %i)", pos0, now0, pos1, now1, tmo );
}


/* @brief USB bulk transfer a value to an address in the LAC board 
 * 
 * @param[in] lac  = Index number of LAC board, 0 or 1
 * @param[in] addr = Remote USB address
 * @param[in] val  = Value to be written 
 *
 * @return    Success = Positive value written, MKD_FAIL = Failure
 */
int lac_xfer( int lac, int addr, int val )
{
//    libusb_error err;  // USB error return
    int ret;
    unsigned int tmo = TMO_USB; // USB timeout in milliseconds 
    unsigned char buf[3];       // Bulk transfer buffer
    unsigned char endpt;        // USB endpoint

//  Populate the transfer buffer. Data is big endian
    buf[0]=addr;        // Destination
    buf[1]=val &  0xFF; // Low
    buf[2]=val >> 0x08; // High

//  USB bulk transfer OUT 
    endpt = 1 | LIBUSB_ENDPOINT_OUT; 
    if ( ret = libusb_bulk_transfer( lac_hnd[lac], endpt, buf, 3, NULL, tmo ) )
        return mkd_log( MKD_FAIL, LOG_ERR, FAC, "libsub_bulk_transfer()=%li", ret);
    mkd_log( MKD_OK, LOG_DBG, FAC, "LAC=%i Addr=0x%2.2X EndPt=0x%2.2X Buf=0x%2.2X 0x%2.2X 0x%2.2X Val=%i",
             lac, addr, endpt, buf[0], buf[1], buf[2], val );

//  USB bulk transfer IN
    endpt = 1 | LIBUSB_ENDPOINT_IN;
    if ( ret = libusb_bulk_transfer( lac_hnd[lac], endpt, buf, 3, NULL, tmo ) )
        return mkd_log(MKD_FAIL, LOG_ERR, FAC, "libsub_bulk_transfer()=%li", ret );
    val = buf[1] + (buf[2] << 8);         
    mkd_log( MKD_OK, LOG_DBG, FAC, "LAC=%i Addr=0x%2.2X EndPt=0x%2.2X Buf=0x%2.2X 0x%2.2X 0x%2.2X Val=%i",
             lac, addr, endpt, buf[0], buf[1], buf[2], val );
	
    return val;
}
