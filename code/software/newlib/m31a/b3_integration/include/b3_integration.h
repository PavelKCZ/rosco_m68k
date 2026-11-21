#ifndef B3_INTEGRATION_H
#define B3_INTEGRATION_H
#include "fat_ro.h"
#include "sd_ro.h"
typedef enum {B3_UNINITIALIZED,B3_INITIALIZING,B3_READY,B3_FAILED} b3_state;
typedef enum {B3_OK,B3_EOF,B3_ARGUMENT,B3_STATE,B3_SD_ERROR,B3_FAT_ERROR} b3_status;
typedef enum {B3_NONE,B3_SD_INIT,B3_CAPACITY,B3_PROBE,B3_OPEN,B3_READ,B3_SEEK,B3_INFO,B3_CLOSE} b3_phase;
typedef struct {
 b3_state state; uint64_t capacity;
 int first_set; b3_status first_status; b3_phase phase;
 b1_status fat_status; sdro_status sd_status;
 sdro_diagnostic sd; uint64_t delivered;
 unsigned cleanup_count; uint8_t cleanup[6]; /* 1=close,2=B1 shutdown,3=B2 shutdown */
} b3_diagnostic;
b3_status b3_init(void);
b3_status b3_open(const char *,b1_handle *);
b3_status b3_read(b1_handle,void *,size_t,size_t *);
b3_status b3_seek(b1_handle,int64_t,b1_whence);
b3_status b3_info(b1_handle,uint32_t *,uint32_t *);
b3_status b3_close(b1_handle);
void b3_shutdown(void);
b3_status b3_diagnostics(b3_diagnostic *);
#endif
