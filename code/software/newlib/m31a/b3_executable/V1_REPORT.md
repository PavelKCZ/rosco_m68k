# M3.1-B3-T2-V1 — first executable validation

2026-11-21. Overall **CONDITIONAL PASS**. No hardware/COM6/physical SD access.
All original runtime/B1/B2/B3/firmware/Kilo/checkpoint inputs preserved. Branch
feature/kilo-editor, HEAD9881e9e0f53ba5888887eaa91ddc982521ea8e89, staging empty.
Original I1 manifest verified before corrections; complete baseline2409 files and
initial-source/ snapshot preserved externally. Prior B3 checkpoint295681B SHA256
3a05440c28095cd3da0b4b82ab5efc2c4cac4327842301cf7845a6609112f17e matched.

## Evidence and gate results

Full external evidence: /tmp/kilo-m31-b3-t2-v1-3y_kc51x/ (volatile /tmp).
Existing artifacts under target/ were generated here, not reused from old builds.
No binary or generated target artifact is stored in the repository.

| Gate | Actual result |
|---|---|
| V1-A | 23/23 host-tool syntax/import/manifest/schema/path/entry checks PASS |
| V1-B | Application MC68010 compile PASS, no warnings; actual app host tests PASS |
| V1-C | Intended12target objects compiled/extracted, no warnings |
| V1-D | First executable link/BIN/map PASS; zero unresolved symbols |
| V1-E | Automatic MACHINE_PASS; default full audit CONDITIONAL/exit3 |
| V1-F | Final wrapper ABI/alignment/read-only static checks PASS; active ROM UNKNOWN |
| V1-G | 14/14 mutations rejected for intended reasons |
| V1-H | Honest manual review: stack/IRQ UNKNOWN; active ROM assumptions CONDITIONAL |

No all-PASS release result is claimed. Default make audit stops nonzero when the
manual review is conditional. All source assertions, archive/member allowlists
and default manual gates remain active. T2-H1 is not authorized.

## Commands and tests

V1-A harness: external v1a.py, v1a-01/02.log; imports executed with
PYTHONDONTWRITEBYTECODE=1. Invalid paths/missing evidence fail nonzero, no compiler
was run until V1-A passed. Compiler/link command construction and subprocess
shell=False/list invocation were inspected. No shell functions in Make expansion.
V1-B exact standalone application probe command/diagnostics: v1b-01.json.

Target commands (executed separately, in this order):

```sh
make -C code/software/newlib/m31a/b3_executable BUILD=/tmp/kilo-m31-b3-t2-v1-3y_kc51x/target objects
make -C code/software/newlib/m31a/b3_executable BUILD=/tmp/kilo-m31-b3-t2-v1-3y_kc51x/target link binary inspect
```

Expanded compiler/tool commands, versions, binary hashes, dependency hashes and
exit codes are in target/build-record.json; no compiler diagnostics. GCC13.3.1
20240927, binutils2.42; exact MC68010 flags/profile retained, original link.ld,
-nostdlib and explicit libc/libgcc group; only extracted machine.o from librosco.
No source/header of an existing component changed. Target application source and
profile.py/Makefile did not require correction; therefore no target relink was
needed after audit-only/documentation/host-test changes.

Actual src/smoke.c is included by tests/test_smoke.c with only B3/runtime/stdio
boundaries faked. Host C11 -O2 -g -Wall -Wextra -Werror -Wpedantic,
-DM31_DIAGNOSTIC; no source outside this subtree changed. 25 unique cases PASS,
0 FAIL,0 SKIP,0 XFAIL. Same25 PASS at -O1 with ASan/UBSan,
-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer.
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1.
Host compile/test logs preserve all outcomes. LSan NOT VERIFIED (historical
ptrace limitation); no new LSan PASS claimed. Host app tests do not execute
Newlib/m68k startup or real B1/B2, and do not replace prior real B3-I1 tests.

Coverage: success,2^32 capacity, init/open/info failures, wrong size, partialIO,
shortEOF, malformed byte, EOF mismatch, SET/boundary/END failures, wrong END
position, close and secondary cleanup errors, invalid shutdown state, console
setup/write/flush failure before init, atexit/BSS/data/constructor failures.
Success exact B3 call sequence IOFRRSREFRCX checked. No init after failed marker;
no double close, cleanup on failed init, primary128-byte IO error retained despite
secondary close error. Lifecycle callbacks simulated from actual app source.

## Corrections / preserved failures

1. v1e-01.json recorded expected MANUAL_REQUIRED after all automatic checks.
   Requiring every manual field PASS would force invented acceptance for an
   unproven stack/active-ROM bound. Audit now separates --machine-only (positive
   control only) from DEFAULT full review. Default is FAIL2 for missing/malformed
   evidence, CONDITIONAL3 for honest UNKNOWN/CONDITIONAL, PASS0 only if all manual
   fields PASS. Negative variants still use the full default and only FAIL with
   the specified reason counts; CONDITIONAL is never a negative PASS. No unsafe
   artifact is accepted by the default release gate. No allowlist was broadened.
2. Driver formerly discarded failed-tool stdout before saving the requested
   evidence file. It now saves stdout before propagating nonzero status, includes
   exit/path in error, and preserves target/audit.json even on CONDITIONAL3.
   default-audit-02.log records make's nonzero2, not a successful full audit.
3. Added25 host application tests; no target application or protected fix needed.

## Actual ELF/BIN and memory

- m31-sdfat-smoke-diag.elf: 408392 bytes; SHA256 `63fa8ecfb0f093010bb97ac66e1d641b0ee08ad093dfacd70d164191e9b65f69`; CRC32 `6C39555C`.
- m31-sdfat-smoke-diag.bin: 60876 bytes; SHA256 `4c392208a6ff2d9c0ccedcf3e505b6855be03a55ef3221be8b7c271cbbf1b368`; CRC32 `EB53AC5E`.
- m31-sdfat-smoke-diag.map: 272020 bytes; SHA256 `dadfa0c1e1b2dfb9540eee9ba842be3afa144b9d51982151a07a56f917b67484`; CRC32 `65F80143`.

| Section | Bytes | VMA |
|---|---:|---|
| .boot |776|0x2000|
| .postboot |102|0x2308|
| .text (includes rodata) |57684|0x2370|
| .ctors |4|0x104c4|
| .dtors |4|0x104c8|
| .data |2304|0x104cc|
| .bss |84884|0x10dcc|

Entry0x2000, _boot_size0x370(880), _tail_size0xea5c(59996),
_image_size0xedcc(60876), _data_end/_bss_start0x10dcc,
_bss_end/_end0x25960(153952), aligned8. No separate .rodata output section;
constants are folded into RX.text by the unchanged script.

RX LOAD VMA0x2000 LMA0x40000 filesz/memsz0xe4cc; RW LOAD VMA0x104cc
LMA0x4e4cc filesz0x900 memsz0x15494; alignment16. GNU_STACK RW/nonexec.
No RWX. Source [0x40000,0x4edcc), initialized destination[0x2000,0x10dcc)
and BSS[0x10dcc,0x25960) are disjoint here; trampoline fully copied before tail,
stack selected before overwriting old menu RAM. ELF/BIN bytes matched.
Static allocated image+BSS145760B, not illustrative137404B. It includes this
application, retained libc/libgcc and final GC/alignment. At ACTUAL metadata top
0x500000 the existing policy would give stack[0x4f8000,0x500000), heap
[0x25960,0x4f8000),5056160bytes. This is a conditional5MiB plan, not a new board
measurement. Startup still validates metadata and lower reset-SP reservations.

## ABI, read-only and stack

Wrappers support0x4cb2/init0x4cb8/read0x4cca/CSD0x4ce4, all even. Exact F1
machine bodies, selector set0/1/2/4 and CMD9, A2 preservation and original stack
argument offsets confirmed. Descriptor22/alignment2; raw initialized@0/type32BE@2,
reserved@6; target DWARF in target/sd_ro.dwarf.txt. Adapter0x104cc, bounce
0x104f4 aligned4, all inside .data. No write wrapper, arbitrary CMD, SPI/ATA,
legacy filesystem object or backend allocation. Console _write_r is legitimate;
Newlib generic fclose/fvwrite are not FAT/SD providers. All storage operations
still route B3->B1->B2->approved read TRAP wrappers, no file FD integration.

CPU review: exact MC68010 compiler flags,175 typed C function ranges and pinned
assembly examined; no unsupported reachable instruction identified. Raw .text
also contains strings/jump tables/soft-float constants: objdump's apparent invalid
instructions there are data, not CPU compatibility evidence. No CPU execution.
_exit final code reads0x490 and jumps through A0, no old menu return.

New .su: main272, line8; B3open36/read44/init32; B1open744/init656/read616/
next_cluster560, sector28; B2read20/init56; wrappers save16 for read/CSD.
Known open path frame subtotal1708 including main but excluding full firmware/
interrupt/call overhead. _vfprintf_r local224+40 register save, _dtoa_r128+40,
__udivdi3 local4+44 saves; allocator paths also inspected. These are components,
NOT an exhaustive nested bound. Console formatting and SD calls are sequential,
not nested by the application, but firmware/IRQ behavior on active ROM is not
bounded. Stack32768 remains an engineering reservation. No limit was increased.
Manual record target/manual-review.json is tied to exact ELF/BIN and records
stack_irq_budget UNKNOWN and firmware_abi_assumptions CONDITIONAL, never PASS.

## Negative audit results

- missing_expected_provider: correctly rejected `OWNER:main`.
- duplicate_syscall_input: correctly rejected `OBJECT_INVENTORY`.
- legacy_fat_input: correctly rejected `LINK_INPUTS`.
- sd_write_selector: correctly rejected `WRAPPER_BODY:sdro_fw_read`.
- odd_wrapper_entry: correctly rejected `WRAPPER_ODD:sdro_fw_read`.
- wrong_load_delta: correctly rejected `LOAD_DELTA`.
- bad_image_size: correctly rejected `IMAGE_BOUNDARY`.
- rwx_segment: correctly rejected `LOAD_PERMISSIONS`.
- executable_stack: correctly rejected `GNU_STACK`.
- bin_disagreement: correctly rejected `BIN_BYTES:.boot`.
- missing_object_hash: correctly rejected `OBJECT_HASH:application.o`.
- mismatched_object_hash: correctly rejected `OBJECT_HASH:application.o`.
- unresolved_symbol: correctly rejected `UNRESOLVED_SYMBOLS`.
- missing_manual_gate: correctly rejected `MANUAL_REQUIRED`.

All14 passed their specific rejection criteria, starting from a MACHINE_PASS
positive image and honest conditional manual record. No synthetic manual PASS.
Positive ELF/BIN untouched. Copies retained in target/negative-cases/.

## Exact object and archive inventory

SHA256 per object (all are new target objects except explicit machine extraction):

- startup.o: `fedb25f40f9ba1fd5c2682c108f2da33be514ebb6b7e6f8bc55524a8dbd49eb6`
- memory.o: `9f86f45e329ac0e7e4c59fafc1cf2fc988b31ab965aa5dfd20573bbc0a98a224`
- runtime.o: `740dea069ab6b805420cb55c6873aa77a12d91d20382162a20ad021d91323f5a`
- syscalls.o: `44a32e2c897269071a774f88bde4e785d27f2683e73f54357d87b33e0b0e2714`
- termios.o: `7aee265a2e34e93b9b41dfb011a8c58577a1402a23b27e127c3938b058a39ac3`
- fat_ro.o: `87525e7b9cda45cd33fb30bc51bdca05594374c399d371d3cf84eaa54da689ca`
- sd_ro.o: `b962bfa1c66b2241f276991ea5ad084abb912787c0a5db3fea3aa886f7fb027d`
- csd_decode.o: `8e66d2401556abf15bcef3fd728e7b4b48f39410738e9bf0425d68c73f92accf`
- sd_ro_traps.o: `b3e83196ef271c96dbd86c166590e6c71dc135fde1c5946fe873c5780b6c3f7c`
- b3_integration.o: `813b068ae881ff0bce41f0d9f790bb8cc60eed8e2cc8c43a48cd7a35cbf9f420`
- application.o: `b629bd532ea65b99777814f85123b5f5104ac324c677382328aa9123730094b1`
- machine.o: `59bf564050fea966918559c7e5f2df6947875de2e35bcd845dbd29e397126aec`

Linked archive members (all within original I1 allowlist; exact member hashes in external object-archive-inventory.json):

- libgcc.a(_clz.o)
- libgcc.a(_divsi3.o)
- libgcc.a(_double.o)
- libgcc.a(_eqdf2.o)
- libgcc.a(_floatex.o)
- libgcc.a(_gtdf2.o)
- libgcc.a(_ledf2.o)
- libgcc.a(_ltdf2.o)
- libgcc.a(_modsi3.o)
- libgcc.a(_muldi3.o)
- libgcc.a(_mulsi3.o)
- libgcc.a(_nedf2.o)
- libgcc.a(_udivdi3.o)
- libgcc.a(_udivsi3.o)
- libgcc.a(_umodsi3.o)
- libgcc.a(fpgnulib.o)
- libgcc.a(xfgnulib.o)
- libc.a(libc_a-__atexit.o)
- libc.a(libc_a-__call_atexit.o)
- libc.a(libc_a-abort.o)
- libc.a(libc_a-assert.o)
- libc.a(libc_a-atexit.o)
- libc.a(libc_a-callocr.o)
- libc.a(libc_a-ctype_.o)
- libc.a(libc_a-dtoa.o)
- libc.a(libc_a-errno.o)
- libc.a(libc_a-exit.o)
- libc.a(libc_a-fclose.o)
- libc.a(libc_a-fflush.o)
- libc.a(libc_a-findfp.o)
- libc.a(libc_a-fini.o)
- libc.a(libc_a-fiprintf.o)
- libc.a(libc_a-freer.o)
- libc.a(libc_a-fvwrite.o)
- libc.a(libc_a-fwalk.o)
- libc.a(libc_a-impure.o)
- libc.a(libc_a-init.o)
- libc.a(libc_a-locale.o)
- libc.a(libc_a-localeconv.o)
- libc.a(libc_a-makebuf.o)
- libc.a(libc_a-malloc.o)
- libc.a(libc_a-mallocr.o)
- libc.a(libc_a-mbtowc_r.o)
- libc.a(libc_a-memchr.o)
- libc.a(libc_a-memcmp.o)
- libc.a(libc_a-memcpy.o)
- libc.a(libc_a-memmove.o)
- libc.a(libc_a-memset.o)
- libc.a(libc_a-mlock.o)
- libc.a(libc_a-mprec.o)
- libc.a(libc_a-reallocr.o)
- libc.a(libc_a-setvbuf.o)
- libc.a(libc_a-signal.o)
- libc.a(libc_a-sprint_r.o)
- libc.a(libc_a-stdio.o)
- libc.a(libc_a-strcmp.o)
- libc.a(libc_a-strlen.o)
- libc.a(libc_a-vfiprintf.o)
- libc.a(libc_a-vfprintf.o)
- libc.a(libc_a-vprintf.o)
- libc.a(libc_a-wctomb_r.o)
- libc.a(libc_a-wsetup.o)

No unresolved symbols. Source/runtime/syscall/libc/libgcc owners checked against
actual CREF map. Full archive paths and SHA256 in PROVENANCE.json/build-record.
Arithmetic closure includes _divsi3/_mulsi3/_udivsi3 internal aliases and _clz.o.
No complete librosco/libnosys or additional ad hoc helpers. Pinned source and
archive/member allowlists unchanged by V1.

## Preservation and next decision

Final preservation compares baseline2409 files; only authorized subtree changes
are permitted, no protected source/test/manifest/checkpoint mutation. No Git
mutation/network/install/hardware. B2 historical one-entry wrapper manifest debt
unchanged. Initial source snapshot and all preliminary/full/negative/host logs
remain externally available, never overwritten with PASS labels.

Recommendation: NO automatic advance or unconditional hardware GO. A separately
specified small read-only H1 could be considered ONLY after explicit user review
and acceptance/resolution of stack/active-ROM assumptions, fixture/BIN identity
and separate authorization for card preparation and execution. No hardware PASS,
full-stack proof, CRC integrity, Newlib FD or Kilo file-I/O result is claimed.

**M3.1-B3-T2-V1 CONDITIONAL PASS — RESIDUAL RISKS DOCUMENTED — HARDWARE NOT AUTHORIZED — REPOSITORY PRESERVED.**
