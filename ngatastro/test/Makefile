
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(NGATASTRO_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(NGATASTRO_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(NGATASTRO_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR)

NGATASTRO_LDFLAGS= -L$(MOOKODI_LIB_HOME) -lngatastro -lrt -lm
NGATASTRO_SRCS 	= date_to_mjd_ngatastro.c date_to_ms.c
OTHER_SRCS	= parse_time.c
SRCS		= $(NGATASTRO_SRCS) $(OTHER_SRCS)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
NGATASTRO_PROGS = $(NGATASTRO_SRCS:%.c=$(BINDIR)/%)

top: $(NGATASTRO_PROGS)

$(BINDIR)/date_to_mjd_ngatastro: $(BINDIR)/date_to_mjd_ngatastro.o $(BINDIR)/parse_time.o
	$(CC) -o $@ $(BINDIR)/date_to_mjd_ngatastro.o $(BINDIR)/parse_time.o $(NGATASTRO_LDFLAGS)

$(BINDIR)/date_to_ms: $(BINDIR)/date_to_ms.o $(BINDIR)/parse_time.o
	$(CC) -o $@ $(BINDIR)/date_to_ms.o $(BINDIR)/parse_time.o -lrt -lm

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

docs:
	-doxygen Doxyfile

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(PROGS) $(SCRIPT_BINS) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
