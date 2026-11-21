#include "fat_fixture.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
fixture disk;
static void need(int b){if(!b){fputs("fixture bounds/allocation failure\n",stderr);abort();}}
void fixture_put16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
void fixture_put32(uint8_t *p,uint32_t n){fixture_put16(p,(uint16_t)n);fixture_put16(p+2,(uint16_t)(n>>16));}
uint8_t *fixture_sector(uint32_t lba){need(lba<disk.stored);return disk.image+(size_t)lba*512;}
static void fat(uint32_t c,uint32_t n){uint8_t *p=fixture_sector(disk.fat)+(size_t)c*(disk.bits/8);if(disk.bits==16)fixture_put16(p,(uint16_t)n);else fixture_put32(p,n);}
void fixture_destroy(void){free(disk.image);memset(&disk,0,sizeof(disk));}
void fixture_create(int bits,int partition,int sdsc,int huge){
 fixture_destroy();disk.bits=bits;disk.base=partition?7:0;disk.fail_lba=UINT64_MAX;
 uint32_t reserved=bits==16?1:32,fsz=bits==16?20:513,roots=bits==16?2:0,clusters=bits==16?5000:65530;
 disk.volume=reserved+fsz+roots+clusters;disk.stored=disk.base+disk.volume;
 disk.capacity=huge?UINT64_C(0x100000000):sdsc?8192:((uint64_t)disk.stored+1023)/1024*1024;
 need(disk.capacity>=disk.stored);disk.image=calloc(disk.stored,512);need(disk.image!=NULL);
 uint8_t *p=fixture_sector(disk.base);fixture_put16(p+510,0xaa55);fixture_put16(p+11,512);p[13]=1;fixture_put16(p+14,(uint16_t)reserved);p[16]=1;
 fixture_put16(p+17,bits==16?32:0);
 if(bits==16){fixture_put16(p+19,(uint16_t)disk.volume);fixture_put16(p+22,(uint16_t)fsz);}else{fixture_put32(p+32,disk.volume);fixture_put32(p+36,fsz);fixture_put32(p+44,2);}
 if(partition){p=fixture_sector(0);fixture_put16(p+510,0xaa55);p[450]=bits==16?6:0xc;fixture_put32(p+454,disk.base);fixture_put32(p+458,disk.volume);}
 disk.fat=disk.base+reserved;disk.data=disk.base+reserved+fsz+roots;disk.root=bits==16?disk.base+reserved+fsz:disk.data;
 uint32_t end=bits==16?0xffffU:0xfffffffU;fat(0,end);fat(1,end);fat(2,end);fat(3,8);fat(8,end);
 p=fixture_sector(disk.root);memcpy(p,"M31RO   BIN",11);p[11]=0x20;fixture_put16(p+26,3);fixture_put32(p+28,513);
 for(unsigned i=0;i<512;i++)fixture_sector(disk.data+1)[i]=(uint8_t)i;
 fixture_sector(disk.data+6)[0]=0;
}
/* Bounded sparse medium: stored prefix + deterministic zero sectors through
 * capacity-1. No allocation proportional to huge advertised capacity. */
int fixture_read(uint32_t lba,uint8_t b[512]){
 if((uint64_t)lba>=disk.capacity)return 0;
 if(lba==disk.fail_lba){memset(b,0xee,117);return 0;}
 if(disk.high_base&&(uint64_t)lba>=disk.high_base&&(uint64_t)lba<disk.high_base+disk.volume)
  memcpy(b,fixture_sector(disk.base+(uint32_t)((uint64_t)lba-disk.high_base)),512);
 else if(lba<disk.stored)memcpy(b,fixture_sector(lba),512);else memset(b,0,512);
 return 1;
}
