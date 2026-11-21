#include "mock_firmware.h"
#include <string.h>
mock_state mock;
static void record(unsigned selector){CHECK(selector<5);CHECK(mock.used<64);mock.calls[selector]++;mock.trace[mock.used++]=selector;}
void mock_csd_v1(uint32_t size,unsigned mult){
 memset(mock.csd,0,16);mock.csd[5]=9;mock.csd[6]=(uint8_t)((size>>10)&3);
 mock.csd[7]=(uint8_t)(size>>2);mock.csd[8]=(uint8_t)((size&3)<<6);
 mock.csd[9]=(uint8_t)((mult>>1)&3);mock.csd[10]=(uint8_t)((mult&1)<<7);mock.csd[15]=1;
}
void mock_csd_v2(uint32_t size){
 memset(mock.csd,0,16);mock.csd[0]=0x40;mock.csd[5]=9;
 mock.csd[7]=(uint8_t)((size>>16)&0x3f);mock.csd[8]=(uint8_t)(size>>8);mock.csd[9]=(uint8_t)size;mock.csd[15]=1;
}
void mock_reset(void){sdro_shutdown();memset(&mock,0,sizeof(mock));mock.support=UINT32_C(0x1234fedc);mock.type=SDRO_SDHC;mock.initialized=1;mock.reg_result=1;mock.read_result=1;mock_csd_v2(4095);}
uint32_t sdro_fw_support(void){record(0);return mock.support;}
uint32_t sdro_fw_init(sdro_fw_card *d){
 record(1);mock.last_descriptor=d->bytes;sdro_diagnostic state;CHECK(sdro_diagnostics(&state)==SDRO_OK&&state.state==SDRO_INITIALIZING);mock.init_descriptor_zero=1;for(unsigned i=0;i<22;i++)if(d->bytes[i])mock.init_descriptor_zero=0;
 CHECK((uintptr_t)d%2==0);d->bytes[0]=mock.initialized;
 for(unsigned i=0;i<4;i++)d->bytes[2+i]=(uint8_t)(mock.type>>(24-i*8));
 memset(d->bytes+6,0xa5,16);return mock.init_result;
}
uint32_t sdro_fw_csd(sdro_fw_card *d,uint8_t b[16]){
 record(4);mock.last_csd=b;mock.last_cmd=9;CHECK(d->bytes[0]==1);mock.csd_aligned=(uintptr_t)b%4==0;
 for(unsigned i=0;i<16;i++)CHECK(b[i]==0);
 if(mock.partial_csd){memset(b,0xee,7);return 0;}
 if(mock.reg_result)memcpy(b,mock.csd,16);
 return mock.reg_result;
}
uint32_t sdro_fw_read(sdro_fw_card *d,uint32_t lba,uint8_t b[512]){
 record(2);mock.last_bounce=b;CHECK(d->bytes[0]==1);mock.last_lba=lba;mock.read_aligned=(uintptr_t)b%4==0;
 if(mock.partial_read){memset(b,0xee,117);return 0;}
 if(mock.read_result)for(unsigned i=0;i<512;i++)b[i]=(uint8_t)(lba+i);
 return mock.read_result;
}
