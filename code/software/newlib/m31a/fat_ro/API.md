# Private B1 read-only API contract

`include/fat_ro.h` is the complete interface. Only seven b1_* symbols are public;
no POSIX/Newlib/SD entry points. One application-local instance, single-threaded,
non-reentrant; no ISR or concurrent caller. The read callback must not recursively
enter B1 and must be a trusted read-only implementation. B1 does not provide
physical memory/device write protection against arbitrary caller code.

## Lifecycle and media

b1_init(callback, context, sector_count): callback takes one uint32 LBA and a
writable 512-byte scratch buffer, returning exactly 1 on complete success; any
other return is I/O failure. It must not write outside that buffer. Media count
must be 1..2^32. A short read must report failure. Callback/context stay valid
until shutdown. The complete medium/metadata is immutable during an execution;
handles retain a validated cluster-chain snapshot. Media replacement or concurrent
metadata mutation is unsupported. No automatic retry/remount exists.

Uninitialized -> ready on successful BPB validation; failed init clears state.
Second init while ready returns STATE without invalidating handles. Shutdown is
bounded, idempotent, clears local state and invalidates handles, with no media I/O.
An explicit init after shutdown/failure is a caller action, not automatic recovery.
No cache dirty/flush/purge state exists in the compiled RO implementation.

## Open and handles

b1_open(path,&handle) opens only an existing regular file for reading. No mode or
flags argument exists; there is no route to request creation or writing. Absolute
ASCII short-name paths only, up to 127 bytes including leading slash, at most eight
components, names 1..8 plus optional 1..3 extension. Letters case-insensitive;
alphanumerics, underscore and hyphen accepted. No cwd, ., .., repeated/trailing
slash, Unicode, DOS drive prefix or LFN lookup. LFN entries are skipped; valid
short aliases may be used. Directories can be traversed, not opened as files.

Four fixed handles. Same path may be opened simultaneously, with independent
offsets and chain snapshots (unlike legacy _check_file_open). Successful open
publishes a nonzero uint64 token only after all validation. Failed open leaves
output untouched and occupies no slot. Tokens are not reused across shutdown;
serial exhaustion returns RESOURCE instead of wrapping. Closed/stale tokens
cannot refer to a newly reused slot. This is a private handle API, not FD 3..6.

## Reads and errors

b1_read(handle, buffer, requested, &delivered) reports exact bytes delivered;
valid output-count pointer is required and initialized to zero. Invalid handle
is still an error on zero count. Zero count on a valid handle succeeds without
accessing media, accepts NULL buffer and does not clear a prior error.

- OK: complete requested count, including a read ending exactly at EOF.
- EOF: zero bytes at/beyond EOF, or a short successful read reaching EOF.
- IO: callback failure; scratch contents are never exposed on failed sector.
- CHAIN/METADATA: structural corruption, never translated into EOF.

Partial error: return IO (or structural error) WITH positive delivered count;
advance logical position only by those bytes. Destination beyond delivered is
unchanged. Error is sticky: subsequent nonzero reads return that status with zero
bytes and no media access. A successful explicit seek clears the sticky error;
failed seek leaves error and offset unchanged. A later FD adapter must preserve
this dual count/status contract, not drop the error after returning partial data.

Maximum file size is INT32_MAX AND no more than 4096 clusters; the latter limits
actual data to at most 256 MiB for 128-sector clusters. Longer chain -> LIMIT,
premature chain -> CHAIN, unrepresentable directory file size -> OVERFLOW.
Open eagerly validates the whole allocated chain, including extra allocation.
A valid-but-overbudget chain is not claimed to be corrupted.

## Seek, info and close

b1_seek(handle, signed int64 delta, SET/CUR/END): logical offset 0..INT32_MAX.
SET base0, CUR current offset, END file size. Arithmetic checked before addition;
negative result -> SEEK, unrepresentable result -> OVERFLOW. Position beyond EOF
is legal, no allocation/writes; read then returns EOF. Successful seek clears
sticky read error. No sector read occurs during seek.

b1_info provides offset and original directory size. b1_close clears one slot,
never flushes/writes/reads; double close -> HANDLE. Shutdown invalidates all handles.

## Status taxonomy

OK, EOF; IO (media); METADATA (invalid geometry/entry); CHAIN (invalid/free/
reserved/bad/out-of-range cluster, premature EOC, loop); UNSUPPORTED (FAT12,
non-primary/non-FAT format); NOT_FOUND; ARGUMENT (path/pointers/config arguments);
HANDLE; RESOURCE (four slots or token space); SEEK; OVERFLOW; STATE (not ready or
second init); LIMIT (depth/chain budget); NOT_FILE (directory requested as file).
No errno is owned or modified by B1. No implicit retry, heap or filesystem logging.
