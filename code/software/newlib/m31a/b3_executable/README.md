# M3.1-B3-T2-I1 — isolated executable smoke profile

## Current T2-V1 status (2026-11-21)

See [V1_REPORT.md](V1_REPORT.md): first target build/link and automatic audit
completed; 25 host application cases PASS normally and with ASan/UBSan;
14 negative audit cases rejected for intended reasons. Overall CONDITIONAL PASS:
full firmware/IRQ stack bound and active ROM assumptions remain unverified.
No hardware, COM6 or physical SD access. The following I1 record is historical.

The audit now has explicit --machine-only for automatic checks (MACHINE_PASS,
exit0, never release approval). Default full audit remains fail-closed: missing
evidence FAIL/exit2; honest UNKNOWN/CONDITIONAL manual checks produce
CONDITIONAL/exit3; only all required reviews PASS can produce full PASS/exit0.
The negative harness uses machine-only positive control and the full default
audit for every mutant. It does not fabricate a manual PASS. Original assertions
and archive allowlists were not weakened. Failed subprocess stdout is retained.

## Historical T2-I1 record

**IMPLEMENTED / NOT EXECUTED.** Source/build-profile implementation only.
No C/ASM compilation, linking, Make invocation, host tests, audit tests, target
execution, physical SD or COM6 access occurred in T2-I1. No executable exists
for this profile. T2-V1 and T2-H1 remain NOT STARTED / NOT AUTHORIZED.

This Option A subtree adds one application and isolated build/audit tools.
It does not modify or copy the proven runtime, link.ld, B1/B2/B3 or their audits.
The original runtime Makefile is not included or invoked. D07 remains CLOSED.

## Exact source ownership

Paths below are relative to the parent m31a/ unless stated otherwise.

| Object | Source / owner |
|---|---|
| startup.o | startup.S: the sole BIN entry/relocation/BSS/_exit owner |
| memory.o | memory.c: checked memory plan and heap-break arithmetic |
| runtime.o | runtime.c: Newlib arrays, constructors, main/exit/atexit/destructors |
| syscalls.o | syscalls.c: sole reentrant syscall/console/heap-hook provider |
| termios.o | unchanged libs/src/termios/termios.c with original private tc* aliases |
| machine.o | exact machine.o extracted from pinned installed librosco.a; archive NOT linked |
| fat_ro.o | fat_ro/src/fat_ro.c: the only FAT provider |
| sd_ro.o | sd_ro/src/sd_ro.c: application-local SD descriptor/bounce/lifecycle |
| csd_decode.o | sd_ro/src/csd_decode.c: byte-wise checked CSD decoder |
| sd_ro_traps.o | corrected sd_ro/src/sd_ro_traps.S: selectors0/1/2/4, fixed CMD9 |
| b3_integration.o | b3_integration/src/b3_integration.c: exclusive B1+B2 orchestration |
| application.o | this subtree src/smoke.c: the only main() |

Original runtime smoke.c is excluded. No legacy FAT/SD/startup/heap/syscall
provider, libnosys, Kilo source, firmware object or Newlib FD adapter is selected.
PROVENANCE.json pins protected inputs and installed archives. All source units
are recompiled into a new external directory; historical /tmp objects are not
inputs. tools/profile.py is the single declarative source/flag/ownership table.

## Build profile — commands for a LATER authorization only

Installed m68k-elf-rosco GCC13.3.1 / binutils, Newlib4.4.0, Python3 stdlib required.
Do not run any of the following in T2-I1:

```sh
make -C code/software/newlib/m31a/b3_executable BUILD=/tmp/NEW-EMPTY-T2-V1 objects link binary inspect
# Perform and record the named manual review described in VALIDATION_PLAN.md.
make -C code/software/newlib/m31a/b3_executable BUILD=/tmp/NEW-EMPTY-T2-V1 audit
make -C code/software/newlib/m31a/b3_executable BUILD=/tmp/NEW-EMPTY-T2-V1 negative-audit
```

BUILD is mandatory, absolute, whitespace-free, resolving to a dedicated directory
below /tmp. A nonempty unrelated directory is refused. No source-tree outputs or
Python bytecode caches are allowed. There are no compiler queries during Make
expansion. The targets are separate: audit does not implicitly compile/link.
Objects are always recompiled when the objects target is explicitly requested;
-MD/-MP records compiler header dependencies for link/audit freshness checks.
.NOTPARALLEL protects the common build record. validate means audit+negative
audit of EXISTING artifacts, not hardware and not an implicit build.

Compiler flags retain the original MC68010 profile:

```text
-mcpu=68010 -march=68010 -mtune=68010 -std=c11 -O2 -g
-Wall -Wextra -Werror -ffreestanding -ffunction-sections -fdata-sections
-fno-common -fno-delete-null-pointer-checks --param=min-pagesize=0
-DM31_DIAGNOSTIC -fstack-usage
```

Headers use isolated public/runtime/compat/B1/B2/B3 directories and -iquote for
the approved machine/termios headers; the legacy SDK include root is absent.
The unchanged link.ld is explicit. Link flags retain -nostdlib, --cref,
--gc-sections, --warn-common, --fatal-warnings, -z noexecstack,
-z max-page-size=16 and -z common-page-size=16. Only explicit libc.a/libgcc.a
are grouped. --undefined=malloc/free deliberately retains the existing allocator
ownership checks; it does NOT claim the application calls malloc/free. The
application/backend buffers are static; Newlib stdio/atexit may internally allocate
through the existing runtime heap.

Distinct future outputs: m31-sdfat-smoke-diag.elf/.bin/.map/.dis. No such outputs
were created here. The original runtime normal/diagnostic outputs remain intact.

## Application contract

The existing diagnostic startup is unchanged. Main uses its M/O/F markers,
checks BSS/data/constructor probes, selects unbuffered stdout before first use
and flushes every diagnostic line. It uses only public B3 operations, not direct
B1/B2/firmware calls, fopen/fread/open/read or a new FD namespace.

Successful application sequence:

```text
APP_BEGIN top=........ stack=........ heap=.................
INIT_BEGIN
INIT_OK sectors=................
OPEN_OK
READ_OK bytes=513
EOF_OK
SEEK_OK
CLOSE_OK
SHUTDOWN_OK
APP_PASS
APP_ATEXIT
APP_DTOR
```

Dots are measured values, not predictions. Existing startup/W/F markers can share
the line with application text. Capacity is sixteen hexadecimal digits (two
32-bit words), preserving 2^32 without decimal-formatting assumptions.
INIT_BEGIN is flushed successfully BEFORE b3_init(). This call includes SD init,
CSD and FAT probing; there are no invented intermediate live markers. INIT_OK
means B3_READY, not a direct card CRC or firmware-version measurement.

The fixed /M31RO.BIN must be exactly513 bytes, byte[i]=i&255. A full B3_OK read
must also report513 delivered bytes. EOF/partial errors are not retried or
silently accepted. A subsequent1-byte read requires B3_EOF/count0; seek511/read2
requires FF00; SEEK_END/info requires offset513 and EOF again. Close and shutdown
are explicit before APP_PASS. Shutdown must report UNINITIALIZED/capacity0, and
a successful run must have no stored first error.

Failures print APP_FAIL phase/status/bytes/reason/index and the pre-cleanup B3
first-error snapshot (including firmware status and 64-bit delivered count).
Wrong content/size may be an application failure with B3_OK; reason/phase make
that distinction explicit. Any still-owned handle is closed, then B3 shuts down.
Secondary close errors never replace the primary result. Shutdown after failed
init is safe because B3 explicitly permits idempotent cleanup. No retries/reset.
Console failure can prevent diagnostics themselves; it is never interpreted as
proof that an SD operation ran. Main returns through existing exit(main()),
atexit/destructor handling and EFP_PROG_EXIT warm boot, NOT the old menu instance.

## Read-only proof boundaries

No application storage mutator or direct firmware operation. Actual B1 has no
write callback/writeback; actual B2 has only selectors0/1/2/4 with CMD9. Cleanup
does not invoke physical writes. The executable audit checks owners, pinned
sources, approved object/archive inputs and exact final wrapper bytes/call sites.
_write_r/stdio console output is allowed; a blanket 'write' symbol ban is wrong.
Unsupported filesystem syscall stubs stay in the unchanged runtime.

This is a software capability argument, not physical write protection, proof
against arbitrary memory corruption, or proof of the active ROM implementation.
Firmware init changes card protocol state and firmware ignores received CSD/data
CRC; no CRC-protected integrity claim is made.

## Audit scope and limitations

tools/audit.py is NEW and UNEXECUTED. It uses only the pinned original audit's
pure ELF/map parsers; it does not run the old profile-specific audit/main. It
checks source/object/dependency/archive identity, commands, owners, link inventory,
ELF/BIN/sections/relocation/lifecycle/permissions, exact firmware wrappers and
unresolved symbols. Newlib/libgcc members are explicitly allowlisted from the
preserved D5 map plus P0 providers and the new vprintf/setvbuf APIs. An additional
legitimate member can therefore STOP T2-V1; do not silently extend the list.

The input object must define sdro_validate_capacity with the approved owner, but
that one unused out-of-line helper may disappear from the final image under GC:
existing B3-I1 disassembly shows both callers inline its checks. No original
runtime ownership check is made optional. All four firmware wrappers remain
required at even final addresses. B2 adapter placement/extent is checked against
the prior target layout; actual descriptor/bounce DWARF remains a T2-V1 review.

CPU flags and simple instruction checks cannot prove every linked instruction or
firmware/IRQ stack depth. A named manual-review.json tied to this exact ELF/BIN is
mandatory. Missing/malformed evidence fails closed. A static/manual PASS would
still not authorize hardware. tools/negative_audit.py defines14 deterministic
mutations of external copies of a POSITIVE build. It cannot run without a prior
positive audit. Neither script was executed in T2-I1.

See VALIDATION_PLAN.md for all NOT RUN gates and the manual-review template.
B2 SHA256SUMS still has its historical stale wrapper entry; its corrected source
is pinned separately. No existing manifest or historical report was rewritten.

## Memory and next authorization

Load0x40000/run0x2000, the existing bounds/relocation/BSS logic, stack reserve32768
and heap policy remain unchanged. B1+B2+B3 historical static footprint91668B.
45736+91668=137404B is ONLY the requested illustrative arithmetic sum; it does
not include the historical2160B runtime BSS. Including that BSS gives139564B,
which explains the earlier differently scoped P0 sum, not a final memory budget.
New application storage, helpers, GC and alignment require measurement in T2-V1.

UNKNOWN: whether this new code compiles/links, audit correctness under execution,
final image/map/heap start, full stack depth and physical SD reads. LSan remains
unverified from earlier stages. No new tests passed or failed: none were run.
T2-V1 requires explicit authorization; T2-H1 separately authorizes both fixture
preparation and physical execution. Kilo file I/O and Newlib FD work remain absent.
