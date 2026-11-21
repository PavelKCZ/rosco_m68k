/* Real B1/B2/B3; replace only the firmware boundary. Include the ONE decoder
 * implementation here to exercise otherwise unreachable defensive guards. */
#include "mock_firmware.h"
#include "b3_integration.h"
#include "b3_csd_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../sd_ro/src/csd_decode.c"
#ifdef SDRO_CSD_DIAGNOSTIC
#include "../../b3_executable/src/csd_format.h"
#endif
static unsigned cases;
void test_check(int ok,const char *s,int n){if(!ok){fprintf(stderr,"FAIL %d %s\n",n,s);exit(1);}}
void test_done(const char *s){CHECK(mock.calls[3]==0);printf("PASS %s\n",s);cases++;}
static void result(const char *name,sdro_status expected){
 sdro_status s=sdro_init();CHECK(s==expected);CHECK(mock.calls[3]==0);
 printf("COMPAT %s status=%u trace=",name,(unsigned)s);
 for(unsigned i=0;i<mock.used;i++)printf("%u",mock.trace[i]);
 puts("");test_done(name);
}
#ifdef SDRO_CSD_DIAGNOSTIC
static sdro_csd_diagnostic snap(void){sdro_csd_diagnostic d;CHECK(sdro_csd_snapshot(&d)==1);return d;}
static void reason(const char *name,uint16_t id){
 uint8_t raw[16];memcpy(raw,mock.csd,16);result(name,SDRO_PROFILE);
 sdro_csd_diagnostic d=snap();CHECK(d.reject_id==id&&d.final_status==SDRO_PROFILE);
 CHECK(d.stage==5&&(d.valid&(SDCD_CSD|SDCD_STRUCTURE|SDCD_MASK|SDCD_FINAL|SDCD_REJECT))==(SDCD_CSD|SDCD_STRUCTURE|SDCD_MASK|SDCD_FINAL|SDCD_REJECT));
 CHECK(memcmp(d.raw_csd,raw,16)==0);CHECK(mock.calls[2]==0);
 sdro_shutdown();sdro_shutdown();sdro_csd_diagnostic after=snap();CHECK(memcmp(&d,&after,48)==0);
}
#endif
int main(void){
#ifdef SDRO_CSD_DIAGNOSTIC
 sdro_csd_diagnostic d;memset(&d,0xa5,sizeof(d));CHECK(sdro_csd_snapshot(&d)==0);
 for(unsigned i=0;i<sizeof(d);i++)CHECK(((uint8_t *)&d)[i]==0);
 CHECK(sdro_csd_snapshot(NULL)==-1&&b3_csd_snapshot(NULL)==-1);test_done("getter_initial_null_zero");
#endif

 const uint8_t physical[16]={0x40,0x0e,0,0x32,0xdb,0x79,0,0,0xed,0xc8,0x7f,0x80,0x0a,0x40,0x40,0xe5};
 mock_reset();memcpy(mock.csd,physical,16);result("physical_csd_accepted",SDRO_OK);
 uint64_t physical_capacity=0;CHECK(sdro_capacity(&physical_capacity)==SDRO_OK&&physical_capacity==UINT64_C(62333952));
#ifdef SDRO_CSD_DIAGNOSTIC
 d=snap();CHECK(d.structure==1&&d.reject_id==0&&!(d.valid&SDCD_REJECT)&&memcmp(d.raw_csd,physical,16)==0);
#endif
 mock_reset();memcpy(mock.csd,physical,16);mock.csd[10]|=0x80;result("physical_reserved_rejected",SDRO_PROFILE);
#ifdef SDRO_CSD_DIAGNOSTIC
 d=snap();CHECK(d.reject_id==0x1403&&d.masked[2]==0x80);
#endif
 const unsigned sectors[]={0,1,63,64,127};
 for(unsigned e=0;e<2;e++)for(unsigned i=0;i<5;i++){
  mock_reset();memcpy(mock.csd,physical,16);mock.csd[10]=(uint8_t)((e<<6)|(sectors[i]>>1));mock.csd[11]=(uint8_t)((sectors[i]&1)<<7);
  char name[48];(void)snprintf(name,sizeof(name),"erase_%u_sector_%u",e,sectors[i]);result(name,SDRO_OK);
  CHECK(sdro_capacity(&physical_capacity)==SDRO_OK&&physical_capacity==UINT64_C(62333952));
 }
 mock_reset();mock.type=SDRO_V1;mock_csd_v1(1234,5);result("v1_byte10_bit7_valid",SDRO_OK);
 mock_reset();result("v2_ok",SDRO_OK);
 mock_reset();mock.type=SDRO_V1;mock_csd_v1(1234,4);result("v1_ok",SDRO_OK);
 mock_reset();mock.support=0;result("support_failure",SDRO_NO_FIRMWARE);CHECK(mock.calls[1]==0&&mock.calls[4]==0);
 mock_reset();mock.init_result=3;result("init_failure",SDRO_INIT_FAILURE);
 mock_reset();mock.initialized=0;result("false_ok",SDRO_NOT_INITIALIZED);
 mock_reset();mock.initialized=2;result("invalid_initialized",SDRO_NOT_INITIALIZED);
 mock_reset();mock.type=UINT32_MAX;result("invalid_type",SDRO_CARD_TYPE);
 mock_reset();mock.reg_result=0;result("cmd9_failure",SDRO_CSD_READ);
 mock_reset();mock.partial_csd=1;result("cmd9_partial",SDRO_CSD_READ);
#ifdef SDRO_CSD_DIAGNOSTIC
 d=snap();CHECK(!(d.valid&(SDCD_CSD|SDCD_STRUCTURE|SDCD_MASK))&&d.cmd9_raw==0&&d.reject_id==0x0105);
 for(unsigned i=0;i<16;i++)CHECK(d.raw_csd[i]==0);
#endif
 mock_reset();mock.csd[0]=0x80;result("unsupported_structure",SDRO_CSD_VERSION);
 mock_reset();mock.csd[15]=0;result("end_bit",SDRO_CSD_MALFORMED);
 mock_reset();mock.type=SDRO_V1;result("type_structure_mismatch",SDRO_MISMATCH);
 mock_reset();mock.csd[10]=0x40;result("erase_characterization",SDRO_OK);
 mock_reset();mock.csd[10]=0x10;result("sector_bit4_characterization",SDRO_OK);
 mock_reset();mock.csd[10]=0x20;result("sector_bit5_characterization",SDRO_OK);
 mock_reset();mock.csd[10]=0x0f;result("sector_lowbits_accepted",SDRO_OK);
 mock_reset();CHECK(sdro_validate_capacity(SDRO_SDHC,0)==SDRO_CAPACITY);test_done("zero_capacity_guard");
 CHECK(sdro_validate_capacity(SDRO_SDHC,UINT64_C(0x100000001))==SDRO_CAPACITY_OVERFLOW);test_done("capacity_overflow_guard");
 CHECK(sdro_validate_capacity(SDRO_V1,0x800001)==SDRO_SDSC_OVERFLOW);test_done("sdsc_capacity_guard");
#ifdef SDRO_CSD_DIAGNOSTIC
 const unsigned at[]={0,12,13,14,5,6,6,7,10};
 const uint8_t mask[]={1,0x20,1,1,10,4,1,0x80,0x80};
 const uint16_t ids[]={0x1101,0x1102,0x1103,0x1104,0x1201,0x1301,0x1401,0x1402,0x1403};
 for(unsigned i=0;i<9;i++){
  mock_reset();if(i==5){mock.type=SDRO_V1;mock_csd_v1(100,2);}
  if(i==4)mock.csd[5]=mask[i];else mock.csd[at[i]]|=mask[i];
  char name[40];(void)snprintf(name,sizeof(name),"predicate_%04X",ids[i]);reason(name,ids[i]);
 }
 mock_reset();mock.csd[0]|=1;mock.csd[12]|=0x20;mock.csd[5]=10;mock.csd[10]=0xf0;reason("common_precedence",0x1101);
 mock_reset();mock.csd[12]|=0x20;mock.csd[13]|=1;reason("common_second_precedence",0x1102);
 mock_reset();mock.csd[13]|=1;mock.csd[14]|=1;reason("common_third_precedence",0x1103);
 mock_reset();mock.csd[5]=10;mock.csd[6]|=1;reason("block_before_v2",0x1201);
 mock_reset();mock.csd[6]|=1;mock.csd[7]|=0x80;mock.csd[10]=0xf0;reason("v2_first_precedence",0x1401);
 mock_reset();mock.csd[7]|=0x80;mock.csd[10]=0xf0;reason("v2_second_precedence",0x1402);
 mock_reset();mock.csd[10]=0x80;reason("reserved_bit_id",0x1403);
 mock_reset();mock.csd[10]=0xf0;reason("reserved_with_erase_id",0x1403);
 mock_reset();mock.type=SDRO_V1;mock.csd[0]|=1;reason("common_before_mismatch",0x1101);
 mock_reset();mock.csd[10]=0xf0;CHECK(sdro_init()==SDRO_PROFILE);d=snap();CHECK(d.masked[2]==0x80);
 d.raw_csd[0]=0xff;CHECK(snap().raw_csd[0]==0x40);test_done("copy_not_mutable_pointer");
 mock_reset();mock.support=0;CHECK(sdro_init()==SDRO_NO_FIRMWARE);d=snap();
 CHECK(d.valid==(SDCD_ATTEMPT|SDCD_SUPPORT|SDCD_REJECT|SDCD_FINAL)&&d.stage==2&&d.reject_id==0x0101);
 for(unsigned i=0;i<16;i++){CHECK(d.raw_csd[i]==0);}
 test_done("new_attempt_no_stale_csd");
 b3_shutdown();mock_reset();mock.csd[10]=0x80;CHECK(b3_init()==B3_SD_ERROR);
 b3_diagnostic bd;CHECK(b3_diagnostics(&bd)==B3_OK&&bd.phase==B3_SD_INIT&&bd.sd_status==SDRO_PROFILE);
 CHECK(b3_csd_snapshot(&d)==1&&d.reject_id==0x1403&&mock.calls[2]==0);sdro_csd_diagnostic saved=d;
 b3_shutdown();b3_shutdown();CHECK(b3_csd_snapshot(&d)==1&&memcmp(&saved,&d,48)==0);test_done("b3_cleanup_snapshot_no_probe");
 struct {uint32_t type;uint64_t n;sdro_status status;uint16_t id;} cap[]={
 {3,1,SDRO_CARD_TYPE,0x0202},{2,0,SDRO_CAPACITY,0x0401},
 {2,UINT64_C(0x100000001),SDRO_CAPACITY_OVERFLOW,0x0402},{0,0x800001,SDRO_SDSC_OVERFLOW,0x0403}};
 for(unsigned i=0;i<4;i++){memset(&d,0,sizeof(d));CHECK(capacity_core(cap[i].type,cap[i].n,&d)==cap[i].status&&d.reject_id==cap[i].id);char n[30];(void)snprintf(n,sizeof(n),"traced_capacity_%u",i);test_done(n);}
 uint64_t n=77;memset(&d,0,sizeof(d));CHECK(sdro_decode_csd_traced(NULL,2,&n,&d)==SDRO_ARGUMENT&&d.reject_id==0x0201&&n==77);test_done("traced_null_decoder");
 const unsigned modes[]={0,1,2};
 for(unsigned i=0;i<3;i++){
  mock_reset();if(i==0)mock.csd[0]=0x80;if(i==1)mock.csd[15]=0;if(i==2)mock.type=SDRO_V1;
  mock.csd[10]=0xf0;sdro_status st=sdro_init();d=snap();CHECK(st==(i==0?SDRO_CSD_VERSION:i==1?SDRO_CSD_MALFORMED:SDRO_MISMATCH));
  CHECK(d.reject_id==(i==0?0x0203:i==1?0x0204:0x0205));char name[30];(void)snprintf(name,sizeof(name),"early_precedence_%u",modes[i]);test_done(name);
 }
 const uint16_t failids[]={0x0101,0x0102,0x0103,0x0104,0x0105};
 for(unsigned i=0;i<5;i++){
  mock_reset();if(i==0)mock.support=0x98765432;if(i==1)mock.init_result=0x87654321;
  if(i==2)mock.initialized=2;
  if(i==3)mock.type=0xaabbccdd;
  if(i==4)mock.reg_result=0;
  CHECK(sdro_init()!=SDRO_OK);d=snap();CHECK(d.reject_id==failids[i]&&(d.valid&SDCD_FINAL));
  if(i==0){CHECK(d.support_raw==0x98765432&&!(d.valid&SDCD_INIT));}
  else{CHECK((d.valid&(SDCD_INIT|SDCD_DESCRIPTOR))==(SDCD_INIT|SDCD_DESCRIPTOR));CHECK(d.initialized_raw==mock.initialized&&d.type_raw==mock.type&&d.init_raw==mock.init_result);}
  if(i<4){CHECK(!(d.valid&SDCD_CMD9)&&mock.calls[4]==0);}else{CHECK((d.valid&SDCD_CMD9)&&d.cmd9_raw==0&&!(d.valid&SDCD_CSD));}
  char name[32];(void)snprintf(name,sizeof(name),"failure_capture_%04X",failids[i]);test_done(name);
 }
 mock_reset();mock.reg_result=0x12345678;CHECK(sdro_init()==SDRO_OK);d=snap();CHECK(d.cmd9_raw==0x12345678&&d.reject_id==0&&d.final_status==0);test_done("raw_cmd9_nonboolean_retained");
 sdro_csd_diagnostic bd_copy;CHECK(b3_csd_snapshot(&bd_copy)==1&&memcmp(&bd_copy,&d,48)==0);test_done("b3_copy_success_after_shutdown_history");
 char text[CSDD_BUFFER_SIZE+8];memset(&d,0,sizeof(d));memset(text,'!',sizeof(text));
 CHECK(csdd_format(&d,text,CSDD_BUFFER_SIZE));CHECK(strlen(text)==CSDD_TEXT_LENGTH&&text[CSDD_BUFFER_SIZE]=='!');
 CHECK(strstr(text,"SUP=-------- OK=-")&&strstr(text,"RAW=-------------------------------- STRUCT=--"));test_done("format_unavailable_length_bounds");
 mock_reset();mock.csd[10]=0xf0;CHECK(sdro_init()==SDRO_PROFILE);d=snap();
 CHECK(csdd_format(&d,text,sizeof(text))&&strlen(text)==CSDD_TEXT_LENGTH);
 CHECK(strstr(text,"REJ=1403 MASK=00,00,80,--")&&strstr(text,"STATUS=0008")&&strstr(text,"STRUCT=01"));test_done("format_profile_exact");
 CHECK(!csdd_format(&d,text,CSDD_BUFFER_SIZE-1)&&text[0]==0);CHECK(!csdd_format(&d,NULL,10));test_done("format_small_null");
 mock_reset();CHECK(sdro_init()==SDRO_OK);d=snap();CHECK(csdd_format(&d,text,sizeof(text))&&strstr(text,"REJ=0000 MASK=--,--,--,--"));test_done("format_success");
 printf("LAYOUT size=%zu align=%zu raw=%zu masked=%zu TEXT_LF=%u TEXT_CRLF=%u\n",sizeof(d),_Alignof(sdro_csd_diagnostic),offsetof(sdro_csd_diagnostic,raw_csd),offsetof(sdro_csd_diagnostic,masked),(unsigned)CSDD_TEXT_LENGTH,(unsigned)CSDD_CRLF_LENGTH);
#endif
 printf("SUMMARY unique=%u pass=%u fail=0 skip=0 xfail=0 writes=0\n",cases,cases);return 0;
}
