include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(MOOKODI_CCD_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(MOOKODI_CCD_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)

CFLAGS 		= -g -I$(INCDIR) $(ANDOR_CFLAGS) -I$(CFITSIOINCDIR)
LDFLAGS		= -L$(MOOKODI_LIB_HOME) -L$(LT_LIB_HOME) -l$(LIBNAME) -lcfitsio $(ANDOR_LDFLAGS) $(TIMELIB) $(SOCKETLIB) -lm -lc 

SRCS 		= test_temperature.c test_exposure.c test_andor_exposure.c 
#  test_andor_multrun.c
OBJS 		= $(SRCS:%.c=%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
SCRIPT_SRCS	= 
SCRIPT_BINS	= $(SCRIPT_SRCS:%=$(BINDIR)/%)
LIBNAME		= $(MOOKODI_CCD_LIBNAME)

top: $(PROGS) scripts
#docs

$(BINDIR)/%: %.o
	$(CC) -o $@ $< $(LDFLAGS) 

scripts: $(SCRIPT_BINS)

$(BINDIR)/%.csh:%.csh
	$(CP) $< $@

$(BINDIR)/%:%
	$(CP) $< $@

docs: $(SRCS)
	-doxygen Doxyfile

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(PROGS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

