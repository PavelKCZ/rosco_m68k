# M3.1-A — isolated Newlib MC68010 runtime

Scope: startup, memory/heap, console, exit and link ownership only. No SD/FAT
initialization, no file handles, no Kilo changes. This directory is not included
by any legacy build. The original hardware smoke failed; D3 reached !01. The corrected D5 diagnostic build passed the physical MC68010 runtime smoke
test (D6); the corrected normal BIN is not yet hardware verified. See
[hardware result and closure](../../../../docs/KILO_M31_H1_D6_HARDWARE_PASS_CHECKPOINT.md).
D5_ABI_FIX.md retains the historical pre-hardware state. No FAT I/O is implemented.

## Build and native validation

From this directory, with the installed m68k-elf-rosco GCC 13.3.1 toolchain:

```
make BUILD=/tmp/kilo-m31a-example all diagnostic host-test
```

Use a fresh external BUILD directory. `all` produces ELF, BIN, MAP, disassembly,
section/size/symbol reports and audit.json. `host-test` compiles production
memory/syscall code and the unchanged libtermios source against a native fake
machine API; no serial adapter or hardware is used. Python needs only its stdlib.
The host reent.h is a test shim; the target always uses installed Newlib headers.

## Ownership and inputs

* startup.S: BIN entry, relocation, BSS, stack, low-level firmware `_exit`.
* runtime.c: heap/console initialization, Newlib array walkers, legacy GCC
  constructor/destructor arrays, main, exit/atexit.
* memory.c: bounded 32-bit firmware-address policy and overflow-safe heap movement.
* syscalls.c: Newlib reentrant hooks, shared console descriptor mapping, unsupported
  filesystem stubs. No private errno implementation and no global syscall errno.
* Newlib libc.a: FILE, malloc/free, __errno, exit/atexit and standard C interfaces.
* Existing libs/src/termios/termios.c is compiled without source modifications.
  Four fd-taking tc* symbols receive private build aliases so fd 0/1/2 can share
  handle 0. Raw/canonical/echo/newline semantics remain in libtermios alone.
* compat/fdlibm.h only satisfies an unused legacy include in termios.c. It prevents
  importing legacy math/errno headers; it provides no math or errno implementation.
* Only machine.o is extracted from the installed librosco.a into BUILD. The
  archive is NOT passed to the linker. audit.py checks its parent archive SHA-256
  and exact extracted bytes. No syscalls/FAT/directory/SD objects are selected.

There is no software.mk inclusion, no installed library replacement, no complete
librosco/libnosys, no legacy libc/printf/heap/startup. All target C/ASM compilation
uses explicit MC68010 flags. -nostdlib selects explicit project objects + Newlib
libc.a + compiler libgcc.a; the map records every pulled archive member.
-z noexecstack explicitly handles old libgcc objects lacking GNU-stack notes;
RX/RW program headers avoid RWX segments. These flags are metadata, not an MMU
protection guarantee on MC68010.

## Loader and memory contract

The normal sdfat_menu BIN load is at 0x40000; execution relocates to 0x2000.
startup.S captures the actual PC-relative source address and supports disjoint,
4-byte aligned source images that fit the validated region. The linker rejects
an initialized runtime image extending into the normal 0x40000 source region.

Before relocation, SP writes, or C calls, assembly validates SDB magic B105D47A,
RAM and reset-stack bounds [0x80000,0xe00000], 4-byte metadata alignment, heap room,
source/destination separation and source end. It chooses min(SDB RAM, reset stack
vector), reserves 32768 bytes and switches SP BEFORE overwriting the old menu's
private stack. Only then does it copy the boot/postboot trampoline, run from low
RAM, copy the tail and zero BSS. Firmware interrupt enable state is unchanged.

memory.c independently revalidates the policy before C runtime initialization.
Heap starts at 8-byte-aligned _end, ends at top-32768, and never grows past this
fixed limit. Negative sbrk changes cannot pass the initial break. Failed changes
preserve the old break and set ENOMEM on the supplied Newlib reent.

At 5 MiB with no reservation: stack [0x4f8000,0x500000), heap
[align(_end),0x4f8000). A lower reset vector lowers BOTH limits. No 5 MiB-only
assumption is encoded. Linker address space is not treated as physical RAM.

Invalid early metadata enters a silent fail-closed branch loop without trusting
a new stack or exit vector. It requires operator recovery, not automatic retry.
After valid startup, _exit jumps through EFP_PROG_EXIT at 0x490, preserving the
firmware warm-boot convention. It does not return to the old menu instance.

32 KiB is an engineering stack reservation, NOT a measured maximum. Stack
exhaustion beyond that reservation is not detected dynamically. Actual ROM,
interrupt behavior, device binding, BIN boot, constructors, heap execution and
exit remain unverified on hardware. Nontrivial C++ exception/TLS startup is outside
this C-only increment.

## Console contract

FD 0 is input, 1/2 output. All use termios handle 0 bound to firmware device 0.
No SD init, implicit read, input flush or extra echo occurs during initialization.
The tcgetattr/tcsetattr/tcdrain/tcflush wrappers map all three console FDs to the
same handle. Other descriptor values fail; no file table is allocated.
Console close is deliberately ENOTSUP, seek ESPIPE, unsupported filesystem
operations ENOSYS. isatty returns 1 for console, otherwise 0 with EBADF.
Termios send completion does not prove physical UART transmission completion.
The smoke application does not wait for interactive input.

## Validation record (2026-11-20)

Evidence: /tmp/kilo-m31a-ul0s3995/
Final output: final/
Final command: make BUILD=/tmp/kilo-m31a-ul0s3995/final all host-test
Result: target build/map/ELF-BIN audit PASS, native tests 33/33 PASS, exit 0.
Native tests: 20 memory/heap tests and 13 console tests, with additional subcases.
No target execution, hardware access, network, installation or Git mutations.

Preliminary logs build-01 through build-04 retain: an unresolved linker-symbol
subtraction in assembly; old libgcc executable-stack warning; GC of declared
unused hooks; and missing abort signal hooks / RWX segment warning. These were
fixed only here. Static review also corrected the trampoline copy extent and
moved SP selection ahead of relocation. A native isatty assertion initially
expected -1; both code and assertion were corrected to the conventional 0+errno
error contract. No failing logs were deleted.

Smoke runtime behavior is only BUILT, not executed: data/BSS/constructor probes,
malloc/free, rejected oversized sbrk preserving the break, main return, atexit and
destructor output. Do not describe these as hardware PASS.

Original 2240-file baseline inventory, final preservation comparison, commands,
source hashes and preliminary/final logs are external. No existing repository
file was intentionally changed. Do not begin M3.1-B or hardware testing without
separate authorization.

## M3.1-A.1 hardening — 2026-11-20 (supersedes integration limitations above)

All changes remain in this directory. No FAT, SD, Kilo, firmware or legacy
libtermios changes. Startup assembly, memory policy, syscalls and original 33
unit test identities are unchanged.

* A-01: `public/termios.h` forwards ONLY the approved project header. `-Ipublic`
  makes `<termios.h>` work without exposing the legacy SDK root. The compile-only
  `tests/angle_include.c` and compiler dependencies prove that termios comes from
  the project while stdio/stdlib/errno/reent come from installed Newlib.
* A-02: runtime.c extracts its existing C lifecycle into m31_run_application().
  Native lifecycle.c includes that exact implementation with injected console,
  Newlib array, main and exit/atexit dependencies. It verifies constructor order,
  main return status, LIFO atexit, destructors and early C failures. This is HOST
  SIMULATION of Newlib exit/array behavior, NOT execution of Newlib on MC68010.
  Separate native probes exercise actual console-bind failure, fstat and errno.
* A-03: audit.py uses explicit exceptions and exit 2, including under Python -O.
  It requires the actual map cross-reference defining object, rejects forbidden
  objects/unresolved symbols, checks ELF/BIN bytes, section/relocation boundaries,
  all LOAD addresses/permissions/alignment and non-executable GNU_STACK. Malformed
  or missing evidence fails. Negative tests corrupt copies, never the real build.
  Audit does not execute instructions, prove physical RAM/ROM identity or measure
  interrupt stack use. Exit status is authoritative if evidence writes fail.
* A-04: -MD -MP records BOTH project and system/Newlib headers in external .d files.
  Make includes these files. Link prerequisites also include libc/libgcc archives.
  Dry-run -W tests validate required rebuilds for every compiler-emitted alias of
  termios/machine/runtime/Newlib headers without touching protected input files.

Final clean build and complete suite:

```
make BUILD=/tmp/kilo-m31a1-bvzuawul/final all host-test
```

Results: target build and strengthened audit PASS; 56/56 host tests PASS,
zero failures/errors/skips/XFAIL. Counts: 33 original + 6 lifecycle/error +
12 negative/positive audit + 5 include/dependency tests. Test runtime 2.627 s.

Evidence: /tmp/kilo-m31a1-bvzuawul/ (original/ preserves the original 16 files).
final/ includes ELF, BIN, map, disassembly, .d files, include-check.json,
audit.json, lifecycle-scenarios.jsonl and dependency-audit.jsonl.

Preliminary evidence is retained: target-01 audit wrongly rejected duplicate
local NOOP symbols; this audit implementation error was corrected to distinguish
local/global names. verify-03 recorded 54 PASS + 2 failed dependency assertions:
-W initially used canonical paths while Make tracks emitted spellings. The
harness now marks all emitted aliases; original headers were never modified.

Final BIN: 44980 bytes, SHA-256
066e667d849623ac682c8153542af1f688209f28c9f09a0bedaafba6943ebeb5

No hardware execution. A-01..A-04 are closed for this scoped increment. Physical
boot, IRQ behavior, 32 KiB stack sufficiency, running ROM identity and real exit
remain hardware-only uncertainties. A future controlled smoke test requires new
user authorization. M3.1-B and D07 work remain outside this stage.

## M3.1-H1-D2 diagnostic profile

The first A.1 physical run was reported FAIL (no visible application output);
root cause remains UNKNOWN. See [DIAGNOSTIC.md](DIAGNOSTIC.md) for the single
opt-in instrumented build, exact marker semantics and safety limitations.

Default normal BIN identity is unchanged. Full D2 verification:
`make BUILD=/tmp/kilo-m31-h1-d2-o3h8zybv/final all diagnostic host-test`
71/71 host tests PASS; both target profiles/audits PASS. These are offline
results, not a successful hardware startup. No hardware run is authorized by
this documentation or by the diagnostic build target.

## Maintained corrected D3 profile

Use `b3_v2fix/README.md` for the current reviewed source pins and portable build.
The older executable/CSD profiles and their manifests describe historical
milestones and are not current-source verification manifests. No historical
manifest has been rewritten. The original boot magic check was removed during
the D5/D6 ABI work; see startup.S and D5_ABI_FIX.md rather than older prose above.
Full runtime tests now require an explicit `--historical-runtime-bin PATH` whose
size44980/SHA256 is verified before any host compilation. The child regression
uses M31_HISTORICAL_RUNTIME_BIN; it never substitutes a newly built image.
