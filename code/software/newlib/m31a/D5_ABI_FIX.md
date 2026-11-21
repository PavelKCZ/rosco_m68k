# M3.1-H1-D5 — SDB ABI compatibility correction (2026-11-20)

## Contract and reason

The user hardware D3 log reached E, printed slot 00001DF8, and stopped at !01.
Local firmware bootstrap writes B105D47A to 0x400; WITH_KERNEL api_init later
writes &api there (kernel/api.c:49-50). Application kernelapi.h:161-168 recognizes
this dual use. A permanent boot signature is not part of the general application
ABI. Exact identity of 1DF8 on the running ROM remains UNKNOWN. No pointer value
is accepted as authenticated and no API pointer is dereferenced by this runtime.
The original normal silent failure is consistent with this gate; that original
run was not instrumented. Corrected builds have NOT been executed on MC68010.

## Minimal correction

startup.S no longer compares the transient slot. memory.c/header now expose:

```c
int m31_plan(uint32_t ram, uint32_t reset, uint32_t program_end,
             uint32_t load, uint32_t image_size, struct m31_layout *out);
```

runtime.c passes only actual planning inputs. M31_MAGIC is removed; no fallback
signature/range/hardcoded 1DF8 is added. All RAM/reset range/alignment, min-boundary,
32 KiB reserved stack, aligned heap, overflow, extent and non-overlap checks remain.
The 32 KiB stack reservation is still an engineering assumption, not a proven bound.
No changes to relocation architecture, console, errno, Newlib lifecycle, firmware,
menu, Kilo, libtermios implementation or FAT profile. M3.1-B not started.

Diagnostic entry prints `E ABI_SLOT=........ RAM=........ RESET=........ SP=........ SR=........`.
Raw ABI_SLOT is a single direct 32-bit read, for reporting only. d2 is unused by
validation. Direct TRAP #14 function 4 remains unchanged. !01 is RETIRED; !02..07
retain their numbers/meanings. All other markers and stop/error behavior remain.
No new safety claim is made about arbitrary firmware, loader entry or physical RAM.

## Verification and test changes

Evidence directory: /tmp/kilo-m31-h1-d5-o4iqet0b/ (volatile; not a permanent archive).
original-source/ and baseline.json preserve pre-edit files/hashes/status.
Clean new-directory command, from this directory:

```
make BUILD=/tmp/kilo-m31-h1-d5-o4iqet0b/final all diagnostic host-test
```

build-test-01.log: both target builds/audits PASS; native suite 75 PASS, 1 FAIL,
make exit 2. New test expected `movel 400,%d2`, but objdump included a symbol
annotation `400 <D_BIAS+0x2>`. Test harness regex corrected to accept annotation
while requiring exact address, operation and destination; runtime unchanged.
build-test-02.log: 76/76 PASS, zero failures/errors/skips/xfail, exit 0 (2.988 s).
build-test-03.log: final source/comment rebuild and 76/76 PASS, exit 0 (3.068 s).
Historical baseline 71 cases retained in count: 69 unchanged test names and two
renamed/replaced cases; three assertions/contracts intentionally updated:
- obsolete bad-magic test becomes null-output failure test; all independent
  memory-boundary negative tests retained;
- old normal-BIN identity test now verifies preserved reference artifact rather
  than requiring a corrected image to equal the defective original;
- diagnostic rejection mapping retains codes 02..07 and retires 01.
Five added D5 tests exercise native five-input planning and inspect actual target
instructions: no raw-slot dependency/dereference, first check RAM lower bound,
ABI_SLOT bytes, no magic comparison in normal/diagnostic C paths. Representative
raw values include boot magic, 1DF8, zero, FFFFFFFF and DEADBEEF. This is static
dataflow independence, NOT a CPU emulator or execution with physical address 400.
Normal profile still has no diagnostic symbols/early calls. Early diagnostic
register/CCR preservation, lifecycle, console and syscall tests remain active.

Audit changes narrowly retire !01 and rename measured label; add rejection of
stale magic gates, C slot reads, normal slot reads, or d2 validation dependence.
Existing ownership, archive exclusions, relocation, ELF RX/RW/no-RWX and
non-executable stack checks remain active. Both final audit.json results PASS.
Actual diagnostic validation starts at runtime 207E with RAM lower-bound compare,
not the old magic gate; this is static reachability, not hardware startup success.

## New artifacts

- /tmp/kilo-m31-h1-d5-o4iqet0b/final/runtime-smoke.bin
  44948 bytes; SHA256 46f1a91c8ebabf226accec5e574829b7e2fa5dc526688d53620613a5e69e37ad; CRC32/IEEE 621432C5.
- /tmp/kilo-m31-h1-d5-o4iqet0b/final/diagnostic/runtime-smoke-diag.bin
  45736 bytes; SHA256 d05a26e7c14fd204c179d6c39f1285012af28dc51f52ec936ead28d8449a88d5; CRC32/IEEE E1ECBF68.

Original normal/diagnostic artifacts remain unchanged and are not overwritten.
Original captures/checkpoints are historical and unchanged. No BIN copied to SD,
no hardware/COM6/physical SD/network/install/Git mutation. Separate authorization
is needed for ONE controlled diagnostic run; do not automatically retry or begin
M3.1-B. Entry/SDB raw dump and later markers must be observed on real MC68010
before declaring runtime hardware PASS. D07 remains closed.
