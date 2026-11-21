# Isolated harness; no established build profile is modified or invoked.
BUILD ?= /tmp/kilo-m31-b3-integration
CC := cc
BASE := $(abspath ..)
FLAGS := -std=c11 -g -Wall -Wextra -Werror -Wpedantic -Iinclude -I$(BASE)/fat_ro/include -I$(BASE)/sd_ro/include
CORE := $(BASE)/fat_ro/src/fat_ro.c $(BASE)/sd_ro/src/sd_ro.c $(BASE)/sd_ro/src/csd_decode.c src/b3_integration.c
MOCK := tests/mock_firmware.c tests/fat_fixture.c tests/test_integration.c
HEADERS := $(wildcard include/*.h tests/*.h $(BASE)/fat_ro/include/*.h $(BASE)/sd_ro/include/*.h $(BASE)/sd_ro/src/private.h)
CROSS := m68k-elf-rosco-
TFLAGS := -mcpu=68010 -march=68010 -mtune=68010 -std=c11 -O2 -g -Wall -Wextra -Werror -Wpedantic -ffreestanding -ffunction-sections -fdata-sections -fno-common -fno-delete-null-pointer-checks --param=min-pagesize=0 -fstack-usage
.PHONY: test sanitized target audit
$(BUILD):
	mkdir -p $@
$(BUILD)/tests: $(CORE) $(MOCK) $(HEADERS) Makefile | $(BUILD)
	$(CC) $(FLAGS) -I$(BASE)/sd_ro/src -O2 $(CORE) $(MOCK) -o $@
$(BUILD)/tests-sanitized: $(CORE) $(MOCK) $(HEADERS) Makefile | $(BUILD)
	$(CC) $(FLAGS) -I$(BASE)/sd_ro/src -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer $(CORE) $(MOCK) -o $@
test: $(BUILD)/tests
	$(BUILD)/tests
sanitized: $(BUILD)/tests-sanitized
	ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/tests-sanitized
target: | $(BUILD)
	$(CROSS)gcc $(TFLAGS) $(FLAGS) -c $(BASE)/fat_ro/src/fat_ro.c -o $(BUILD)/fat_ro.o
	$(CROSS)gcc $(TFLAGS) $(FLAGS) -c $(BASE)/sd_ro/src/sd_ro.c -o $(BUILD)/sd_ro.o
	$(CROSS)gcc $(TFLAGS) $(FLAGS) -c $(BASE)/sd_ro/src/csd_decode.c -o $(BUILD)/csd_decode.o
	$(CROSS)gcc $(TFLAGS) $(FLAGS) -c $(BASE)/sd_ro/src/sd_ro_traps.S -o $(BUILD)/sd_ro_traps.o
	$(CROSS)gcc $(TFLAGS) $(FLAGS) -c src/b3_integration.c -o $(BUILD)/b3_integration.o
	$(CROSS)ld -r -z noexecstack --fatal-warnings -Map=$(BUILD)/combined.map -o $(BUILD)/combined.o $(BUILD)/fat_ro.o $(BUILD)/sd_ro.o $(BUILD)/csd_decode.o $(BUILD)/sd_ro_traps.o $(BUILD)/b3_integration.o
audit:
	python3 tests/audit.py $(BUILD)
