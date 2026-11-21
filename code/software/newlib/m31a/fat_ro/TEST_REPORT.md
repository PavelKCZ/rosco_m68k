# B1 host validation and audit report

Evidence directory /tmp/kilo-m31-b1-49mtdg1o/; essential raw logs copied into
fat_ro/evidence/. No target build or M3.1 runtime test executed in B1.
Compiler: cc (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0.
Flags -std=c11 -Wall -Wextra -Werror -Wpedantic; optimized -O2 normal,
-O1 -fsanitize=address,undefined -fno-sanitize-recover=all sanitized.

## Runs, unique counts and failures

- test-01.log: make exit2, test harness compilation failed: non-variadic RUN macro
  misparsed commas in C declarations. No test executed. Fixed to variadic RUN;
  no assertion removed or runtime altered for this issue.
- test-02.log: initial 89 unique cases PASS, exit0.
- test-03.log: expanded 111 cases PASS normal AND sanitized, exit0.
- audit-01.log: exit2, audit initially rejected the host compiler's automatic
  __stack_chk_fail dependency. Allowlisted only this stack-protector termination
  support in addition to memcpy/memset/memcmp; no filesystem dependency allowed.
- final-validation-01.log: `make ... test sanitized audit`, exit0. Final 112 unique
  cases PASS, 0 failures/errors/skips/xfail. Same 112 identities executed in both
  normal and ASan/UBSan binaries; total is 112, NOT 224. No sanitizer findings.
  One case added to cover sectors-per-cluster=2; byte comparisons across boundaries.
- leak-validation.log: separate detect_leaks=1 run exit134; LSan fatal runtime error,
  explicitly says unsupported under ptrace. LeakSanitizer NOT VERIFIED. This is
  not recorded as backend PASS or an ignored backend defect. No core file found
  in repository after the aborted process. Normal ASan/UBSan run completed with
  detect_leaks=0. Handle exhaustion/reuse/failed-open cleanup verified separately.

## Fixtures and assertions

C harness deterministically generates full in-memory images, then frees each one.
FAT16: 5000 data clusters, reserved1, FAT20 sectors, 32 root entries.
FAT32: 65530 clusters, reserved32, FAT513 sectors, root cluster2.
Both superfloppy and MBR primary partition at LBA7 exercised. Exact 513-byte
pattern i&0xFF, empty file, 512-byte file, 1300-byte fragmented file, nested directory.
No physical SD. Invalid callback LBA aborts test, so out-of-supplied-media access
cannot silently pass. Fault callbacks can partially overwrite scratch then fail;
public output remains untouched beyond delivered count.

Covered: valid FAT16/FAT32, missing/invalid paths, LFN/deleted skip, metadata
signature/geometry/capacity/version/partition failures, truncated declared medium,
invalid entry clusters/size/attributes, free/reserved/bad cluster markers, file and
directory loops, premature EOC, chain budget, FAT/root/data I/O failure, sticky
partial errors, failed open without handle loss, four simultaneous handles,
same-file independent positions, reuse/stale tokens, repeated shutdown, all seek
origins, negative/INT64 overflow, maximum/beyond-EOF offsets, multi-sector clusters,
SIZE_MAX read clamping and cleanup without media access. Full identities are in
evidence/test-identities.json. Token exhaustion at 2^64 was statically inspected,
not dynamically exhausted. Not an exhaustive test of all hostile FAT images.

## Write-protection evidence

All 112 cases check the host media write-attempt counter before AND after shutdown;
all counters zero. Host media defines a write function, but API intentionally has
no write-callback registration. Counter is corroborating evidence, not proof against
arbitrary malicious callbacks: B1 only calls the trusted read callback.

Audit PASS: exact seven b1_* exports, only allowed C memory/stack-protector imports,
actual compiler dependencies private, no reference originals or mutation sources
compiled, write/format config0, no SD/legacy/Newlib/malloc/mutation symbols in object,
no write path in object disassembly. The compiled subset contains no writeback,
flush, purge or format function; close/shutdown memset private state only.
Reference sources retain historical write code for provenance but are NOT linked.
Object SHA-256 and detailed dependencies are in evidence/audit.json.

## Next-stage recommendation

GO for separately authorized B2 firmware read adapter preparation, subject to
review/approval of B1's substantial RO pruning, 8.3/4096-chain limits, fixed memory
budget and immutable-media contract. B2 must explicitly retain no SD write wrapper,
review descriptor ABI, target footprint and stack use; first hardware work is NOT
approved by B1. No Newlib FD/syscall integration and no Kilo integration performed.
