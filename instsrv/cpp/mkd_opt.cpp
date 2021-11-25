/** @file   mkd_opt.cpp
  *
  * @brief  Read command ine options 
  *
  * @author asp
  *
  * @date   2021-11-14
  *
  * @version $Id$
  */

#define FAC FAC_OPT
#include "mkd.h"

/* @brief Read command line 
 *
 * @param[in]   argc  
 * @param[in]  *argv[] 
 *
 * @return      MKD_OK = Success, MKD_FAIL = Failure
 */
int mkd_opts( int argc, char *argv[] )
{
    int c;

    opterr = 0;
    while (( c = getopt( argc, argv, "hsd:D:c:")) != -1 )
    {
        switch (c)
        {
            case 's':
                mkd_simulate = TRUE;
                break;

            case 'c':
                if ( !access( optarg, F_OK ) ) 
                {
                    gen_FileInit = optarg;
                    mkd_log( MKD_OK, LOG_INF, FAC, "Using alternate config. file %", gen_FileInit ); 
                }
                else
                {
                    mkd_log( MKD_OK, LOG_ERR, FAC, "Alternate config. file %s not found. Exiting.", optarg ); 
                    exit( EXIT_FAILURE );
                }
                break;

            case 'd':
                log_lvl = atoi( optarg );
                log_dbg = FALSE;
                break;

            case 'D':
                log_lvl = atoi( optarg );
                log_dbg = TRUE;
                break;

            case 'h':
                printf("Usage: %s [Options]\n", argv[0] );
                printf(" -s  Simulate\n"  );
                printf(" -c  Config file [%s]\n", gen_FileInit  );
                printf(" -d  Debug level 0-8 (to file)\n"  );
                printf(" -D  Debug level 0-8 (to screen)\n");
                printf(" -h  Help\n"          );
                exit( EXIT_SUCCESS );
                break;

            case '?': // Catch argument errors
//                if ( strchr( check, optopt ) )
//                    mkd_log( false, LOG_WRN, FAC, "Option -%c missing argument", optopt);
                if ( isprint(optopt) )
                    mkd_log( false, LOG_WRN, FAC, "Option -%c unsupported", optopt);
                else
                    mkd_log( false, LOG_WRN, FAC, "Option character -0x%02x unsupported", optopt);
                break;

            default:
                printf( "Invalid option. Exiting\n"); 
                exit( EXIT_SUCCESS );
                break;
        }
    }
    return MKD_OK;
}
