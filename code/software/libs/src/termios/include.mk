# libtermios 1.0: integrated rosco_m68k libs build.
# Included by libs/Makefile; preserves existing LIBS/OBJECTS convention.
LIB=termios
LIBOBJECTS=$(DIR)/termios.o
LIBINCLUDES=$(DIR)/include

DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
UPPERLIB := $(shell echo $(LIB) | tr '[:lower:]' '[:upper:]')
BINARY := lib$(LIB).a
CFLAGS := $(CFLAGS) -I$(LIBINCLUDES) -I$(dir $(lastword $(MAKEFILE_LIST)))../machine/include -DBUILD_ROSCOM68K_$(UPPERLIB)_LIB
OBJECTS := $(OBJECTS) $(LIBOBJECTS)
INCLUDES := $(INCLUDES) $(DIR)/include/*
LIBS := $(LIBS) $(DIR)/$(BINARY)

$(DIR)/$(BINARY): $(LIBOBJECTS)
	$(AR) $(ARFLAGS) rs $@ $^
	$(RANLIB) $@

# In addition to normal make install, make all stages termios in build/lib
# and its public header in build/include. No other library is touched.
.PHONY: termios-stage termios-clean-stage
all: termios-stage
termios-stage: $(INSTALL_LIB)/libtermios.a $(INSTALL_INC)/termios.h

$(INSTALL_LIB)/libtermios.a: $(DIR)/$(BINARY)
	$(MKDIR_P) $(INSTALL_LIB)
	cp $< $@

$(INSTALL_INC)/termios.h: $(DIR)/include/termios.h
	$(MKDIR_P) $(INSTALL_INC)
	cp $< $@

clean: termios-clean-stage
termios-clean-stage:
	$(RM_F) $(INSTALL_LIB)/libtermios.a $(INSTALL_INC)/termios.h
