#include "mkd.h"
#define FAC FAC_PIO

#include "InstSrv.h"
//#include <thrift/protocol/TBinaryProtocol.h>
//#include <thrift/server/TSimpleServer.h>
//#include <thrift/transport/TServerSocket.h>
//#include <thrift/transport/TBufferTransports.h>



static int pio_fd = 0;

int pio_open( const char *device  )
{
    if ( (pio_fd = open( device, O_RDWR | O_NOCTTY | O_SYNC )) < 0 )
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "open(%s)=%i=%s", device, errno, strerror(errno)); 
    else
        return MOK_OK;   
}


/* @brief Set serial port attributes
 *
 */
int pio_set_attrib( int baud, int parity)
{
    struct termios tty;

    if (tcgetattr(pio_fd, &tty) != 0)
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "tcgetattr()=%i=%s", errno, strerror(errno)); 

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched baud tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK; // disable break processing

    tty.c_lflag     = 0;    // no signaling chars, no echo,
    tty.c_oflag     = 0;    // no remapping, no delays
    tty.c_cc[VMIN]  = 0;    // read doesn't block
    tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |=  (CLOCAL | CREAD);       // ignore modem controls,
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |=  parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (pio_fd, TCSANOW, &tty) != 0)
        return mkd_log( MOK_FAIL, LOG_ERR, FAC, "tcsetattr()=%i=%s", errno, strerror(errno)); 

    return MOK_OK;
}


/* @brief Enable or disable serial block when reading
 *
 * @param[in]   block = true | false
 *
 * @return      MOK_OK = Success, MOK_FAIL = Failure 
 */
int pio_set_blocking(int block)
{
    struct termios tty;

    memset(&tty, 0, sizeof tty);
    if (tcgetattr(pio_fd, &tty) != 0)
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "tcgetattr()=%i=%s", errno, strerror(errno)); 

    tty.c_cc[VMIN]  = block ? 1 : 0;
    tty.c_cc[VTIME] = 5;             // 0.5 seconds read timeout

    if (tcsetattr (pio_fd, TCSANOW, &tty) != 0)
        return mkd_log(MOK_FAIL, LOG_ERR, FAC, "tcsetattr()=%i=%s", errno, strerror(errno)); 

    return MOK_OK;
}


/* @brief Set PIO port 0 output state
 *
 * @param[out]  out = PIO output state as unsigned byte   
 *
 * @return  0 = Success, -1 = Device accces fail
 */
int pio_set_output( unsigned char out, int ret ) 
{
    char cmd[MAX_STR+1];
    char chk[MAX_STR+1];
    char buf[MAX_STR+1];

//  Set port 0 to be output 
    sprintf( cmd, "@00D000" );
    sprintf( chk, "!00" );
    if (MOK_FAIL == pio_command( cmd, chk, buf, MAX_STR )) 
        return DeployState::ERR; 
  
//  Set output value 
    sprintf( cmd, "@00P0%2.2X", out );
    sprintf( chk, "!00" );
    if (MOK_FAIL == pio_command( cmd, chk, buf, MAX_STR ) )
        return DeployState::ERR; 

    return ret;
}


/* @brief Get PIO port 0 output state
 *
 * @param[out] *out = PIO output state as unsigned byte   
 *
 * @return  MOK_OK = Success, MOK_FAIL = Device access or set value fail
 */
int pio_get_output( unsigned char *out ) 
{
    char c; // Check for trailing character

    char cmd[MAX_STR+1];
    char chk[MAX_STR+1];
    char buf[MAX_STR+1];

//  Set port 0 to be output 
    sprintf( cmd, "@00D000" );
    sprintf( chk, "!00" );
    if ( MOK_FAIL == pio_command( cmd, chk, buf, MAX_STR )) 
        return MOK_FAIL; 

// Create output query command. No check 
   sprintf( cmd, "@00P0?" );
   if ( MOK_FAIL == pio_command( cmd, NULL, buf, MAX_STR ) )
       return MOK_FAIL;

//  Parse returned hex value into out byte. Only 1 parameter should be converted
    if ( 1 == sscanf( buf, "!00%hhu%c", out, &c ) )
        return  MOK_OK;  
    else
        return MOK_FAIL;
}


/* @brief Get PIO port 1 input state
 *
 * @param[out] *inp = PIO device input state as unsigned byte
 *
 * @return  MOK_OK = Success, MOK_FAIL = Device accces or set device fail
 */
int pio_get_input( unsigned char *inp ) 
{
    char c; // Check for trailing character

    char buf[MAX_STR+1];
    char cmd[MAX_STR+1];
    char chk[MAX_STR+1];

    int ret;

//  Set port 1  to input 
    sprintf( cmd, "@00D1FF" );
    sprintf( chk, "!00" );
    if ( MOK_FAIL == pio_command( cmd, chk, buf, MAX_STR ) )
        return MOK_FAIL;

// Create query command. No check. 
    sprintf( cmd, "@00P1?" );
    if ( MOK_FAIL == pio_command( cmd, NULL, buf, MAX_STR ) )
        return MOK_FAIL;

//  Parse returned hex string into byte. Only 1 parameter should be converted
    if ( 1 == sscanf( buf, "!00%hhu%c", inp, &c ) )
        return MOK_OK;  
    else
        return MOK_FAIL; 
}


/* @brief Issue a command to PIO module and optionally wait or check the reply 
 *
 * @param[in]  *cmd = Command string to send
 * @param[in]  *chk = Expected reply. NULL suppresses checking reply.
 * @param[out] *rep = Buffer to hold reply. NULL suppresses reading reply.
 * @param[in[   max = Maximum size of *rep reply buffer 
 * 
 * @return      Length of reply, -1 = error 
 */
int pio_command( char *cmd, char *chk, char *rep, int max )
{
    int len;
    char buf[MAX_STR+1]; // Flush buffer

//  Sync to prepare flush  
    if ( !fsync(pio_fd) )
        return mkd_log( MOK_FAIL, LOG_SYS, FAC, " fsync()=%i=%s", errno, strerror(errno)); 

//  Flush any input data, there should be none
    len = read(pio_fd, buf, MAX_STR);
    if ( len == -1 ) 
        return mkd_log( MOK_FAIL, LOG_SYS, FAC, "  read()=%i=%s", errno, strerror(errno)); 
    else if ( len > 0 )
        mkd_log( len, LOG_WRN, FAC, "  read() flush=%s", buf ); 

//  Send command to PIO interface
    len = strlen( cmd );
    cmd[len] = '\r';
    if ( -1 == write( pio_fd, cmd, len+1 ) )
        return mkd_log( MOK_FAIL, LOG_SYS, FAC, " write()=%i=%s", errno, strerror(errno)); 
    cmd[len] = '\0';
    mkd_log( len, LOG_DBG, FAC, " write()=%s", cmd); 

//  Serial comms. is slow, so pause for data 
//    usleep( (len+25) * 100 );

//  Need to check the reply?
    if ( rep )
    {
        len = read( pio_fd, rep, max );
        if ( len <= 0 )
            return mkd_log( MOK_FAIL, LOG_SYS, FAC, "  read()=%i%s", errno, strerror(errno)); 
        else
        {
            rep[--len]='\0'; 
            mkd_log( len, LOG_DBG, FAC, "  read()=%s", rep ); 
            if ( chk )
                if ( strcmp( chk, rep ) )
                    mkd_log( MOK_FAIL, LOG_ERR, FAC, "strcmp()=%s, expected=%s", rep, chk); 
                else              
                    return mkd_log( len, LOG_DBG, FAC, "strcmp()=%s", rep); 
        }
    }
    else
    {
        return mkd_log( MOK_OK, LOG_DBG, FAC, "read() suppressed"); 
    }

    return -1;
}
