TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += bigsubRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += devlecroy.dbd

# Build an IOC support library
LIBRARY_IOC += lecroy_Support

# Compile and add the code to the support library
lecroy_Support_SRCS += access.c
lecroy_Support_SRCS += telnet.c
lecroy_Support_SRCS += global.c
lecroy_Support_SRCS += hvstart.c
lecroy_Support_SRCS += lecroy.c
lecroy_Support_SRCS += util.c
lecroy_Support_SRCS += devLecroy.c
lecroy_Support_SRCS += bigsubRecord.c
lecroy_Support_SRCS += bigsub.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application.
# This is required for Windows DLL builds.
#_APPNAME_Support_SRCS += dbSubExample.c
#_APPNAME_Support_SRCS += _APPNAME_Hello.c
#_APPNAME_Support_SRCS += initTrace.c

lecroy_Support_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build the IOC application
PROD_IOC = lecroy

# _APPNAME_.dbd will be created and installed
DBD += lecroy.dbd

# _APPNAME_.dbd will include these files:
lecroy_DBD += base.dbd
lecroy_DBD += devlecroy.dbd

# _APPNAME__registerRecordDeviceDriver.cpp derives from _APPNAME_.dbd
lecroy_SRCS += lecroy_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
lecroy_SRCS_DEFAULT += lecroyMain.cpp
lecroy_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#_APPNAME__OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
lecroy_LIBS += lecroy_Support

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into lecroySupport
    sncExample_SNCFLAGS += +r
    lecroy_DBD += sncExample.dbd
    lecroySupport_SRCS += sncExample.stt
    lecroySupport_LIBS += seq pv
    lecroy_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Finally link IOC to the EPICS Base libraries
lecroy_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

