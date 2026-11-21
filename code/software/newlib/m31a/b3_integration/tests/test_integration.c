#include "mock_firmware.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static unsigned tests;
void check(int ok,const char *s,int line){if(!ok){fprintf(stderr,"FAIL line%d %s\n",line,s);exit(1);}}
static b3_diagnostic diag(void){b3_diagnostic d;CHECK(b3_diagnostics(&d)==B3_OK);return d;}
static void done(const char *name){
 unsigned calls=mock.count;b3_shutdown();CHECK(mock.count==calls);CHECK(mock.forbidden==0&&mock.calls[3]==0);
 for(unsigned i=0;i<mock.count;i++)CHECK(mock.sequence[i]==0||mock.sequence[i]==1||mock.sequence[i]==2||mock.sequence[i]==4);
 printf("PASS %s forbidden=0\n",name);tests++;fixture_destroy();
}
static b1_handle open_file(void){b1_handle h=0;CHECK(b3_open("/M31RO.BIN",&h)==B3_OK&&h);return h;}
static void pattern(b1_handle h){uint8_t bytes[513];size_t got=99;CHECK(b3_read(h,bytes,513,&got)==B3_OK&&got==513);for(unsigned i=0;i<513;i++)CHECK(bytes[i]==(uint8_t)i);}
#define RUN(name,...) do{mock_setup(16,0,0,0);__VA_ARGS__;done(name);}while(0)
int main(void){
 for(int bits=16;bits<=32;bits+=16)for(int part=0;part<2;part++){
  mock_setup(bits,part,0,0);CHECK(b3_init()==B3_OK);CHECK(diag().capacity==disk.capacity);CHECK(mock.sequence[0]==0&&mock.sequence[1]==1&&mock.sequence[2]==4&&mock.sequence[3]==2);pattern(open_file());char n[50];(void)snprintf(n,sizeof(n),"fat%d_%s_pipeline",bits,part?"mbr":"superfloppy");done(n);
 }
 RUN("sdsc_csd_capacity_pipeline",{b3_shutdown();fixture_destroy();mock_setup(16,0,1,0);CHECK(b3_init()==B3_OK&&diag().capacity==8192);pattern(open_file());});
 RUN("huge_capacity_2pow32_pipeline",{b3_shutdown();fixture_destroy();mock_setup(16,1,0,1);CHECK(b3_init()==B3_OK&&diag().capacity==UINT64_C(0x100000000));pattern(open_file());uint8_t b[512];CHECK(sdro_read(sdro_context(),UINT32_MAX,b)==1&&mock.last_lba==UINT32_MAX);});
 RUN("last_sector_and_first_invalid",{CHECK(b3_init()==B3_OK);uint8_t b[512];CHECK(sdro_read(sdro_context(),(uint32_t)(disk.capacity-1),b)==1);unsigned calls=mock.calls[2];CHECK(sdro_read(sdro_context(),(uint32_t)disk.capacity,b)==0&&mock.calls[2]==calls);});
 RUN("sdsc_address_guard",{mock_setup(16,0,1,0);CHECK(b3_init()==B3_OK);uint8_t b[512];unsigned n=mock.calls[2];CHECK(sdro_read(sdro_context(),0x800000,b)==0&&mock.calls[2]==n);CHECK(sdro_check_lba(SDRO_V1,0x800000,0x7fffff)==SDRO_OK);});
 RUN("eof_separate",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();pattern(h);uint8_t b;size_t n;CHECK(b3_read(h,&b,1,&n)==B3_EOF&&n==0&&!diag().first_set);});
 RUN("short_eof",{CHECK(b3_init()==B3_OK);uint8_t b[600];size_t n;CHECK(b3_read(open_file(),b,600,&n)==B3_EOF&&n==513);});
 RUN("chunks_across_sector",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();uint8_t b[513];size_t n;CHECK(b3_read(h,b,511,&n)==B3_OK&&n==511);CHECK(b3_read(h,b+511,2,&n)==B3_OK&&n==2);for(unsigned i=0;i<513;i++)CHECK(b[i]==(uint8_t)i);});
 RUN("seek_positions",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();CHECK(b3_seek(h,255,B1_SET)==B3_OK);uint8_t x;size_t n;CHECK(b3_read(h,&x,1,&n)==B3_OK&&x==255);CHECK(b3_seek(h,-1,B1_CUR)==B3_OK);CHECK(b3_seek(h,0,B1_END)==B3_OK);CHECK(b3_read(h,&x,1,&n)==B3_EOF);CHECK(b3_seek(h,0,B1_SET)==B3_OK);pattern(h);});
 RUN("seek_beyond_eof",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();CHECK(b3_seek(h,INT32_MAX,B1_SET)==B3_OK);uint32_t pos,size;CHECK(b3_info(h,&pos,&size)==B3_OK&&pos==INT32_MAX&&size==513);char x;size_t n;CHECK(b3_read(h,&x,1,&n)==B3_EOF&&n==0);});
 RUN("seek_failure_preserves_position",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();CHECK(b3_seek(h,12,B1_SET)==B3_OK);CHECK(b3_seek(h,INT64_MAX,B1_CUR)==B3_FAT_ERROR&&diag().fat_status==B1_OVERFLOW);uint32_t pos,size;CHECK(b3_info(h,&pos,&size)==B3_OK&&pos==12);});
 RUN("open_close_reuse",{CHECK(b3_init()==B3_OK);b1_handle old=open_file();CHECK(b3_close(old)==B3_OK);b1_handle fresh=open_file();CHECK(fresh!=old);size_t n;char x;CHECK(b3_read(old,&x,1,&n)==B3_FAT_ERROR);pattern(fresh);});
 RUN("four_handles_and_exhaustion",{CHECK(b3_init()==B3_OK);b1_handle h[4];for(unsigned i=0;i<4;i++)h[i]=open_file();b1_handle fifth=77;CHECK(b3_open("/M31RO.BIN",&fifth)==B3_FAT_ERROR&&fifth==77&&diag().fat_status==B1_RESOURCE);for(unsigned i=0;i<4;i++)pattern(h[i]);});
 RUN("missing_file",{CHECK(b3_init()==B3_OK);b1_handle h=44;CHECK(b3_open("/MISSING.BIN",&h)==B3_FAT_ERROR&&h==44);CHECK(diag().phase==B3_OPEN&&diag().fat_status==B1_NOT_FOUND);});
 RUN("unsupported_firmware",{mock.support=0;CHECK(b3_init()==B3_SD_ERROR);CHECK(mock.calls[1]==0&&mock.calls[4]==0&&mock.calls[2]==0);CHECK(diag().state==B3_FAILED&&diag().sd_status==SDRO_NO_FIRMWARE);CHECK(diag().cleanup_count==1&&diag().cleanup[0]==3);});
 RUN("init_failure",{mock.init_status=2;CHECK(b3_init()==B3_SD_ERROR);CHECK(mock.calls[4]==0);CHECK(diag().sd.firmware_init_status==2);});
 RUN("init_false_ok",{mock.initialized=0;CHECK(b3_init()==B3_SD_ERROR&&diag().sd_status==SDRO_NOT_INITIALIZED);CHECK(mock.calls[4]==0&&mock.calls[2]==0);});
 RUN("unknown_descriptor_type",{mock.type=99;CHECK(b3_init()==B3_SD_ERROR&&diag().sd_status==SDRO_CARD_TYPE);});
 RUN("csd_failure",{mock.csd_result=0;CHECK(b3_init()==B3_SD_ERROR&&diag().sd_status==SDRO_CSD_READ);CHECK(mock.calls[2]==0);});
 RUN("partial_csd_failure",{mock.partial_csd=1;CHECK(b3_init()==B3_SD_ERROR);CHECK(mock.calls[2]==0&&diag().sd_status==SDRO_CSD_READ);});
 RUN("bad_csd_end",{mock.csd[15]=0;CHECK(b3_init()==B3_SD_ERROR&&diag().sd_status==SDRO_CSD_MALFORMED);});
 RUN("bad_csd_profile",{mock.csd[5]=10;CHECK(b3_init()==B3_SD_ERROR&&diag().sd_status==SDRO_PROFILE);});
 RUN("bad_geometry",{fixture_put16(fixture_sector(disk.base)+510,0);CHECK(b3_init()==B3_FAT_ERROR&&diag().phase==B3_PROBE&&diag().fat_status==B1_METADATA);CHECK(diag().cleanup_count==2&&diag().cleanup[0]==2&&diag().cleanup[1]==3);});
 RUN("geometry_exceeds_csd",{fixture_put16(fixture_sector(disk.base)+19,10000);CHECK(b3_init()==B3_FAT_ERROR&&diag().fat_status==B1_METADATA);});
 RUN("truncated_backing_read_failure",{disk.fail_lba=0;CHECK(b3_init()==B3_FAT_ERROR&&diag().fat_status==B1_IO);CHECK(diag().sd_status==SDRO_READ_FAILURE);});
 RUN("directory_io_failure",{CHECK(b3_init()==B3_OK);disk.fail_lba=disk.root;b1_handle h;CHECK(b3_open("/M31RO.BIN",&h)==B3_FAT_ERROR&&diag().fat_status==B1_IO);});
 RUN("broken_chain",{fixture_put16(fixture_sector(disk.fat)+3*2,0xffff);CHECK(b3_init()==B3_OK);b1_handle h;CHECK(b3_open("/M31RO.BIN",&h)==B3_FAT_ERROR&&diag().fat_status==B1_CHAIN);});
 RUN("read_failure_unchanged",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();disk.fail_lba=disk.data+1;uint8_t b[513];memset(b,0xaa,513);size_t n;CHECK(b3_read(h,b,513,&n)==B3_FAT_ERROR&&n==0);for(unsigned i=0;i<513;i++)CHECK(b[i]==0xaa);CHECK(diag().fat_status==B1_IO&&diag().sd_status==SDRO_READ_FAILURE);});
 RUN("partial_read_and_seek_recovery",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();disk.fail_lba=disk.data+6;uint8_t b[513];memset(b,0xaa,513);size_t n;CHECK(b3_read(h,b,513,&n)==B3_FAT_ERROR&&n==512&&b[512]==0xaa);for(unsigned i=0;i<512;i++)CHECK(b[i]==(uint8_t)i);CHECK(diag().delivered==512&&diag().phase==B3_READ);unsigned reads=mock.calls[2];CHECK(b3_read(h,b,1,&n)==B3_FAT_ERROR&&n==0&&mock.calls[2]==reads);disk.fail_lba=UINT64_MAX;CHECK(b3_seek(h,0,B1_CUR)==B3_OK);CHECK(b3_read(h,b,1,&n)==B3_OK&&b[0]==0);CHECK(diag().delivered==512&&diag().fat_status==B1_IO);});
 RUN("first_failure_preserved_during_cleanup",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();b1_handle x;CHECK(b3_open("/NONE.BIN",&x)==B3_FAT_ERROR);CHECK(b1_close(h)==B1_OK);b3_shutdown();CHECK(diag().phase==B3_OPEN&&diag().fat_status==B1_NOT_FOUND);});
 RUN("close_failure_external_invalidation",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();CHECK(b1_close(h)==B1_OK);CHECK(b3_close(h)==B3_FAT_ERROR&&diag().phase==B3_CLOSE&&diag().fat_status==B1_HANDLE);});
 RUN("double_close",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();CHECK(b3_close(h)==B3_OK);CHECK(b3_close(h)==B3_FAT_ERROR);});
 RUN("no_reset_while_ready",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();unsigned init=mock.calls[1],n=mock.count;CHECK(b3_init()==B3_STATE&&mock.calls[1]==init&&mock.count==n);pattern(h);});
 RUN("shutdown_order",{CHECK(b3_init()==B3_OK);(void)open_file();(void)open_file();unsigned n=mock.count;b3_shutdown();b3_diagnostic d=diag();CHECK(d.cleanup_count==4&&d.cleanup[0]==1&&d.cleanup[1]==1&&d.cleanup[2]==2&&d.cleanup[3]==3);CHECK(mock.count==n);sdro_diagnostic sd;CHECK(sdro_diagnostics(&sd)==SDRO_OK&&sd.state==SDRO_UNINITIALIZED);});
 RUN("reinit_only_after_shutdown",{mock.support=0;CHECK(b3_init()==B3_SD_ERROR);mock.support=0x1234fedc;unsigned n=mock.count;CHECK(b3_init()==B3_STATE&&mock.count==n);b3_shutdown();CHECK(b3_init()==B3_OK);pattern(open_file());});
 RUN("stale_handle_after_reinit",{CHECK(b3_init()==B3_OK);b1_handle old=open_file();b3_shutdown();CHECK(b3_init()==B3_OK);b1_handle fresh=open_file();CHECK(old!=fresh);char x;size_t n;unsigned calls=mock.count;CHECK(b3_read(old,&x,1,&n)==B3_FAT_ERROR&&mock.count==calls);pattern(fresh);});
 RUN("before_ready_rejected",{size_t n=99;char x;CHECK(b3_read(1,&x,1,&n)==B3_STATE&&n==0);CHECK(b3_seek(1,0,B1_SET)==B3_STATE);});
 RUN("zero_read_no_media",{CHECK(b3_init()==B3_OK);b1_handle h=open_file();unsigned n=mock.count;size_t got;CHECK(b3_read(h,NULL,0,&got)==B3_OK&&got==0&&mock.count==n);});
 RUN("invalid_arguments",{CHECK(b3_init()==B3_OK);CHECK(b3_open("/M31RO.BIN",NULL)==B3_ARGUMENT);b1_handle h=open_file();CHECK(b3_read(h,NULL,1,NULL)==B3_ARGUMENT);CHECK(b3_diagnostics(NULL)==B3_ARGUMENT);});
 RUN("b1_file_read_at_final_uint32_lba",{mock_setup(16,1,0,1);disk.high_base=UINT64_C(0x100000000)-disk.volume;fixture_put32(fixture_sector(0)+454,(uint32_t)disk.high_base);fixture_put16(fixture_sector(disk.fat)+5000*2,5001);fixture_put16(fixture_sector(disk.fat)+5001*2,0xffff);fixture_put16(fixture_sector(disk.root)+26,5000);for(unsigned i=0;i<512;i++)fixture_sector(disk.base+disk.volume-2)[i]=(uint8_t)i;fixture_sector(disk.base+disk.volume-1)[0]=0;CHECK(b3_init()==B3_OK&&diag().capacity==UINT64_C(0x100000000));pattern(open_file());CHECK(mock.last_lba==UINT32_MAX);});
 RUN("unsupported_fat_format",{fixture_put16(fixture_sector(0)+19,1000);CHECK(b3_init()==B3_FAT_ERROR&&diag().fat_status==B1_UNSUPPORTED);});
 RUN("descriptor_csd_mismatch",{mock.type=SDRO_V1;CHECK(b3_init()==B3_SD_ERROR&&diag().sd_status==SDRO_MISMATCH);});
 RUN("failed_open_does_not_exhaust_handles",{CHECK(b3_init()==B3_OK);b1_handle h;CHECK(b3_open("/MISSING.BIN",&h)==B3_FAT_ERROR);for(unsigned i=0;i<4;i++)(void)open_file();});
 RUN("info_invalid_handle",{CHECK(b3_init()==B3_OK);uint32_t pos=77,size=88;CHECK(b3_info(123,&pos,&size)==B3_FAT_ERROR&&pos==77&&size==88&&diag().phase==B3_INFO);});
 RUN("shutdown_idempotent_after_probe_failure",{fixture_put16(fixture_sector(0)+510,0);CHECK(b3_init()==B3_FAT_ERROR);unsigned calls=mock.count,steps=diag().cleanup_count;b3_shutdown();b3_shutdown();CHECK(mock.count==calls&&diag().cleanup_count==steps);});
 printf("SUMMARY unique=%u pass=%u fail=0 skip=0 xfail=0 forbidden_calls=0\n",tests,tests);return 0;
}
