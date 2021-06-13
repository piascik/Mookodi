/** @file   mkd_ini.h
  *
  * @brief Look-up tables and associated #defines 
  *
  * @author asp
  *
  * @date   2021-05-21
  *
  * @version $Id$
  */

// Configuration information is read from a .ini file into global variables  
// Data types that can be read from .ini file
#define CFG_TYPE_STR      0
#define CFG_TYPE_INT      1
#define CFG_TYPE_DOUBLE   2
#define CFG_TYPE_BOOLEAN  3
#define CFG_TYPE_LIST     4

// Section names within .ini file
#define CFG_SECT_GEN "General"
#define CFG_SECT_LAC "LAC"
#define CFG_SECT_TMO "Timeouts"
#define CFG_SECT_PIO "PIO"

#ifdef MAIN
// Table defining configuration data that can be read from .ini file
mkd_ini_t mkd_ini[] = {
// Field text           Section          Field Type      Pointer to storage               Default  Min Max   Description
{ "WorkingDirectory"   ,CFG_SECT_GEN, CFG_TYPE_STR, &gen_DirWork            ,GEN_DIR_WORK , 0, 0}, //
{ "LogFile"            ,CFG_SECT_GEN, CFG_TYPE_STR, &gen_FileLog            ,GEN_FILE_LOG,  0, 0}, //
{ "Speed"              ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_Speed              ,NULL, 1023, 0, 1023}, //   
{ "Accuracy"           ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_Accuracy           ,NULL,    4, 0, 1023}, //   
{ "RetractLimit"       ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_RetractLimt        ,NULL,    0, 0, 1023}, //   
{ "ExtendLimit"        ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_ExtendLimit        ,NULL, 1023, 0, 1023}, //   
{ "MovementThreshold"  ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_MovementThreshold  ,NULL,    3, 0, 1023}, //   
{ "StallTime"          ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_StallTime          ,NULL,10000, 0, 1023}, //   
{ "PWMThreshold"       ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_PWMThreshold       ,NULL,   80, 0, 1023}, //   
{ "DerivativeThreshold",CFG_SECT_LAC, CFG_TYPE_INT, &lac_DerivativeThreshold,NULL,   10, 0, 1023}, //   
{ "DerivativeMaximum"  ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_DerivativeMaximum  ,NULL, 1023, 0, 1023}, //   
{ "DerivativeMinimum"  ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_DerivativeMinimum  ,NULL,    0, 0, 1023}, //   
{ "PWMMaximum"         ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_PWMMaximum         ,NULL, 1023, 0, 1023}, //   
{ "PWMMinimum"         ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_PWMMinimum         ,NULL,   80, 0, 1023}, //   
{ "ProportionalGain"   ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_ProportionalGain   ,NULL,    1, 0, 1023}, //   
{ "DerivativeGain"     ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_DerivativeGain     ,NULL,   10, 0, 1023}, //   
{ "AverageRC"          ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_AverageRC          ,NULL,    4, 0, 1023}, //   
{ "AverageADC"         ,CFG_SECT_LAC, CFG_TYPE_INT, &lac_AverageADC         ,NULL,    8, 0, 1023},
// 
{ "LAC0Filter0Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[0].pos[0] ,NULL,    0, 0, 1023},
{ "LAC0Filter1Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[0].pos[1] ,NULL,  256, 0, 1023},
{ "LAC0Filter2Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[0].pos[2] ,NULL,  512, 0, 1023},
{ "LAC0Filter3Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[0].pos[3] ,NULL,  768, 0, 1023},
{ "LAC0Filter4Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[0].pos[4] ,NULL, 1023, 0, 1023},
{ "LAC1Filter0Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[1].pos[0] ,NULL,    0, 0, 1023},
{ "LAC1Filter1Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[1].pos[1] ,NULL,  256, 0, 1023},
{ "LAC1Filter2Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[1].pos[2] ,NULL,  512, 0, 1023},
{ "LAC1Filter3Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[1].pos[3] ,NULL,  768, 0, 1023},
{ "LAC1Filter4Position",CFG_SECT_LAC, CFG_TYPE_INT, &lac_Actuator[1].pos[4] ,NULL, 1023, 0, 1023},
{ "LAC0Filter0Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[0].name[0],"LAC0Filter0",0,0,0},
{ "LAC0Filter1Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[0].name[1],"LAC0Filter1",0,0,0},
{ "LAC0Filter2Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[0].name[2],"LAC0Filter2",0,0,0},
{ "LAC0Filter3Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[0].name[3],"LAC0Filter3",0,0,0},
{ "LAC0Filter4Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[0].name[4],"LAC0Filter4",0,0,0},
{ "LAC1Filter0Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[1].name[0],"LAC1Filter0",0,0,0},
{ "LAC1Filter1Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[1].name[1],"LAC1Filter1",0,0,0},
{ "LAC1Filter2Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[1].name[2],"LAC1Filter2",0,0,0},
{ "LAC1Filter3Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[1].name[3],"LAC1Filter3",0,0,0},
{ "LAC1Filter4Name"    ,CFG_SECT_LAC, CFG_TYPE_STR, &lac_Actuator[1].name[4],"LAC1Filte4r",0,0,0},
//
{ "PIODevice"          ,CFG_SECT_PIO, CFG_TYPE_STR, &pio_device             , PIO_DEV_NAME,0,0,0}
};//

  size_t mkd_ini_siz = sizeof(mkd_ini_t);
  size_t mkd_ini_num = sizeof(mkd_ini) / sizeof(mkd_ini_t);
#else
  extern mkd_ini_t mkd_ini[];
  extern size_t mkd_ini_siz;
  extern size_t mkd_ini_num;
#endif

