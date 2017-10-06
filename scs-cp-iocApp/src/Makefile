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

# Include dbd files from all support modules:
#scs-cp-ioc_DBD += xxx.dbd

# Add all the support libraries used by this IOC
# scs-cp-ioc_LIBS += xxx
# scs-cp-ioc_LIBS += seq pv seqDev

# scs-cp-ioc_registerRecordDeviceDriver.cpp derives from scs-cp-ioc.dbd
scs-cp-ioc_SRCS += scs-cp-ioc_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
scs-cp-ioc_SRCS_DEFAULT += scs-cp-iocMain.cpp

# Finally link to the EPICS Base libraries
scs-cp-ioc_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

