/** @file   mok.h
  *
  * @brief  MOPTOP general header included by all modules
  *
  * @author asp 
  *
  * @date   2021-04-22 
  */

// Needed for extended dirent() functionality
//#define _GNU_SOURCE

// Standard C headers 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <wchar.h>
#include <termios.h>
#include <assert.h>

// System headers
#include <sys/dir.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

// Linux headers
#include <linux/hidraw.h>

// Network headers
#include <arpa/inet.h>
#include <netinet/in.h>

// Non-standard 3rd party headers
#include <libusb-1.0/libusb.h> // libusb 1.0
#include <plibsys.h>           // plibsys 0.4


// C++ headers
#include <iostream>

// log4cxx
#include "log4cxx/logger.h"
#include "log4cxx/basicconfigurator.h"
#include "log4cxx/helpers/exception.h"


/** CJM 
 * This enum describes a verbosity filtering level of a log message. The idea is that the high priority/
 * terse level messages are always displayed, whilst the detail/very verbose messages can be filtered out.
 * This enum copied from log_udp.h, so we can remove log_udp dependancy from mookodi.
 * <dl>
 * <dt>LOG_VERBOSITY_VERY_TERSE</dt> <dd>High priority/top level message.</dd>
 * <dt>LOG_VERBOSITY_TERSE</dt> <dd> Higher priority message.</dd>
 * <dt>LOG_VERBOSITY_INTERMEDIATE</dt> <dd>Intermediate level message.</dd>
 * <dt>LOG_VERBOSITY_VERBOSE</dt> <dd>Lower priority/more detailed/verbose message.</dd>
 * <dt>LOG_VERBOSITY_VERY_VERBOSE</dt> <dd>Lowest level/most verbose message.</dd>
 * </dl>
 */
enum LOG_VERBOSITY
{
	LOG_VERBOSITY_VERY_TERSE=1,
	LOG_VERBOSITY_TERSE=2,
	LOG_VERBOSITY_INTERMEDIATE=3,
	LOG_VERBOSITY_VERBOSE=4,
	LOG_VERBOSITY_VERY_VERBOSE=5
};

#define PROC_NAME     "InstSrv" //!< Process name

#define MAX_STR       256          //!<  Maximum string length

// Log levels
#define LOG_NONE 0  //!< Suppress all log errors
#define LOG_CRIT 1  //!< Log Critical errors, process will exit 
#define LOG_SYS  2  //!< Log System errors, operation failed
#define LOG_ERR  3  //!< Log Programme errors, operation failed 
#define LOG_WRN  4  //!< Log Warnings, will continue 
#define LOG_IMG  5  //!< Log Image acquisition   
#define LOG_INF  6  //!< Log Informational  
#define LOG_MSG  7  //!< Log Message events 
#define LOG_DBG  8  //!< Debug logging, verbose  
#define LOG_CMD  9  //!< Log Command process events 

// Facilities (software module)
#define FAC_NUL  0  //!< Unused 
#define FAC_MKD  1  //!< Main programme
#define FAC_LOG  2  //!< Logging
#define FAC_UTL  3  //!< Utilties
#define FAC_INI  4  //!< Initialisation parsing
#define FAC_CAM  5  //!< Camera operations
#define FAC_PIO  6  //!< PIO operations
#define FAC_FTS  7  //!< FITS file handling
#define FAC_ACT  8  //!< Actuator 
#define FAC_WHL  9  //!< Filter wheel     
#define FAC_CMD  10 //!< Commands to service 

// ANSI text colour 30=Black, 31=red, 32=green, 33=yellow, 34=blue, 35=magenta, 36=cyan, 37=white  
#define COL_RED     "\x1b[31m"  
#define COL_GREEN   "\x1b[32m"  
#define COL_YELLOW  "\x1b[33m"  
#define COL_BLUE    "\x1b[34m"  
#define COL_MAGENTA "\x1b[35m"    
#define COL_CYAN    "\x1b[36m"   
#define COL_WHITE   "\x1b[37m"    
#define COL_RESET   "\x1b[0m"    
#define COL_NULL    NULL

// Time related constants
#define TIM_TICK         1000000
#define TIM_MILLISECOND  1000.0
#define TIM_MICROSECOND  1000000.0
#define TIM_NANOSECOND   1000000000

// PIO BMCM module commands
#define PIO_DEV_NAME     "/dev/ttyACM0"
#define PIO_CMD_GET_NAME "$00M\r"
#define PIO_CMD_SET0_OUTPUT "@00D0FF\r"
#define PIO_REP_SET0_OUTPUT "@00FF\r"
#define PIO_CMD_CHK0_OUTPUT "@00D0?\r"

#define PIO_CMD_SET1_INPUT  "@00D100\r"
#define PIO_CMD_CHK1_INPUT  "@00D1?\r"
#define PIO_REP_CHK1_INPUT  "@0000\r"

// PIO via BMCM module input/output bit assignments
#define PIO_INP_SPARE_0       0b00000001
#define PIO_INP_SPARE_1       0b00000010
#define PIO_INP_GRISM_DEPLOY  0b00000100
#define PIO_INP_GRISM_STOW    0b00001000
#define PIO_INP_SLIT_DEPLOY   0b00010000
#define PIO_INP_SLIT_STOW     0b00100000
#define PIO_INP_MIRROR_DEPLOY 0b01000000
#define PIO_INP_MIRROR_STOW   0b10000000

#define PIO_OUT_SPARE0        0b00000001
#define PIO_OUT_SPARE1        0b00000010
#define PIO_OUT_SPARE2        0b00000100
#define PIO_OUT_GRISM_DEPLOY  0b00001000
#define PIO_OUT_SLIT_DEPLOY   0b00010000
#define PIO_OUT_MIRROR_DEPLOY 0b00100000
#define PIO_OUT_ARC_ON        0b01000000
#define PIO_OUT_WLAMP_ON      0b10000000

// Imaging. Nothing deployed
#define CFG_INP_IMAG_OBJ       (0b00000000)
#define CFG_OUT_IMAG_OBJ       (0b00000000)

// Object spectrum. Grism and slit deployed
#define CFG_INP_SPEC_OBJ       (PIO_INP_GRISM_DEPLOY & PIO_INP_SLIT_DEPLOY)
#define CFG_OUT_SPEC_OBJ       (PIO_OUT_GRISM_DEPLOY & PIO_OUT_SLIT_DEPLOY)

// Arc spectrum. Grism, slit and mirror deployed, arc on
#define CFG_INP_SPEC_ARC       (PIO_INP_GRISM_DEPLOY & PIO_INP_SLIT_DEPLOY & PIO_INP_MIRROR_DEPLOY)
#define CFG_OUT_SPEC_ARC       (PIO_OUT_GRISM_DEPLOY & PIO_OUT_SLIT_DEPLOY & PIO_OUT_MIRROR_DEPLOY & PIO_OUT_ARC_ON)

// Lamp flat. Mirror deployed and lamp on
#define CFG_INP_FLAT_LAMP      (PIO_INP_MIRROR_DEPLOY)
#define CFG_OUT_FLAT_LAMP      (PIO_OUT_MIRROR_DEPLOY & PIO_OUT_WLAMP_ON)

// General purpose configs
#define GEN_DIR_WORK          "./"
#define GEN_FILE_INIT         "mok.ini"
#define GEN_FILE_LOG          "mok.log"

// Linear Actuator Controller (LAC) 
#define LAC_VID               0x04D8 // Vendor ID  = Microchip Technology Inc.
#define LAC_PID               0xFC5F // Product ID = Custom USB device             
#define LAC_COUNT             2 // Number of LACs
#define LAC_ONE		      0 // Index into LAC tables 
#define LAC_TWO		      1 
#define LAC_POSITIONS         5 // Number of filter positions

// LAC registers 
#define LAC_SET_ACCURACY              0x01
#define LAC_SET_RETRACT_LIMIT         0x02
#define LAC_SET_EXTEND_LIMIT          0x03
#define LAC_SET_MOVEMENT_THRESHOLD    0x04
#define LAC_SET_STALL_TIME            0x05
#define LAC_SET_PWM_THRESHOLD         0x06
#define LAC_SET_DERIVATIVE_THRESHOLD  0x07
#define LAC_SET_DERIVATIVE_MAXIMUM    0x08
#define LAC_SET_DERIVATIVE_MINIMUM    0x09
#define LAC_SET_PWM_MAXIMUM           0x0A
#define LAC_SET_PWM_MINIMUM           0x0B
#define LAC_SET_PROPORTIONAL_GAIN     0x0C 
#define LAC_SET_DERIVATIVE_GAIN       0x0D
#define LAC_SET_AVERAGE_RC            0x0E
#define LAC_SET_AVERAGE_ADC           0x0F
#define LAC_GET_FEEDBACK              0x10
#define LAC_SET_POSITION              0x20
#define LAC_SET_SPEED                 0x21
#define LAC_DISABLE_MANUAL            0x30
#define LAC_RESET                     0xFF

// Function return values 
#define MKD_OK    0   
#define MKD_FAIL -1

//using namespace ::apache::thrift;
//using namespace ::apache::thrift::protocol;
//using namespace ::apache::thrift::transport;
//using namespace ::apache::thrift::server;

// Mookodi configuration table entry
typedef struct mkd_ini_s {
    const char   *key;  // Key name
    const char   *sect; // Section name
    int     type; // Data type
    void   *ptr;  // Pointer to variable
    const char   *str;  // Default string value
    double  val;  // Default value
    double  lo;   // Limit sanity check 
    double  hi;   // Limit sanity check
} mkd_ini_t;

// Pre-defined LAC positions
typedef struct mkd_lac_s{
    int pos [LAC_POSITIONS];
    int name[LAC_POSITIONS];
} mkd_lac_t;

// Configuration request  
typedef struct mkd_cfg_s {
    int  pos [LAC_COUNT]; // LAC positions
    int *name[LAC_COUNT]; // LAC names
    unsigned char out;    // PIO output bit mask
    unsigned char inp;    // PIO input bit mask 
    int tmo;              // Timeout in milliseconds
    int status;
} mkd_cfg_t;

// Function prototypes

// Logging
int mkd_log( int ret, int level, int fac, const char *fmt, ... );
static void log_to_stdout( char *sub_system, 
                           char *source_filename, 
                           char *function,
                           int   level, 
                           char *category, 
                           char *string);


static void log_to_log4cxx( char *sub_system, 
                            char *source_filename, 
                            char *function,
                            int   level, 
                            char *category, 
                            char *string);

// I/O module functions 
int pio_open        ( const char *device );
int pio_set_attrib  ( int baud, int parity );
int pio_set_blocking( int block );
int pio_get_input   ( unsigned char *inp );
int pio_get_output  ( unsigned char *out );
int pio_set_output  ( unsigned char  out, int ret );
int pio_command     ( char *cmd, char *chk, char *rep, int max );

// Linear actuator functions
void lac_init ( void  );
int  lac_open ( void  );
void lac_close( void  );
int  lac_conf ( void  );
int  lac_info ( int n );
void lac_debug( int n );
int  lac_set_pos( int pos1, int pos2, int tmo );
int  lac_xfer( int act, int addr, int val );

// 
int  ini_read( const char *fname );
int  utl_rnd( int lo, int  hi );
 
#include "mkd_dat.h"
#include "mkd_ini.h"
