#include "fat_ro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
struct media {uint8_t *bytes;uint32_t sectors,base,data,root,fat;int bits;uint64_t reads,writes;uint32_t fail;int partial;};
static struct media m;
static unsigned tests;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static void p16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void p32(uint8_t *p,uint32_t x){p16(p,(uint16_t)x);p16(p+2,(uint16_t)(x>>16));}
static uint8_t *sec(uint32_t n){CHECK(n<m.sectors);return m.bytes+(size_t)n*512;}
static uint32_t clba(uint32_t c){return m.data+c-2;}
static void fat(uint32_t c,uint32_t n){uint8_t *p=sec(m.fat)+(size_t)c*(m.bits/8);if(m.bits==16)p16(p,(uint16_t)n);else p32(p,n);}
static void entry(uint8_t *p,const char *name,uint8_t attr,uint32_t c,uint32_t size){memcpy(p,name,11);p[11]=attr;p16(p+20,(uint16_t)(c>>16));p16(p+26,(uint16_t)c);p32(p+28,size);}
static int read_sector(void *ctx,uint32_t lba,uint8_t out[512]){
 struct media *s=ctx;s->reads++;CHECK(lba<s->sectors);
 if(lba==s->fail){if(s->partial)memset(out,0xEE,117);return 0;}
 memcpy(out,s->bytes+(size_t)lba*512,512);return 1;
}
/* A real host write abstraction exists but is never passed to B1. */
static int write_sector(struct media *s,uint32_t lba,const void *p){(void)lba;(void)p;s->writes++;return 0;}
static void fixture(int bits,int partition){
 b1_shutdown();memset(&m,0,sizeof(m));m.bits=bits;m.base=partition?7:0;m.fail=UINT32_MAX;
 uint32_t clusters=bits==16?5000:65530, reserved=bits==16?1:32,fsize=bits==16?20:513,rootsecs=bits==16?2:0;
 uint32_t total=reserved+fsize+rootsecs+clusters;m.sectors=m.base+total;
 m.bytes=calloc(m.sectors,512);CHECK(m.bytes);
 uint8_t *b=sec(m.base);p16(b+510,0xaa55);p16(b+11,512);b[13]=1;p16(b+14,(uint16_t)reserved);b[16]=1;
 p16(b+17,bits==16?32:0);if(bits==16){p16(b+19,(uint16_t)total);p16(b+22,(uint16_t)fsize);}else{p32(b+32,total);p32(b+36,fsize);p32(b+44,2);}
 if(partition){uint8_t *p=sec(0);p16(p+510,0xaa55);p[450]=bits==16?6:0xc;p32(p+454,m.base);p32(p+458,total);}
 m.fat=m.base+reserved;m.data=m.base+reserved+fsize+rootsecs;m.root=bits==16?m.base+reserved+fsize:m.data;
 uint32_t end=bits==16?0xffffU:0xfffffffU;
 fat(0,end);fat(1,end);fat(2,end);fat(3,end);fat(4,8);fat(8,end);fat(5,9);fat(9,6);fat(6,end);fat(10,end);fat(11,end);
 for(uint32_t c=3;c<=11;c++)for(unsigned i=0;i<512;i++)sec(clba(c))[i]=(uint8_t)i;
 memset(sec(clba(10)),0,512);entry(sec(clba(10)),"INNER   BIN",0x20,11,8);
 uint8_t *d=sec(m.root);entry(d,"EMPTY   BIN",0x20,0,0);entry(d+32,"ONE     BIN",0x20,3,512);entry(d+64,"DATA    BIN",0x20,4,513);entry(d+96,"MULTI   BIN",0x20,5,1300);entry(d+128,"SUB        ",0x10,10,0);
}
static void finish(const char *name){CHECK(m.writes==0);b1_shutdown();CHECK(m.writes==0);free(m.bytes);m.bytes=NULL;printf("PASS %s writes=0\n",name);tests++;}
static void init(void){CHECK(b1_init(read_sector,&m,m.sectors)==B1_OK);}
static b1_handle openfile(const char *p){b1_handle h=0;CHECK(b1_open(p,&h)==B1_OK);CHECK(h);return h;}
static void pattern(b1_handle h,size_t len){uint8_t buf[1600];size_t n=0;CHECK(len<=sizeof(buf));CHECK(b1_read(h,buf,len,&n)==B1_OK);CHECK(n==len);for(size_t i=0;i<n;i++)CHECK(buf[i]==(uint8_t)i);}
#define RUN(name,...) do{fixture(16,0);__VA_ARGS__;finish(name);}while(0)
int main(void){
 /* Prevent unused-function warning, without invoking the write abstraction. */
 int (*write_api)(struct media *,uint32_t,const void *)=write_sector;(void)write_api;
 for(int bits=16;bits<=32;bits+=16)for(int part=0;part<=1;part++){
  fixture(bits,part);init();b1_handle h=openfile("/DATA.BIN");pattern(h,513);size_t n=99;uint8_t b=0;CHECK(b1_read(h,&b,1,&n)==B1_EOF&&n==0);CHECK(b1_close(h)==B1_OK);
  char name[50];(void)snprintf(name,sizeof(name),"fat%d_%s_513",bits,part?"partition":"superfloppy");finish(name);
 }
 RUN("empty_eof",{init();b1_handle h=openfile("/EMPTY.BIN");size_t n;char x;CHECK(b1_read(h,&x,1,&n)==B1_EOF&&n==0);});
 RUN("one_sector",{init();pattern(openfile("/ONE.BIN"),512);});
 RUN("fragmented_multi_cluster",{init();pattern(openfile("/MULTI.BIN"),1300);});
 RUN("short_eof",{init();uint8_t buf[600];size_t n;CHECK(b1_read(openfile("/DATA.BIN"),buf,600,&n)==B1_EOF&&n==513);});
 RUN("zero_read_no_media",{init();b1_handle h=openfile("/DATA.BIN");uint64_t reads=m.reads;size_t n=88;CHECK(b1_read(h,NULL,0,&n)==B1_OK&&n==0&&reads==m.reads);});
 RUN("subdirectory",{init();pattern(openfile("/SUB/INNER.BIN"),8);});
 RUN("case_insensitive",{init();pattern(openfile("/data.bin"),513);});
 RUN("missing",{init();b1_handle h=77;CHECK(b1_open("/MISSING.BIN",&h)==B1_NOT_FOUND&&h==77);});
 const char *badpaths[]={"","/","relative","//DATA.BIN","/../DATA.BIN","/SUB/","/TOOLONGXX.BIN","/DATA.TOOLONG","/A B","/A..B","/A/./B","/A:\\B"};
 for(unsigned i=0;i<sizeof(badpaths)/sizeof(*badpaths);i++){fixture(16,0);init();b1_handle h=77;uint64_t before=m.reads;CHECK(b1_open(badpaths[i],&h)==B1_ARGUMENT&&h==77&&m.reads==before);char name[30];(void)snprintf(name,sizeof(name),"invalid_path_%u",i);finish(name);}
 RUN("path_length",{init();char path[129];memset(path,'A',128);path[0]='/';path[128]=0;b1_handle h;CHECK(b1_open(path,&h)==B1_ARGUMENT);});
 RUN("path_depth",{init();b1_handle h;CHECK(b1_open("/A/A/A/A/A/A/A/A/A",&h)==B1_LIMIT);});
 RUN("directory_not_regular",{init();b1_handle h;CHECK(b1_open("/SUB",&h)==B1_NOT_FILE);});
 RUN("parent_not_directory",{init();b1_handle h;CHECK(b1_open("/DATA.BIN/A",&h)==B1_METADATA);});
 RUN("invalid_handle",{size_t n;char x;CHECK(b1_read(0,&x,1,&n)==B1_HANDLE);CHECK(b1_close(88)==B1_HANDLE);});
 RUN("double_close_reuse",{init();b1_handle old=openfile("/DATA.BIN");CHECK(b1_close(old)==B1_OK);CHECK(b1_close(old)==B1_HANDLE);b1_handle fresh=openfile("/DATA.BIN");CHECK(fresh!=old);CHECK(b1_seek(old,0,B1_SET)==B1_HANDLE);pattern(fresh,513);});
 RUN("four_handles_exhaustion",{init();b1_handle h[4];for(int i=0;i<4;i++)h[i]=openfile("/DATA.BIN");b1_handle x=99;CHECK(b1_open("/ONE.BIN",&x)==B1_RESOURCE&&x==99);CHECK(b1_close(h[1])==B1_OK);CHECK(b1_open("/ONE.BIN",&x)==B1_OK);pattern(h[0],513);pattern(h[2],513);});
 RUN("same_path_independent_positions",{init();b1_handle a=openfile("/DATA.BIN"),b=openfile("/DATA.BIN");pattern(a,513);pattern(b,513);});
 RUN("shutdown_invalidates",{init();b1_handle h=openfile("/ONE.BIN");b1_shutdown();CHECK(b1_close(h)==B1_HANDLE);init();b1_handle n=openfile("/ONE.BIN");CHECK(h!=n);CHECK(b1_close(h)==B1_HANDLE);});
 RUN("second_init_rejected",{init();b1_handle h=openfile("/ONE.BIN");CHECK(b1_init(read_sector,&m,m.sectors)==B1_STATE);pattern(h,512);});
 RUN("open_before_init",{b1_handle h;CHECK(b1_open("/ONE.BIN",&h)==B1_STATE);});
 RUN("null_init",{CHECK(b1_init(NULL,&m,m.sectors)==B1_ARGUMENT);});
 RUN("zero_media",{CHECK(b1_init(read_sector,&m,0)==B1_ARGUMENT);});
 RUN("media_range",{CHECK(b1_init(read_sector,&m,UINT64_C(0x100000001))==B1_ARGUMENT);});
 RUN("null_args",{init();b1_handle h=openfile("/ONE.BIN");size_t n;uint32_t x;CHECK(b1_open(NULL,&h)==B1_ARGUMENT);CHECK(b1_open("/ONE.BIN",NULL)==B1_ARGUMENT);CHECK(b1_read(h,NULL,1,&n)==B1_ARGUMENT);CHECK(b1_read(h,NULL,0,NULL)==B1_ARGUMENT);CHECK(b1_info(h,NULL,&x)==B1_ARGUMENT);});
 RUN("seek_all_origins",{init();b1_handle h=openfile("/DATA.BIN");CHECK(b1_seek(h,511,B1_SET)==B1_OK);uint8_t b[2];size_t n;CHECK(b1_read(h,b,2,&n)==B1_OK&&n==2&&b[0]==255&&b[1]==0);CHECK(b1_seek(h,-2,B1_CUR)==B1_OK);CHECK(b1_seek(h,-1,B1_END)==B1_OK);CHECK(b1_read(h,b,1,&n)==B1_OK&&b[0]==0);});
 RUN("seek_beyond_eof",{init();b1_handle h=openfile("/DATA.BIN");CHECK(b1_seek(h,INT32_MAX,B1_SET)==B1_OK);size_t n;char x;CHECK(b1_read(h,&x,1,&n)==B1_EOF&&n==0);uint32_t p,z;CHECK(b1_info(h,&p,&z)==B1_OK&&p==INT32_MAX&&z==513);});
 RUN("seek_failure_preserves_position",{init();b1_handle h=openfile("/DATA.BIN");CHECK(b1_seek(h,10,B1_SET)==B1_OK);CHECK(b1_seek(h,-11,B1_CUR)==B1_SEEK);CHECK(b1_seek(h,INT64_MAX,B1_CUR)==B1_OVERFLOW);CHECK(b1_seek(h,INT64_MIN,B1_CUR)==B1_SEEK);CHECK(b1_seek(h,INT32_MAX,B1_CUR)==B1_OVERFLOW);CHECK(b1_seek(h,0,(b1_whence)99)==B1_SEEK);uint32_t p,z;CHECK(b1_info(h,&p,&z)==B1_OK&&p==10);});
 RUN("seek_fragmented_cluster",{init();b1_handle h=openfile("/MULTI.BIN");CHECK(b1_seek(h,1023,B1_SET)==B1_OK);uint8_t b[2];size_t n;CHECK(b1_read(h,b,2,&n)==B1_OK&&b[0]==255&&b[1]==0);});
 RUN("failed_sector_no_stale_bytes",{init();b1_handle h=openfile("/DATA.BIN");m.fail=clba(4);m.partial=1;uint8_t b[513];memset(b,0xaa,sizeof(b));size_t n=9;CHECK(b1_read(h,b,513,&n)==B1_IO&&n==0);for(unsigned i=0;i<513;i++)CHECK(b[i]==0xaa);uint32_t p,z;CHECK(b1_info(h,&p,&z)==B1_OK&&p==0);});
 RUN("partial_read_error_sticky_and_seek_recovery",{init();b1_handle h=openfile("/DATA.BIN");m.fail=clba(8);m.partial=1;uint8_t b[513];memset(b,0xaa,sizeof(b));size_t n;CHECK(b1_read(h,b,513,&n)==B1_IO&&n==512&&b[512]==0xaa);for(size_t i=0;i<512;i++)CHECK(b[i]==(uint8_t)i);uint32_t p,z;CHECK(b1_info(h,&p,&z)==B1_OK&&p==512);uint64_t reads=m.reads;CHECK(b1_read(h,b,1,&n)==B1_IO&&n==0&&reads==m.reads);CHECK(b1_read(h,NULL,0,&n)==B1_OK&&reads==m.reads);m.fail=UINT32_MAX;CHECK(b1_seek(h,0,B1_CUR)==B1_OK);CHECK(b1_read(h,b,1,&n)==B1_OK&&n==1&&b[0]==0);});
 RUN("boot_io_failure",{m.fail=0;CHECK(b1_init(read_sector,&m,m.sectors)==B1_IO);});
 RUN("open_directory_io_failure_no_leak",{init();m.fail=m.root;b1_handle h;CHECK(b1_open("/ONE.BIN",&h)==B1_IO);m.fail=UINT32_MAX;for(int i=0;i<4;i++)(void)openfile("/ONE.BIN");});
 RUN("fat_read_error_not_eoc",{init();m.fail=m.fat;b1_handle h;CHECK(b1_open("/ONE.BIN",&h)==B1_IO);});
 RUN("truncated_media",{CHECK(b1_init(read_sector,&m,m.sectors-1)==B1_METADATA);});
 RUN("bad_signature",{p16(sec(0)+510,0);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("unsupported_fat12",{p16(sec(0)+19,1000);CHECK(b1_init(read_sector,&m,m.sectors)==B1_UNSUPPORTED);});
 RUN("zero_spc",{sec(0)[13]=0;CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("nonpower_spc",{sec(0)[13]=3;CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("oversized_spc",{sec(0)[13]=255;CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("zero_reserved",{p16(sec(0)+14,0);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("zero_fats",{sec(0)[16]=0;CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("bad_fat_count",{sec(0)[16]=3;CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("fat_capacity_short",{p16(sec(0)+22,1);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("metadata_extent_overhead",{p16(sec(0)+14,65535);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("conflicting_totals",{p32(sec(0)+32,1);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("root_entry_alignment",{p16(sec(0)+17,31);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 const uint32_t bad[]={0,1,0xfff0,0xfff7,7000};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){fixture(16,0);fat(4,bad[i]);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);char n[40];(void)snprintf(n,sizeof(n),"fat16_bad_chain_%u",i);finish(n);}
 RUN("premature_eoc",{fat(4,0xffff);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);});
 RUN("self_loop",{fat(4,4);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);});
 RUN("two_cluster_loop",{fat(8,4);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);});
 RUN("invalid_entry_start",{p16(sec(m.root)+64+26,7000);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);});
 RUN("entry_size_overflow",{p32(sec(m.root)+64+28,UINT32_MAX);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_OVERFLOW);});
 RUN("nonempty_zero_start",{p16(sec(m.root)+64+26,0);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);});
 RUN("fat16_high_cluster_word",{p16(sec(m.root)+64+20,1);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_METADATA);});
 RUN("invalid_attribute",{sec(m.root)[64+11]=0x80;init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_METADATA);});
 RUN("corrupt_directory_chain",{fat(10,10);init();b1_handle h;CHECK(b1_open("/SUB/INNER.BIN",&h)==B1_CHAIN);});
 RUN("directory_missing_at_capacity",{memset(sec(m.root),0xe5,1024);init();b1_handle h;CHECK(b1_open("/NONE.BIN",&h)==B1_NOT_FOUND);});
 RUN("deleted_and_lfn_skip",{sec(m.root)[0]=0xe5;sec(m.root)[32+11]=0x0f;init();pattern(openfile("/DATA.BIN"),513);});
 fixture(16,1);p32(sec(0)+454,UINT32_MAX);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);finish("partition_overflow");
 fixture(16,1);p32(sec(0)+458,1);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);finish("partition_short");
 fixture(16,1);sec(0)[450]=5;CHECK(b1_init(read_sector,&m,m.sectors)==B1_UNSUPPORTED);finish("extended_partition_unsupported");
 fixture(16,1);m.fail=m.base;CHECK(b1_init(read_sector,&m,m.sectors)==B1_IO);finish("partition_boot_io");
 fixture(32,0);p16(sec(0)+42,1);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);finish("fat32_version");
 fixture(32,0);p16(sec(0)+40,0x81);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);finish("fat32_invalid_active_fat");
 fixture(32,0);p32(sec(0)+44,1);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);finish("fat32_invalid_root");
 fixture(32,0);fat(2,2);init();{b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);}finish("fat32_root_loop");
 fixture(32,0);fat(4,0xffffff7);init();{b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);}finish("fat32_bad_cluster");
 fixture(32,0);fat(4,0xfffffff);init();{b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);}finish("fat32_premature_eoc");
 fixture(32,0);fat(4,0xf0000008);init();pattern(openfile("/DATA.BIN"),513);finish("fat32_upper_fat_bits_masked");
 RUN("chain_budget",{for(uint32_t c=20;c<20+B1_MAX_CHAIN;c++)fat(c,c+1);fat(20+B1_MAX_CHAIN,0xffff);p16(sec(m.root)+64+26,20);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_LIMIT);});

 RUN("seek_negative_set",{init();b1_handle h=openfile("/DATA.BIN");CHECK(b1_seek(h,-1,B1_SET)==B1_SEEK);});
 RUN("failed_open_preserves_other_handle",{init();b1_handle a=openfile("/ONE.BIN"),b;fat(4,4);CHECK(b1_open("/DATA.BIN",&b)==B1_CHAIN);pattern(a,512);});
 RUN("closed_handle_zero_read",{init();b1_handle h=openfile("/ONE.BIN");CHECK(b1_close(h)==B1_OK);size_t n=9;CHECK(b1_read(h,NULL,0,&n)==B1_HANDLE&&n==0);});
 RUN("info_size_offset",{init();b1_handle h=openfile("/MULTI.BIN");uint32_t pos,size;CHECK(b1_info(h,&pos,&size)==B1_OK&&pos==0&&size==1300);});
 RUN("all_closes_shutdown_no_read_or_write",{init();b1_handle a=openfile("/ONE.BIN"),b=openfile("/DATA.BIN");uint64_t reads=m.reads;CHECK(b1_close(a)==B1_OK);CHECK(b1_close(b)==B1_OK);b1_shutdown();CHECK(m.reads==reads);});
 RUN("large_read_count_clamped_without_overflow",{init();uint8_t b[513];size_t n;CHECK(b1_read(openfile("/DATA.BIN"),b,SIZE_MAX,&n)==B1_EOF&&n==513);});
 RUN("short_sector_callback_failure_open",{m.partial=1;m.fail=m.root;init();b1_handle h;CHECK(b1_open("/ONE.BIN",&h)==B1_IO);});
 RUN("empty_file_nonzero_bad_start",{p16(sec(m.root)+26,1);init();b1_handle h;CHECK(b1_open("/EMPTY.BIN",&h)==B1_CHAIN);});
 RUN("root_zero_entries_invalid_fat16",{p16(sec(0)+17,0);CHECK(b1_init(read_sector,&m,m.sectors)==B1_METADATA);});
 RUN("invalid_bytes_per_sector",{p16(sec(0)+11,1024);CHECK(b1_init(read_sector,&m,m.sectors)!=B1_OK);});
 RUN("root_chain_eoc_before_file_size",{p32(sec(m.root)+64+28,1536);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);});
 for(unsigned i=0;i<5;i++){
  const uint32_t values[]={0,1,0xffffff0,0xffffff7,70000};fixture(32,0);fat(4,values[i]);init();b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);char n[40];(void)snprintf(n,sizeof(n),"fat32_bad_chain_%u",i);finish(n);
 }
 fixture(32,0);fat(4,4);init();{b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_CHAIN);}finish("fat32_file_self_loop");
 fixture(32,0);p16(sec(0)+40,0x80);init();pattern(openfile("/DATA.BIN"),513);finish("fat32_active_fat_zero");
 fixture(32,0);p16(sec(m.root)+64+20,0xf000);init();{b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_METADATA);}finish("fat32_reserved_entry_cluster_bits");
 fixture(32,0);m.fail=m.root;init();{b1_handle h;CHECK(b1_open("/DATA.BIN",&h)==B1_IO);}finish("fat32_root_io_failure");
 fixture(32,0);fat(10,0xffffff7);init();{b1_handle h;CHECK(b1_open("/SUB/INNER.BIN",&h)==B1_CHAIN);}finish("fat32_directory_bad_cluster");
 fixture(32,0);init();{b1_handle h=openfile("/DATA.BIN");uint8_t b[513];size_t n;m.fail=clba(8);CHECK(b1_read(h,b,513,&n)==B1_IO&&n==512);}finish("fat32_partial_io");

 fixture(16,0);
 {uint32_t oldsectors=m.sectors,newsectors=m.data+(m.sectors-m.data)*2;
  uint8_t *old=m.bytes;m.bytes=calloc(newsectors,512);CHECK(m.bytes);m.sectors=newsectors;
  memcpy(m.bytes,old,(size_t)m.data*512);sec(0)[13]=2;p16(sec(0)+19,(uint16_t)newsectors);
  for(uint32_t c=3;c<=11;c++)for(unsigned j=0;j<1024;j++)m.bytes[(size_t)(m.data+(c-2)*2)*512+j]=(uint8_t)j;
  (void)oldsectors;free(old);init();pattern(openfile("/MULTI.BIN"),1300);
 }finish("two_sectors_per_cluster");
 printf("SUMMARY tests=%u pass=%u fail=0 skip=0 xfail=0 write_attempts=0\n",tests,tests);return 0;
}
