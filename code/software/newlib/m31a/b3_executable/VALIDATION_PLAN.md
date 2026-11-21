# T2-I1 validation plan — all execution gates NOT RUN

T2-V1 execution update 2026-11-21: see V1_REPORT.md. The plan below describes
the original I1 state; compile/link/automatic and negative audits and 25 host
cases have now run. Manual stack/active-ROM acceptance is still CONDITIONAL.
Machine-only positive controls enable negative testing without inventing manual
acceptance. Full default audit returns nonzero3 while a required manual property
is UNKNOWN/CONDITIONAL. It does not silently grant release or hardware approval.

Status: IMPLEMENTED / NOT EXECUTED, 2026-11-21. No Make command (even -n), compiler,
linker, test binary, audit or sanitizer was invoked. Source readback, Python AST
inspection and file-hash preservation are static checks, not functional tests.

## Gate T2-V1-A: authorized compilation and link

1. Verify checkpoint, clean staging and preservation hashes again. Preserve all
   original components; all source fixes, if needed, require explicit authority.
2. Select NEW empty /tmp directory. Invoke objects, link, binary, inspect in that
   order as documented in README. Never consume historical objects as build inputs.
3. Require all twelve objects and MC68010 ELF/BIN/map generation, strict-warning
   success, compiler/tool versions and hashes, actual command/return records and
   compiler-generated dependency hashes. Failure STOP; no automatic source repair.
4. Confirm original smoke.c excluded; only application.o defines main. Confirm
   machine.o bytes equal the sole selected librosco archive member.

Record exact sizes/SHA256/CRC32, source/objects/archives/tools, expanded commands,
diagnostics and exit codes. No result exists for this new profile yet.

## Gate T2-V1-B: positive executable audit

Require exact object set, approved archive-member closure, correct ownership,
zero unresolved symbols, single runtime/heap/syscall/backend, no legacy or write
provider. Memory helper providers: libc_a-memcmp/memcpy/memset.o; arithmetic
providers include _modsi3/_muldi3/_mulsi3/_udivdi3/_udivsi3/_umodsi3.o and
transitive _divsi3.o/_clz.o plus internal aliases. Additional members STOP review.

Inspect final entry/BIN first instructions, load/run delta3E000, two RX/RW LOADs,
GNU_STACK RW/nonexec, aligned lifecycle arrays, complete trampoline before tail
copy, BSS and _end, all ELF/BIN section bytes, no overlapping initialized source
and destination. Original link.ld assertions and runtime memory gates are retained.
The tool also checks the minimum allowed RAM profile, not just assumed5MiB.

All four wrappers must have exact F1 bodies, final even addresses, object-section
alignment>=2 and precisely the four TRAP13 sites. Inspect descriptor22/align2,
initialized@0/type32BE@2/reserved@6, bounce@40/align4 in actual target DWARF/map.
No arbitrary CMD, selector3, SPI/ATA fallback, or dynamic allocation in B1/B2/B3.

Manual work remains necessary. Review the final disassembly including linked
libgcc instructions for MC68010 compatibility; indirect callback/firmware calls
and nested stack use are not certified merely by -mcpu or no unresolved symbols.
Record a complete artifact-specific engineering stack/IRQ review, app buffer/
diagnostics, B1 automatic512B buffers, compiler .su, libgcc, stdio and firmware
TRAP paths. The historical largest B1 frame744B is not a total bound. Do not
increase the32768 reservation or memory limits to make this pass.

Create manual-review.json ONLY under BUILD, based on manual-review.template.json.
Replace hashes/reviewer/notes and mark checks PASS only after actual review.
Never auto-generate this acceptance file. The audit fails closed until it exists
and matches this ELF/BIN. A reviewer may reject the engineering assumptions; in
that case STOP before hardware. This records acceptance, not dynamic stack proof.
The executable audit must also be reviewed for adequacy; new code is not trusted
merely because it could report PASS. Run audit only after authorization.

## Gate T2-V1-C: negative audit tests — NOT RUN

14 case definitions in tools/negative_cases.json, implemented by
tools/negative_audit.py. Start from a positive audited artifact, copy into new
BUILD/negative-cases/<name>, mutate one property and require a nonzero FAIL with
the expected rejection code. Never alter positive artifacts or repository files.

Coverage: missing main owner; extra duplicate syscall input; legacy FAT input;
selector3; odd wrapper address; wrong LMA/VMA delta; incorrect image-size symbol;
RWX LOAD; executable stack; BIN mismatch; missing/mismatched object hash;
unresolved main symbol; missing manual review. Some are rejected by an earlier
inventory gate (e.g. extra duplicate provider) intentionally; checks are not
weakened to reach a later test. Every test's positive control must PASS first.
Report unique case counts, exact errors and logs. These are definitions, NOT
fourteen PASS results. Validate mutation/parser correctness during T2-V1.

## Future host application logic validation — NOT RUN

Use a separately approved host-only harness with actual src/smoke.c, renaming its
main for the test harness. Fake runtime/diagnostic/console/B3 boundaries only for
APPLICATION branch testing; do not call it a replacement for real B3-I1 tests.
No mock may access serial/hardware. All generated objects stay under /tmp.

At minimum verify: successful513byte path; initialization failure/partial cleanup;
open/info failure; wrong size; first read EOF/partialIO with delivered prefix;
bad byte; first EOF mismatch; SET/END seek errors; boundaryFF00 failure; close
failure; cleanup secondary failure preserving primary snapshot; stale state after
shutdown; failed setvbuf/vprintf/fflush before INIT prevents init; ctor/BSS/data
probe failure; atexit registration failure; all buffer bounds and exact call order.
Verify no direct B1/B2/firmware call and no malloc buffer. Report actual unique
counts and sanitizer outcomes only when executed under future authorization.
Source-only review does not establish these outcomes.

## Gate T2-H1: separately authorized hardware smoke — NOT AUTHORIZED

User prepares/verifies /M31RO.BIN (513 bytes, byte[i]=i&FF), expected SHA256
2ac6d9e2b50bdd65511fb4dc0f24b7f6a0c1b2e92d1cf6901df565802e380234.
No fixture is assumed present. Card preparation modifies media and requires
explicit authorization. Use existing supported FAT16/FAT32 primary/SD profile;
no formatting/hotplug assumptions. Verify copied BIN offline SHA/size/CRC and
loader size/CRC, distinguishing local identity from physical-card hashing.

Exclusive COM6, Tera Term38400/8N1, normal sdfat_menu BIN selection on MC68010.
Capture startup/raw metadata, APP_BEGIN, INIT_BEGIN/INIT_OK, all read/EOF/seek/
close/shutdown results, APP_PASS, atexit/destructor and firmware warm boot.
No automatic retries or board access by the host program.

On failure preserve full transcript/last complete marker and artifact identities.
INIT_BEGIN with no return cannot identify which internal init stage blocked.
No keyboard abort is promised. Use only the operator's agreed manual reset or
power-cycle recovery. Returning main uses firmware warm boot, not old menu return.
Physical read PASS would not prove CRC integrity, all cards or full stack stress.

## Authorization stops

T2-I1 -> STOP (this task). T2-V1 -> separate compile/link/test/audit authorization.
T2-H1 -> separate physical fixture and hardware authorization. Newlib FD/stdio
and Kilo file operations remain later, separately designed milestones. D07 closed.
