#ifndef CSD_FORMAT_H
#define CSD_FORMAT_H
#include "b3_csd_diag.h"
/* Widths are invariant even for unavailable values; length excludes NUL.
 * Formatter emits LF; existing console may translate each LF to CRLF. */
enum { CSDD_TEXT_LENGTH =
 sizeof("CSDD1 VALID=00000000 ST=00 STATUS=0000\n")-1 +
 sizeof("CSDD1 SUP=00000000 OK=0 INIT=00000000 DI=00 TYPE=00000000 CMD9=00000000\n")-1 +
 sizeof("CSDD1 RAW=00000000000000000000000000000000 STRUCT=00\n")-1 +
 sizeof("CSDD1 REJ=0000 MASK=00,00,00,00\n")-1,
 CSDD_BUFFER_SIZE=CSDD_TEXT_LENGTH+1, CSDD_CRLF_LENGTH=CSDD_TEXT_LENGTH+4 };
static void csdd_literal(char **p,const char *s){while(*s)*(*p)++=*s++;}
static void csdd_hex(char **p,uint32_t value,unsigned width,int valid){
 static const char hex[]="0123456789ABCDEF";
 for(unsigned n=width;n;n--)*(*p)++=valid?hex[(value>>((n-1)*4))&15]:'-';
}
static int csdd_format(const sdro_csd_diagnostic *d,char *out,size_t capacity){
 if(!out||!capacity)return 0;
 out[0]=0;
 if(!d||capacity<CSDD_BUFFER_SIZE||(d->valid&&d->schema!=1))return 0;
 char *p=out;uint32_t v=d->valid;
 csdd_literal(&p,"CSDD1 VALID=");csdd_hex(&p,v,8,1);
 csdd_literal(&p," ST=");csdd_hex(&p,d->stage,2,v&SDCD_ATTEMPT);
 csdd_literal(&p," STATUS=");csdd_hex(&p,d->final_status,4,v&SDCD_FINAL);
 csdd_literal(&p,"\nCSDD1 SUP=");csdd_hex(&p,d->support_raw,8,v&SDCD_SUPPORT);
 csdd_literal(&p," OK=");*p++=(v&SDCD_SUPPORT)?(d->support_raw==UINT32_C(0x1234fedc)?'1':'0'):'-';
 csdd_literal(&p," INIT=");csdd_hex(&p,d->init_raw,8,v&SDCD_INIT);
 csdd_literal(&p," DI=");csdd_hex(&p,d->initialized_raw,2,v&SDCD_DESCRIPTOR);
 csdd_literal(&p," TYPE=");csdd_hex(&p,d->type_raw,8,v&SDCD_DESCRIPTOR);
 csdd_literal(&p," CMD9=");csdd_hex(&p,d->cmd9_raw,8,v&SDCD_CMD9);
 csdd_literal(&p,"\nCSDD1 RAW=");
 for(unsigned i=0;i<16;i++)csdd_hex(&p,d->raw_csd[i],2,v&SDCD_CSD);
 csdd_literal(&p," STRUCT=");csdd_hex(&p,d->structure,2,v&SDCD_STRUCTURE);
 csdd_literal(&p,"\nCSDD1 REJ=");
 csdd_hex(&p,d->reject_id,4,(v&SDCD_REJECT)||((v&SDCD_FINAL)&&!d->final_status));
 csdd_literal(&p," MASK=");
 unsigned count=d->reject_id>=0x1101&&d->reject_id<=0x1104?4:
                d->reject_id==0x1201||d->reject_id==0x1301?1:
                d->reject_id>=0x1401&&d->reject_id<=0x1403?3:0;
 for(unsigned i=0;i<4;i++){
  if(i)*p++=',';
  csdd_hex(&p,d->masked[i],2,(v&SDCD_MASK)&&i<count);
 }
 *p++='\n';*p=0;
 return p-out==CSDD_TEXT_LENGTH;
}
#endif
