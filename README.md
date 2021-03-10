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

Currently, the package can be built on Ubuntu 18.04 or Ubuntu 20.04. A version of the Andor SDK2 is required for CCD camera control, and CFITSIO is used to write FITS images.

The server interfaces are written using thrift, which needs to be installed.

Other software packages that were installed on the mookodi machine to facilitate building the camera server include:

* **tcsh** *sudo apt install tcsh* The environment setup script is currently written in tcsh.
* **emacs** *sudo apt-get  install emacs* Used for editing.
* **python-setuptools** *sudo apt-get install python3-setuptools* / *sudo apt-get install python-setuptools* Used whilst installing the python client packages.
* **pip** *sudo apt install python3-pip* Used for requests installation
* **requests** *sudo pip3 install requests* Used by the thrift python libraries.
