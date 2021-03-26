#include "mkd.h"
#define FAC FAC_INI


/*!
 * Read configuration .ini file
 */
int ini_read( const char *fname )
{
    int i;

    PIniFile *file;
    PError   *error;

    file = p_ini_file_new( fname );

    if ( !p_ini_file_parse( file, &error ) )
    {
        return mkd_log( MOK_FAIL, LOG_ERR, FAC, "Failed to read init. file = %s", fname );
    }
    else
    {
        for( i = mkd_ini_num; i-- ; ) {
            switch ( mkd_ini[i].type) {
                case CFG_TYPE_INT:
                    *(int *)mkd_ini[i].ptr = p_ini_file_parameter_int( file, mkd_ini[i].sect, mkd_ini[i].key, mkd_ini[i].val );
                    mkd_log( MOK_OK, LOG_DBG, FAC, "%s = %i", mkd_ini[i].key, *(int *)mkd_ini[i].ptr );
                    break;

                case CFG_TYPE_DOUBLE:
                    *(double *)mkd_ini[i].ptr = p_ini_file_parameter_double( file, mkd_ini[i].sect, mkd_ini[i].key, mkd_ini[i].val );
                    mkd_log( MOK_OK, LOG_DBG, FAC, "%s = %f", mkd_ini[i].key, *(double *)mkd_ini[i].ptr );
                    break;

                case CFG_TYPE_BOOLEAN:
                    *(unsigned char *)mkd_ini[i].ptr = p_ini_file_parameter_boolean( file, mkd_ini[i].sect, mkd_ini[i].key, mkd_ini[i].val );
                    mkd_log( MOK_OK, LOG_DBG, FAC, "%s = %s", mkd_ini[i].key, *(int *)mkd_ini[i].ptr ? "TRUE":"FALSE");
                    break;

                case CFG_TYPE_STR:
                    mkd_ini[i].ptr = (char *)p_ini_file_parameter_string( file, mkd_ini[i].sect, mkd_ini[i].key, mkd_ini[i].str );
                    mkd_log( MOK_OK, LOG_DBG, FAC, "%s = %s", mkd_ini[i].key, (char *)mkd_ini[i].ptr );
                    break;

                default:
                    break;
            }
        }
    }
    return MOK_OK;
}

