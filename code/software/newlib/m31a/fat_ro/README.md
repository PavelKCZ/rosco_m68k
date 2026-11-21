# B1 — isolated read-only local FAT derivative

Host-only preparation for later M3.1 Newlib integration. No target/Newlib/firmware
or hardware integration is present. Original libsdfat, M3.1 runtime, Makefile and
audits are unchanged. D07 is closed; M3.1-H1 remains closed after D6 hardware PASS.

## Derivation and ownership

PROVENANCE.json records exact local source paths, SHA-256 and repository revision.
reference/ holds unmodified local originals for review/license, NEVER compiled.
include/fat_defs.h and fat_types.h are byte-identical copies actually used by B1.
src/fat_ro.c is a deliberately pruned RO derivative of local fatfs_init,
fatfs_lba_of_cluster, fatfs_find_next_cluster, directory lookup and sector/file
reads. It is NOT another downloaded FAT implementation, NOT a drop-in FL_FILE ABI,
and NOT a claim that the full original feature set was retained. Large portions
of the original write-oriented core were removed instead of leaving fragile
conditional write/cache paths in the runtime. This substantial pruning is explicit.
The original GPL copyright/license notice is retained in the compiled derivative.
Upstream fat_io_lib V2.6 notice is available; exact upstream revision is UNKNOWN.

## Corrections relative to recorded originals

- fat_access initialization: bounded 64-bit geometry arithmetic, explicit media
  extent, partition and FAT/data bounds, power-of-two cluster size, FAT capacity,
  FAT16/32 layout/version/root/active-FAT validation. No EBR/GPT/FAT12 support.
- fat_table lookup: no conflation of I/O failure with EOC; explicit reserved/free/
  bad/out-of-range detection and bounded full-chain loop validation.
- fat_filelib read: EOF separate from I/O/corruption, sector scratch commit only
  on complete success, explicit partial count/status and sticky read errors.
- original fseek clamping/overflow replaced by checked signed logical offsets,
  with SET/CUR/END and legal beyond-EOF position without mutation.
- original global pool/raw FL_FILE pointers replaced by fixed private token handles,
  safe stale-handle rejection, independent opens and bounded cleanup.
- all mutators, format, dirty cache, FAT writeback/purge and media-write callback
  removed from the compiled subset. No silent write-mode downgrade.
- private endianness configuration is selected for host architecture; future target
  compiler validation remains necessary. Original m68k packed-structure definitions
  retained; actual parsing uses byte reads, not unaligned integer dereferences.

## Build (host only)

```
make -C code/software/newlib/m31a/fat_ro BUILD=/tmp/kilo-m31-b1-example test sanitized audit
```

No original library/build profile is invoked. Only src/fat_ro.c is compiled as
backend. fat_write.c, fat_format.c, sdcard.c/sdcard_asm.S and any installed FAT
archive are excluded. Build outputs go to BUILD; default also under /tmp.
The sanitized target uses ASan/UBSan with leak detection disabled because the
current ptrace environment cannot run LSan. See TEST_REPORT.md for actual results.

## Limits and risks

Single fixed medium, one instance, non-reentrant. Four handles, 4096 clusters per
chain, eight path components, ASCII 8.3 lookup. Only first primary FAT partition
or superfloppy, FAT16/FAT32, 512B sectors. LFN aliases usable but LFN matching,
Unicode/cwd/directory enumeration and public stdio are not implemented. Mirror
FAT copies are not cross-compared; FAT32 active FAT is selected from BPB flags.
FSInfo hints and timestamps are not relied on or updated. No media CRC is added;
callback integrity is trusted. Full-chain validation is bounded O(n^2) comparison
work; target latency is UNKNOWN and is a B2/target review risk.

Handles retain up to 4096 uint32 clusters each plus a shared directory work chain.
Host object static BSS is 82120 bytes, not a measured MC68010 RAM or stack figure.
No backend heap allocation. Automatic sector buffers and bounded path arrays exist;
no hardware stack sufficiency claim. RO is absence of storage-write capabilities
in this backend, not electrical/hardware write protection.

API.md defines all statuses and precise partial-read/seek/lifecycle semantics.
TEST_REPORT.md records tests, limitations and proposed B2 acceptance. No B2 or
hardware action is authorized by completion of B1.
