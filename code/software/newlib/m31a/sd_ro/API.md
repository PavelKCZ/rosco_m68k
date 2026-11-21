# Adapter contract

Public declarations: include/sd_ro.h. Private decoder and firmware prototypes live
in src/private.h; no arbitrary CMD/register-read function is public.

## Lifecycle

sdro_init() explicitly restarts acquisition: clear descriptor, scratch, bounce,
readiness, prior capacity/type, then UNINITIALIZED -> INITIALIZING -> READY or FAILED.
Support must equal1234FEDC; init D0 must be0; descriptor byte0 must be exactly1;
type must be V1/V2/SDHC; fixed CMD9 must succeed; CSD/profile/capacity must validate.
An explicit repeated init is permitted but no automatic retries are added. The
firmware itself has bounded protocol retry loops; adapter does not remove them.
Failed initialization clears descriptor/buffers and capacity. Observed raw type
and init result may remain in diagnostics, but cannot be queried as usable state.

sdro_shutdown() zeroes local resources and returns UNINITIALIZED; no firmware
call, physical unmount, reset or write. sdro_capacity/card_type only publish outputs
when READY; failures leave caller output untouched. No dynamic allocation.

## Sector callback

sdro_read(sdro_context(), uint32_lba, destination512) is synchronously compatible
with b1_sector_read. It checks exact instance context, READY, non-NULL destination,
validated capacity and SDSC addressability before firmware invocation. Invalid
context pointers are compared, not dereferenced. Firmware receives only the private
4-byte-aligned bounce buffer. On success exactly512 bytes are copied and return1;
on failure return0, caller buffer unchanged. Partial firmware buffers are discarded
and bounce cleared on firmware failure. Repeated explicit reads are permitted.
Sector errors leave adapter READY and update last error; there is no hidden retry.
A later successful read updates error to OK. No error becomes EOF.

Single synchronous caller; callbacks must not recurse or retain firmware buffer
pointers. Caller destination must be valid writable storage512 bytes. No alignment
requirement imposed on caller destination (copy happens after firmware returns).

## Capacity and decoding

Raw bytes only, no compiler-dependent CSD bitfields. Version = raw[0]>>6.
CSDv1 C_SIZE=((raw[6]&3)<<10)|(raw[7]<<2)|(raw[8]>>6),
C_SIZE_MULT=((raw[9]&3)<<1)|(raw[10]>>7), READ_BL_LEN=raw[5]&15.
Checked uint64 bytes=(C_SIZE+1)*2^(C_SIZE_MULT+2)*2^READ_BL_LEN; require nonzero
and divisible512, then sectors=bytes/512. Shifts bounded before evaluation.
CSDv2 C_SIZE=((raw[7]&3F)<<16)|(raw[8]<<8)|raw[9];
sectors=((uint64)C_SIZE+1)*1024. C_SIZE=3FFFFF yields exactly2^32 sectors.

Public media count uint64, individual LBA uint32. SDHC supported count1..2^32;
SDSC must not exceed2^23 sectors (max LBA7FFFFF). Reject, never clamp. Every read
checks lba<count and SDSC lba<=7FFFFF before firmware can perform its 32-bit shift.
With the deliberately restricted v1 READ_BL_LEN9 field profile, maximum encoded
capacity is2^21 sectors. Therefore the full2^23 SDSC guard boundary is exercised
by pure shared guard tests, not fabricated as a successful CSDv1 init fixture.
C_SIZE0 is valid minimum, NOT zero capacity. Malformed zero capacity is tested
at the shared capacity validator because supported CSD formulas cannot encode0.

Profile checks: raw0 bits5..0, raw12 bits6..5, raw13 bits4..0, raw14 bits1..0
must be0. v1 raw6 bits3..2 must be0; v2 raw6 bits3..0, raw7 bits7..6 and raw10
bits7..4 must be0. These conservative supported-profile constraints are based on
local reserved-field definitions, not a claim of exhaustive normative validation.
End bit raw15 bit0 must be1; CRC bits are not validated. Decoder leaves output
untouched on every failure. No capacity inferred from FAT or arithmetic clamp.

## Errors and diagnostics

sdro_diagnostics copies state, last error, raw firmware init status, observed type
and active sector count. UINT32_MAX means no init result when state/error indicates
support has not passed; an actual init UINT32_MAX is preserved with INIT_FAILURE.

ARGUMENT; NO_FIRMWARE; INIT_FAILURE; NOT_INITIALIZED (including false-OK);
CARD_TYPE; CSD_READ; CSD_VERSION; PROFILE; CSD_MALFORMED (end bit);
CAPACITY (zero/non-sector multiple); CAPACITY_OVERFLOW; MISMATCH;
SDSC_OVERFLOW; RANGE; READ_FAILURE; STATE; OK.
Boolean firmware failure cannot distinguish removal, timeout, busy, CRC error or
transport fault. Do not invent these details. No CRC-protected integrity claim.
