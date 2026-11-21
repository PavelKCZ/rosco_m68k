# B3-I1 validation report

Recorded 2026-11-20. Evidence directory for full build outputs:
`/tmp/kilo-m31-b3-i1-i27o50q0/`; repository evidence copies retain textual results.

## Results

VERIFIED — host execution:
- 48 unique named integration scenarios: 48 PASS, 0 FAIL, 0 SKIP, 0 XFAIL.
- Same 48 scenarios under ASan/UBSan: 48 PASS, no reported sanitizer errors.
- No compiler warnings in final normal/sanitized/target compilation.
- Zero forbidden calls, selector-3 or sector-write calls across the mock traces.
- LSan separately attempted with detect_leaks=1; process exits 134 with explicit
  "LeakSanitizer does not work under ptrace". LSan NOT VERIFIED, not a backend
  leak diagnosis. Ordinary ASan/UBSan run uses detect_leaks=0.

Actual B1 and B2 sources are linked, not test doubles. Only firmware wrappers
are mocked. Coverage includes four FAT16/FAT32 x superfloppy/MBR combinations;
CSDv1/SDSC and v2/SDHC; uint64 2^32-sector propagation, actual B1 file read at
LBA UINT32_MAX; first/last/invalid sectors; SDSC guard; EOF/short EOF; bytewise
513-byte fixture verification; cross-sector chunks; seeks SET/CUR/END/beyond EOF;
seek overflow preservation; four handles/exhaustion/reuse/stale tokens; missing
file; init failures and false-OK descriptor; CSD partial failure/profile/end/type
mismatch; invalid/unsupported/over-capacity FAT geometry; directory I/O error;
broken chain; no-byte and partial-byte media errors and explicit seek recovery;
first-error retention; partial cleanup/order; READY init refusal; idempotent
shutdown and fresh initialization. See named PASS lines for exact identities.

The test named truncated_backing_read_failure injects an unreadable sector 0;
it does not pretend the CSD reports a smaller physical medium. Sparse backing
always bounds every read by its advertised capacity. Bad geometry beyond the
card capacity is independently tested. B1/B2 standalone suites were not rerun
or counted as B3 cases. Capacity-query error is static-only (see API.md).

## Target compilation and link

VERIFIED — target static inspection, GCC 13.3.1 20240927, binutils 2.42:
Five units: actual fat_ro.c, sd_ro.c, csd_decode.c, corrected F1 sd_ro_traps.S,
and new b3_integration.c. Strict MC68010 options are recorded verbatim in logs.
Only `ld -r -z noexecstack --fatal-warnings` of these five objects, no libraries.
ET_REL, m68k big-endian ELF32, entry zero, no program headers, no executable/BIN.

| Object | text | rodata | data | bss |
|---|---:|---:|---:|---:|
| fat_ro.o | 4892 | 0 | 0 | 82086 |
| sd_ro.o | 1160 | 0 | 576 | 0 |
| csd_decode.o | 742 | 0 | 0 | 0 |
| sd_ro_traps.o | 74 | 0 | 0 | 0 |
| b3_integration.o | 2024 | 0 | 0 | 114 |
| combined.o | 8892 | 0 | 576 | 82200 |

Static allocatable total = 91,668 bytes, excluding final-link alignment, runtime,
libc, libgcc, heap and stack. Other sections are debug/comment/zero-size stack
metadata, not extra allocated storage. B1 fs=82,078 plus serial=8 BSS; four chain
maps and shared work map dominate. Host BSS is not used as target evidence.

Maximum reported individual B1 frames: open 744, init 656, read 616,
next_cluster 560 bytes. B2 init 56/read 20; B3 read 44/open-close 36/init 32.
Assembly read/CSD wrappers save 16 bytes, init saves 8, support saves none,
excluding return addresses/TRAP exception stack. These figures are not a total
firmware/interrupt stack bound and do not prove the runtime's 32 KiB sufficient.

## ABI and ownership inspection

All four wrapper entries remain section offset zero, section alignment 2, with
exact F1 machine-code bodies, preserved A2 and stack argument offsets. Selectors
0/1/2/4, fixed CMD9. Descriptor static assertions compile: size22/alignment>=2;
initialized@0, card type big-endian bytes@2, reserved@6. Target DWARF confirms
bounce alignment4/offset40; .data.adapter alignment4, size576.

B3 init disassembly: capacity query writes an 8-byte local; moves both words to
D2/D3, pushes D3 then D2, then context and sdro_read, calls b1_init, cleans16bytes.
No narrowing or callback casts. Final executable addresses remain UNKNOWN.

Exact unresolved relocatable imports:
`__modsi3 __muldi3 __mulsi3 __udivdi3 __udivsi3 __umodsi3 memcmp memcpy memset`.
Arithmetic helpers originate from target B1 operations and are supplied by the
installed libgcc (provider archive path in audit.json); none was linked here.
Future executable integration must resolve/audit these explicitly. No malloc,
Newlib syscall provider, legacy FAT/SD adapter or firmware implementation import.

The audit checks pinned source hashes, exact per-object symbol owners, exactly
five map inputs, known imports, ET_REL/section permissions, wrapper bytes and
alignment in both individual and combined objects, host production objects and
header dependency graph. GCC host defaults add __stack_chk_fail and
_GLOBAL_OFFSET_TABLE_; these are allowed only for host objects, never target.

Individual target C objects omit GNU-stack notes. Initial trial ld-r warned
about inferred executable stack; the new isolated link explicitly uses
-z noexecstack (as the existing runtime does), with --fatal-warnings. Final
combined note is non-executable. This does not establish future executable
program-header permissions; final executable audit remains required.

## Audit development history

Preliminary validation-01 used 42 cases and exposed the stack-note warning;
validation-02 ran 48 cases after adding boundary cases. First full audit rejected
host hardening imports; they were explicitly identified and confined to the host
allowlist. No B1/B2 source, test or original audit was altered. Final validation
and final audit PASS; preliminary logs are preserved without rewriting results.

## Limits / next gate

UNKNOWN: actual firmware SD behavior, CRC integrity, final executable placement,
complete stack bound, file stdio/FD integration and physical media contents.
CONDITIONAL GO for separately authorized executable-integration planning/build:
retain project startup/console, resolve helpers deliberately, inspect final map,
relocation/BSS extent, entry addresses, permissions, descriptor/bounce alignment
and read-only paths before any separately authorized hardware run.

SDSC boundary qualification: the 0x007FFFFF last-address guard is checked through
B2's real pure address validator, and 0x00800000 is rejected before firmware.
The integrated SDSC fixture is 8192 sectors. Under the intentionally restricted
CSDv1 512-byte profile, an actual CSD-derived card reaching the wider firmware
2^23-sector limit is not represented; this is not claimed as an integrated
last-SDSC-sector transfer. SDHC UINT32_MAX transfer is exercised end-to-end.
