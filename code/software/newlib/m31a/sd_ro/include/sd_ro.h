#ifndef SDRO_H
#define SDRO_H
#include <stdint.h>
typedef enum {SDRO_UNINITIALIZED,SDRO_INITIALIZING,SDRO_READY,SDRO_FAILED} sdro_state;
typedef enum {SDRO_V1=0,SDRO_V2=1,SDRO_SDHC=2,SDRO_UNKNOWN=3} sdro_type;
typedef enum {
 SDRO_OK,SDRO_ARGUMENT,SDRO_NO_FIRMWARE,SDRO_INIT_FAILURE,
 SDRO_NOT_INITIALIZED,SDRO_CARD_TYPE,SDRO_CSD_READ,SDRO_CSD_VERSION,
 SDRO_PROFILE,SDRO_CSD_MALFORMED,SDRO_CAPACITY,SDRO_CAPACITY_OVERFLOW,
 SDRO_MISMATCH,SDRO_SDSC_OVERFLOW,SDRO_RANGE,SDRO_READ_FAILURE,SDRO_STATE
} sdro_status;
typedef struct {
 sdro_state state; sdro_status error;
 uint32_t firmware_init_status; /* UINT32_MAX when no init result exists */
 uint32_t reported_type; /* raw descriptor type; UNKNOWN before acquisition */
 uint64_t sectors; /* zero unless READY */
} sdro_diagnostic;
sdro_status sdro_init(void);
sdro_status sdro_capacity(uint64_t *);
sdro_status sdro_card_type(sdro_type *);
void *sdro_context(void);
int sdro_read(void *,uint32_t lba,uint8_t buffer[512]);
sdro_status sdro_diagnostics(sdro_diagnostic *);
void sdro_shutdown(void);
#endif
