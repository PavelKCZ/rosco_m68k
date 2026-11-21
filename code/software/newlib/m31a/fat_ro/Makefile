# Host only, independent of the M3.1 target profiles.
BUILD ?= /tmp/kilo-m31-b1-build
CC := cc
COMMON := -std=c11 -g -Wall -Wextra -Werror -Wpedantic -Iinclude
SOURCES := src/fat_ro.c tests/test_backend.c
.PHONY: all test sanitized audit
all: $(BUILD)/tests $(BUILD)/fat_ro.o
$(BUILD):
	mkdir -p $@
$(BUILD)/tests: $(SOURCES) include/fat_ro.h include/fat_defs.h include/fat_types.h include/fat_opts.h | $(BUILD)
	$(CC) $(COMMON) -O2 $(SOURCES) -o $@
$(BUILD)/fat_ro.o: src/fat_ro.c include/fat_ro.h include/fat_defs.h include/fat_types.h include/fat_opts.h | $(BUILD)
	$(CC) $(COMMON) -O2 -c $< -o $@
$(BUILD)/tests-sanitized: $(SOURCES) include/fat_ro.h include/fat_defs.h include/fat_types.h include/fat_opts.h | $(BUILD)
	$(CC) $(COMMON) -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all $(SOURCES) -o $@
test: all
	$(BUILD)/tests
sanitized: $(BUILD)/tests-sanitized
	ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/tests-sanitized
audit: all
	python3 tests/audit.py $(BUILD)
