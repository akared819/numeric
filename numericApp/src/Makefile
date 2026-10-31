# SPDX-License-Identifier: GPL-3.0-or-later
TOP=../..
include $(TOP)/configure/CONFIG
LIBRARY_HOST += numeric
INC += numeric.h
numeric_SRCS += numeric.cpp
numeric_SRCS += mean.cpp
PROD_HOST += numericTest
numericTest_SRCS += numericTest.cpp
numericTest_LIBS += numeric
USR_SYS_LIBS += gsl gslcblas
ifneq ($(strip $(GSL_DIR)),)
USR_INCLUDES += -I$(GSL_DIR)/include
USR_LDFLAGS += -L$(GSL_DIR)/lib
endif
LIBRARY_IOC += numericIoc
numericIoc_SRCS += numericAsub.cpp
numericIoc_LIBS += numeric $(EPICS_BASE_IOC_LIBS)
DBD += numericSupport.dbd
DBD += numericIocTest.dbd
numericIocTest_DBD += base.dbd numericSupport.dbd
PROD_HOST += numericIocTest
numericIocTest_SRCS += numericIocTest.cpp numericIocTest_registerRecordDeviceDriver.cpp
numericIocTest_LIBS += numericIoc numeric $(EPICS_BASE_IOC_LIBS)
include $(TOP)/configure/RULES
