# This Makefile is used to create several test cases for Tcl's load
# command.  It also illustrates how to take advantage of configuration
# exported by Tcl to set up Makefiles for shared libraries.
# RCS: @(#) $Id: Makefile.in,v 1.6 1999/09/21 06:37:32 hobbs Exp $

TOP = ..
ifneq (,$(findstring cdaql,$(shell hostname -s)))
  EPICS_BASE = $(TOP)/../../R3.14.12.5/base
else	     
  EPICS_BASE = $(TOP)/../../base-3.14.12.5
endif

TCL_DBGX =		
CC = gcc
EPICSLIBS =             -lca -lCom -L$(EPICS_BASE)/lib/linux-x86_64 
LIBS =			-L/usr/lib64 -ltclstub8.5 -ldl  -lieee -lm $(EPICSLIBS) -lc
AC_FLAGS =		-O -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic -pipe 
SHLIB_CFLAGS =		-fPIC
SHLIB_LD =		${CC} -shared ${CFLAGS} ${LDFLAGS} -fPIC -Wl,-soname,${@}
SHLIB_SUFFIX =		.so
SHLIB_VERSION =		
SRC_DIR =		.
TCL_VERSION=		8.5

CFLAGS = -g
CC_SWITCHES = $(CFLAGS) -I${SRC_DIR}/../../generic -DTCL_MEM_DEBUG \
	${SHLIB_CFLAGS} -DUSE_TCL_STUBS ${AC_FLAGS} \
	-I$(EPICS_BASE)/include -I$(EPICS_BASE)/include/os/Linux \
	-I$(EPICS_BASE)/include/compiler/gcc

all: epics${SHLIB_SUFFIX}

epics${SHLIB_SUFFIX}: $(SRC_DIR)/tcl_ca.c
	$(CC) -c $(CC_SWITCHES) $(SRC_DIR)/tcl_ca.c
	${SHLIB_LD} -o epics${SHLIB_SUFFIX} tcl_ca.o ${LIBS}

clean:
	rm -f *.o *${SHLIB_SUFFIX} config.cache config.log config.status lib.exp

distclean: clean
	rm -f Makefile

install: all
