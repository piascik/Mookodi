# Mookodi CCD library

This directory contains the sources to build C library to control the Andor CCD camera used for Mookodi (Andor IKon M934). It wraps the Andor SDK. The library is used by the camera server to control the CCD.

The location of the Andor library used is specified in *Makefile.common* and may need to be changed for your installation.

This directory requires the Andor SDK2, and CFITSIO, to be installed to compile.

## Directory structure

* **c**    The C source code, and Makefiles to build the library.
* **include** The header include files.
* **test** Source code for command line test programs for the library.
