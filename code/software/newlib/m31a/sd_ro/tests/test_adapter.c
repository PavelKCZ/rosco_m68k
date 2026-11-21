#include "mock_firmware.h"
#include "fat_ro.h" /* compile-time callback compatibility only; no B1 object linked */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned test_count;
void test_check(int ok,const char *s,int line){if(!ok){fprintf(stderr,"FAIL line%d: %s\n",line,s);exit(1);}}
void test_done(const char *name){
 CHECK(mock.calls[3]==0);
 for(unsigned i=0;i<mock.used;i++)CHECK(mock.trace[i]==0||mock.trace[i]==1||mock.trace[i]==2||mock.trace[i]==4);
 printf("PASS %s\n",name);test_count++;
}
static sdro_diagnostic diag(void){sdro_diagnostic d;CHECK(sdro_diagnostics(&d)==SDRO_OK);return d;}
static void notready(void){uint64_t n=88;sdro_type t=SDRO_V1;CHECK(sdro_capacity(&n)==SDRO_STATE&&n==88);CHECK(sdro_card_type(&t)==SDRO_STATE&&t==SDRO_V1);CHECK(diag().sectors==0);}
static void output(uint32_t lba){uint8_t b[512];memset(b,0xaa,512);CHECK(sdro_read(sdro_context(),lba,b)==1);for(unsigned i=0;i<512;i++)CHECK(b[i]==(uint8_t)(lba+i));CHECK(mock.last_lba==lba);}
static void unchanged(const uint8_t *b){for(unsigned i=0;i<512;i++)CHECK(b[i]==0xaa);}
#define RUN(name,...) do{mock_reset();__VA_ARGS__;test_done(name);}while(0)
int main(void){
 RUN("initial_state",{notready();CHECK(diag().state==SDRO_UNINITIALIZED);CHECK(diag().firmware_init_status==UINT32_MAX);});
 RUN("support_sequence",{CHECK(sdro_init()==SDRO_OK);CHECK(mock.used==3&&mock.trace[0]==0&&mock.trace[1]==1&&mock.trace[2]==4);CHECK(mock.last_cmd==9);CHECK(mock.calls[2]==0);});
 RUN("support_mismatch",{mock.support=0;CHECK(sdro_init()==SDRO_NO_FIRMWARE);CHECK(mock.calls[1]==0&&mock.calls[4]==0);CHECK(diag().state==SDRO_FAILED);notready();});
 RUN("support_other_value",{mock.support=UINT32_MAX;CHECK(sdro_init()==SDRO_NO_FIRMWARE);CHECK(mock.used==1);});
 for(uint32_t code=1;code<=3;code++){mock_reset();mock.init_result=code;CHECK(sdro_init()==SDRO_INIT_FAILURE);CHECK(diag().firmware_init_status==code);CHECK(mock.calls[4]==0);notready();char n[40];(void)snprintf(n,sizeof(n),"init_status_%u",code);test_done(n);}
 RUN("init_unknown_failure_status",{mock.init_result=UINT32_MAX;CHECK(sdro_init()==SDRO_INIT_FAILURE);CHECK(diag().firmware_init_status==UINT32_MAX);});
 RUN("init_false_ok",{mock.initialized=0;CHECK(sdro_init()==SDRO_NOT_INITIALIZED);CHECK(diag().firmware_init_status==0);CHECK(mock.calls[4]==0);notready();});
 RUN("init_noncanonical_boolean",{mock.initialized=2;CHECK(sdro_init()==SDRO_NOT_INITIALIZED);});
 RUN("unknown_card",{mock.type=SDRO_UNKNOWN;CHECK(sdro_init()==SDRO_CARD_TYPE);CHECK(mock.calls[4]==0);CHECK(diag().reported_type==SDRO_UNKNOWN);});
 RUN("invalid_card_enum",{mock.type=UINT32_MAX;CHECK(sdro_init()==SDRO_CARD_TYPE);CHECK(diag().reported_type==UINT32_MAX);});
 RUN("sdhc_capacity",{CHECK(sdro_init()==SDRO_OK);uint64_t n=0;sdro_type t=SDRO_UNKNOWN;CHECK(sdro_capacity(&n)==SDRO_OK&&n==UINT64_C(4194304));CHECK(sdro_card_type(&t)==SDRO_OK&&t==SDRO_SDHC);CHECK(mock.init_descriptor_zero&&mock.csd_aligned);});
 RUN("sdsc_v1",{mock.type=SDRO_V1;mock_csd_v1(1023,3);CHECK(sdro_init()==SDRO_OK);uint64_t n;CHECK(sdro_capacity(&n)==SDRO_OK&&n==32768);output(0);});
 RUN("sdsc_v2_standard_capacity",{mock.type=SDRO_V2;mock_csd_v1(4095,7);CHECK(sdro_init()==SDRO_OK);uint64_t n;CHECK(sdro_capacity(&n)==SDRO_OK&&n==UINT64_C(2097152));output(2097151);});
 RUN("sdhc_maximum",{mock_csd_v2(0x3fffff);CHECK(sdro_init()==SDRO_OK);uint64_t n;CHECK(sdro_capacity(&n)==SDRO_OK&&n==UINT64_C(4294967296));output(UINT32_MAX);});
 RUN("repeat_initialization_clears_descriptor",{CHECK(sdro_init()==SDRO_OK);output(0);CHECK(sdro_init()==SDRO_OK);CHECK(mock.init_descriptor_zero);CHECK(mock.calls[0]==2&&mock.calls[1]==2&&mock.calls[4]==2);});
 RUN("failed_reinitialization_clears_capacity",{CHECK(sdro_init()==SDRO_OK);mock.support=0;CHECK(sdro_init()==SDRO_NO_FIRMWARE);notready();uint8_t b[512];CHECK(sdro_read(sdro_context(),0,b)==0);CHECK(mock.calls[2]==0);});
 RUN("shutdown_reinitialize",{CHECK(sdro_init()==SDRO_OK);unsigned calls=mock.used;sdro_shutdown();sdro_shutdown();CHECK(mock.used==calls);notready();CHECK(diag().state==SDRO_UNINITIALIZED);CHECK(sdro_init()==SDRO_OK&&mock.init_descriptor_zero);});
 RUN("csd_failure",{mock.reg_result=0;CHECK(sdro_init()==SDRO_CSD_READ);notready();CHECK(mock.calls[4]==1);});
 RUN("partial_csd_discarded",{mock.partial_csd=1;CHECK(sdro_init()==SDRO_CSD_READ);notready();mock.partial_csd=0;CHECK(sdro_init()==SDRO_OK);CHECK(mock.calls[4]==2);});
 RUN("nonzero_csd_success",{mock.reg_result=UINT32_MAX;CHECK(sdro_init()==SDRO_OK);});
 RUN("malformed_csd_init",{mock.csd[15]=0;CHECK(sdro_init()==SDRO_CSD_MALFORMED);notready();});
 RUN("mismatch_init",{mock.type=SDRO_V1;CHECK(sdro_init()==SDRO_MISMATCH);notready();});
 RUN("unsupported_profile_init",{mock.csd[5]=10;CHECK(sdro_init()==SDRO_PROFILE);notready();});
 RUN("read_first",{CHECK(sdro_init()==SDRO_OK);output(0);CHECK(mock.read_aligned);});
 RUN("read_last",{CHECK(sdro_init()==SDRO_OK);output(4194303);});
 RUN("equal_capacity",{CHECK(sdro_init()==SDRO_OK);uint8_t b[512];memset(b,0xaa,512);CHECK(sdro_read(sdro_context(),4194304,b)==0);CHECK(diag().error==SDRO_RANGE&&mock.calls[2]==0);unchanged(b);});
 RUN("above_capacity",{CHECK(sdro_init()==SDRO_OK);uint8_t b[512];CHECK(sdro_read(sdro_context(),UINT32_MAX,b)==0);CHECK(diag().error==SDRO_RANGE&&mock.calls[2]==0);});
 RUN("sdsc_first_overflow",{mock.type=SDRO_V1;mock_csd_v1(4095,7);CHECK(sdro_init()==SDRO_OK);uint8_t b[512];CHECK(sdro_read(sdro_context(),0x800000,b)==0);CHECK(diag().error==SDRO_SDSC_OVERFLOW&&mock.calls[2]==0);});
 RUN("sdsc_uint32_max",{mock.type=SDRO_V1;mock_csd_v1(4095,7);CHECK(sdro_init()==SDRO_OK);uint8_t b[512];CHECK(sdro_read(sdro_context(),UINT32_MAX,b)==0);CHECK(diag().error==SDRO_SDSC_OVERFLOW&&mock.calls[2]==0);});
 RUN("repeat_read",{CHECK(sdro_init()==SDRO_OK);output(12);output(12);CHECK(mock.calls[2]==2);});
 RUN("null_destination",{CHECK(sdro_init()==SDRO_OK);CHECK(sdro_read(sdro_context(),0,NULL)==0);CHECK(diag().error==SDRO_ARGUMENT&&mock.calls[2]==0);});
 RUN("invalid_context",{CHECK(sdro_init()==SDRO_OK);uint8_t b[512];CHECK(sdro_read(NULL,0,b)==0);int x=0;CHECK(sdro_read(&x,0,b)==0);CHECK(mock.calls[2]==0);});
 RUN("read_before_init",{uint8_t b[512];CHECK(sdro_read(sdro_context(),0,b)==0);CHECK(diag().error==SDRO_STATE&&mock.used==0);});
 RUN("read_after_shutdown",{CHECK(sdro_init()==SDRO_OK);void *old=sdro_context();sdro_shutdown();uint8_t b[512];CHECK(sdro_read(old,0,b)==0);CHECK(mock.calls[2]==0);});
 RUN("read_failure_preserves_output",{CHECK(sdro_init()==SDRO_OK);mock.read_result=0;uint8_t b[512];memset(b,0xaa,512);CHECK(sdro_read(sdro_context(),0,b)==0);unchanged(b);CHECK(diag().error==SDRO_READ_FAILURE&&diag().state==SDRO_READY);CHECK(mock.calls[2]==1);});
 RUN("partial_bounce_failure",{CHECK(sdro_init()==SDRO_OK);output(12);mock.partial_read=1;uint8_t b[512];memset(b,0xaa,512);CHECK(sdro_read(sdro_context(),1,b)==0);unchanged(b);CHECK(mock.calls[2]==2);mock.partial_read=0;output(1);CHECK(diag().error==SDRO_OK);});
 RUN("nonzero_read_normalized",{CHECK(sdro_init()==SDRO_OK);mock.read_result=UINT32_MAX;output(0);});
 RUN("null_query_outputs",{CHECK(sdro_capacity(NULL)==SDRO_ARGUMENT);CHECK(sdro_card_type(NULL)==SDRO_ARGUMENT);CHECK(sdro_diagnostics(NULL)==SDRO_ARGUMENT);});
 RUN("b1_callback_type",{b1_sector_read callback=sdro_read;CHECK(sdro_init()==SDRO_OK);uint8_t b[512];CHECK(callback(sdro_context(),0,b)==1);});
 RUN("failed_init_zeroes_descriptor",{mock.init_result=2;CHECK(sdro_init()==SDRO_INIT_FAILURE);CHECK(mock.last_descriptor);for(unsigned i=0;i<22;i++)CHECK(mock.last_descriptor[i]==0);});
 RUN("failed_csd_clears_scratch_and_descriptor",{mock.partial_csd=1;CHECK(sdro_init()==SDRO_CSD_READ);for(unsigned i=0;i<16;i++)CHECK(mock.last_csd[i]==0);for(unsigned i=0;i<22;i++)CHECK(mock.last_descriptor[i]==0);});
 RUN("successful_csd_scratch_cleared",{CHECK(sdro_init()==SDRO_OK);for(unsigned i=0;i<16;i++)CHECK(mock.last_csd[i]==0);});
 RUN("failed_read_clears_bounce",{CHECK(sdro_init()==SDRO_OK);mock.partial_read=1;uint8_t b[512];CHECK(sdro_read(sdro_context(),0,b)==0);CHECK(mock.last_bounce);for(unsigned i=0;i<512;i++)CHECK(mock.last_bounce[i]==0);});
 RUN("shutdown_zeroes_acquired_resources",{CHECK(sdro_init()==SDRO_OK);output(0);sdro_shutdown();for(unsigned i=0;i<22;i++)CHECK(mock.last_descriptor[i]==0);for(unsigned i=0;i<512;i++)CHECK(mock.last_bounce[i]==0);});
 test_decoder();printf("SUMMARY unique=%u pass=%u fail=0 skip=0 xfail=0 forbidden_calls=0\n",test_count,test_count);return 0;
}
