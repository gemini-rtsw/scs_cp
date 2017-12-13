TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC_RTEMS = scs-cp-ioc
# scs-cp-ioc.dbd will be created and installed
DBD += scs-cp-ioc.dbd

# scs-cp-ioc.dbd will be made up from these files:
scs-cp-ioc_DBD += base.dbd
scs-cp-ioc_DBD += geminiRecords.dbd
scs-cp-ioc_DBD += vmi5588.dbd
scs-cp-ioc_DBD += xy240.dbd
scs-cp-ioc_DBD += tcslib.dbd
scs-cp-ioc_DBD += bancomm.dbd
scs-cp-ioc_DBD += pvload.dbd
scs-cp-ioc_DBD += iocAdmin.dbd
scs-cp-ioc_DBD += menuScan.dbd

# Include dbd files from all support modules:
#scs-cp-ioc_DBD += xxx.dbd

# Add all the support libraries used by this IOC
scs-cp-ioc_LIBS += geminiRecords
scs-cp-ioc_LIBS += vmi5588
scs-cp-ioc_LIBS += xy240
scs-cp-ioc_LIBS += bancomm 
scs-cp-ioc_LIBS += pvload
scs-cp-ioc_LIBS += tcslib
scs-cp-ioc_LIBS += timelib
scs-cp-ioc_LIBS += slalib
scs-cp-ioc_LIBS += seq pv #seqDev
scs-cp-ioc_LIBS += devIocStats 

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
scs-cp-ioc_SRCS += tilt_st.st

# Build the main IOC entry point on workstation OSs.
scs-cp-ioc_SRCS_DEFAULT += scs-cp-iocMain.cpp

# Finally link to the EPICS Base libraries
scs-cp-ioc_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

