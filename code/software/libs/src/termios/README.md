# libtermios 1.0 — integrated rosco_m68k library

This directory is installed directly under `code/software/libs/src/termios`. The `termios.c` and public `include/termios.h` are **byte-for-byte identical** to the hardware-tested RC1 sources. Only integration `include.mk` and documentation are adapted.

## Install

Back up your existing `libs/src/termios` first. From `code/software/libs`, extract the installation archive (which contains paths starting with `src/termios/`):

```sh
tar -xzf /path/to/libtermios-1.0-libs-install.tar.gz
make clean
make all
```

The **existing** `libs/Makefile` already includes `src/termios/include.mk`; no edit to it is required. `make all` builds `src/termios/libtermios.a` and stages it to `build/lib/libtermios.a`, plus the header to `build/include/termios.h`. This is the actual build layout; `build/lib`, not `build` root, is where rosco_m68k installs archives. `make install` continues to work with the project's standard rules. `make clean` also removes the staged termios archive/header. Other libraries are unchanged.

## Supported hardware profile

Real rosco_m68k firmware UART: 38400 baud, 8N1. Software `termios` reports `B38400` and does not program UART hardware. Limited POSIX-like API; see `API.md` and `LIMITATIONS.md`.

## Validation

R1–R18: 73/73 scenarios PASS. RC1: 5/5 scenarios PASS on real hardware. The final integrated build still needs to be verified with the actual cross-toolchain on your Ubuntu system. See `TEST_REPORT.md`.
