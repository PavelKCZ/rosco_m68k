# H1-D2-I1 isolated CSD failure diagnostic

Opt-in SDRO_CSD_DIAGNOSTIC; never relaxes validators (including B10 & F0).
Uses original startup/runtime/linker and B1; unchanged TRAP wrappers.
Copy-out record survives shutdown, resets on next sdro_init. No threads/hotplug.
Snapshot validity means captured, not successful or CRC-verified.
Getters return 1 available, 0 none with zero output, -1 null; no state mutation.
Normal builds do not export diagnostic getters. b3_diagnostic layout unchanged.

Separate target build: python3 tools/build.py --build /tmp/NEW-DIRECTORY compile
then link, binary, inspect in separate actions. Outputs m31-csd-failure-diag.*.
Full audit requires image-specific manual review and remains conditional for
active ROM / full firmware-IRQ stack. No automatic hardware approval.
Original b3_executable historical manifests/reports remain unchanged and may
now identify historical source versions; this profile pins the new sources.
See REPORT.md for actual validation and limitations.
