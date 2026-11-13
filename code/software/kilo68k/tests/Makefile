# Native tests only: never included by the firmware Makefile.
HOST_CC ?= cc
CPPFLAGS = -I.. -I../../libs/src/termios/include
CFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic
SOURCES = test.c ansi.c ../editor.c ../terminal.c ../main.c
HEADERS = ansi.h ../editor.h ../terminal.h ../../libs/src/termios/include/termios.h

.PHONY: test sanitize
test: build/test
	./build/test

sanitize: build/test-sanitize
	./build/test-sanitize

build/test: $(SOURCES) $(HEADERS) Makefile
	mkdir -p build
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

build/test-sanitize: $(SOURCES) $(HEADERS) Makefile
	mkdir -p build
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all $(SOURCES) -o $@

.PHONY: test-diagnostics sanitize-diagnostics
test-diagnostics: build/test-diagnostics
	./build/test-diagnostics

sanitize-diagnostics: build/test-diagnostics-sanitize
	./build/test-diagnostics-sanitize

build/test-diagnostics: $(SOURCES) $(HEADERS) Makefile
	mkdir -p build
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -DKILO_RX_DIAGNOSTICS $(SOURCES) -o $@

build/test-diagnostics-sanitize: $(SOURCES) $(HEADERS) Makefile
	mkdir -p build
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -DKILO_RX_DIAGNOSTICS -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all $(SOURCES) -o $@
