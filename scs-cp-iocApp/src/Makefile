TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = scs-cp-ioc
# scs-cp-ioc.dbd will be created and installed
DBD += scs-cp-ioc.dbd

# scs-cp-ioc.dbd will be made up from these files:
scs-cp-ioc_DBD += base.dbd
scs-cp-ioc_DBD += xy240.dbd

# Include dbd files from all support modules:
#scs-cp-ioc_DBD += xxx.dbd

# Add all the support libraries used by this IOC
# scs-cp-ioc_LIBS += xxx
# scs-cp-ioc_LIBS += seq pv seqDev

# scs-cp-ioc_registerRecordDeviceDriver.cpp derives from scs-cp-ioc.dbd
scs-cp-ioc_SRCS += scs-cp-ioc_registerRecordDeviceDriver.cpp


scs-cp-ioc_SRCS += archive.c
scs-cp-ioc_SRCS += chop.c
scs-cp-ioc_SRCS += chopControl.c
scs-cp-ioc_SRCS += config.c
scs-cp-ioc_SRCS += control.c
scs-cp-ioc_SRCS += dmDrive.c
scs-cp-ioc_SRCS += guide.c
scs-cp-ioc_SRCS += house.c
scs-cp-ioc_SRCS += interlock.c
scs-cp-ioc_SRCS += interp.c
scs-cp-ioc_SRCS += scs.c
scs-cp-ioc_SRCS += setup.c
scs-cp-ioc_SRCS += testFunctions.c
scs-cp-ioc_SRCS += tiltSim.c

scs-cp-ioc_SRCS += utilities.c
scs-cp-ioc_SRCS += eventBus.c
scs-cp-ioc_SRCS += scs_st.st
scs-cp-ioc_SRCS += tilt_t.st

# Build the main IOC entry point on workstation OSs.
scs-cp-ioc_SRCS_DEFAULT += scs-cp-iocMain.cpp

# Finally link to the EPICS Base libraries
scs-cp-ioc_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

