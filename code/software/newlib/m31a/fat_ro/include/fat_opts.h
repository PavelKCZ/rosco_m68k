/* Private B1 configuration. No legacy SD/FAT adapter is compiled. */
#ifndef B1_FAT_OPTS_H
#define B1_FAT_OPTS_H
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FATFS_IS_LITTLE_ENDIAN 1
#else
#define FATFS_IS_LITTLE_ENDIAN 0
#endif
#define FATFS_INC_WRITE_SUPPORT 0
#define FATFS_INC_FORMAT_SUPPORT 0
#define FAT_SECTOR_SIZE 512
#endif
