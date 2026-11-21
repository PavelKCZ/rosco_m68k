# M3.1-B3-I1 — isolated read-only integration

VERIFIED: actual B1 FAT and B2 SD adapter implementations are connected through
`b1_init(sdro_read, sdro_context(), capacity)` without callback casts. Only the
four private firmware functions are replaced by host mocks. No Newlib FD,
stdio-file integration, runtime modification, executable link or hardware test
is included. D07 remains closed.

## Reproduce

From this directory, select a NEW external directory:

```
make BUILD=/tmp/your-new-b3-build test sanitized target audit
```

Requires installed host C11 compiler, Python 3 and m68k-elf-rosco GCC/binutils.
No downloads. The Makefile writes build outputs to BUILD; keep BUILD outside
the repository. `target` compiles five production units and uses only `ld -r`.
`audit` also compiles independent host production objects into BUILD.
See TEST_REPORT.md and evidence/ for the recorded run and exact expanded flags.

## Ownership and ordering

B3 exclusively owns both single-instance modules during a session. Runtime
initialization is outside this harness. B3 calls B2 init, queries its uint64_t
CSD-derived sector count, then initializes B1 with the B2 callback/context.
READY requires both successes. It tracks existing B1 tokens for cleanup (four
slots, no second FD namespace). Shutdown closes tracked tokens, shuts down B1,
then shuts down B2. It never resets B2 while B1 is active. No automatic retries.
Direct external B1/B2 lifecycle calls while B3 is active violate ownership.

Failure before/inside B2 initialization cleans B2; FAT-probe failure cleans B1
then B2. FAILED requires explicit shutdown before another init. Repeated init
in READY returns STATE without any firmware call. File-operation failures keep
READY so successful explicit seek can clear B1's sticky read error. Diagnostics
preserve the first backend error across cleanup; the next permitted init clears
it. Shutdown itself never issues firmware calls or writes.

## Fixtures and boundary

The test generator creates FAT16/FAT32 superfloppy and MBR media, fragmented
`/M31RO.BIN`, 513 bytes, byte i = i & 255. CSD matches the bounded sparse medium:
stored prefix plus zero-filled sectors through validated capacity. Huge media
represent 2^32 sectors without allocating 2 TiB. A high-partition test maps a
valid FAT16 volume at the end of that medium and reads file data at UINT32_MAX
through actual B1 and B2, detecting any capacity narrowing. See fixtures.json.
The evidence helper emits prefixes to stdout; redirect only to an external
regular file, never a device. It is a test fixture tool, not a formatter.

## Read-only basis

B1 accepts only a read callback and contains no mutation implementation. B2
accepts no write callback and exposes no arbitrary CMD. The target wrapper
machine bytes are pinned to selectors 0,1,2,4 with fixed CMD9. The link map must
contain exactly the five approved objects; per-object public symbols and
undefined imports are checked. Mock call traces accept only those selectors;
forbidden/write counters remain zero in all 48 cases. No physical hardware
write protection or firmware CRC integrity is claimed.

## Known limits

No target execution or final executable addresses have been verified. The
firmware ignores received data/CSD CRC. Per-function stack reports exclude
firmware, interrupts and complete nested call-chain overhead. B1's original
limits (four handles, 4096-cluster chains, INT32_MAX offsets, ASCII 8.3 paths)
remain. LSan is NOT VERIFIED due to ptrace environment limitations.

B2's historical SHA256SUMS remains unchanged with its one known stale assembly
entry. B3 uses the corrected F1 wrapper; PROVENANCE.json records both identities.
No original component was corrected or substituted by this integration.
