
include			../../Makefile.common
include			../Makefile.common

CNAME		= c
BINDIR		= $(NGATASTRO_BIN_HOME)/$(CNAME)/$(HOSTTYPE)
INCDIR 		= $(NGATASTRO_SRC_HOME)/include
DOCSDIR 	= $(NGATASTRO_DOC_HOME)/cdocs

# Do we want to enable logging?
LOGGINGCFLAGS = -DLOGGING=10

CFLAGS = -g $(CCHECKFLAG) $(SHARED_LIB_CFLAGS) -I$(INCDIR) $(LOGGINGCFLAGS)
LINTFLAGS = -I$(INCDIR) -
DOCFLAGS = -static
SRCS 		= 	ngat_astro.c ngat_astro_mjd.c
HEADERS		=	$(SRCS:%.c=$(INCDIR)/%.h)
OBJS		=	$(SRCS:%.c=$(BINDIR)/%.o)

top: shared

shared: $(MOOKODI_LIB_HOME)/libngatastro.so

$(MOOKODI_LIB_HOME)/libngatastro.so: $(OBJS)
	cc $(CCSHAREDFLAG) $(CFLAGS) $(OBJS) -o $@ $(TIMELIB)

static: $(MOOKODI_LIB_HOME)/libngatastro.a

$(MOOKODI_LIB_HOME)/libngatastro.a: $(OBJS)
	ar rcv $@ $?

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

docs:
	-doxygen Doxyfile

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	-$(RM) $(RM_OPTIONS) $(OBJS) $(MOOKODI_LIB_HOME)/libngatastro.so $(MOOKODI_LIB_HOME)/libngatastro.a $(TIDY_OPTIONS)

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)
