# H1-D2-I1 — CSD failure diagnostic implementation

2026-11-21. **CONDITIONAL — OPEN VALIDATION ITEMS — HARDWARE NOT AUTHORIZED.**
VERIFIED by new host execution and target static inspection, not target execution.
One previous H1 physical failure remains the only hardware attempt. Its root cause
is still UNKNOWN; no raw physical CSD was acquired here. No validator fix.

## Preservation / authority

feature/kilo-editor, HEAD9881e9e0f53ba5888887eaa91ddc982521ea8e89; staging empty.
D0 checkpoint288358B SHA256975f7aa1468b6658cc265e1d303b3d78eb6aca3e599d3505bac3d3b92ab8a9f4
verified. Initial inventory2412 regular files excluding.git is baseline.json under
/tmp/kilo-h1-d2-i1-4kwrjxj_/. CHANGES.json records exact five authorized modified
files and before/after hashes. All other2407 pre-existing files must be unchanged;
final preservation result is external preservation-final.json. No firmware,
B1, runtime/startup/memory/syscalls/link.ld, TRAP wrapper or historical checkpoint,
manifest, test or report changed. Original seven tracked modifications preserved.
New files are this subtree plus three headers listed below. No Git mutation,
network/install, hardware, serial or physical media access. Outputs only in /tmp.

## Source inventory and architecture

Modified (opt-in SDRO_CSD_DIAGNOSTIC only for new API/capture/output):
- sd_ro/src/sd_ro.c: independent inert snapshot, once-only raw wrapper captures,
  descriptor fields, successful16-byte CSD capture, final status, copy-out getter.
- sd_ro/src/csd_decode.c: ONE shared validation core with optional trace sink;
  original public decoder and capacity API preserved. Trace IDs assigned inside
  rejecting branches. Normal calls use no trace, never mutate the init snapshot.
- sd_ro/src/private.h: diagnostic-only traced decoder declaration.
- b3_integration/src/b3_integration.c: diagnostic-only copy-out forwarding getter.
- b3_executable/src/smoke.c: prints evidence after BACKEND/before cleanup without
  replacing the first status or making B1/B2 lifecycle calls.

New shared headers:
- sd_ro/include/sd_ro_csd_diag.h: schema/validity bits,48B layout asserts/getter.
- b3_integration/include/b3_csd_diag.h: forwarding getter, no b3_diagnostic change.
- b3_executable/src/csd_format.h: bounded four-line field formatter, no heap.

New profile inventory: CHANGES.json,PROVENANCE.json,README.md,REPORT.md,SHA256SUMS;
tests/test_diagnostics.c,test_application.c; tools/profile.py,build.py,audit.py,
negative_audit.py,negative_cases.json,validate_host.py.
The build/audit/negative tools derive from the prior b3_executable tools. All old
runtime/layout/provider/archive/wrapper/negative gates retained; only additive
new diagnostic provider owners and48B record/output-marker checks. Existing
archive-member allowlist is unchanged. Legacy decoder is explicitly retained
with --undefined=sdro_decode_csd so no ownership check is weakened under GC.

## Schema and lifetime

Five uint32 fields: valid,support_raw,init_raw,type_raw,cmd9_raw.
Two uint16: final_status,reject_id. Four uint8: schema,stage,initialized_raw,structure.
raw_csd[16] and masked[4]. Size48 confirmed host and MC68010; all offsets asserted:
0/4/8/12/16,20/22,24/25/26/27,28,44. Host alignment4, target alignment2.
No padding bytes are exposed; never serialize the C struct. Copy getters return
1 available,0 none with zero output,-1 null. No firmware/error-state side effects.
Ordinary objects export neither diagnostic getter nor traced decoder.

valid bits0..9: attempt,support,init,descriptor,CMD9,CSD,structure,reject,final,mask.
Captured does NOT mean success. Stage0 none,1 begun,2 support returned,3 init
returned,4 CMD9 returned,5 decoder returned. Failed/partial CMD9 publishes no CSD.
Snapshot is separate from adapter; shutdown still clears all operational state
but preserves inert evidence. Next init zeros the whole record before capture.
B3 cleanup cannot erase it. No mutable pointer, allocation, retry or extra TRAP.
Single instance/thread; no concurrent getter/initialization contract.

## Exact rejection identifiers

1101:B0&3F;1102:B12&60;1103:B13&1F;1104:B14&03.
1201:READ_BL_LEN !=9;1301:v1 B6&0C.
1401:v2 B6&0F;1402:v2 B7&C0;1403:v2 B10&F0.
All IDs are hex. Composite first-nonzero precedence matches source order.
Original complete predicates and order retained, including raw[10]&0xf0.
Additional IDs:0101 support,0102 init return,0103 initialized,0104 type,
0105 CMD9;0201 decoder args,0202 type,0203 structure,0204 end bit,0205 mismatch;
0301 v1 shift guard,0302 v1 multiply guard group,0303 byte divisibility/zero,
0304 v2 multiplication;0401 zero sectors,0402 capacity>2^32,0403 SDSC capacity.
Reject0000 is printed only for a completed success; unavailable uses hyphens.
Some defensive arithmetic paths cannot be reached by already-bounded CSD fields;
they are statically instrumented, not claimed dynamically covered. Capacity-core
failure branches are directly unit-tested through the single included core.

## Serial format and resource accounting

Four lines, uppercase fixed-width hex/no0x; unavailable width-matched hyphens,
unused mask slots--. Raw16 bytes in received order, no descriptor reserved dump:
CSDD1 VALID=vvvvvvvv ST=ss STATUS=ssss
CSDD1 SUP=rrrrrrrr OK=o INIT=iiiiiiii DI=dd TYPE=tttttttt CMD9=cccccccc
CSDD1 RAW=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx STRUCT=vv
CSDD1 REJ=rrrr MASK=aa,bb,cc,dd

Exact196 characters with LF,200 with CRLF, plus NUL in static197B text buffer.
Tests check boundaries, unavailable fields, exact rejection output and no
truncation. New static payload48B B2+48B app+197B text=293B. Measured executable
BSS delta292B due to alignment/layout. No new large automatic buffers. Target
main remains272B .su; line8B; csdd_hex20B; sdro_init72B (previous56), getter16B,
decode core24B, old API wrapper16B, traced wrapper0B, B3 getter0B.
These are individual frames, NOT a total firmware/IRQ bound. Stack reserve32768
unchanged. Console formatter/stdio may use existing Newlib heap paths; backend
and diagnostic storage do not allocate. No output from CSD/firmware/TRAP layers.

## Executed host validation

Reproducible command:
PYTHONDONTWRITEBYTECODE=1 python3 tools/validate_host.py /tmp/NEW-HOST-DIRECTORY
Run from this profile; explicit commands/env/return codes stored in host-03/commands.json.
Host flags C11,-Wall,-Wextra,-Werror,-Wpedantic; normalO2; sanitizedO1 with
ASan/UBSan,no recover,frame pointers; detect_leaks=0,halt/abort on error.

Final host-03 results (no modes counted twice as unique):
- diagnostic harness60 unique PASS; same60 ASan/UBSan PASS. Includes19 shared
  ordinary-mode cases also PASS, with exact status/firmware-sequence comparison.
- original B2 suite91/91 PASS diagnostics off AND on, normal AND ASan/UBSan.
- original B3 integration48/48 PASS off AND on, normal AND ASan/UBSan.
- application25 original cases PASS off; on retains all25 original expectations
  plus3 diagnostic-specific cases =28/28 PASS; same under ASan/UBSan.
Final0fail,0skip,0XFAIL,0forbidden writes,0compiler warnings. LSan NOT VERIFIED;
no new leak-sanitizer claim. These are host mocks, not real card/CPU execution.

Coverage: all9 profile subconditions/all4 sites; common/block/version-specific
precedence; early structure/end/mismatch precedence; v1/v2 success; current
ERASE_BLK_EN/SECTOR_SIZE false-profile characterization; capacity guards;
support/init/initialized/type/CMD9 failures and raw values; partial CSD discard;
copy isolation; B3 cleanup/repeated shutdown survival; new attempt no stale data;
getter null/zero output; output length/content/order; unavailable snapshot;
diagnostic console failure preserves primary APP_FAIL and cleanup.
New decoder tests include the actual single decoder C source into the harness
TU to reach static capacity guards, not a second replacement validator.

Preserved initial failures: initial-failures.txt and host-02/app log. First new
harness had misleading-indentation compile error, fixed with braces (no weakened
warnings). A new ordering assertion incorrectly included the original shutdown-
only failure detected after the diagnostic block; narrowed to pre-cleanup errors.
Original shutdown test unchanged. Final full suite rerun in host-03.

## Target build and audits

Commands (external only, all source recompiled, no historical object reuse):
python3 tools/build.py --build /tmp/kilo-h1-d2-i1-4kwrjxj_/target compile
then same driver link,binary,inspect.
GCC13.3.1 / binutils2.42, established68010 CPU/architecture/tune flags, freestanding
Newlib explicit libc/libgcc group, extracted machine.o only, original link.ld.
Added only SDRO_CSD_DIAGNOSTIC and legacy decoder retention. Twelve objects,
no warnings/unresolved symbols. Exact commands, tools, dependencies and hashes:
target/build-record.json, objects.json.62 archive members (45libc,17libgcc),
no additions relative to V1 allowlist; hashes in machine-audit-01.json.

Machine audit PASS. Full audit CONDITIONAL/exit3: complete instruction-level
review CONDITIONAL, full stack/IRQ UNKNOWN, active-ROM assumptions CONDITIONAL;
memory layout PASS. No fabricated manual PASS. Current manual record is tied
exactly to this ELF/BIN. Original V1 manual record untouched.
All14 inherited negative cases rejected for their intended reasons:
- missing_expected_provider: OWNER:main
- duplicate_syscall_input: OBJECT_INVENTORY
- legacy_fat_input: LINK_INPUTS
- sd_write_selector: WRAPPER_BODY:sdro_fw_read
- odd_wrapper_entry: WRAPPER_ODD:sdro_fw_read
- wrong_load_delta: LOAD_DELTA
- bad_image_size: IMAGE_BOUNDARY
- rwx_segment: LOAD_PERMISSIONS
- executable_stack: GNU_STACK
- bin_disagreement: BIN_BYTES:.boot
- missing_object_hash: OBJECT_HASH:application.o
- mismatched_object_hash: OBJECT_HASH:application.o
- unresolved_symbol: UNRESOLVED_SYMBOLS
- missing_manual_gate: MANUAL_REQUIRED

Read-only target audit checks approved actual objects/archive closure, no unresolved
symbols, exact four immutable TRAP13 wrappers/selectors0/1/2/4 and CMD9, no selector3,
no legacy FAT/SD/syscall/startup provider. Additional host-off nm audit confirms
no diagnostic exports; B1/B2/B3 target imports no allocator/stdio/write symbols.
Original _write_r console owner remains allowed. Source capability proof is not
physical write protection. CSD CRC still ignored by firmware.

Final load0x40000/run0x2000,delta0x3e000. Boot776/postboot102; _boot_size0x370;
.text58856 (includesrodata),ctors4/dtors4,data2304,BSS85176.
_image_size0xf260(62048),_tail_size0xeef0,_data_end/BSSstart0x11260,
_end/BSSend0x25f18 aligned8. Allocated image+BSS147224B. Original145760B;
delta1464B. Initialized image/BIN delta1172B; BSS delta292B.
RX LOAD VMA2000/LMA40000 filesz0xe960;RW VMA10960/LMA4e960 filesz900,
memsz155b8; GNU_STACK RW/nonexec, noRWX. No relocation overlap.
Conditional5MiB heap[0x25f18,0x4f8000)5054696B; stack[4f8000,500000).
Not a new board measurement. Current raw snapshot48B object alignment>=2,
allfield offsets confirmed by separate MC68010 layout-probe.o (not linked).
The unchanged original startup still validates actual RAM/reset metadata.

## Artifact identities

- ELF: /tmp/kilo-h1-d2-i1-4kwrjxj_/target/m31-csd-failure-diag.elf
  421740B SHA256 4b5d4eefae0bc62cfcc5d2f7520079c9a699fea63517065474a227ee61395838 CRC32 03158970

- BIN: /tmp/kilo-h1-d2-i1-4kwrjxj_/target/m31-csd-failure-diag.bin
  62048B SHA256 dcabab90fb2c9bb6c91d44e46045811fba0ffe81b3f257716faece99d1d11d8a CRC32 28E9A5C0

- MAP: /tmp/kilo-h1-d2-i1-4kwrjxj_/target/m31-csd-failure-diag.map
  271827B SHA256 63104b841b6bfef18930933888a81acce8832a1015c7b32fd2083608bf0ecea8 CRC32 F1384A3E

Original reference remains immutable at
/tmp/kilo-m31-b3-t2-v1-3y_kc51x/target/m31-sdfat-smoke-diag.bin,
60876B SHA2564c392208a6ff2d9c0ccedcf3e505b6855be03a55ef3221be8b7c271cbbf1b368
CRC EB53AC5E. Original ELF/map and D0 checkpoint are rechecked in preservation.
Historical source manifests (including pre-existing stale B2 wrapper entry) are
not rewritten. New diagnostic source/object pins are authoritative for this build.

## Remaining gates / stop

UNKNOWN until separately authorized physical diagnostics: actual card CSD,
actual rejection branch, CMD9/descriptor fidelity on deployed ROM. Known F0 mask
issue deliberately UNFIXED, no hardware root cause claimed. Also no full firmware/
IRQ stack bound, LSan, all-config compatibility, physical fixture/BIN-copy SHA,
Newlib fileFD or Kilo integration claim. No target code executed.
Next proposed milestone is operator review of this diagnostic artifact and
separate authorization for a controlled diagnostic hardware run, never automatic.
No BIN copied to SD; no COM6/hardware/network access; no Git mutation.

**H1-D2-I1 CONDITIONAL — OPEN VALIDATION ITEMS — HARDWARE NOT AUTHORIZED**
