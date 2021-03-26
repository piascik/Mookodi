#include "mok.h"
#define FAC FAC_ACT

// Local static data
static int                    usb_if;     // USB interface
static struct libusb_context *usb_ctx;    // USB context
static libusb_device_handle  *lac_hnd[LAC_COUNT]; // LAC USB device handles

/* @brief Initialise USB and static LAC variables  
 */
void lac_init( void )
{
    usb_if  = 0;
    usb_ctx = NULL;

    for ( int i=0; i < LAC_COUNT; i++ )
        lac_hnd[i] = NULL;

    libusb_init( &usb_ctx );
}


/* @brief Release LAC USB handles and exit USB context
 */
void lac_close( void )
{
    int ret;

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
 * @return   MOK_OK = Success, MOK_FAIL = Failure 
 */
int lac_conf( void )
{
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
            return MOK_OK;
        }
        else                         
        {
            return MOK_FAIL;
        }
    }
}


/* @brief    Search for LAC USB devices, open handles and claim  
 *
 * @return   Number of LAC devices found,  MOK_FAIL = Failure
 */
int lac_open( void )
{
    int ret;
    int count;   // Number of USB devices found
    int lac = 0; // Number of LAC devices found

    libusb_device **usb_dev;
    libusb_device  *dev = NULL;
    struct libusb_device_descriptor ddesc;

    if ( 0 >= (count = libusb_get_device_list( usb_ctx, &usb_dev)))
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "libusb_get_device()=%i=%s", count, libusb_strerror(count));
    mkd_log(MOK_OK, LOG_DBG, FAC, "Found %i USB devices", count);

//  Search all USB devices for LACs 
    for( int i=0; i < count; i++)
    {
        dev = usb_dev[i];
        ret = libusb_get_device_descriptor(dev, &ddesc);
        if ( ret < 0 )
            return mkd_log(MOK_FAIL, LOG_ERR, FAC, "libsub_get_device_descriptor()=%i=%s", ret, libusb_strerror(ret));

        if ((ddesc.idVendor == LAC_VID) && (ddesc.idProduct == LAC_PID))
        {
            mkd_log( lac, LOG_DBG, FAC, "Found LAC device vid=0x%4.4X pid=0x%4.4X index=%i", LAC_VID, LAC_PID, lac);
            if (ret = libusb_open(dev, &lac_hnd[lac]))
                return mkd_log (MOK_FAIL, LOG_SYS, FAC, "libusb_open()=%i=%s", ret, libusb_strerror(ret));

            if (ret = libusb_claim_interface(lac_hnd[lac], usb_if))
                return mkd_log( MOK_FAIL, LOG_SYS, FAC, "libusb_claim_interface()=%i=%s", ret, libusb_strerror(ret));
            lac++;
        }
    }

//  Must find correct number of devices 
    if ( lac == LAC_COUNT )
        return mkd_log(lac, LOG_DBG, FAC, "Found %i LAC devices", lac);
    else
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "Found %i LAC devices, must be %i", lac, LAC_COUNT );
}


/* @brief Simultaneous set of the LAC destinations positions and wait to be in tolerance
 * 
 * @param[in] pos1 = LAC1 destination position 
 * @param[in] pos2 = LAC2 destination position 
 * @param[in] tmo  = Timeout 
 *
 * @return    MOK_OK = Success, MOK_FAIL = Failure
 */
int lac_set_pos( int pos1, int pos2, int tmo )
{
    int tick  = TIM_TICK;                     // Timer ticks [ms] 
    int count = TIM_MICROSECOND * tmo / tick; // Timer count [ms] 
    int now1;  // Current LAC #1 position
    int now2;  // Current LAC #2 position 

//  Set positions
    if ( MOK_FAIL == lac_xfer( LAC_ONE, LAC_SET_POSITION, pos1 ))
        return mkd_log( MOK_FAIL, LOG_ERR, FAC, "lac_xfer(LAC1) fail" );
    if ( MOK_FAIL == lac_xfer( LAC_TWO, LAC_SET_POSITION, pos2 ))
        return mkd_log( MOK_FAIL, LOG_ERR, FAC, "lac_xfer(LAC2) fail" );

//  Wait for positions to be reached before timeout
    do
    {
        if ( MOK_FAIL == ( now1 = lac_xfer( LAC_ONE, LAC_GET_FEEDBACK, 0 )))
            return mkd_log( MOK_FAIL, LOG_ERR, FAC, "lac_xfer(LAC1) fail" );
        usleep(tick);
        if ( MOK_FAIL == ( now2 = lac_xfer( LAC_TWO, LAC_GET_FEEDBACK, 0 )))
            return mkd_log( MOK_FAIL, LOG_ERR, FAC, "lac_xfer(LAC2) fail" );

        if (( abs( now1 - pos1 ) <= lac_Accuracy )&&  // Position is within tolerance
            ( abs( now2 - pos2 ) <= lac_Accuracy )  )
            return mkd_log( MOK_OK, LOG_DBG, FAC, "Position Actual(Requested) LAC1=%i(%i) LAC2=%i(%i)", now1, pos1, now2, pos2 );
    } while( count-- );


    return mkd_log( MOK_FAIL, LOG_ERR, FAC, "lac_set_pos() timeout" );
}


/* @brief USB bulk transfer a value to an address in the LAC board 
 * 
 * @param[in] lac  = Index number of LAC board, 0 or 1
 * @param[in] addr = Remote USB address
 * @param[in] val  = Value to be written 
 *
 * @return    Success = Positive value written, MOK_FAIL = Failure
 */
int lac_xfer( int lac, int addr, int val )
{
    int ret;
    unsigned int tmo = 1000; // USB timeout in milliseconds 
    unsigned char buf[3];    // Bulk transfer buffer
    unsigned char endpt;     // USB endpoint

//  Populate the transfer buffer. Data is big endian
    buf[0]=addr;        // Destination
    buf[1]=val &  0xFF; // Low
    buf[2]=val >> 0x08; // High

//  USB bulk transfer OUT 
    endpt = 1 | LIBUSB_ENDPOINT_OUT; 
    if ( ret = libusb_bulk_transfer( lac_hnd[lac], endpt, buf, 3, NULL, tmo ) )
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "libsub_bulk_transfer()=%i=%s", ret, libusb_strerror(ret));
    mkd_log( MOK_OK, LOG_DBG, FAC, "LAC=%i Addr=0x%2.2X EndPt=0x%2.2X Buf=0x%2.2X 0x%2.2X 0x%2.2X Val=%i",
             lac, addr, endpt, buf[0], buf[1], buf[2], val );

//  USB bulk transfer IN
    endpt = 1 | LIBUSB_ENDPOINT_IN;
    if ( ret = libusb_bulk_transfer( lac_hnd[lac], endpt, buf, 3, NULL, tmo ) )
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "libsub_bulk_transfer()=%i=%s", ret, libusb_strerror(errno)  );
    val = buf[1] + (buf[2] << 8);         
    mkd_log( MOK_OK, LOG_DBG, FAC, "LAC=%i Addr=0x%2.2X EndPt=0x%2.2X Buf=0x%2.2X 0x%2.2X 0x%2.2X Val=%i",
             lac, addr, endpt, buf[0], buf[1], buf[2], val );
	
    return val;
}
