# Explicit standalone profile. Does not include software.mk or legacy libs build.
ROOT := $(abspath ../../../..)
BUILD ?= /tmp/kilo-m31a-build
DIAGNOSTIC ?= 0
ifeq ($(DIAGNOSTIC),1)
PROGRAM := runtime-smoke-diag
DIAG_FLAGS := -DM31_DIAGNOSTIC
AUDIT_FLAGS := --diagnostic
else
PROGRAM := runtime-smoke
DIAG_FLAGS :=
AUDIT_FLAGS :=
endif
CROSS := m68k-elf-rosco-
CC := $(CROSS)gcc
AR := $(CROSS)ar
TARGET_LIB := $(dir $(shell $(CC) -print-file-name=libc.a))
LIBGCC := $(shell $(CC) -print-libgcc-file-name)
TERMIOS := $(ROOT)/code/software/libs/src/termios
MACHINE := $(ROOT)/code/software/libs/src/machine/include
CFLAGS := -mcpu=68010 -march=68010 -mtune=68010 -std=c11 -O2 -g -Wall -Wextra -Werror -ffreestanding -ffunction-sections -fdata-sections -fno-common -fno-delete-null-pointer-checks --param=min-pagesize=0 $(DIAG_FLAGS)
INCLUDES := -Ipublic -Iinclude -Icompat -iquote $(TERMIOS)/include -iquote $(MACHINE)
TERM_RENAME := -Dtcgetattr=m31_termios_tcgetattr -Dtcsetattr=m31_termios_tcsetattr -Dtcdrain=m31_termios_tcdrain -Dtcflush=m31_termios_tcflush
OBJECTS := $(addprefix $(BUILD)/,startup.o memory.o runtime.o syscalls.o smoke.o termios.o machine.o)
.PHONY: all audit host-test include-check diagnostic
all: $(BUILD)/$(PROGRAM).bin $(BUILD)/$(PROGRAM).dis include-check audit
$(BUILD):
	mkdir -p $@
$(BUILD)/%.o: %.c Makefile include/memory.h include/runtime.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -MD -MP -MF $(@:.o=.d) -c $< -o $@
$(BUILD)/startup.o: startup.S include/memory.h diagnostic.inc diagnostic_helpers.inc Makefile | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -MD -MP -MF $(@:.o=.d) -c $< -o $@
$(BUILD)/termios.o: $(TERMIOS)/termios.c compat/fdlibm.h Makefile | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $(TERM_RENAME) -MD -MP -MF $(@:.o=.d) -c $< -o $@
$(BUILD)/machine.o: $(TARGET_LIB)librosco.a | $(BUILD)
	$(AR) p $< machine.o > $@
$(BUILD)/$(PROGRAM).elf: $(OBJECTS) link.ld Makefile $(TARGET_LIB)libc.a $(LIBGCC)
	$(CC) $(CFLAGS) -nostdlib -Wl,-T,link.ld,-Map,$(BUILD)/$(PROGRAM).map,--cref,--gc-sections,--warn-common,--fatal-warnings,-z,noexecstack,-z,max-page-size=16,-z,common-page-size=16 -o $@ $(OBJECTS) -Wl,--start-group $(TARGET_LIB)libc.a -lgcc -Wl,--end-group
$(BUILD)/$(PROGRAM).bin: $(BUILD)/$(PROGRAM).elf
	$(CROSS)objcopy -O binary $< $@
$(BUILD)/$(PROGRAM).dis: $(BUILD)/$(PROGRAM).elf
	$(CROSS)objdump -dr $< > $@
audit: $(BUILD)/$(PROGRAM).elf $(BUILD)/$(PROGRAM).bin
	python3 audit.py $(BUILD) $(AUDIT_FLAGS)
host-test:
	python3 tests/run.py $(BUILD)

$(BUILD)/angle_include.o: tests/angle_include.c Makefile | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -MD -MP -MF $(@:.o=.d) -c $< -o $@
include-check: $(BUILD)/angle_include.o
	python3 tests/check_includes.py $(BUILD) $(ROOT)
-include $(wildcard $(BUILD)/*.d)

diagnostic:
	$(MAKE) BUILD=$(BUILD)/diagnostic DIAGNOSTIC=1 all
