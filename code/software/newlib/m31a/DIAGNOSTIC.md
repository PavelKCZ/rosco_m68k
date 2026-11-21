# M3.1-H1-D2 — one opt-in startup diagnostic

Historical original hardware result remains FAIL: 44980-byte runtime-smoke.bin,
SHA-256 066e667d849623ac682c8153542af1f688209f28c9f09a0bedaafba6943ebeb5,
loaded CRC-32 636A5A00, no visible application output. D3 subsequently reached entry and stopped at !01; see D5_ABI_FIX.md.
D2 instrumentation was not a fix. Current D5 removes the invalid persistent-magic gate.

## Build / evidence (no hardware execution authorized here)

```
make BUILD=/tmp/kilo-m31-h1-d2-example all diagnostic host-test
```

Default `make all` is still normal. `diagnostic` explicitly invokes the SAME
Makefile with DIAGNOSTIC=1 and a distinct BUILD/diagnostic directory. No second
startup implementation is used. Both profiles must be built before the expanded
host suite (it inspects both target outputs).

The complete clean verification used:
`make BUILD=/tmp/kilo-m31-h1-d2-o3h8zybv/final all diagnostic host-test`

Evidence: /tmp/kilo-m31-h1-d2-o3h8zybv/; original-source/ preserves A.1 sources.
All original build evidence under /tmp/kilo-m31a1-bvzuawul/ is unchanged.

Final diagnostic BIN:
/tmp/kilo-m31-h1-d2-o3h8zybv/final/diagnostic/runtime-smoke-diag.bin
45800 bytes; SHA-256 d5dc9ab1d0c68249ca0dc50a638eeef65b60c5c6bf18ea5881a454d568f8aac8
CRC-32/IEEE 5BBB34D2.

Normal rebuilt BIN is bit-for-bit identical to A.1, not merely the same size/name.
Diagnostic image overhead is 820 bytes; its _end is 0xDB58 (normal 0xD820).
Boot copy is 924 bytes including helpers/postboot; initialized image 45800 bytes,
run range [0x2000,0xD2E8), BSS [0xD2E8,0xDB58). The same bounds policy uses the
new sizes. No SD/FAT initialization or writes are added.

## Primitive and safety contract

TRAP #14 function 4: D0.B character, D1=4 (same PRINTCHAR route used by menu).
No Newlib, termios, heap, strings in .data, or initialized globals are needed.
All early string addressing and helper calls are PC-relative within .boot.
The helpers are copied with the trampoline and then work at the run address.

An early TRAP is NOT stackless. It requires the inherited valid menu exception
stack and normal supervisor-mode entry. SR capture is privileged on MC68010;
user-mode/arbitrary-loader entry is outside this diagnostic contract. Absence
of E cannot distinguish failure to enter from failure in the first diagnostic
save/TRAP. This is intentionally documented rather than hidden by a claim of
universal safety.

Entry saves 15 integer/address registers (60 bytes) then original SR (2 bytes).
SP printed is current SP+62, i.e. SP before instrumentation. SR is printed as
8 hex digits with the captured 16-bit value zero-extended. The raw ABI slot/RAM/reset are
read once after E is emitted and held through the dump. Only RAM/reset enter
validation; the raw slot is not authenticated or dereferenced. d2/d4/d5 are the deliberate snapshot outputs; other entry registers
and original CCR are restored. Original startup has no incoming d2/d4/d5 input
contract. Regular checkpoints save/restore all live registers and CCR.

Raw putchar additionally preserves D0/D1/A0/A1 and CCR around TRAP. Firmware's
PRINTCHAR contract preserves the other registers. CCR is restored before
flag-neutral MOVEM/RTS. Interrupt mask and supervisor state are not changed.
Nested diagnostic frames use under 192 bytes on the inspected DUART PRINTCHAR
path excluding interrupt nesting; this is static accounting, not measured stack
use. The original 32 KiB runtime stack reservation remains an assumption.

## Output / checkpoints

Entry line (values measured, dots below are placeholders):

```
E ABI_SLOT=........ RAM=........ RESET=........ SP=........ SR=........
```

| Marker | Emitted after / at |
|---|---|
| E | entry save and initial SP/SR capture; before metadata reads |
| V | all assembly validation branches passed |
| S | SP switched and firmware output is being attempted on the new stack |
| R | relocated trampoline entry |
| Z | initialized image copied and BSS zeroed |
| C | first C runtime statement (after C prologue) |
| P | C memory plan accepted and heap state initialized |
| B / T | immediately before console binding / successful return |
| A | runtime atexit registration succeeded |
| I / K | Newlib arrays / legacy constructors completed |
| M | main body entered |
| O / F | before first printf / first printf returned |
| W | first _write_r entry, before validation or termios TX |

Markers use a trailing space. Entry/error lines end with explicit CR+LF.
Typical successful prefix is `V S R Z C P B T A I K M O`, then W at the actual
first syscall write, ordinary application output, and F after printf. W position
is determined by actual stdio behavior, not forced by the diagnostics. No earlier
stdio call is explicitly made by this smoke application. W is one-time BSS state;
it is NOT proof of successful device TX and does not recurse through stdio.

Rejections print a line then retain the original stop loop:

* !01 retired by D5: the ABI slot is not a permanent magic signature
* !02 RAM outside supported range
* !03 reset vector outside supported range
* !04 RAM/reset alignment
* !05 aligned _end overflow or no heap room below reserved stack
* !06 load-source alignment or initialized source/destination overlap
* !07 source extent overflow or extension into reserved stack

C runtime error exits retain their status and emit !C plus 8 hex digits:
!C00000064 = status 100 (C plan/top check), !C00000065 = 101 (console bind),
!C00000066 = 102 (runtime atexit registration). Existing firmware _exit follows.
No automatic retry, reset, alternate memory policy or recovery is added.

## Historical D2 validation and limitations

The 71-test totals and bit-identical normal BIN below describe D2, not D5.
Current D5 results and changed BIN identities are in D5_ABI_FIX.md.

## Validation and limitations

Clean target builds and shared layout/symbol/permissions audit PASS for both
profiles. Normal audit still prohibits early stack calls. Only explicit
--diagnostic selection permits the documented inherited-stack instrumentation;
it additionally checks the exact raw primitive bytes, save-frame prefix,
PC-relative helper call targets and actual rejection-branch targets/counts.
All FAT/legacy-object exclusions and ELF/BIN checks remain active.

71/71 host tests PASS: 56 existing + 15 diagnostic. Native tests verify C marker
ordering, error statuses and exactly one W before fake TX. Static target tests
inspect register/CCR save/restore, snapshot offsets, TRAP convention, branch
mapping and loader layout. They do NOT execute MC68010 instructions.

Preliminary logs retained: build-01 failed because diagnostic audit still used
the normal map filename; corrected. tests-01 had 70 PASS + 1 assertion failure:
it assumed textual basic-block ordering in _write_r. GCC puts the first-write
branch after the common TX block. The test now checks the actual branch/resume
path plus native TX-at-marker accounting; the runtime was not changed for it.

If a separately authorized physical run stops: preserve the full capture and
last complete marker. No keyboard abort is promised. Use only the operator's
agreed manual reset/power-cycle procedure; do not automatically retry. The
firmware output primitive may itself block, so a last marker bounds progress but
does not establish root cause. No BIN was copied to SD and no hardware run was
performed in D2. Another hardware action requires explicit authorization.
