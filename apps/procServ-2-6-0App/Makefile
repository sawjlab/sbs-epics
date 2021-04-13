# Process Server (for soft IOCs)
# David H. Thompson 8/29/2003
# Ralph Lange 03/17/2010
# GNU Public License (GPLv3) applies - see www.gnu.org

TOP=..
include $(TOP)/configure/CONFIG

A2X = a2x
A2X_FLAGS = -a revdate=04/16/2012 -a revnumber=2.6.0

PROD_HOST = procServ
procServ_SRCS = procServ.cc connectionItem.cc acceptFactory.cc \
                clientFactory.cc processFactory.cc
procServ_OBJS =  ${LIBOBJDIR}libtelnet$U.o

USR_CXXFLAGS += -DPACKAGE_NAME=\"procServ\ Process\ Server\" -DPACKAGE_TARNAME=\"procServ\" -DPACKAGE_VERSION=\"2.6.0\" -DPACKAGE_STRING=\"procServ\ Process\ Server\ 2.6.0\" -DPACKAGE_BUGREPORT=\"tech-talk@aps.anl.gov\" -DPACKAGE_URL=\"http://sourceforge.net/projects/procserv/\" -DPACKAGE=\"procServ\" -DVERSION=\"2.6.0\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_PTY_H=1 -DHAVE__BOOL=1 -DHAVE_STDBOOL_H=1 -Drestrict=__restrict -DHAVE_FORK=1 -DHAVE_VFORK=1 -DHAVE_WORKING_VFORK=1 -DHAVE_WORKING_FORK=1 -DGETGROUPS_T=gid_t -DHAVE_GETGROUPS=1 -DHAVE_STRFTIME=1 -DHAVE_FORKPTY=1
procServ_SYS_LIBS += $(subst -l,,-lutil )

include $(TOP)/configure/RULES

maintainer-clean:: realclean
	rm -f Makefile

doc: procServ.1 procServ.pdf procServ.html

procServ.1: procServ.txt
	$(A2X) $(A2X_FLAGS) -f manpage $<

procServ.pdf: procServ.txt
	$(A2X) $(A2X_FLAGS) -f pdf $<

procServ.html: procServ.txt
	$(A2X) $(A2X_FLAGS) -f xhtml $<
