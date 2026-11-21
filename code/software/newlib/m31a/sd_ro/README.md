# M3.1-B2-I1 — isolated read-only firmware SD adapter

Host-validated implementation only; no target assembly/build/link or hardware run.
No B1, Newlib runtime, firmware, libsdfat, menu or Kilo implementation changed.
Only this subtree is new. D07 remains closed; M3.1-H1 closure is preserved.

## Architecture

One static instance owns the firmware-compatible 22-byte descriptor, aligned 16B
CSD scratch, aligned 512B bounce buffer and diagnostic state. No heap, filesystem,
FD table, errno, stdio or Newlib hooks. Initialization resets all resources before
support/init/CMD9/checked decode. Both firmware status AND initialized/type fields
must pass; false-OK firmware init is rejected. Public sector callback has exactly
B1's signature and normalized 1/0 return. No B1 object is linked in host tests;
only its unchanged header checks callback type compatibility.

## Supported subset

Firmware descriptor type V1/V2 uses CSD structure0 (SDSC); SDHC uses structure1.
READ_BL_LEN must be9 for both. SDSC larger native block profiles are intentionally
unsupported because the inspected firmware path always reads512 and no CMD16 is
introduced. This does not declare all other SDSC cards malformed.

Selected reserved fields documented by local CSD definitions must be zero in this
conservative profile; END bit must be1. No complete SD-spec compliance claim.
Card-reported capacity is not authenticated or independently physically measured.
Firmware consumes but ignores transfer CRC, and disables CRC during initialization.
Adapter neither validates CSD CRC7 nor data CRC16. A valid-looking corrupted CSD
cannot be ruled out. BPB is never used as physical capacity.

## Build and evidence

```
make -C code/software/newlib/m31a/sd_ro BUILD=/tmp/kilo-m31-b2-i1-example test sanitized audit
```

Host compiler only. src/sd_ro_traps.S is deliberately not a build input.
Actual evidence/build: /tmp/kilo-m31-b2-i1-it8fuan9/; essential logs copied here in
evidence/. API.md and ABI.md define contracts. TEST_REPORT.md records results.
PROVENANCE.json records local source/object identities. SHA256SUMS covers all
preserved files except itself; no temporary binaries are put in the repository.

## Limits and next step

Single-instance, single-threaded, synchronous, no ISR/reentrant use or hotplug.
The opaque context address remains stable across explicit reinitialization; it is
not a generation-tagged capability. Shutdown invalidates READY, not the address.
Firmware initialization necessarily changes SPI/card protocol state; read-only
means no storage-sector writes, not electrically inert execution.

Physical support for CMD9, active ROM ABI, actual stack use and target wrappers
remain UNKNOWN. Next proposed stage: separately authorized target-only compile/
ABI/map/read-only audit, still without hardware or Newlib FD integration.
Do not automatically integrate B1/B2 or run an experiment.
