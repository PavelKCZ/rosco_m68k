# Kilo Editor Porting Context — rosco_m68k

## 1. Project purpose

Port the original Kilo text editor by Salvatore Sanfilippo (antirez) to the rosco_m68k single-board computer.

Original Kilo repository: https://github.com/antirez/kilo

The initial implementation must not depend on curses or ncurses.

The first milestone is a minimal working editor. Advanced functionality and optimizations will follow only after the basic port has been tested on real hardware.

The editor will ultimately load and save text files stored on a connected MicroSD card using a FAT filesystem.

## 2. Hardware and development environment

**Target hardware:** rosco_m68k with Motorola MC68010 CPU.

The board uses firmware-provided serial I/O rather than a full POSIX operating system.

Development environment:

- Ubuntu host for source development and cross-compilation.
- m68k-elf-rosco GCC 13.3.1 toolchain (previously used; verify current installation).
- Windows host for serial testing.
- COM6, 38400 baud, 8N1.
- Tera Term terminal emulator with local echo disabled.
- Python 3.14 for automated serial tests.
- Connected MicroSD card with FAT filesystem for persistent text files.

Firmware timing reference: 100 ticks per second.

The target is a freestanding embedded environment. Availability of standard C library functions and POSIX system calls must be checked individually.

## 3. Repository and Git configuration

Local repository:

`~/rosco/rosco_m68k`

Git remotes:

- `origin`: https://github.com/PavelKCZ/rosco_m68k.git
- `upstream`: https://github.com/rosco-m68k/rosco_m68k.git

Branches:

- `develop`: stable integration baseline.
- `feature/libtermios-1.0`: completed termios integration.
- `feature/kilo-editor`: active Kilo development branch.

Starting commit for Kilo development: `78fcb15`.

Relevant commits:

- `c4a6745` — Add POSIX-like termios 1.0 library.
- `78fcb15` — Allow overriding PROGRAM_BASENAME in software builds.

Do not modify the upstream repository or rewrite existing Git history.

## 4. Existing libtermios 1.0

Location:

`code/software/libs/src/termios/`

The library provides POSIX-like terminal configuration over the rosco_m68k firmware serial API.

Supported functionality includes:

- Canonical input processing.
- Noncanonical (raw) input.
- VMIN/VTIME behavior.
- Character echo and erase.
- VKILL (Ctrl+U).
- VEOF (Ctrl+D).
- Terminal attribute retrieval and configuration.
- Selected input flushing operations.

The implementation does not provide a complete POSIX terminal subsystem.

Known limitations must be checked in:

- `LIMITATIONS.md`
- `API.md`
- `termios.h`

Do not assume unsupported POSIX operations are implemented.

### Raw input semantics

Previously tested configurations include:

| VMIN | VTIME | Expected behavior |
|---|---|---|
| 0 | 0 | Immediate polling |
| 0 | 10 | Wait up to approximately one second |
| 4 | 0 | Wait for at least four bytes |
| 4 | 10 | Interbyte timeout |

These behaviors were tested on real hardware.

### Canonical input

Canonical editing includes Backspace, VKILL (Ctrl+U), and VEOF (Ctrl+D).

Canonical line buffering has a 256-byte boundary: up to 255 data bytes plus a line terminator.

Ctrl+C signal handling is not implemented.

### Validation

The library passed **78 out of 78 real-hardware test scenarios**.

Testing covered:

- Canonical line editing.
- VMIN/VTIME combinations.
- Timeout boundaries.
- Binary data integrity.
- Large and repeated serial transfers.
- Input flushing and recovery.
- Mode switching.
- Unsupported-operation rejection.
- Terminal speed configuration checks.

The completed release is `libtermios 1.0`.

This validation applies to the library, not to the future Kilo port.

## 5. Build integration

The library was integrated into:

`code/software/libs/`

Relevant integration files:

- `code/software/libs/Makefile`
- `code/software/libs/src/termios/include.mk`
- `code/software/libs/src/libm/include.mk`

The libm build definition was corrected to search only its own source directory:

```make
SOURCES=$(shell find src/libm -name "*.c")
```

This prevents unrelated source files, including `termios.c`, from being accidentally included in `libm.a`.

A clean build previously completed successfully.

The build produced:

- `code/software/libs/build/lib/libtermios.a`
- `code/software/libs/build/include/termios.h`

The Kilo port must use the actual existing build conventions rather than inventing a separate incompatible toolchain.

The repository also contains a modification to `code/software/software.mk` that allows overriding `PROGRAM_BASENAME`.

## 6. Kilo porting challenges

Before implementation, inspect the actual upstream Kilo source code and identify dependencies.

Areas requiring investigation:

### Terminal input

- Raw mode setup and restoration.
- Key reading and escape-sequence parsing.
- Arrow keys and editing commands.
- VMIN/VTIME compatibility.
- Behavior when no input is available.

### Terminal output

- ANSI/VT100 cursor positioning.
- Screen clearing and line clearing.
- Terminal size handling.
- Status bar and prompt display.
- Serial output throughput.

At 38400 baud, transmitting 2000 characters requires approximately 0.52 seconds with 8N1 framing, excluding control sequences and processing overhead.

Avoid assuming that full-screen redraw on every keypress will be sufficiently responsive.

### Memory

Investigate:

- `malloc`
- `realloc`
- `free`
- Buffer growth.
- Memory limits.
- Stack usage.

### File I/O and MicroSD/FAT filesystem

The Kilo editor will load and save text files stored on a MicroSD card connected to the rosco_m68k board. The card uses a FAT filesystem.

Before implementing file operations, inspect the existing rosco_m68k libraries, their headers, implementations, and example applications.

The following interfaces are particularly important:

- `basicio.h` — investigate the available basic input/output functions and their relationship to firmware services.
- `machine.h` — investigate relevant hardware and firmware interfaces.
- **`sdfat.h`** — investigate the existing SD-card/FAT filesystem API, especially file opening, reading, writing, closing, and error handling.

The `sdfat.h` interface is the primary candidate for implementing Kilo's persistent file operations.

Important requirements:

1. Determine the exact capabilities and limitations of the existing SD/FAT implementation.
2. Identify how the MicroSD card and filesystem are initialized.
3. Verify file creation, opening, sequential reading, writing, closing, and persistence.
4. Investigate file sizes, buffering, available memory, filename handling, and FAT filesystem limitations.
5. Determine whether POSIX-like file operations are already available or whether a small compatibility layer is required.
6. Prefer adapting Kilo's file I/O through a dedicated platform-specific abstraction rather than modifying the existing SD/FAT library unnecessarily.
7. Ensure that terminal input/output and file input/output remain logically separate.
8. Test loading and saving real text files on the MicroSD card using the physical MC68010 board.

Do not assume that standard POSIX file descriptors or C stdio functions are directly compatible with the SD/FAT implementation.

The exact API and supported operations must be verified from the repository source code before implementation.

### Build

Determine:

- Required source files.
- Required headers.
- Compiler and linker flags.
- Existing library dependencies.
- Suitable location within the rosco_m68k software tree.

## 7. Development milestones

### Milestone 1 — Source and platform API analysis

Inspect original Kilo and prepare a dependency inventory.

Identify supported, missing, and replaceable functionality.

Inspect the existing rosco_m68k interfaces relevant to both terminal and file I/O:

- `termios.h` and its implementation.
- `basicio.h`.
- `machine.h`.
- `sdfat.h` and its implementation.
- Existing applications demonstrating SD/FAT file operations.

Document the results in a dependency matrix, including the relevant source files and verified API signatures.

Identify any missing functionality and propose minimal adaptations.

Do not modify `libtermios` or the SD/FAT implementation during this phase.

### Milestone 2 — Minimal terminal prototype

Verify:

- Entering and leaving raw mode.
- Reading keyboard input.
- Processing arrow-key escape sequences.
- Moving the cursor.
- Drawing simple text.

Test on the actual MC68010 board.

### Milestone 3 — Minimal Kilo port

Adapt the editor to compile and run.

Prioritize:

- Text display.
- Cursor navigation.
- In-memory editing.
- Basic screen refresh.

File operations may initially be isolated behind a platform-specific interface.

### Milestone 4 — MicroSD/FAT file operations

Implement or adapt file loading and saving using the verified existing rosco_m68k SD/FAT APIs.

Prioritize:

- MicroSD and FAT initialization requirements.
- Opening existing text files.
- Reading file contents into the editor.
- Creating new text files.
- Saving modified text.
- Closing files and handling errors.
- Verifying persistence after reopening files.

Investigate safe handling of partial writes, storage errors, and file replacement based on the actual filesystem API.

Prefer a small compatibility or abstraction layer rather than invasive modifications to existing libraries.

Verify data integrity on the physical MicroSD card and real MC68010 hardware.

### Milestone 5 — Usability and optimization

Improve:

- Refresh efficiency.
- Responsiveness over 38400-baud serial.
- Error handling.
- Editing features.
- Documentation.

## 8. Long-term direction

The preferred development sequence is:

1. Original Kilo without curses/ncurses.
2. Improved Kilo adapted to rosco_m68k.
3. Potential reusable terminal abstraction (`libterminal`).
4. Optional minimalist curses-compatible interface.
5. Consideration of more advanced editors only if justified.

A possible long-term project name is **Kilo68k**.

This is a direction for future development, not a requirement for the initial port.

## 9. Testing and evidence

Distinguish clearly between:

- Source analysis completed.
- Compilation successful.
- Linking successful.
- Execution on hardware confirmed.
- Functional test passed.

Only the user can confirm actual hardware results.

Preserve build logs and reproducible test procedures where practical.

Do not mark unexecuted tests as successful.

## 10. Immediate next steps

1. Review and commit `AGENTS.md` and this context document.
2. Inspect the original Kilo repository and its licensing.
3. Analyze `basicio.h`, `machine.h`, and `sdfat.h`, including implementations and example programs.
4. Create a dependency matrix covering terminal I/O, memory management, and MicroSD/FAT file operations.
5. Select a minimal first porting target.
6. Begin controlled implementation using Codex CLI on Ubuntu.

The guiding principle is to preserve the verified `libtermios 1.0` baseline, reuse the existing MicroSD/FAT facilities, and make small, testable changes.


## 11. Aktuální checkpoint po F3-I1 (aktualizováno 2026-11-13)

**Při pokračování po restartu nejprve přečíst [KILO_F3_CHECKPOINT.md](KILO_F3_CHECKPOINT.md)
a AGENTS.md.** Sekce 10 výše zachovává historický počáteční plán; není aktuálním
pokynem ke commitu ani zahájení implementace. Také KILO_DEVELOPMENT_CHECKPOINT.md
zachovává dřívější historii M2.2-A; další krok určuje nový F3 checkpoint.

- Větev `feature/kilo-editor`, HEAD `e2e8db67c6a7da6b66b0be1a8cf51d175189409c`.
- libtermios 1.0: 78/78 reálných HW testů PASS, stabilní, neměnit.
- M2.1 dokončen a HW ověřen, commit `b67ae1d`; M2.2-A dokončen,
  18/18 HW scénářů PASS, commit uvedený výše. B1 změny jsou stále necommitnuté.
- Potvrzená fyzická RAM: základ 1 MB, rozšíření 5 MB; aktuální testovací deska
  MC68010 má 5 MB. To neurčuje automaticky RAM dostupnou aplikaci nebo linkeru.
- M2.2-B1: 64 řádků × 256 ASCII znaků, statická RAM, viewport 23×80,
  status řádek 24 a warning řádek 25; žádné soubory/FAT/MicroSD ani heap.
- B1.3b H01–H11: 11/11 PASS na hardware před F3. F1 opravil H07 autorepeat;
  F1 používá FIFO 512 bajtů a maximálně 16 RX čtení na pumpu.
- B1-P01: před F3 rychlý PuTTY paste 59/100, Python 5 ms a 2 ms 100/100,
  Python 1 ms opakovaně 96/100. Ztráta před úspěšným návratem RX do aplikace;
  bez zjištěného aplikačního overflow. UART overrun není potvrzen.
- D2 je pod `KILO_RX_DIAGNOSTICS`: Ctrl+R reset, Ctrl+T zmrazený report,
  Ctrl+Q exit; report označuje započtení Ctrl+T do RX součtu.
- F3-I1 implementováno: bounded neblokující RX služba během konstrukce rámce,
  beze změny ANSI obsahu a limitu 2247 bajtů. Host regrese, ASan/UBSan a oba
  MC68010 buildy PASS. **F3-I1 je hardware-validated: 12/12 scénářů PASS**,
  potvrzeno uživatelem 2026-11-13 na skutečném rosco_m68k MC68010.
  Python 2 ms, Python 1 ms ve dvou bězích, rychlý paste v Tera Term i PuTTY,
  autorepeat obou šipek 10 s, mazání, editace, scrolling a integrita: PASS.
  Úplný inventář F3-H1a až F3-H5a je v checkpointu; B1 jako celek se tím
  automaticky neoznačuje za uzavřený.
- Existující BIN: `code/software/kilo68k/kilo68k.bin` 7988 B a
  `code/software/kilo68k/kilo68k-rxdiag.bin` 10416 B.
- Po F3 nebyla v provedených scénářích pozorována ztráta znaků. UART overrun
  nebyl přímo změřen; F3 nezaručuje bezchybný přenos při libovolném zatížení.
- Tera Term je nyní preferovaný terminál; DELETE a Ctrl+Q jsou nakonfigurovány
  na stejné kódy jako v PuTTY. Historické anomálie nepředstavují aktuální
  nastavení a jejich původní příčina zůstává neurčena.
- Další krok: vyčkat na výslovné instrukce. Žádný nový test, F2,
  implementace ani Git publikace nejsou touto dokumentační aktualizací povoleny.

Přesný postup, velikosti, otisky, Git stav a omezení jsou v novém checkpointu.
