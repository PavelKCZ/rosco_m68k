# Host validation only. sd_ro_traps.S is NOT assembled or linked in I1.
BUILD ?= /tmp/kilo-m31-sd-ro-build
CC := cc
FLAGS := -std=c11 -g -Wall -Wextra -Werror -Wpedantic -Iinclude -Isrc
CORE := src/sd_ro.c src/csd_decode.c
TEST := tests/mock_firmware.c tests/test_adapter.c tests/test_csd.c
HEADERS := include/sd_ro.h src/private.h tests/mock_firmware.h
.PHONY: test sanitized audit
$(BUILD):
	mkdir -p $@
$(BUILD)/tests: $(CORE) $(TEST) $(HEADERS) Makefile | $(BUILD)
	$(CC) $(FLAGS) -I../fat_ro/include -O2 $(CORE) $(TEST) -o $@
$(BUILD)/tests-sanitized: $(CORE) $(TEST) $(HEADERS) Makefile | $(BUILD)
	$(CC) $(FLAGS) -I../fat_ro/include -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer $(CORE) $(TEST) -o $@
$(BUILD)/sd_ro.o: src/sd_ro.c include/sd_ro.h src/private.h Makefile | $(BUILD)
	$(CC) $(FLAGS) -O2 -c $< -o $@
$(BUILD)/csd_decode.o: src/csd_decode.c include/sd_ro.h src/private.h Makefile | $(BUILD)
	$(CC) $(FLAGS) -O2 -c $< -o $@
test: $(BUILD)/tests
	$(BUILD)/tests
sanitized: $(BUILD)/tests-sanitized
	ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/tests-sanitized
audit: $(BUILD)/sd_ro.o $(BUILD)/csd_decode.o
	python3 tests/audit.py $(BUILD)
