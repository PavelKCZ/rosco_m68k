#ifndef B3_FIXTURE_H
#define B3_FIXTURE_H
#include <stdint.h>
#include <stddef.h>
typedef struct {
 uint8_t *image;uint32_t stored,base,fat,root,data,volume;
 uint64_t capacity;int bits;uint64_t fail_lba,high_base;
} fixture;
extern fixture disk;
void fixture_create(int bits,int partition,int sdsc,int huge);
void fixture_destroy(void);
int fixture_read(uint32_t,uint8_t [512]);
uint8_t *fixture_sector(uint32_t);
void fixture_put16(uint8_t *,uint16_t);
void fixture_put32(uint8_t *,uint32_t);
#endif
