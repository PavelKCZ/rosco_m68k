# AGENTS.md — rosco_m68k / Kilo Editor Port

## Project overview

This repository is a fork of the rosco_m68k project, maintained at:

- Fork: https://github.com/PavelKCZ/rosco_m68k
- Upstream: https://github.com/rosco-m68k/rosco_m68k
- Local repository: `~/rosco/rosco_m68k`
- Development branch: `feature/kilo-editor`
- Base commit: `78fcb15`

The current task is to port the original Kilo text editor by antirez to the rosco_m68k single-board computer.

**Target hardware: Motorola MC68010 CPU.**

The project already includes a custom POSIX-like `libtermios 1.0` library, integrated into the software build system and validated on real hardware.

Read `docs/KILO_PORTING_CONTEXT.md` before making changes related to the Kilo port.

## Development principles

1. Make small, incremental, reviewable changes.
2. Inspect existing code, headers, Makefiles, and firmware APIs before proposing implementations.
3. Reuse existing rosco_m68k libraries and firmware interfaces wherever possible.
4. Do not introduce curses, ncurses, terminfo, or other large dependencies without explicit approval.
5. Do not replace or substantially rewrite the existing `libtermios 1.0` implementation without explicit approval.
6. Do not modify unrelated parts of the repository.
7. Do not delete existing source files, test programs, archives, or backups without explicit approval.
8. Explain significant architectural decisions and trade-offs before implementing them.
9. Never claim that hardware functionality has been verified unless the user has actually tested it on the physical board.

## Build and target constraints

- Primary hardware target: MC68010.
- Compiler toolchain: m68k-elf-rosco GCC 13.3.1, subject to local verification.
- The project is a freestanding embedded environment, not a full POSIX operating system.
- Do not assume that standard Unix system calls are available.
- Inspect actual compiler flags and linker scripts before modifying build configuration.
- Preserve compatibility with the existing rosco_m68k build system.
- Do not assume that successful compilation proves runtime correctness.

## Existing libtermios 1.0

Library location:

`code/software/libs/src/termios/`

Important files:

- `termios.c`
- `include/termios.h`
- `include.mk`
- `README.md`
- `API.md`
- `LIMITATIONS.md`
- `INSTALL.md`
- `TEST_REPORT.md`

The library supports canonical and noncanonical terminal input, including tested VMIN/VTIME behavior.

The completed library passed 78 out of 78 test scenarios on real MC68010 hardware.

Treat this as the established baseline. Investigate integration problems before changing the library itself.

## Kilo porting strategy

The initial goal is a minimal, functional port of the original antirez Kilo editor, without curses or ncurses.

Investigate and adapt:

- Raw terminal input and keyboard handling.
- ANSI/VT100 terminal output.
- Terminal dimensions and screen refresh.
- Dynamic memory allocation.
- Standard C and POSIX dependencies.
- File loading and saving via the existing MicroSD/FAT facilities; inspect `basicio.h`, `machine.h`, and especially `sdfat.h` and their implementations before choosing an API.
- Build and linking requirements.

Prefer a working minimal editor over adding advanced features prematurely.

At 38400 baud, full-screen refreshes may be slow. Measure performance before implementing redraw optimizations.

## Testing

- Test changes incrementally.
- Separate build verification from hardware verification.
- Record compiler diagnostics and test outcomes accurately.
- The user performs physical hardware testing.
- Typical serial setup: Windows host, COM6, 38400 baud, 8N1, Tera Term.
- Do not report a test as PASS unless there is actual evidence.
- Preserve reproducible test procedures.

## Git workflow

- Work on `feature/kilo-editor`.
- Keep `develop` as the established integration baseline.
- Do not commit, push, merge, rebase, reset, or force-push without explicit user approval.
- Never push to the upstream repository.
- Review changes before staging or committing.
- Do not include generated object files, binaries, logs, temporary files, or backups in commits unless explicitly requested.

## Collaboration

- Communicate with the user in Czech unless requested otherwise.
- Prefer precise, verified information over speed.
- Explain the next proposed action before making significant changes.
- Ask for confirmation when a change could affect existing working functionality.
- Clearly distinguish verified facts, assumptions, and proposals.
