#include "private.h"
#include <string.h>
#ifdef SDRO_CSD_DIAGNOSTIC
static sdro_csd_diagnostic csd_evidence;
int sdro_csd_snapshot(sdro_csd_diagnostic *out){
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!(csd_evidence.valid&SDCD_ATTEMPT))return 0;
 *out=csd_evidence;return 1;
}
static void evidence_final(sdro_status s,uint16_t reason){
 if(reason && !(csd_evidence.valid&SDCD_REJECT)){
  csd_evidence.reject_id=reason;csd_evidence.valid|=SDCD_REJECT;
 }
 csd_evidence.final_status=(uint16_t)s;csd_evidence.valid|=SDCD_FINAL;
}
#define EVIDENCE_FINAL(s,r) evidence_final(s,r)
#else
#define EVIDENCE_FINAL(s,r) ((void)0)
#endif
#define SUPPORT UINT32_C(0x1234fedc)
static struct {
 sdro_fw_card card;
 _Alignas(4) uint8_t csd[16];
 _Alignas(4) uint8_t bounce[512];
 sdro_diagnostic diag;
} adapter={.diag={SDRO_UNINITIALIZED,SDRO_OK,UINT32_MAX,SDRO_UNKNOWN,0}};
static uint32_t descriptor_type(void){
 const uint8_t *p=adapter.card.bytes+2;
 return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
void sdro_shutdown(void){
 memset(&adapter,0,sizeof(adapter));
 adapter.diag.firmware_init_status=UINT32_MAX;
 adapter.diag.reported_type=SDRO_UNKNOWN;
}
static sdro_status failed(sdro_status s){
 memset(&adapter.card,0,sizeof(adapter.card));
 memset(adapter.csd,0,sizeof(adapter.csd));memset(adapter.bounce,0,sizeof(adapter.bounce));
 EVIDENCE_FINAL(s,0);
 adapter.diag.sectors=0;adapter.diag.state=SDRO_FAILED;adapter.diag.error=s;return s;
}
sdro_status sdro_init(void){
 sdro_shutdown();adapter.diag.state=SDRO_INITIALIZING;
#ifdef SDRO_CSD_DIAGNOSTIC
 memset(&csd_evidence,0,sizeof(csd_evidence));csd_evidence.schema=1;
 csd_evidence.valid=SDCD_ATTEMPT;csd_evidence.stage=1;
#endif
 uint32_t support=sdro_fw_support();
#ifdef SDRO_CSD_DIAGNOSTIC
 csd_evidence.support_raw=support;csd_evidence.valid|=SDCD_SUPPORT;csd_evidence.stage=2;
#endif
 if(support!=SUPPORT){EVIDENCE_FINAL(SDRO_NO_FIRMWARE,0x0101);return failed(SDRO_NO_FIRMWARE);}
 adapter.diag.firmware_init_status=sdro_fw_init(&adapter.card);
 #ifdef SDRO_CSD_DIAGNOSTIC
 csd_evidence.init_raw=adapter.diag.firmware_init_status;
 csd_evidence.initialized_raw=adapter.card.bytes[0];csd_evidence.type_raw=descriptor_type();
 csd_evidence.valid|=SDCD_INIT|SDCD_DESCRIPTOR;csd_evidence.stage=3;
#endif
 if(adapter.diag.firmware_init_status!=0){EVIDENCE_FINAL(SDRO_INIT_FAILURE,0x0102);return failed(SDRO_INIT_FAILURE);}
 if(adapter.card.bytes[0]!=1){EVIDENCE_FINAL(SDRO_NOT_INITIALIZED,0x0103);return failed(SDRO_NOT_INITIALIZED);}
 adapter.diag.reported_type=descriptor_type();
 if(adapter.diag.reported_type>SDRO_SDHC){EVIDENCE_FINAL(SDRO_CARD_TYPE,0x0104);return failed(SDRO_CARD_TYPE);}
 uint32_t cmd9=sdro_fw_csd(&adapter.card,adapter.csd);
#ifdef SDRO_CSD_DIAGNOSTIC
 csd_evidence.cmd9_raw=cmd9;csd_evidence.valid|=SDCD_CMD9;csd_evidence.stage=4;
#endif
 if(!cmd9){EVIDENCE_FINAL(SDRO_CSD_READ,0x0105);return failed(SDRO_CSD_READ);}
 uint64_t n=0;sdro_status s;
#ifdef SDRO_CSD_DIAGNOSTIC
 memcpy(csd_evidence.raw_csd,adapter.csd,16);csd_evidence.valid|=SDCD_CSD;
 s=sdro_decode_csd_traced(adapter.csd,adapter.diag.reported_type,&n,&csd_evidence);
 csd_evidence.stage=5;
#else
 s=sdro_decode_csd(adapter.csd,adapter.diag.reported_type,&n);
#endif
 memset(adapter.csd,0,sizeof(adapter.csd));
 if(s!=SDRO_OK)return failed(s);
 EVIDENCE_FINAL(SDRO_OK,0);
 adapter.diag.sectors=n;adapter.diag.state=SDRO_READY;adapter.diag.error=SDRO_OK;return SDRO_OK;
}
sdro_status sdro_capacity(uint64_t *n){
 if(!n)return SDRO_ARGUMENT;
 if(adapter.diag.state!=SDRO_READY)return SDRO_STATE;
 *n=adapter.diag.sectors;return SDRO_OK;
}
sdro_status sdro_card_type(sdro_type *t){
 if(!t)return SDRO_ARGUMENT;
 if(adapter.diag.state!=SDRO_READY)return SDRO_STATE;
 *t=(sdro_type)adapter.diag.reported_type;return SDRO_OK;
}
void *sdro_context(void){return &adapter;}
int sdro_read(void *context,uint32_t lba,uint8_t buffer[512]){
 sdro_status s;
 if(context!=&adapter){adapter.diag.error=SDRO_ARGUMENT;return 0;}
 if(adapter.diag.state!=SDRO_READY){adapter.diag.error=SDRO_STATE;return 0;}
 if(!buffer){adapter.diag.error=SDRO_ARGUMENT;return 0;}
 s=sdro_check_lba(adapter.diag.reported_type,adapter.diag.sectors,lba);
 if(s!=SDRO_OK){adapter.diag.error=s;return 0;}
 if(!sdro_fw_read(&adapter.card,lba,adapter.bounce)){
  memset(adapter.bounce,0,sizeof(adapter.bounce));adapter.diag.error=SDRO_READ_FAILURE;return 0;
 }
 memcpy(buffer,adapter.bounce,512);adapter.diag.error=SDRO_OK;return 1;
}
sdro_status sdro_diagnostics(sdro_diagnostic *d){if(!d)return SDRO_ARGUMENT;*d=adapter.diag;return SDRO_OK;}
