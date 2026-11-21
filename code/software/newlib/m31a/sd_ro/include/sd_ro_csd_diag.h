#ifndef SDRO_CSD_DIAG_H
#define SDRO_CSD_DIAG_H
#include <stdint.h>
#include <stddef.h>
#ifdef SDRO_CSD_DIAGNOSTIC
enum {
 SDCD_ATTEMPT=1u, SDCD_SUPPORT=2u, SDCD_INIT=4u, SDCD_DESCRIPTOR=8u,
 SDCD_CMD9=16u, SDCD_CSD=32u, SDCD_STRUCTURE=64u, SDCD_REJECT=128u,
 SDCD_FINAL=256u, SDCD_MASK=512u
};
typedef struct {
 uint32_t valid, support_raw, init_raw, type_raw, cmd9_raw;
 uint16_t final_status, reject_id;
 uint8_t schema, stage, initialized_raw, structure;
 uint8_t raw_csd[16], masked[4];
} sdro_csd_diagnostic;
_Static_assert(sizeof(sdro_csd_diagnostic)==48,"CSD diagnostic size");
_Static_assert(_Alignof(sdro_csd_diagnostic)>=2,"CSD diagnostic alignment");
#define SDCD_OFFSET(field, offset) _Static_assert(offsetof(sdro_csd_diagnostic,field)==offset,"CSD offset: " #field)
SDCD_OFFSET(valid,0); SDCD_OFFSET(support_raw,4); SDCD_OFFSET(init_raw,8);
SDCD_OFFSET(type_raw,12); SDCD_OFFSET(cmd9_raw,16); SDCD_OFFSET(final_status,20);
SDCD_OFFSET(reject_id,22); SDCD_OFFSET(schema,24); SDCD_OFFSET(stage,25);
SDCD_OFFSET(initialized_raw,26); SDCD_OFFSET(structure,27);
SDCD_OFFSET(raw_csd,28); SDCD_OFFSET(masked,44);
#undef SDCD_OFFSET
/* Copy only: 1 = attempt available, 0 = none (zero output), -1 = null.
 * Inert evidence survives shutdown; next sdro_init clears it. Single thread.
 * valid bits mean captured, not successful. Never serialize this C struct. */
int sdro_csd_snapshot(sdro_csd_diagnostic *out);
#endif
#endif
