# Mookodi

This is the source code repository for the Mookodi instrument, a low resolution imager/spectrograph to go on the Lesedi telescope.

## Directory structure

* **bin** The root of the binary tree where the current build system puts libraries, object files and binaries.
* **camera** The camera server directory, this is a C++ server with a thrift interface for controlling the CCD camera.
* **ccd** This is a C library that uses the Andor SDK libraries to provide a library to control the Andor CCD camera. Used by the camera server.
* **docs** Some documentation / notes.
* **public_html** The root of the Doxygen generated API documentation.

The Makefile.common file is included in Makefile's to provide common root directory information.

The mookodi_environment.csh can be sourced to setup various environment variables needed to build the camera system.

## Dependancies / Prerequisites

Currently, the package can be built on Ubuntu 18.04 or Ubuntu 20.04. 

The server interfaces are written using thrift, which needs to be installed.

The camera dependancies are documented in the camera directory's README.md . 

