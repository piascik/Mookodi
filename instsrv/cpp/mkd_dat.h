/** @file   mkd_dat.h
  *
  * @brief  Mookodi global variables. Data declared in main module, external everywhere else
  * 
  * @author asp 
  * 
  * @date   2021-11-15 
  * 
  * @version $Id$
  */

#ifdef MAIN

// Colour codes for text output
const char *log_colour[] =
{   
    COL_NULL,  // NONE - Suppress all 
    COL_RED,   // CRIT - Critical error
    COL_RED,   // SYS  - Unrecoverable system error
    COL_RED,   // ERR  - Unrecoverable application
    COL_YELLOW,// WRN  - Recoverable/retry warning 
    COL_NULL,  // IMG  - Image acquisition 
    COL_NULL,  // INF  - Informational 
    COL_BLUE,  // MSG  - Messaging    
    COL_GREEN, // DBG  - Debug
    COL_NULL,  // CMD  - Default 
};

// Facility names, NUL is unused. Must match FAC order in mdk.h
const char *fac_lvls[] = {"NUL","MKD","LOG","UTL","INI","CAM","PIO","FTS","LAC","OPT"};

// In decreasing order of severity as used for debug level, Must match LOG order in mdk.h
const char *log_lvls[] = {"NONE","CRIT","SYS","ERR","WRN","IMG","INF","MEC","DBG"};

int   log_lvl = LOG_WRN;      // Default log level
char  log_pfx[MAX_STR];       // Prefix log lines with this text (debug)
char *log_fn  = NULL;         // Log file name 
FILE *log_fp  = stdout;       // Log file handle
bool  log_dbg = FALSE;        // Log mode, to file or screen debugging

const char *pio_device = PIO_DEV_NAME; // Default PIO serial device 

const char *gen_DirWork      = "./";
const char *gen_FileInit     = GEN_FILE_INIT;
const char *gen_FileLog      = GEN_FILE_LOG;

int   lac_Speed              = 1023;
int   lac_Accuracy           =    4;
int   lac_RetractLimt        =    0;
int   lac_ExtendLimit        = 1023;
int   lac_MovementThreshold  =    3;
int   lac_StallTime          =10000;
int   lac_PWMThreshold       =   80;
int   lac_DerivativeThreshold=   10;
int   lac_DerivativeMaximum  = 1023;
int   lac_DerivativeMinimum  =    0;
int   lac_PWMMaximum         = 1023;
int   lac_PWMMinimum         =   80;
int   lac_ProportionalGain   =    1;
int   lac_DerivativeGain     =   10;
int   lac_AverageRC          =    4;
int   lac_AverageADC         =    8;

int        lac_State   [LAC_COUNT] = {-1,-1};
mkd_lac_t  lac_Actuator[LAC_COUNT];

bool mkd_simulate            = FALSE;
unsigned char mkd_sim_inp    =     0;
unsigned char mkd_sim_out    =     0;
int           mkd_sim_pos[2] =  {0,0};

#else

extern char *log_colour[]         ;
extern char *whl_colour[]         ;
extern char *fac_lvls[]           ;

extern char *log_lvls[]           ;
extern int   log_lvl              ;
extern char  log_pfx[MAX_STR]     ;
extern char *log_fn               ;
extern FILE *log_fp               ;
extern bool  log_dbg              ;

extern int   gen_DirWork          ; 
extern const char *gen_FileInit   ; 
extern const char *gen_FileLog    ;

extern char *pio_device           ; 

extern int lac_Speed              ;
extern int lac_Accuracy           ;
extern int lac_RetractLimt        ; 
extern int lac_ExtendLimit        ;       
extern int lac_MovementThreshold  ;
extern int lac_StallTime          ; 
extern int lac_PWMThreshold       ; 
extern int lac_DerivativeThreshold;
extern int lac_DerivativeMaximum  ; 
extern int lac_DerivativeMinimum  ; 
extern int lac_PWMMaximum         ; 
extern int lac_PWMMinimum         ;
extern int lac_ProportionalGain   ;
extern int lac_DerivativeGain     ;
extern int lac_AverageRC          ;
extern int lac_AverageADC         ;

extern int        lac_State   [LAC_COUNT];
extern mkd_lac_t  lac_Actuator[LAC_COUNT];

extern bool          mkd_simulate ;
extern unsigned char mkd_sim_inp  ;
extern unsigned char mkd_sim_out  ;
extern          int  mkd_sim_pos[2];
#endif                            
