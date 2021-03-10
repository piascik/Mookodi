include ../../../Makefile.common
include ../../Makefile.common

BINDIR		= $(MOOKODI_CAMERA_BIN_HOME)/server/interface/$(HOSTTYPE)

CC=g++
CFLAGS=-Wall -c -std=c++11 -g -rdynamic
SOURCES=CameraService.cpp camera_interface_constants.cpp camera_interface_types.cpp
OBJECTS=$(SOURCES:%.cpp=$(BINDIR)/%.o) 
#OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(OBJECTS)

#.cpp.o:
$(BINDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm -f $(OBJECTS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
