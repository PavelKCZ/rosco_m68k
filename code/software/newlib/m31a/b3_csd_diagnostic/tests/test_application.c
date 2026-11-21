/* Run all original application expectations unchanged with diagnostics enabled. */
#include "b3_csd_diag.h"
#include <string.h>
static int fake_csd_unavailable, fake_csd_print_bad;
int b3_csd_snapshot(sdro_csd_diagnostic *out){
 if(!out)return -1;
 memset(out,0,sizeof(*out));if(fake_csd_unavailable)return 0;out->schema=1;out->stage=5;
 out->valid=SDCD_ATTEMPT|SDCD_SUPPORT|SDCD_INIT|SDCD_DESCRIPTOR|SDCD_CMD9|SDCD_CSD|SDCD_STRUCTURE|SDCD_REJECT|SDCD_FINAL|SDCD_MASK;
 out->support_raw=UINT32_C(0x1234fedc);out->type_raw=2;out->cmd9_raw=1;
 out->initialized_raw=1;out->structure=1;out->raw_csd[0]=0x40;
 out->raw_csd[10]=0xf0;out->masked[2]=0x80;out->final_status=8;out->reject_id=0x1403;
 return 1;
}
/* Actual application source, fake B3/runtime/stdio boundaries only. No hardware. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "b3_integration.h"
#include "runtime.h"
static int fake_vprintf(const char *, va_list);
static int fake_fflush(FILE *);
static int fake_setvbuf(FILE *, char *, int, size_t);
static int fake_atexit(void (*)(void));
#define main application_main
#define vprintf fake_vprintf
#define fflush fake_fflush
#define setvbuf fake_setvbuf
#define atexit fake_atexit
#include "../../b3_executable/src/smoke.c"
#undef main
#undef vprintf
#undef fflush
#undef setvbuf
#undef atexit

enum {GOOD,HUGE,INIT_FAIL,OPEN_FAIL,INFO_FAIL,SIZE_BAD,READ_IO,READ_SHORT,BYTE_BAD,
 EOF_BAD,SEEK_SET_BAD,SEEK_BYTES_BAD,SEEK_END_BAD,END_INFO_BAD,END_EOF_BAD,
 CLOSE_BAD,SECONDARY_BAD,SHUTDOWN_BAD,SETVBUF_BAD,PRINT_BAD,FLUSH_BAD,ATEXIT_BAD,
 BSS_BAD,DATA_BAD,CTOR_BAD};
static int mode, init_calls, close_calls, shutdown_calls, marker_flushed, eof_calls;
static uint32_t pos;
static char output[12000], calls[128], last[256];
static size_t used, call_used;
static b3_diagnostic diagnostic;
static void (*exit_callback)(void);
struct m31_layout m31_memory = {0x500000,0x4f8000,0x26000,0x4f8000};
static void check(int condition,const char *text,int n){if(!condition){fprintf(stderr,"FAIL line %d %s mode=%d output=%s\n",n,text,mode,output);exit(1);}}
#define CHECK(x) check(!!(x),#x,__LINE__)
static void call(char c){CHECK(call_used+1<sizeof(calls));calls[call_used++]=c;calls[call_used]=0;}
void m31_diag_mark(unsigned c){(void)c;}
void m31_diag_error(unsigned c){(void)c;CHECK(0);}
static int fake_setvbuf(FILE *f,char *b,int v,size_t n){(void)f;(void)b;(void)v;(void)n;return mode==SETVBUF_BAD?-1:0;}
static int fake_vprintf(const char *fmt,va_list ap){
 int n=vsnprintf(last,sizeof(last),fmt,ap);CHECK(n>=0);CHECK(used+strlen(last)<sizeof(output));strcpy(output+used,last);used+=strlen(last);
 return (mode==PRINT_BAD&&strstr(last,"INIT_BEGIN"))||(fake_csd_print_bad&&strstr(last,"CSDD1 VALID="))?-1:n;
}
static int fake_fflush(FILE *f){(void)f;if(mode==FLUSH_BAD&&strstr(last,"INIT_BEGIN"))return -1;if(strstr(last,"INIT_BEGIN"))marker_flushed=1;return 0;}
static int fake_atexit(void (*f)(void)){exit_callback=f;return mode==ATEXIT_BAD?-1:0;}
static b3_status backend_error(b3_phase phase,b1_status fat,size_t n){
 if(!diagnostic.first_set){diagnostic.first_set=1;diagnostic.phase=phase;diagnostic.first_status=B3_FAT_ERROR;diagnostic.fat_status=fat;diagnostic.delivered=n;diagnostic.sd_status=fat==B1_IO?SDRO_READ_FAILURE:SDRO_OK;}
 return B3_FAT_ERROR;
}
b3_status b3_init(void){
 call('I');init_calls++;CHECK(marker_flushed);diagnostic.state=B3_READY;diagnostic.capacity=mode==HUGE?UINT64_C(0x100000000):8192;
 if(mode==INIT_FAIL){diagnostic.state=B3_FAILED;diagnostic.first_set=1;diagnostic.phase=B3_SD_INIT;diagnostic.sd_status=SDRO_INIT_FAILURE;return B3_SD_ERROR;}return B3_OK;
}
b3_status b3_open(const char *p,b1_handle *h){call('O');CHECK(strcmp(p,"/M31RO.BIN")==0);if(mode==OPEN_FAIL)return backend_error(B3_OPEN,B1_NOT_FOUND,0);*h=7;pos=0;return B3_OK;}
b3_status b3_info(b1_handle h,uint32_t *p,uint32_t *s){
 call('F');CHECK(h==7);if(mode==INFO_FAIL)return backend_error(B3_INFO,B1_HANDLE,0);
 *p=mode==END_INFO_BAD&&pos==513?512:pos;*s=mode==SIZE_BAD?512:513;return B3_OK;
}
b3_status b3_read(b1_handle h,void *dst,size_t n,size_t *got){
 call('R');CHECK(h==7);*got=0;
 if(n==513&&(mode==READ_IO||mode==SECONDARY_BAD||mode==READ_SHORT)){
  *got=128;pos+=128;for(unsigned i=0;i<128;i++)((uint8_t *)dst)[i]=(uint8_t)i;
  return mode==READ_SHORT?B3_EOF:backend_error(B3_READ,B1_IO,128);
 }
 if(pos>=513){eof_calls++;if(mode==EOF_BAD||(mode==END_EOF_BAD&&eof_calls==2)){*got=1;return B3_OK;}return B3_EOF;}
 CHECK(n<=513-pos);for(size_t i=0;i<n;i++)((uint8_t *)dst)[i]=(uint8_t)(pos+i);
 *got=n;pos+=(uint32_t)n;
 if(mode==BYTE_BAD&&n==513)((uint8_t *)dst)[123]^=1;
 if(mode==SEEK_BYTES_BAD&&n==2)((uint8_t *)dst)[0]=0;
 return B3_OK;
}
b3_status b3_seek(b1_handle h,int64_t off,b1_whence w){
 CHECK(h==7);if(w==B1_SET){call('S');CHECK(off==511);if(mode==SEEK_SET_BAD)return backend_error(B3_SEEK,B1_SEEK,0);pos=511;}
 else{call('E');CHECK(w==B1_END&&off==0);if(mode==SEEK_END_BAD)return backend_error(B3_SEEK,B1_SEEK,0);pos=513;}return B3_OK;
}
b3_status b3_close(b1_handle h){call('C');close_calls++;CHECK(h==7);return mode==CLOSE_BAD||mode==SECONDARY_BAD?backend_error(B3_CLOSE,B1_HANDLE,0):B3_OK;}
void b3_shutdown(void){call('X');shutdown_calls++;diagnostic.state=mode==SHUTDOWN_BAD?B3_READY:B3_UNINITIALIZED;diagnostic.capacity=0;}
b3_status b3_diagnostics(b3_diagnostic *p){*p=diagnostic;return B3_OK;}
int inherited_main(void){
 const char *names[]={"success","capacity_2pow32","init_failure","open_failure","info_failure","size_mismatch","partial_io","short_eof","bad_byte","eof_status","seek_set","boundary_bytes","seek_end","end_position","end_eof","close_failure","secondary_preserves_primary","shutdown_state","stdout_setup","stdout_write","stdout_flush","atexit_failure","bss_probe","data_probe","ctor_probe"};
 unsigned count=0;
 for(mode=GOOD;mode<=CTOR_BAD;mode++){
  used=call_used=0;output[0]=calls[0]=last[0]=0;memset(&diagnostic,0,sizeof(diagnostic));
  init_calls=close_calls=shutdown_calls=marker_flushed=eof_calls=0;pos=0;exit_callback=NULL;console_error=0;
  bss_probe=mode==BSS_BAD;data_probe=mode==DATA_BAD?0:0x31b3;ctor_probe=0;app_construct();if(mode==CTOR_BAD)ctor_probe=0;
  int result=application_main();
  if(init_calls && result && mode!=SHUTDOWN_BAD){
   const char *backend=strstr(output,"BACKEND "),*csd=strstr(output,"CSDD1 VALID="),*shutdown=strstr(output,"SHUTDOWN_OK");
   CHECK(backend&&csd&&backend<csd);if(shutdown)CHECK(csd<shutdown);
   unsigned lines=0;for(const char *q=output;(q=strstr(q,"CSDD1 "));q+=6)lines++;
   CHECK(lines==4);
  }
  if(mode==GOOD||mode==HUGE){
   CHECK(result==0&&strstr(output,"APP_PASS\n"));CHECK(strcmp(calls,"IOFRRSREFRCX")==0);CHECK(close_calls==1&&shutdown_calls==1);
   CHECK(exit_callback);exit_callback();app_destruct();CHECK(strstr(output,"APP_ATEXIT\n")&&strstr(output,"APP_DTOR\n"));
   CHECK(strstr(output,mode==HUGE?"sectors=0000000100000000":"sectors=0000000000002000"));
  }else{
   CHECK(result==1&&!strstr(output,"APP_PASS"));
   int no_init=mode>=SETVBUF_BAD;
   CHECK(init_calls==!no_init&&shutdown_calls==!no_init);
   if(!no_init){CHECK(strstr(output,"APP_FAIL phase="));CHECK(calls[call_used-1]=='X');}
   if(mode==INIT_FAIL||mode==OPEN_FAIL||no_init)CHECK(close_calls==0);else CHECK(close_calls==1);
   if(mode==READ_IO||mode==SECONDARY_BAD){CHECK(strstr(output,"APP_FAIL phase=READ status=5 bytes=128"));CHECK(strstr(output,"delivered=0000000000000080"));CHECK(diagnostic.phase==B3_READ&&diagnostic.delivered==128);}
   if(mode==SECONDARY_BAD)CHECK(strstr(output,"CLEANUP_FAIL close="));
   if(mode==BYTE_BAD)CHECK(strstr(output,"index=123"));
   if(mode==READ_SHORT)CHECK(strstr(output,"phase=READ status=1 bytes=128"));
  }
  printf("PASS %s\n",names[mode]);count++;
 }
 printf("SUMMARY unique=%u pass=%u fail=0 skip=0 xfail=0\n",count,count);return 0;
}

int main(void){
 CHECK(inherited_main()==0);
 for(unsigned scenario=0;scenario<3;scenario++){
  mode=INIT_FAIL;used=call_used=0;output[0]=calls[0]=last[0]=0;
  memset(&diagnostic,0,sizeof(diagnostic));init_calls=close_calls=shutdown_calls=marker_flushed=eof_calls=0;
  console_error=0;bss_probe=0;data_probe=ctor_probe=0x31b3;
  fake_csd_unavailable=scenario==0;fake_csd_print_bad=scenario==1;
  CHECK(application_main()==1&&shutdown_calls==1&&close_calls==0);
  CHECK(strstr(output,"APP_FAIL phase=INIT status=4 bytes=0"));
  if(scenario==0)CHECK(strstr(output,"SUP=-------- OK=- INIT=-------- DI=-- TYPE=-------- CMD9=--------"));
  if(scenario==1)CHECK(console_error&&!strstr(output,"APP_PASS"));
  if(scenario==2)CHECK(strstr(output,"REJ=1403 MASK=00,00,80,--"));
  printf("PASS csd_application_%u\n",scenario);
 }
 puts("SUMMARY unique=28 pass=28 fail=0 skip=0 xfail=0");return 0;
}
