#ifndef B3_CSD_DIAG_H
#define B3_CSD_DIAG_H
#include "sd_ro_csd_diag.h"
#ifdef SDRO_CSD_DIAGNOSTIC
/* Same copy/availability contract as B2; no lifecycle or error-state mutation. */
int b3_csd_snapshot(sdro_csd_diagnostic *out);
#endif
#endif
