TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Use typed rset structure (see 3.16.1 release notes)
USR_CPPFLAGS += -DUSE_TYPED_RSET

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += xxxSupport.dbd

# Build an IOC support library
LIBRARY_IOC += caen1527Support

# Compile and add code to the support library
caen1527Support_SRCS += xxxRecord.c
caen1527Support_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application, that
# causes problems on Windows DLL builds
caen1527Support_SRCS += dbSubExample.c
caen1527Support_SRCS += devcaen1527Version.c
caen1527Support_SRCS += caen1527Hello.c
caen1527Support_SRCS += initTrace.c

caen1527Support_LIBS += $(EPICS_BASE_IOC_LIBS)

# Auto-generate a header file containing a version string.
# Version comes from the VCS if available, else date+time.
GENVERSION = caen1527Version.h
# Macro name
GENVERSIONMACRO = caen1527VERSION

# Build the IOC application
PROD_IOC = caen1527

# caen1527.dbd will be created and installed
DBD += caen1527.dbd

# caen1527.dbd will include these files:
caen1527_DBD += base.dbd
caen1527_DBD += xxxSupport.dbd
caen1527_DBD += dbSubExample.dbd
caen1527_DBD += devcaen1527Version.dbd
caen1527_DBD += caen1527Hello.dbd
caen1527_DBD += initTrace.dbd

# caen1527_registerRecordDeviceDriver.cpp derives from caen1527.dbd
caen1527_SRCS += caen1527_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
caen1527_SRCS_DEFAULT += caen1527Main.cpp
caen1527_SRCS_vxWorks += -nil-

# Link in the code from our support library
caen1527_LIBS += caen1527Support

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into caen1527Support
    sncExample_SNCFLAGS += +r
    caen1527_DBD += sncExample.dbd
    # A .stt sequence program is *not* pre-processed:
    caen1527Support_SRCS += sncExample.stt
    caen1527Support_LIBS += seq pv
    caen1527_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    # A .st sequence program *is* pre-processed:
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Link QSRV (pvAccess Server) if available
ifdef EPICS_QSRV_MAJOR_VERSION
    caen1527_LIBS += qsrv
    caen1527_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
    caen1527_DBD += PVAServerRegister.dbd
    caen1527_DBD += qsrv.dbd
endif

# Finally link IOC to the EPICS Base libraries
caen1527_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

# Explicit dependency needed for generated header file
devcaen1527Version$(DEP): $(COMMON_DIR)/$(GENVERSION)
