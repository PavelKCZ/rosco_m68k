#include "private.h"
#include <limits.h>
#ifdef SDRO_CSD_DIAGNOSTIC
#define TRACE_ARG , sdro_csd_diagnostic *trace
#define TRACE_PASS , trace
#define NO_TRACE , NULL
static void rejection(sdro_csd_diagnostic *d,uint16_t id){
 if(d && !(d->valid&SDCD_REJECT)){d->reject_id=id;d->valid|=SDCD_REJECT;}
}
#define REJECT(s,id) do{rejection(trace,id);return s;}while(0)
static void masks(sdro_csd_diagnostic *d,uint8_t a,uint8_t b,uint8_t c,uint8_t e){
 if(d){d->masked[0]=a;d->masked[1]=b;d->masked[2]=c;d->masked[3]=e;d->valid|=SDCD_MASK;}
}
#define MASKS(a,b,c,d) masks(trace,a,b,c,d)
#else
#define TRACE_ARG
#define TRACE_PASS
#define NO_TRACE
#define REJECT(s,id) return s
#define MASKS(a,b,c,d) ((void)0)
#endif
static int supported(uint32_t t){return t==SDRO_V1||t==SDRO_V2||t==SDRO_SDHC;}
static sdro_status capacity_core(uint32_t type,uint64_t n TRACE_ARG){
 if(!supported(type))REJECT(SDRO_CARD_TYPE,0x0202);
 if(!n)REJECT(SDRO_CAPACITY,0x0401);
 if(n>UINT64_C(0x100000000))REJECT(SDRO_CAPACITY_OVERFLOW,0x0402);
 if(type!=SDRO_SDHC&&n>UINT64_C(0x800000))REJECT(SDRO_SDSC_OVERFLOW,0x0403);
 return SDRO_OK;
}
sdro_status sdro_validate_capacity(uint32_t type,uint64_t n){return capacity_core(type,n NO_TRACE);}
sdro_status sdro_check_lba(uint32_t type,uint64_t n,uint32_t lba){
 sdro_status s=sdro_validate_capacity(type,n);if(s!=SDRO_OK)return s;
 if(type!=SDRO_SDHC&&lba>UINT32_C(0x7fffff))return SDRO_SDSC_OVERFLOW;
 return (uint64_t)lba<n?SDRO_OK:SDRO_RANGE;
}
static int mul(uint64_t a,uint64_t b,uint64_t *out){
 if(b&&a>UINT64_MAX/b)return 0;
 *out=a*b;return 1;
}
static sdro_status decode_core(const uint8_t raw[16],uint32_t type,uint64_t *out TRACE_ARG){
 if(!raw||!out)REJECT(SDRO_ARGUMENT,0x0201);
 if(!supported(type))REJECT(SDRO_CARD_TYPE,0x0202);
 unsigned version=raw[0]>>6;
#ifdef SDRO_CSD_DIAGNOSTIC
 if(trace){trace->structure=(uint8_t)version;trace->valid|=SDCD_STRUCTURE;}
#endif
 if(version>1)REJECT(SDRO_CSD_VERSION,0x0203);
 if(!(raw[15]&1))REJECT(SDRO_CSD_MALFORMED,0x0204);
 /* Conservative supported profile, not a complete SD specification validator.
  * Reserved fields identified by local public CSD definitions must be zero. */
 if((raw[0]&0x3f)||(raw[12]&0x60)||(raw[13]&0x1f)||(raw[14]&3)){
  MASKS(raw[0]&0x3f,raw[12]&0x60,raw[13]&0x1f,raw[14]&3);
  REJECT(SDRO_PROFILE,(raw[0]&0x3f)?0x1101:(raw[12]&0x60)?0x1102:(raw[13]&0x1f)?0x1103:0x1104);
 }
 if((version==1)!=(type==SDRO_SDHC))REJECT(SDRO_MISMATCH,0x0205);
 unsigned read_len=raw[5]&15;
 /* Firmware always reads 512 bytes; no CMD16 is added by this adapter. */
 if(read_len!=9){MASKS(read_len,0,0,0);REJECT(SDRO_PROFILE,0x1201);}
 uint64_t n;
 if(version==0){
  if(raw[6]&0x0c){MASKS(raw[6]&0x0c,0,0,0);REJECT(SDRO_PROFILE,0x1301);}
  uint32_t size=((uint32_t)(raw[6]&3)<<10)|((uint32_t)raw[7]<<2)|(raw[8]>>6);
  unsigned mult=((raw[9]&3U)<<1)|(raw[10]>>7);
  if(mult+2>=64||read_len>=64)REJECT(SDRO_CAPACITY_OVERFLOW,0x0301);
  uint64_t blocks,bytes;
  if(!mul((uint64_t)size+1,UINT64_C(1)<<(mult+2),&blocks)||
     !mul(blocks,UINT64_C(1)<<read_len,&bytes))REJECT(SDRO_CAPACITY_OVERFLOW,0x0302);
  if(!bytes||bytes%512)REJECT(SDRO_CAPACITY,0x0303);
  n=bytes/512;
 }else{
  /* CSD v2 byte 10: bit 7 is reserved; bits 6..0 are erase fields. */
  if((raw[6]&15)||(raw[7]&0xc0)||(raw[10]&0x80)){
   MASKS(raw[6]&15,raw[7]&0xc0,raw[10]&0x80,0);
   REJECT(SDRO_PROFILE,(raw[6]&15)?0x1401:(raw[7]&0xc0)?0x1402:0x1403);
  }
  uint32_t size=((uint32_t)(raw[7]&0x3f)<<16)|((uint32_t)raw[8]<<8)|raw[9];
  if(!mul((uint64_t)size+1,1024,&n))REJECT(SDRO_CAPACITY_OVERFLOW,0x0304);
 }
 sdro_status s=capacity_core(type,n TRACE_PASS);if(s!=SDRO_OK)return s;
 *out=n;return SDRO_OK;
}

sdro_status sdro_decode_csd(const uint8_t raw[16],uint32_t type,uint64_t *out){return decode_core(raw,type,out NO_TRACE);}
#ifdef SDRO_CSD_DIAGNOSTIC
sdro_status sdro_decode_csd_traced(const uint8_t raw[16],uint32_t type,uint64_t *out,sdro_csd_diagnostic *trace){
 return decode_core(raw,type,out,trace);
}
#endif
