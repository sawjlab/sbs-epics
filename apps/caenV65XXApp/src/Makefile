TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += bigsubRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += devV65XX.dbd

# Build an IOC support library
LIBRARY_IOC += V65XX_Support

# Compile and add the code to the support library
V65XX_Support_SRCS += init.c
V65XX_Support_SRCS += V65XX.c
V65XX_Support_SRCS += global.c
V65XX_Support_SRCS += devV65XX.c

V65XX_Support_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build the IOC application
PROD_IOC = V65XXApp

# _APPNAME_.dbd will be created and installed
DBD += V65XXApp.dbd

# _APPNAME_.dbd will include these files:
V65XXApp_DBD += base.dbd
V65XXApp_DBD += devV65XX.dbd

# _APPNAME__registerRecordDeviceDriver.cpp derives from _APPNAME_.dbd
V65XXApp_SRCS += V65XXApp_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
V65XXApp_SRCS_DEFAULT += caenV65XXAppMain.cpp
V65XXApp_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#_APPNAME__OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from our support library
V65XXApp_LIBS += V65XX_Support

USR_INCLUDES += -I$(LINUXVME_INC)
PROD_LIBS += jvme
jvme_DIR += $(LINUXVME_LIB)

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into V65XXAppSupport
    sncExample_SNCFLAGS += +r
    V65XXApp_DBD += sncExample.dbd
    V65XXAppSupport_SRCS += sncExample.stt
    V65XXAppSupport_LIBS += seq pv
    V65XXApp_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Finally link IOC to the EPICS Base libraries
V65XXApp_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

