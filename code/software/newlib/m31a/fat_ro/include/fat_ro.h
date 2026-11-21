#ifndef B1_FAT_RO_H
#define B1_FAT_RO_H
#include <stdint.h>
#include <stddef.h>
#define B1_MAX_HANDLES 4
#define B1_MAX_CHAIN 4096
#define B1_MAX_PATH 128
/* Callback must return 1 only after filling all 512 bytes. No write interface. */
typedef int (*b1_sector_read)(void *, uint32_t, uint8_t [512]);
typedef uint64_t b1_handle;
typedef enum {
 B1_OK, B1_EOF, B1_IO, B1_METADATA, B1_CHAIN, B1_UNSUPPORTED,
 B1_NOT_FOUND, B1_ARGUMENT, B1_HANDLE, B1_RESOURCE, B1_SEEK,
 B1_OVERFLOW, B1_STATE, B1_LIMIT, B1_NOT_FILE
} b1_status;
typedef enum { B1_SET, B1_CUR, B1_END } b1_whence;
b1_status b1_init(b1_sector_read, void *, uint64_t media_sectors);
b1_status b1_open(const char *, b1_handle *);
b1_status b1_read(b1_handle, void *, size_t, size_t *delivered);
b1_status b1_seek(b1_handle, int64_t, b1_whence);
b1_status b1_info(b1_handle, uint32_t *offset, uint32_t *size);
b1_status b1_close(b1_handle);
void b1_shutdown(void);
#endif
