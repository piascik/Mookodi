include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(MOOKODI_CCD_SRC_HOME)/include
BINDIR		= $(MOOKODI_CCD_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(MOOKODI_CCD_DOC_HOME)/cdocs

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) $(ANDOR_CFLAGS) -I$(CFITSIOINCDIR) \
		$(MUTEX_CFLAGS) $(LOGGING_CFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= -L$(LT_LIB_HOME) $(ANDOR_LDFLAGS) $(CFITSIO_LIBS)

SRCS 		= ccd_exposure.c ccd_general.c ccd_setup.c ccd_temperature.c ccd_fits_header.c ccd_fits_filename.c
HEADERS		= $(SRCS:%.c=%.h)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)

top: shared
#docs

shared: $(MOOKODI_LIB_HOME)/lib$(MOOKODI_CCD_LIBNAME).so

$(MOOKODI_LIB_HOME)/lib$(MOOKODI_CCD_LIBNAME).so: $(OBJS)
	$(CC) $(CCSHAREDFLAG) $(LDFLAGS) $(OBJS) -o $@ 

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

docs: $(SRCS)
	-doxygen Doxyfile

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(MOOKODI_LIB_HOME)/lib$(MOOKODI_CCD_LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS) ;)

