# Private B3 API contract

The declarations in include/b3_integration.h are authoritative. This is a
single-instance, synchronous, non-reentrant API for immutable media.

| Function | Contract |
|---|---|
| b3_init() | UNINITIALIZED -> INITIALIZING -> READY or FAILED; no implicit retry/reset |
| b3_open(path, &token) | Read-only regular-file open; returns an existing B1 token; output unchanged on failure |
| b3_read(token, buf, count, &got) | B1 semantics; got initialized to zero; delivered bytes valid even if result is FAT_ERROR |
| b3_seek(token, int64_offset, B1_SET/CUR/END) | B1 semantics, including logical position beyond EOF; successful seek clears sticky read error |
| b3_info(token, &pos, &size) | Underlying B1 uint32 positions/sizes; no filesystem metadata invention |
| b3_close(token) | Releases tracked B1 token; invalid/double/stale tokens rejected |
| b3_shutdown() | Closes tracked tokens, B1 shutdown, B2 shutdown; idempotent; state UNINITIALIZED |
| b3_diagnostics(&snapshot) | Copies state, uint64 sector count and first error plus B1/B2 details and partial-byte count |

B3_OK is success; B3_EOF is normal EOF, including short reads reaching EOF.
B3_FAT_ERROR retains the precise B1 status, B3_SD_ERROR the B2 status. A partial
media failure is FAT_ERROR/B1_IO, not EOF, and reports exactly the delivered
bytes; B2's detailed error is snapshotted. Subsequent read fails without media
access until successful seek. First backend error remains even if recovery
later succeeds. B3_ARGUMENT/B3_STATE are guard errors, not replacements for the
first experiment/backend failure; these guards do not overwrite diagnostics.

Read/seek/info/close/open require READY. Shutdown invalidates tracked handles;
B1 generation tokens additionally reject old handles after a fresh init.
Capacity is valid only while READY; cleanup sets it to zero. The first failure
snapshot survives shutdown until another permitted init. Cleanup trace codes
1=close completed, 2=B1 shutdown completed, 3=B2 shutdown completed.

| Failure stage | Cleanup / observable result |
|---|---|
| Support/init/descriptor/CSD | B2 shutdown; FAILED, original B2 diagnostic |
| Capacity query | B2 shutdown; FAILED (defensive branch; not naturally injectable after successful B2 init) |
| FAT probe | B1 shutdown then B2 shutdown; FAILED, original B1/B2 error |
| Open/read/seek/info/close | Preserve READY and first error, explicit cleanup/recovery possible |
| Shutdown with open files | Close all tracked handles before B1 then B2 shutdown |
| Init while READY/FAILED | STATE; no firmware call, existing session untouched |

A close failure is only naturally possible for invalid/stale ownership. Tests
explicitly invalidate a B1 token externally to exercise cleanup error handling;
that is a negative ownership-violation test, not a supported production pattern.
Capacity-query failure cannot be induced using actual successful B2 initialization
without replacing a real B2 function or violating the single-thread assumption;
the branch is statically inspected, not claimed dynamically covered.

No write, create, truncate, rename, deletion, format, POSIX FD or Newlib syscall
operation is exposed. There is no mount/remount or hotplug manager.
