ROSCO_M68K_DIR ?= ../../..
override CPU = 68010
override ARCH = 68010
override TUNE = 68010

include $(ROSCO_M68K_DIR)/code/software/software.mk

# Existing libm provides __errno used by libtermios.
EXTRA_LIBS = -ltermios -lm
