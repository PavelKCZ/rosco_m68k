#include "b3_integration.h"
#include <string.h>
static struct {
 b3_diagnostic d; b1_handle handles[B1_MAX_HANDLES];
 int sd_owned,fat_owned;
} session;
static b3_status record(b3_status s,b3_phase p,b1_status fat,sdro_status sd,size_t delivered){
 if(!session.d.first_set){
  session.d.first_set=1;session.d.first_status=s;session.d.phase=p;
  session.d.fat_status=fat;session.d.sd_status=sd;session.d.delivered=delivered;
  (void)sdro_diagnostics(&session.d.sd);
  if(sd==SDRO_OK&&fat==B1_IO)session.d.sd_status=session.d.sd.error;
 }
 return s;
}
static void event(uint8_t n){if(session.d.cleanup_count<sizeof(session.d.cleanup))session.d.cleanup[session.d.cleanup_count++]=n;}
static int owned(b1_handle h){for(unsigned i=0;i<B1_MAX_HANDLES;i++)if(h&&session.handles[i]==h)return (int)i;return -1;}
static b3_status fat_result(b1_status s,b3_phase phase,size_t n){
 if(s==B1_OK)return B3_OK;
 if(s==B1_EOF)return B3_EOF;
 return record(B3_FAT_ERROR,phase,s,SDRO_OK,n);
}
static void cleanup(void){
 if(session.fat_owned){
  for(unsigned i=0;i<B1_MAX_HANDLES;i++)if(session.handles[i]){
   b1_status s=b1_close(session.handles[i]);session.handles[i]=0;event(1);
   (void)fat_result(s,B3_CLOSE,0);
  }
  b1_shutdown();event(2);session.fat_owned=0;
 }
 if(session.sd_owned){sdro_shutdown();event(3);session.sd_owned=0;}
 session.d.capacity=0;
}
b3_status b3_init(void){
 if(session.d.state!=B3_UNINITIALIZED)return B3_STATE;
 memset(&session,0,sizeof(session));session.d.state=B3_INITIALIZING;session.sd_owned=1;
 sdro_status s=sdro_init();
 if(s!=SDRO_OK){record(B3_SD_ERROR,B3_SD_INIT,B1_OK,s,0);cleanup();session.d.state=B3_FAILED;return B3_SD_ERROR;}
 uint64_t capacity=0;s=sdro_capacity(&capacity);
 if(s!=SDRO_OK){record(B3_SD_ERROR,B3_CAPACITY,B1_OK,s,0);cleanup();session.d.state=B3_FAILED;return B3_SD_ERROR;}
 session.fat_owned=1;
 b1_status f=b1_init(sdro_read,sdro_context(),capacity);
 if(f!=B1_OK){fat_result(f,B3_PROBE,0);cleanup();session.d.state=B3_FAILED;return B3_FAT_ERROR;}
 session.d.capacity=capacity;session.d.state=B3_READY;return B3_OK;
}
b3_status b3_open(const char *path,b1_handle *h){
 if(session.d.state!=B3_READY)return B3_STATE;
 if(!h)return B3_ARGUMENT;
 b1_handle new_handle=0;b1_status s=b1_open(path,&new_handle);
 if(s!=B1_OK)return fat_result(s,B3_OPEN,0);
 /* Exclusive ownership means B1 cannot return more handles than this array. */
 for(unsigned i=0;i<B1_MAX_HANDLES;i++)if(!session.handles[i]){session.handles[i]=new_handle;*h=new_handle;return B3_OK;}
 (void)b1_close(new_handle);return record(B3_FAT_ERROR,B3_OPEN,B1_RESOURCE,SDRO_OK,0);
}
b3_status b3_read(b1_handle h,void *buf,size_t count,size_t *got){
 if(got)*got=0;
 if(session.d.state!=B3_READY)return B3_STATE;
 if(owned(h)<0)return fat_result(B1_HANDLE,B3_READ,0);
 if(!got)return B3_ARGUMENT;
 b1_status s=b1_read(h,buf,count,got);return fat_result(s,B3_READ,*got);
}
b3_status b3_seek(b1_handle h,int64_t off,b1_whence w){
 if(session.d.state!=B3_READY)return B3_STATE;
 if(owned(h)<0)return fat_result(B1_HANDLE,B3_SEEK,0);
 return fat_result(b1_seek(h,off,w),B3_SEEK,0);
}
b3_status b3_info(b1_handle h,uint32_t *pos,uint32_t *size){
 if(session.d.state!=B3_READY)return B3_STATE;
 if(owned(h)<0)return fat_result(B1_HANDLE,B3_INFO,0);
 return fat_result(b1_info(h,pos,size),B3_INFO,0);
}
b3_status b3_close(b1_handle h){
 if(session.d.state!=B3_READY)return B3_STATE;
 int i=owned(h);if(i<0)return fat_result(B1_HANDLE,B3_CLOSE,0);
 b1_status s=b1_close(h);session.handles[i]=0;return fat_result(s,B3_CLOSE,0);
}
void b3_shutdown(void){cleanup();session.d.state=B3_UNINITIALIZED;}
b3_status b3_diagnostics(b3_diagnostic *d){if(!d)return B3_ARGUMENT;*d=session.d;return B3_OK;}

#ifdef SDRO_CSD_DIAGNOSTIC
#include "b3_csd_diag.h"
int b3_csd_snapshot(sdro_csd_diagnostic *out){return sdro_csd_snapshot(out);}
#endif
