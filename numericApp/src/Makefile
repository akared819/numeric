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
include $(TOP)/configure/RULES
