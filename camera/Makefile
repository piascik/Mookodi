include ../Makefile.common
include Makefile.common


MOOKODI_CAMERA_INTERFACE_DIR   			= $(MOOKODI_CAMERA_SRC_HOME)/interface
MOOKODI_CAMERA_INTERFACE_FILE  			= $(MOOKODI_CAMERA_INTERFACE_DIR)/camera_interface.thrift

MOOKODI_CAMERA_SERVER_SRC_HOME 			= $(MOOKODI_CAMERA_SRC_HOME)/server
MOOKODI_CAMERA_SERVER_INTERFACE_SRC_HOME 	= $(MOOKODI_CAMERA_SERVER_SRC_HOME)/interface
MOOKODI_CAMERA_CLIENT_SRC_HOME 			= $(MOOKODI_CAMERA_SRC_HOME)/mookodi/camera/client

DIRS = server

top: make_server_interface make_client_interface dirs


make_server_interface:
	thrift -out $(MOOKODI_CAMERA_SERVER_INTERFACE_SRC_HOME) --gen cpp $(MOOKODI_CAMERA_INTERFACE_FILE)

make_client_interface:
	thrift -out $(MOOKODI_CAMERA_CLIENT_SRC_HOME) --gen py $(MOOKODI_CAMERA_INTERFACE_FILE)

dirs:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

docs:
	@for i in $(DIRS); \
	do \
		(echo docs in $$i...; cd $$i; $(MAKE) docs);\
	done;

depend:
	@for i in $(DIRS); \
	do \
		(echo depend in $$i...; cd $$i; $(MAKE) depend);\
	done;

clean:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; cd $$i; $(MAKE) clean); \
	done;

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; cd $$i; $(MAKE) tidy); \
	done;
