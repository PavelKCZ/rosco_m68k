# Kilo68k — M3.1-B3-T2-H1-D3-H1-PASS

Permanent validation checkpoint. Documentation-only preservation, 2026-11-21.
Repository: /home/pavelk/rosco/rosco_m68k.

## 1. Executive Status

**H1-D3-H1 PASS — COMPLETE SD/FAT READ-ONLY SMOKE ON REAL MC68010**

The operator reports one authorized run of the corrected executable completing
the entire observed read-only smoke path and returning normally to firmware
warm boot. This is **operator-supplied physical hardware evidence**. Codex did
not independently execute or observe the board through a hardware connection.

Evidence classifications used here:

- VERIFIED LOCAL: source, existing logs, artifact hashes or repository state
  inspected during this documentation milestone. No tests were rerun.
- OPERATOR-SUPPLIED PHYSICAL EVIDENCE: the supplied hardware transcript and
  reported setup/workflow.
- HISTORICALLY VERIFIED: preserved prior test/build/audit evidence.
- DERIVED: arithmetic from the observed values.
- UNKNOWN / CONDITIONAL: not established by this smoke.

Architecture and milestone sequence:

- M3.1-A: standalone Newlib startup, relocation, memory, reentrant console/syscalls,
  explicit machine.o selection and exclusion of legacy syscall providers.
- M3.1-H1-D6: earlier successful runtime diagnostic on physical MC68010; it did
  not establish SD/FAT functionality.
- B1: isolated FAT16/FAT32 read-only backend, four handles and explicit EOF/media
  error behavior; historical112/112 normal and112/112 ASan/UBSan host baseline.
- B2: application-owned firmware SD descriptor, TRAP13 support/init/read/CMD9,
  CSD-derived uint64 capacity and private sector bounce buffer; historical91/91
  host baseline. T1/F1 wrapper alignment correction remains preserved.
- B3-I1: real B1+B2 integration through host firmware mocks,48/48 normal and
  ASan/UBSan; isolated MC68010 compile and relocatable link.
- B3-T2-V1: executable built, automatic audits passed, full audit CONDITIONAL.
- Original physical H1: B3 SD initialization failed at SDRO_PROFILE.
- H1-D2: diagnostic instrumentation identified actual CSD rejection1403.
- H1-D3-I1: minimal byte10 reserved-mask correction, host/target validation;
  full static audit remains CONDITIONAL.
- H1-D3-H1: corrected physical read-only smoke PASS as recorded below.

This checkpoint supersedes older descriptions of the current physical smoke
status. It does not rewrite historical failure reports or promote their scope.

## 2. Hardware and Firmware Context

OPERATOR-SUPPLIED / HISTORICAL context:

- rosco_m68k Classic; MC68010 CPU at10MHz.
- 5MiB RAM,5242880 bytes; runtime RAM top00500000.
- SDHC card.
- Windows Tera Term, COM6,38400baud,8N1, local echo OFF.
- Previously observed boot banner: Classic2.42.
- Previously observed SD Card Menu firmware banner: FW:2.43.
- According to the operator's authorization/reported workflow, the successful
  run used the same physical board and SD card as the H1-D2 investigation.

The version strings do not establish the exact active ROM binary identity.
Current physical connections/card contents were not independently inspected by
Codex in this documentation task. No serial, hardware or physical-media access
occurred.

## 3. Executable and Artifact Identity

VERIFIED LOCAL: these artifacts existed and were hashed read-only for this
checkpoint; no artifact was regenerated or modified.

Corrected target source directory:
`/tmp/kilo-h1-d3-i1-fqm4pko7/target/`

- BIN: m31-csd-v2fix-diag.bin,62044bytes.
  SHA-256 d0110563cf819b6023d3b5b31883c7eb0aa222640cbdfc39978e5af233f10c0f
  CRC-32 0xEBB863C5.
- ELF: m31-csd-v2fix-diag.elf,421740bytes.
  SHA-256 f139f488b76130c6c2555ba4560c36f2acb656e673ceed5b17704047a3ef08df.
- MAP: m31-csd-v2fix-diag.map,271747bytes.
  SHA-256 a663774c1a48f1b638b6d600c3cc0997b4ad85f1e4ad105b37dfcabd9803cf1f.

SD-card filename supplied by operator: `/CSDV2FIX.BIN`.
The menu transcript prints the equivalent lowercase spelling
`/csdv2fix.bin`.

The loader reported62044bytes andCRC0xEBB863C5, matching the local build.
This is physical loader size/CRC evidence for the intended image; it is not a
direct SHA-256 measurement of the physical SD-card copy, nor a cryptographic
attestation of execution identity.

Reference and diagnostic historical artifacts also remained present/unchanged:

- `/tmp/kilo-h1-d3-i1-fqm4pko7/target/m31-csd-v2fix-diag.elf`
  - 421740 bytes; SHA-256 `f139f488b76130c6c2555ba4560c36f2acb656e673ceed5b17704047a3ef08df`; CRC-32 `0x39935382`.
- `/tmp/kilo-h1-d3-i1-fqm4pko7/target/m31-csd-v2fix-diag.bin`
  - 62044 bytes; SHA-256 `d0110563cf819b6023d3b5b31883c7eb0aa222640cbdfc39978e5af233f10c0f`; CRC-32 `0xEBB863C5`.
- `/tmp/kilo-h1-d3-i1-fqm4pko7/target/m31-csd-v2fix-diag.map`
  - 271747 bytes; SHA-256 `a663774c1a48f1b638b6d600c3cc0997b4ad85f1e4ad105b37dfcabd9803cf1f`; CRC-32 `0x3E860153`.
- `/tmp/kilo-h1-d2-i1-4kwrjxj_/target/m31-csd-failure-diag.elf`
  - 421740 bytes; SHA-256 `4b5d4eefae0bc62cfcc5d2f7520079c9a699fea63517065474a227ee61395838`; CRC-32 `0x03158970`.
- `/tmp/kilo-h1-d2-i1-4kwrjxj_/target/m31-csd-failure-diag.bin`
  - 62048 bytes; SHA-256 `dcabab90fb2c9bb6c91d44e46045811fba0ffe81b3f257716faece99d1d11d8a`; CRC-32 `0x28E9A5C0`.
- `/tmp/kilo-h1-d2-i1-4kwrjxj_/target/m31-csd-failure-diag.map`
  - 271827 bytes; SHA-256 `63104b841b6bfef18930933888a81acce8832a1015c7b32fd2083608bf0ecea8`; CRC-32 `0xF1384A3E`.
- `/tmp/kilo-m31-b3-t2-v1-3y_kc51x/target/m31-sdfat-smoke-diag.elf`
  - 408392 bytes; SHA-256 `63fa8ecfb0f093010bb97ac66e1d641b0ee08ad093dfacd70d164191e9b65f69`; CRC-32 `0x6C39555C`.
- `/tmp/kilo-m31-b3-t2-v1-3y_kc51x/target/m31-sdfat-smoke-diag.bin`
  - 60876 bytes; SHA-256 `4c392208a6ff2d9c0ccedcf3e505b6855be03a55ef3221be8b7c271cbbf1b368`; CRC-32 `0xEB53AC5E`.
- `/tmp/kilo-m31-b3-t2-v1-3y_kc51x/target/m31-sdfat-smoke-diag.map`
  - 272020 bytes; SHA-256 `dadfa0c1e1b2dfb9540eee9ba842be3afa144b9d51982151a07a56f917b67484`; CRC-32 `0x65F80143`.

These paths are temporary storage, not permanent archival guarantees.

## 4. Original Failure

The original physical H1 failed during B3 initialization:

```text
APP_FAIL phase=INIT status=4 bytes=0 reason=B3 initialization index=0
BACKEND first=1 status=4 phase=1 fat=0 sd=8 fw=00000000 delivered=0000000000000000
```

Decoded from actual source during the forensic investigation:

- B3status4: B3_SD_ERROR.
- phase1: B3_SD_INIT.
- SDstatus8: SDRO_PROFILE.
- FAT initialization was not reached.

The original failure executable remains m31-sdfat-smoke-diag.bin,60876bytes,
SHA-2564c392208a6ff2d9c0ccedcf3e505b6855be03a55ef3221be8b7c271cbbf1b368,
CRC0xEB53AC5E. Its identity is distinct from both later diagnostic images.

Historical failure preservation:
`docs/KILO_M31_B3_T2_H1_FAILURE_RESUME_CHECKPOINT.md`,
288358bytes,
SHA-256975f7aa1468b6658cc265e1d303b3d78eb6aca3e599d3505bac3d3b92ab8a9f4.

## 5. H1-D2 Diagnostic Evidence

OPERATOR-SUPPLIED PHYSICAL EVIDENCE from the preceding diagnostic run:

```text
CSDD1 VALID=000003FF ST=05 STATUS=0008
CSDD1 SUP=1234FEDC OK=1 INIT=00000000 DI=01 TYPE=00000002 CMD9=00000001
CSDD1 RAW=400E0032DB790000EDC87F800A4040E5 STRUCT=01
CSDD1 REJ=1403 MASK=00,00,70,--
```

Support signature was accepted, firmware initialization returned0, descriptor
initialized1/type2, CMD9 wrapper returned1, and all16 CSD bytes were captured.
Structure1 means CSDv2. The snapshot identifies the first rejecting predicate,
not merely a status inferred after cleanup.

raw[10]=0x7F; original raw[10]&0xF0=0x70; rejection1403; finalSDRO_PROFILE.
The old mask incorrectly classified defined erase-related fields as reserved.
CSD capture and accepted wrapper returns do not prove independent CSD CRC checking.

Diagnostic source/report:
`code/software/newlib/m31a/b3_csd_diagnostic/REPORT.md`,
CHANGES.json,PROVENANCE.json,tests/ and tools/.
Historical D2 output root: /tmp/kilo-h1-d2-i1-4kwrjxj_/.

## 6. Minimal CSD v2 Correction

Current source:
`code/software/newlib/m31a/sd_ro/src/csd_decode.c`, v2 branch aroundlines71–74.

```diff
- (raw[10] & 0xF0)
+ (raw[10] & 0x80)
```

The implementation changed BOTH the validation predicate and diagnostic MASKS
capture, with a concise explanatory comment. ID1403 remains for the genuinely
reserved bit7. Bits6..0 are defined erase fields:
bit6 ERASE_BLK_EN; bits5..0 upper SECTOR_SIZE, continued in raw[11]bit7.

Local layout sources:
- code/software/libs/src/sdfat/include/sdfat.h:135–141.
- code/firmware/rosco_m68k_firmware/stage1/blockdev/include/bbsd.h:180–186.

This is a narrow local-layout correction, not a complete SD specification
compliance audit. CSDv1, other v2 checks, validation order, capacity calculations,
addressing bounds, B2/B3 APIs, TRAP wrappers, runtime/linker, snapshot lifetime
and four-line diagnostic format remain unchanged.

Exactly four H1-D3-I1 source/test files changed; current hashes were independently
rechecked before this documentation write:

- `code/software/newlib/m31a/b3_csd_diagnostic/tests/test_application.c`
  - Before H1-D3-I1: `3ecdddceafdb6842c963e210e924112aa5b758759faddea3f0252b5d4ae9554e`
  - Current corrected source: `5d3c03e8d35ea901c494486e87a2ca1bd89ca5d63b6bb14d610deffbc331dfb6`
- `code/software/newlib/m31a/b3_csd_diagnostic/tests/test_diagnostics.c`
  - Before H1-D3-I1: `81c9154cacc3c19341ad1ebca55bbeb123512bdb94cd120b86aa9c3039117f35`
  - Current corrected source: `3f88ec66165db8ac063a17029235782b16fb375955620ee1f1a52bd9512c47e8`
- `code/software/newlib/m31a/sd_ro/src/csd_decode.c`
  - Before H1-D3-I1: `efc8ff0f02896d082df1080b7f313eda03106510b7dc258e1a9cdef234d49b46`
  - Current corrected source: `0d0afc5495a18853cc55f53681a470eb7c78a55e4da58c2ae5f2f68388326ddd`
- `code/software/newlib/m31a/sd_ro/tests/test_csd.c`
  - Before H1-D3-I1: `a9ff822b512630bdd162ceb523f16abef51ddb79d548baa58e0a4a17a95de206`
  - Current corrected source: `43580417b035394ad647b0fada0050ce6e6e34c3d5bce3b32c01383745a97139`

## 7. H1-D3-I1 Host and Target Validation

HISTORICALLY VERIFIED, cross-checked against existing report and saved summaries;
not rerun during checkpoint creation.

| Suite | Diagnostic OFF | Diagnostic ON |
|---|---:|---:|
| Diagnostic harness |32/32|73/73|
| B2 |91/91|91/91|
| B3 integration |48/48|48/48|
| Application |25/25|28/28|

The same matrix passed under ASan/UBSan. Zero failures, skips, XFAIL, forbidden
writes and compiler warnings were reported. Repeated configurations and shared
scenarios are not separate unique new tests. The ON application28 cases include
the inherited25, not53 total. **LSan NOT VERIFIED**; detect_leaks=0 was explicit.

Exact physical CSD400E0032DB790000EDC87F800A4040E5:
SDRO_OK,62333952 sectors,structure1,no rejection ID.
Same CSD with byte10bit7 set:
SDRO_PROFILE,1403,mask80.
Defined erase bits, genuine reserved-bit rejection, first-error precedence,
V1 behavior, snapshot copy/reset/cleanup and formatter bounds were covered.

Evidence root: `/tmp/kilo-h1-d3-i1-fqm4pko7/`:

- REPORT.md; CHANGES.json; baseline.json; preservation.json; decoder.diff.
- host/summary.json,host/commands.json and per-suite logs.
- profile/PROVENANCE.json,profile/SHA256SUMS and isolated build tools.
- target/build-record.json, .su files, disassembly, ELF/BIN/MAP.
- machine-audit.json,full-audit.json,negative-audit.json.
- ARTIFACTS.json,SHA256SUMS,target/manual-review.json.

Historical host command:
`PYTHONDONTWRITEBYTECODE=1 python3 code/software/newlib/m31a/b3_csd_diagnostic/tools/validate_host.py /tmp/kilo-h1-d3-i1-fqm4pko7/host`.
Do not execute it without new authorization.

Target: GCC13.3.1/binutils2.42, MC68010, M31_DIAGNOSTIC and
SDRO_CSD_DIAGNOSTIC.12objects;45libc+17libgcc archive members; no unresolved
symbols. Explicit machine.o only, unchanged runtime and link.ld.
Automatic executable/provenance/read-only audit PASS; negative audits14/14PASS.
**Full audit remained CONDITIONAL**; this checkpoint does not upgrade it.

Measured corrected layout: entry/run0x2000,load0x40000,delta0x3E000;
BIN62044B,BSS85172B,_end0x25F10;32KiB reserved stack.
Individual .su frames main272B,sdro_init72B,decoder core24B are not a complete
firmware/IRQ stack bound.

Historical source manifests were not rewritten for the correction. Old D2
decoder/test pins and the earlier known B2 wrapper-manifest mismatch must not
be reported as a fully current passing manifest. The external D3 source/object
provenance identifies this actual build.

## 8. H1-D3-H1 Physical Transcript

**Operator-supplied physical hardware evidence.**
Preserved verbatim below, including capitalization, ordering and blank lines.
Codex did not independently execute the hardware test.

```text
A-V to run, SPACE to reload, RETURN for prompt:P
Loading "/csdv2fix.bin"...............
Loaded 62044 bytes in 2.89 sec.; CRC-32=0xEBB863C5; Starting...

E ABI_SLOT=00001DF8 RAM=00500000 RESET=00500000 SP=0000DF04 SR=00002004
V S R Z C P B T A I K M O W APP_BEGIN top=00500000 stack=004f8000 heap=00025f10..004f8000
F INIT_BEGIN
INIT_OK sectors=0000000003b72400
OPEN_OK
READ_OK bytes=513
EOF_OK
SEEK_OK
CLOSE_OK
SHUTDOWN_OK
APP_PASS
APP_ATEXIT
APP_DTOR


**********************
rosco_m68k - Warm boot
**********************
```

The operator reports one authorized corrected run and normal firmware warm boot.

## 9. Hardware Acceptance Matrix

Results below refer specifically to the supplied physical transcript.

| Stage | Evidence | Result |
|---|---|---|
| Image load |62044B,CRC0xEBB863C5|PASS|
| Startup |APP_BEGIN|PASS|
| SD/B3 initialization |INIT_OK|PASS|
| FAT file open |OPEN_OK|PASS|
| File read |READ_OK bytes=513|PASS|
| EOF |EOF_OK|PASS|
| Seek |SEEK_OK|PASS|
| Close |CLOSE_OK|PASS|
| Shutdown |SHUTDOWN_OK|PASS|
| Application result |APP_PASS|PASS|
| atexit |APP_ATEXIT|PASS|
| Destructor |APP_DTOR|PASS|
| Firmware return |Warm boot banner|PASS|

No APP_FAIL, BACKEND error or CSD failure dump appears in the supplied success
transcript. Current smoke.c prints READ_OK only after validating all513 bytes;
SEEK_OK follows SET511,readingFF00,END positioning513 and EOF withzero delivered
bytes. There is no separate physical SHA computation in these markers.

## 10. CSD Capacity Verification

Observed:
`INIT_OK sectors=0000000003b72400`.

DERIVED by independent integer arithmetic during this documentation task:

- 0x03B72400 =62333952 sectors.
- Sector size512bytes.
- 62333952*512 =31914983424bytes.
- 31914983424/2^30 =29.72314453125GiB, approximately29.72GiB.

Previously captured CSD gives:
C_SIZE=((raw[7]&0x3F)<<16)|(raw[8]<<8)|raw[9]
=0x00EDC8=60872.
CSDv2 capacity=(60872+1)*1024=62333952sectors.
The reported hardware capacity therefore matches the earlier CSD-derived value.

This is CSD-derived capacity propagated through B2/B3 and printed by the
application. It is not an independent full physical-media capacity measurement.

## 11. Read-Only Fixture and Scope

Fixture: `/M31RO.BIN`,513bytes,byte[i]=i&0xFF.
Historical expected SHA-256:
`2ac6d9e2b50bdd65511fb4dc0f24b7f6a0c1b2e92d1cf6901df565802e380234`.

The transcript and application checks establish successful513-byte read,
byte-pattern validation,EOF and seek boundary behavior for this observed run.
The supplied transcript does not expose an independent physical fixture SHA-256.

The tested path is application -> B3 lifecycle -> B1 FAT -> B2 synchronous sector
callback -> firmware. Capacity remains uint64_t; B1 shuts down before B2.
No Newlib FD/stdio FAT integration or Kilo68k file I/O was introduced.
TRAP13 selectors0/1/2/4 only; selector4 fixedCMD9; no selector3/storage write.
Console _write_r is legitimate and distinct from SD writes.

This establishes the tested read-only path on the observed card/firmware setup,
not every card, FAT variant, partition arrangement or media failure mode.
Source-level read-only behavior is not physical write protection.

## 12. Open Risks and Conditional Audits

Still UNKNOWN or CONDITIONAL:

- Exact active ROM binary identity; banners alone are insufficient.
- Complete firmware/IRQ/libc worst-case stack bound.
- Full manual MC68010 instruction review.
- Independent physical CSD CRC verification.
- Broader SD-card/FAT compatibility and full SD-spec conformance.
- Extended hardware stress/fault-injection coverage.
- LSan verification.
- Direct physical SHA-256 of executable copy and fixture absent from transcript.

Successful smoke is valuable physical evidence but does not resolve these
general properties. H1-D3-I1 full static audit remains CONDITIONAL.
Do not rewrite historical reports to imply otherwise.

## 13. Repository Preservation

Documentation milestone baseline:
branch feature/kilo-editor;
HEAD9881e9e0f53ba5888887eaa91ddc982521ea8e89;
stagingempty; git diff --check exit0;2428pre-existing regular repository files
(including untracked,excluding.git) SHA-256 inventoried in memory.

Initial complete Git porcelain:

```text
 M code/software/kilo68k/Makefile
 M code/software/kilo68k/editor.c
 M code/software/kilo68k/main.c
 M code/software/kilo68k/terminal.c
 M code/software/kilo68k/terminal.h
 M code/software/kilo68k/tests/test.c
 M docs/KILO_DEVELOPMENT_CHECKPOINT.md
?? code/software/kilo68k/d07_diag.c
?? code/software/kilo68k/d07_diag.h
?? code/software/kilo68k/f2_d08_sender.py
?? code/software/kilo68k/f2_d09_2ms_sender.py
?? code/software/kilo68k/f3_join_stress_sender.py
?? code/software/kilo68k/tests/test_f3_join_stress_sender.py
?? code/software/newlib/m31a/
?? docs/D07_E2_H1_RESUME_CHECKPOINT.md
?? docs/KILO_M31_B3_I1_FINAL_CHECKPOINT.md
?? docs/KILO_M31_B3_T2_H1_FAILURE_RESUME_CHECKPOINT.md
?? docs/KILO_M31_H1_D4_RESUME_CHECKPOINT.md
?? docs/KILO_M31_H1_D6_HARDWARE_PASS_CHECKPOINT.md
?? docs/KILO_M3_SD_FAT_ARCHITECTURE_CHECKPOINT.md
```

The only authorized addition is this checkpoint. Every pre-existing file,
including the four corrected H1-D3-I1 files above, must remain byte-identical.
Existing seven tracked dirty changes are intentionally retained.
Original H1/D2/D3 artifacts remain immutable; no regenerated/replaced image.

Final preservation results and final porcelain are recorded below after the
write. The checkpoint's own final byte size and SHA-256 are reported in the
Codex response; no self-referential file hash is embedded.

Final verification: all2428 pre-existing file SHA-256 values unchanged;
exactlyone addition (this checkpoint),0 modifications/deletions of existing files.
Allfour H1-D3-I1 corrected identities and allnine local H1/D2/D3 artifact hashes,
sizes and CRCs remain unchanged. Branch/HEAD unchanged, stagingempty,
git diff --check exit0. Transcript section8 matches the supplied text exactly.
Capacity arithmetic independently confirmed62333952sectors/31914983424bytes.

Final complete Git porcelain:

```text
 M code/software/kilo68k/Makefile
 M code/software/kilo68k/editor.c
 M code/software/kilo68k/main.c
 M code/software/kilo68k/terminal.c
 M code/software/kilo68k/terminal.h
 M code/software/kilo68k/tests/test.c
 M docs/KILO_DEVELOPMENT_CHECKPOINT.md
?? code/software/kilo68k/d07_diag.c
?? code/software/kilo68k/d07_diag.h
?? code/software/kilo68k/f2_d08_sender.py
?? code/software/kilo68k/f2_d09_2ms_sender.py
?? code/software/kilo68k/f3_join_stress_sender.py
?? code/software/kilo68k/tests/test_f3_join_stress_sender.py
?? code/software/newlib/m31a/
?? docs/D07_E2_H1_RESUME_CHECKPOINT.md
?? docs/KILO_M31_B3_I1_FINAL_CHECKPOINT.md
?? docs/KILO_M31_B3_T2_H1_FAILURE_RESUME_CHECKPOINT.md
?? docs/KILO_M31_H1_D3_HARDWARE_PASS_CHECKPOINT.md
?? docs/KILO_M31_H1_D4_RESUME_CHECKPOINT.md
?? docs/KILO_M31_H1_D6_HARDWARE_PASS_CHECKPOINT.md
?? docs/KILO_M3_SD_FAT_ARCHITECTURE_CHECKPOINT.md
```

No build,link,test,audit execution,hardware,COM6,terminal,physical SD or network
access occurred during this documentation-only task. No Git mutation occurred.

## 14. Resume Instructions

Read AGENTS.md and this checkpoint first, then the referenced historical context:

- docs/KILO_DEVELOPMENT_CHECKPOINT.md.
- docs/KILO_M3_SD_FAT_ARCHITECTURE_CHECKPOINT.md.
- docs/KILO_M31_H1_D6_HARDWARE_PASS_CHECKPOINT.md.
- docs/KILO_M31_B3_I1_FINAL_CHECKPOINT.md.
- docs/KILO_M31_B3_T2_H1_FAILURE_RESUME_CHECKPOINT.md.
- code/software/newlib/m31a/b3_executable/V1_REPORT.md.
- code/software/newlib/m31a/b3_csd_diagnostic/REPORT.md.

Older next-step lists are historical, not authorization.
Locate corrected decoder/tests using the four exact paths/hashes in section6.
Inspect the actual current sources before further work. Keep existing dirty work
and do not stage,reset,clean,checkout or silently repair historical manifests.

Locate corrected evidence and new BIN under
/tmp/kilo-h1-d3-i1-fqm4pko7/, specifically
target/m31-csd-v2fix-diag.bin.
**/tmp may not survive restart or cleanup. Recording an identity here is not
permanent archival of the executable or logs.** If missing, report UNKNOWN;
do not substitute another image or silently regenerate it.
A later durable artifact archive would require separate authorization.

Fresh-session prompt:
“Restore Kilo68k from docs/KILO_M31_H1_D3_HARDWARE_PASS_CHECKPOINT.md.
Last physical milestone H1-D3-H1 PASS; H1-D3-I1 full static audit CONDITIONAL.
Verify feature/kilo-editor/HEAD9881e9e0f53ba5888887eaa91ddc982521ea8e89,
empty staging and preserve the dirty tree. Read-only context restoration only;
no build,tests,hardware,source edits or Git mutations unless separately asked.”

Future builds, commits, hardware tests and feature expansion require separate
explicit authorization. No subsequent development milestone is authorized by
saving this checkpoint.

## 15. Final Verdict

H1-D3-H1 PASS — COMPLETE SD/FAT READ-ONLY SMOKE ON REAL MC68010

