# I1 host validation report

Evidence /tmp/kilo-m31-b2-i1-it8fuan9/; essential logs preserved here.
Compiler: cc Ubuntu11.4.0. -std=c11 -Wall -Wextra -Werror -Wpedantic.
Normal -O2; sanitized -O1 -fsanitize=address,undefined -fno-sanitize-recover=all.

Command:
```
make -C code/software/newlib/m31a/sd_ro BUILD=/tmp/kilo-m31-b2-i1-it8fuan9/build test sanitized audit
```

validation-01: exit2, compilation stopped on misleading-indentation in the mock
CSD function (unconditional return on the same line as guarded memcpy). Split
lines, no assertion weakened and no runtime correction required. No test ran.
validation-02: exit0, 86 cases normal and sanitized PASS.
validation-03: exit0, final91 unique cases normal and sanitized PASS; audit PASS.
No failures/errors/skips/xfail in completed final runs. Final compiler warnings0.
No count inflation: same91 cases rerun in two configurations, NOT182 unique.
Identities preserved in evidence/test-identities.json.

Coverage: support ordering/no init after mismatch, init codes1..3 and unknown,
false-OK/noncanonical initialized, raw enum type, descriptor zeroing/reinit,
SDSC V1/V2 and SDHC, CSDv1/v2 formulas, exact2^32 sector SDHC count, unsupported
structures/profiles/end/reserved constraints, type mismatch, zero/oversized
capacity helper, first/last/out-of-range LBA, SDSC guard7FFFFF/800000/FFFFFFFF,
private alignment, no public output on partial read failure, scratch clearing,
explicit repeated reads, shutdown, queries and callback1/0. Mock traces assert
only0/1/2/4 selectors, CMD9 and counts, with no forbidden calls. Pure guard tests
are distinguished from end-to-end descriptor/CSD tests in API.md.
B1 compatibility is a callback function-pointer assignment using original B1
header; no FAT implementation or public stdio path is exercised.

ASan/UBSan final91/91 PASS, no reported findings. LeakSanitizer NOT VERIFIED:
prior B1 run established ptrace incompatibility in this same environment. No
new LSan attempt made because authorization says attempt only if supported;
sanitizer command explicitly detect_leaks=0. Adapter has no dynamic allocation.

Static host object audit: exact intended exports/imports; no legacy SD/FAT,
Newlib/POSIX hooks or write dependency. Compiler dependency output private.
Assembly SOURCE audit: exactly four wrappers, selectors0/1/2/4 only, fixed CMD9,
expected stack offsets and preservation. Target code is NOT assembled or linked.
No claim that host mocks verify real register transfers or physical SD behavior.

Remaining validation: target descriptor/assembly ABI, compiler output/map/imports,
active firmware service availability, real CSD and read behavior, stack footprint.
GO only for separately authorized target compile/static audit preparation;
CONDITIONAL GO for later integration after those checks. No hardware authorization.
