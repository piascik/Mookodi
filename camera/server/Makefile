include ../../Makefile.common
include ../Makefile.common
include ../../ccd/Makefile.common

BINDIR		= $(MOOKODI_CAMERA_BIN_HOME)/server/$(HOSTTYPE)
INTERFACE_BINDIR= $(MOOKODI_CAMERA_BIN_HOME)/server/interface/$(HOSTTYPE)

CC=g++
INCLUDE=-I ./interface/ -I /usr/local/include/ -I $(MOOKODI_CCD_SRC_HOME)/include $(CFITSIO_CFLAGS)
CFLAGS=-Wall -std=c++17 -g -O3
LDFLAGS=-L/usr/local/lib -L$(MOOKODI_LIB_HOME) -L$(CFITSIOLIBDIR) $(ANDOR_LDFLAGS)

LIBS=-lthriftnb -lthrift -levent -lboost_program_options -lboost_filesystem -lboost_system -lboost_iostreams -llog4cxx -lpthread -lm -lmookodi_ccd $(ANDOR_LIBS) $(CFITSIO_LIBS)
#-lIDSAC -largtable2 -lopts -lCCfits 

SOURCES=Camera.cpp EmulatedCamera.cpp CameraServer.cpp
OBJECTS=$(SOURCES:%.cpp=$(BINDIR)/%.o) 

INTERFACE_SRCS=CameraService.cpp camera_interface_constants.cpp camera_interface_types.cpp
#INTERFACE_OBJS=./interface/CameraService.o ./interface/camera_interface_constants.o ./interface/camera_interface_types.o
INTERFACE_OBJS=$(INTERFACE_SRCS:%.cpp=$(INTERFACE_BINDIR)/%.o)

EXECUTABLE=$(BINDIR)/MookodiCameraServer

CONFIG_SRCS=log4cxx.properties
CONFIG_BINS=$(CONFIG_SRCS:%=$(BINDIR)/%)

DIRS = interface

all: dirs $(SOURCES) $(EXECUTABLE) $(CONFIG_BINS)

$(EXECUTABLE): $(OBJECTS) $(INTERFACE_OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(INCLUDE) $(OBJECTS) $(INTERFACE_OBJS) $(LIBS) 

#.cpp.o:
$(BINDIR)/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

$(BINDIR)/%.properties: %.properties
	cp $< $@

dirs:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

# This doesn't seem to work
#$(INTERFACE_OBJS):
#	cd ./interface/; make

docs: 
	-doxygen Doxyfile

install: MookodiCameraServer
	install -m 0755 MookodiCameraServer /usr/local/bin

clean:
	rm -f $(MOOKODI_CAMERA_BIN_HOME)/server/$(HOSTTYPE)/*.o || TRUE; rm -f $(EXECUTABLE) || TRUE
	cd ./interface; make clean

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	cd ./interface; make tidy
