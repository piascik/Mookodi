#!/bin/csh
# Source this file to set various environment variables used to build mookodi
setenv SRC_HOME         "/home/dev/src/"
setenv MOOKODI_LIB_HOME "${SRC_HOME}/Mookodi/bin/lib/x86_64-linux/"
setenv CCSHAREDFLAG     "-shared"
if ( ${?LD_LIBRARY_PATH} ) then
    setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${MOOKODI_LIB_HOME}:${SRC_HOME}/andor/andor-2.104.30000/lib/
else
    setenv LD_LIBRARY_PATH ${MOOKODI_LIB_HOME}:${SRC_HOME}/andor/andor-2.104.30000/lib/
endif
if( -d ${SRC_HOME}/cfitsio3310_x64/include ) then
# ltdevx64
    setenv CFITSIOINCDIR "${SRC_HOME}/cfitsio3310_x64/include"
else if ( -d ${SRC_HOME}/cfitsio-3.47 ) then
# zen
    setenv CFITSIOINCDIR "${SRC_HOME}/cfitsio-3.47"
else if ( -d ${SRC_HOME}/cfitsio-3.49 ) then
# mookodi
    setenv CFITSIOINCDIR "${SRC_HOME}/cfitsio-3.49"
else
    echo "Unknown CFITSIOINCDIR."
endif
setenv CFITSIOLIBDIR "/home/dev/bin/lib/x86_64-linux"
set hostname = `hostname`
if( "${hostname}" == "zen" ) then
# zen / ltdevx64
#setenv PYTHONPATH /usr/lib/python2.7/:/usr/lib/python2.7/site-packages
    setenv PYTHONPATH /usr/lib/python3.6/:/usr/lib/python3.6/site-packages
else if( "${hostname}" == "zmookodi" ) then
# mookodi
    setenv PYTHONPATH /usr/lib/python3.8/:/usr/lib/python3.8/site-packages/
else
    echo "Failed to set PYTHONPATH for ${hostname}."
endif
