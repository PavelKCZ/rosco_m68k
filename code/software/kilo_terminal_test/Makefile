ROSCO_M68K_DIR ?= ../../..

# Keep this test explicitly on the MC68010, including architecture/tuning.
override CPU = 68010
override ARCH = 68010
override TUNE = 68010

include $(ROSCO_M68K_DIR)/code/software/software.mk

# Existing libm supplies __errno, required by libtermios error reporting.
EXTRA_LIBS = -ltermios -lm
