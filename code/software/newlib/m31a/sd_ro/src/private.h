#ifndef SDRO_PRIVATE_H
#define SDRO_PRIVATE_H
#include "sd_ro.h"
#include "sd_ro_csd_diag.h"
#include <stddef.h>
/* Public SDCard ABI, verified against existing MC68010 sdcard.o DWARF:
 * size22, initialized@0, enum32@2 (big-endian), reserved[4]@6. */
typedef struct { _Alignas(2) uint8_t bytes[22]; } sdro_fw_card;
_Static_assert(sizeof(sdro_fw_card)==22,"firmware descriptor extent");
_Static_assert(_Alignof(sdro_fw_card)>=2,"firmware descriptor alignment");
_Static_assert(offsetof(sdro_fw_card,bytes)==0,"firmware descriptor pointer");
uint32_t sdro_fw_support(void);
uint32_t sdro_fw_init(sdro_fw_card *);
uint32_t sdro_fw_csd(sdro_fw_card *,uint8_t buffer[16]);
uint32_t sdro_fw_read(sdro_fw_card *,uint32_t,uint8_t buffer[512]);
/* Pure internal helpers, not a public command/register interface. */
sdro_status sdro_decode_csd(const uint8_t raw[16],uint32_t type,uint64_t *);
sdro_status sdro_validate_capacity(uint32_t type,uint64_t);
sdro_status sdro_check_lba(uint32_t type,uint64_t,uint32_t);
#ifdef SDRO_CSD_DIAGNOSTIC
sdro_status sdro_decode_csd_traced(const uint8_t raw[16],uint32_t,uint64_t *,sdro_csd_diagnostic *);
#endif
#endif
