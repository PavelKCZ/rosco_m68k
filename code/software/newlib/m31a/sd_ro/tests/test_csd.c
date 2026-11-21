#include "mock_firmware.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#define RUN(name,...) do{mock_reset();__VA_ARGS__;test_done(name);}while(0)
void test_decoder(void){
 RUN("decode_v1_formula",{mock_csd_v1(1234,4);uint64_t n=99;CHECK(sdro_decode_csd(mock.csd,SDRO_V1,&n)==SDRO_OK&&n==UINT64_C(1235)*64);});
 RUN("decode_v2_formula",{mock_csd_v2(123456);uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_OK&&n==UINT64_C(123457)*1024);});
 RUN("decode_v2_boundary",{mock_csd_v2(0x3fffff);uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_OK&&n==(UINT64_C(1)<<32));});
 RUN("decode_v1_minimum",{mock_csd_v1(0,0);uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_V1,&n)==SDRO_OK&&n==4);});
 RUN("decode_v2_minimum",{mock_csd_v2(0);uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_OK&&n==1024);});
 for(unsigned version=2;version<4;version++){mock_reset();mock.csd[0]=(uint8_t)(version<<6);uint64_t n=77;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_CSD_VERSION&&n==77);char name[40];(void)snprintf(name,sizeof(name),"unsupported_csd_version_%u",version);test_done(name);}
 RUN("decode_end_bit",{mock.csd[15]=0xfe;uint64_t n=77;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_CSD_MALFORMED&&n==77);});
 RUN("decode_null",{uint64_t n;CHECK(sdro_decode_csd(NULL,SDRO_SDHC,&n)==SDRO_ARGUMENT);CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,NULL)==SDRO_ARGUMENT);});
 RUN("decode_unknown_type",{uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_UNKNOWN,&n)==SDRO_CARD_TYPE);});
 RUN("v1_on_sdhc_mismatch",{mock_csd_v1(1,1);uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_MISMATCH);});
 RUN("v2_on_sdsc_mismatch",{uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_V2,&n)==SDRO_MISMATCH);});
 for(unsigned len=0;len<16;len++)if(len!=9){mock_reset();mock.csd[5]=(uint8_t)len;uint64_t n=77;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_PROFILE&&n==77);char name[40];(void)snprintf(name,sizeof(name),"v2_block_profile_%u",len);test_done(name);}
 RUN("sdsc_long_block_profile",{mock_csd_v1(4095,7);mock.csd[5]=11;uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_V1,&n)==SDRO_PROFILE);});
 const unsigned positions[]={0,6,7,10,12,13,14};const uint8_t masks[]={1,1,0x80,0x80,0x20,1,1};
 for(unsigned i=0;i<7;i++){mock_reset();mock.csd[positions[i]]|=masks[i];uint64_t n=77;CHECK(sdro_decode_csd(mock.csd,SDRO_SDHC,&n)==SDRO_PROFILE&&n==77);char name[40];(void)snprintf(name,sizeof(name),"reserved_profile_%u",i);test_done(name);}
 RUN("v1_reserved_profile",{mock_csd_v1(2,2);mock.csd[6]|=4;uint64_t n;CHECK(sdro_decode_csd(mock.csd,SDRO_V1,&n)==SDRO_PROFILE);});
 RUN("zero_capacity",{CHECK(sdro_validate_capacity(SDRO_SDHC,0)==SDRO_CAPACITY);});
 RUN("oversized_capacity",{CHECK(sdro_validate_capacity(SDRO_SDHC,UINT64_C(0x100000001))==SDRO_CAPACITY_OVERFLOW);CHECK(sdro_validate_capacity(SDRO_SDHC,UINT64_MAX)==SDRO_CAPACITY_OVERFLOW);});
 RUN("sdsc_capacity_exact_address_limit",{CHECK(sdro_validate_capacity(SDRO_V1,0x800000)==SDRO_OK);});
 RUN("sdsc_capacity_exceeds_limit",{CHECK(sdro_validate_capacity(SDRO_V1,0x800001)==SDRO_SDSC_OVERFLOW);});
 RUN("sdsc_last_addressable_lba",{CHECK(sdro_check_lba(SDRO_V1,0x800000,0x7fffff)==SDRO_OK);CHECK(sdro_check_lba(SDRO_V2,0x800000,0x800000)==SDRO_SDSC_OVERFLOW);});
 RUN("lba_capacity_zero",{CHECK(sdro_check_lba(SDRO_SDHC,0,0)==SDRO_CAPACITY);});
 RUN("lba_sdsc_range",{CHECK(sdro_check_lba(SDRO_V1,100,99)==SDRO_OK);CHECK(sdro_check_lba(SDRO_V1,100,100)==SDRO_RANGE);});
 RUN("lba_sdhc_full_range",{CHECK(sdro_check_lba(SDRO_SDHC,UINT64_C(0x100000000),UINT32_MAX)==SDRO_OK);});
 RUN("lba_unknown_card",{CHECK(sdro_check_lba(SDRO_UNKNOWN,100,0)==SDRO_CARD_TYPE);});
}
