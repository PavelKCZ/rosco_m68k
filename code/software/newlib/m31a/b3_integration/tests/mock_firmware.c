#include "mock_firmware.h"
#include <string.h>
mock_state mock;
static void call(unsigned n){if(n!=0&&n!=1&&n!=2&&n!=4){mock.forbidden++;CHECK(0);}CHECK(mock.count<4096);mock.sequence[mock.count++]=n;mock.calls[n]++;}
void mock_setup(int bits,int partition,int sdsc,int huge){
 b3_shutdown();fixture_create(bits,partition,sdsc,huge);memset(&mock,0,sizeof(mock));mock.support=0x1234fedc;mock.initialized=1;mock.type=sdsc?SDRO_V1:SDRO_SDHC;mock.csd_result=1;mock.csd[5]=9;mock.csd[15]=1;
 if(sdsc){uint32_t csize=2047;mock.csd[6]=(uint8_t)(csize>>10);mock.csd[7]=(uint8_t)(csize>>2);mock.csd[8]=(uint8_t)(csize<<6);}else{uint32_t csize=(uint32_t)(disk.capacity/1024-1);mock.csd[0]=0x40;mock.csd[7]=(uint8_t)(csize>>16);mock.csd[8]=(uint8_t)(csize>>8);mock.csd[9]=(uint8_t)csize;}
}
uint32_t sdro_fw_support(void){call(0);return mock.support;}
uint32_t sdro_fw_init(sdro_fw_card *d){call(1);for(unsigned i=0;i<22;i++)CHECK(d->bytes[i]==0);d->bytes[0]=(uint8_t)mock.initialized;for(unsigned i=0;i<4;i++)d->bytes[2+i]=(uint8_t)(mock.type>>(24-i*8));return mock.init_status;}
uint32_t sdro_fw_csd(sdro_fw_card *d,uint8_t b[16]){call(4);CHECK(d->bytes[0]==1);CHECK((uintptr_t)b%4==0);if(mock.partial_csd){memset(b,0xee,7);return 0;}if(mock.csd_result)memcpy(b,mock.csd,16);return mock.csd_result;}
uint32_t sdro_fw_read(sdro_fw_card *d,uint32_t lba,uint8_t b[512]){call(2);CHECK(d->bytes[0]==1);CHECK((uintptr_t)b%4==0);mock.last_lba=lba;return (uint32_t)fixture_read(lba,b);}
