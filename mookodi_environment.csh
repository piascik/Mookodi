#!/bin/csh
# Source this file to set various environment variables used to build mookodi
setenv LT_HOME "/home/dev/"
setenv LT_SRC_HOME "${LT_HOME}/src"
setenv LT_BIN_HOME "${LT_HOME}/bin"
setenv LT_DOC_HOME "${LT_HOME}/public_html"
setenv LT_LIB_HOME "${LT_BIN_HOME}/lib/x86_64-linux"
setenv MOOKODI_LIB_HOME "/home/dev/src/mookodi/bin/lib/x86_64-linux/"
setenv CCSHAREDFLAG "-shared"
if ( ${?LD_LIBRARY_PATH} ) then
    setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${MOOKODI_LIB_HOME}:${LT_SRC_HOME}/andor/andor-2.104.30000/lib/
else
    setenv LD_LIBRARY_PATH ${MOOKODI_LIB_HOME}:${LT_SRC_HOME}/andor/andor-2.104.30000/lib/
endif
if( -d /home/dev/src/cfitsio3310_x64/include ) then
# ltdevx64
    setenv CFITSIOINCDIR "/home/dev/src/cfitsio3310_x64/include"
else if ( -d /home/dev/src/cfitsio-3.47 ) then
# zen
    setenv CFITSIOINCDIR "/home/dev/src/cfitsio-3.47"
else if ( -d /home/dev/src/cfitsio-3.49 ) then
# mookodi
    setenv CFITSIOINCDIR "/home/dev/src/cfitsio-3.49"
else
    echo "Unknown CFITSIOINCDIR."
endif
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
