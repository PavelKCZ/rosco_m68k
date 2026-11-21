//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                            FAT16/32 File IO Library
//                                    V2.6
//                              Ultra-Embedded.com
//                            Copyright 2003 - 2012
//
//                         Email: admin@ultra-embedded.com
//
//                                License: GPL
//   If you would like a version with a more permissive license for use in
//   closed source commercial applications please contact me for details.
//-----------------------------------------------------------------------------
//
// This file is part of FAT File IO Library.
//
// FAT File IO Library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// FAT File IO Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FAT File IO Library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//-----------------------------------------------------------------------------
/* RO derivative of local ultraembedded fat_io_lib. See PROVENANCE.json and
 * reference/fat_io_lib/LICENSE (GPL-2.0-or-later notices retained there).
 * Algorithms: fatfs_init, fatfs_lba_of_cluster, fatfs_find_next_cluster,
 * directory lookup and file sector reads. Mutating paths are removed, not hidden.
 */
#include "fat_ro.h"
#include "fat_defs.h"
#include <string.h>
#include <limits.h>
_Static_assert(sizeof(uint32)==4, "FAT uint32 width");
_Static_assert(sizeof(struct fat_dir_entry)==32, "FAT entry packing");
struct file {
 b1_handle token;
 uint32_t size, pos, n;
 b1_status error;
 uint32_t chain[B1_MAX_CHAIN];
};
static struct {
 b1_sector_read read; void *ctx; uint64_t media, end;
 uint32_t base, fat, fat_sectors, data, clusters, root, root_lba, root_entries;
 uint8_t spc, bits; int ready;
 struct file files[B1_MAX_HANDLES];
 uint32_t work[B1_MAX_CHAIN];
} fs;
/* Tokens are never reused, including across shutdown/reinitialization. */
static uint64_t serial;
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0]|((uint16_t)p[1]<<8)); }
static uint32_t le32(const uint8_t *p) { return (uint32_t)le16(p)|((uint32_t)le16(p+2)<<16); }
static b1_status raw(uint64_t lba, uint8_t *b) {
 if (lba>=fs.media || lba>UINT32_MAX) return B1_METADATA;
 return fs.read(fs.ctx,(uint32_t)lba,b)==1 ? B1_OK : B1_IO;
}
static b1_status sector(uint64_t lba, uint8_t *b) {
 if(lba<fs.base || lba>=fs.end) return B1_METADATA;
 return raw(lba,b);
}
static int valid_cluster(uint32_t c) {
 return c>=2 && c-2<fs.clusters && c<(fs.bits==16?0xfff0U:0xffffff0U);
}
/* Corrected FAT table lookup: no conflation of I/O error with end-of-chain. */
static b1_status next_cluster(uint32_t c,uint32_t *next,int *eoc) {
 uint8_t b[512]; uint64_t off=(uint64_t)c*(fs.bits/8U);
 if(!valid_cluster(c) || off/512>=fs.fat_sectors) return B1_CHAIN;
 b1_status s=sector((uint64_t)fs.fat+off/512,b); if(s!=B1_OK)return s;
 uint32_t n=fs.bits==16?le16(b+off%512):le32(b+off%512)&0xfffffffU;
 *eoc=n>=(fs.bits==16?0xfff8U:0xffffff8U);
 if(*eoc){*next=0;return B1_OK;}
 if(!valid_cluster(n))return B1_CHAIN; /* free, reserved, bad or out of range */
 *next=n;return B1_OK;
}
/* Full bounded validation at open; immutable chain snapshot belongs to handle.
 * Explicit maximum avoids unbounded FAT loops and allocations. */
static b1_status chain(uint32_t start,uint32_t *out,uint32_t *count) {
 uint32_t n=0,c=start;
 for(;;){
  if(!valid_cluster(c))return B1_CHAIN;
  for(uint32_t i=0;i<n;i++)if(out[i]==c)return B1_CHAIN;
  if(n==B1_MAX_CHAIN)return B1_LIMIT;
  out[n++]=c;
  uint32_t next;int eoc;b1_status s=next_cluster(c,&next,&eoc);if(s!=B1_OK)return s;
  if(eoc){*count=n;return B1_OK;} c=next;
 }
}
static b1_status cluster_sector(uint32_t c,uint32_t index,uint8_t *b) {
 if(!valid_cluster(c)||index>=fs.spc)return B1_CHAIN;
 return sector((uint64_t)fs.data+(uint64_t)(c-2)*fs.spc+index,b);
}
void b1_shutdown(void){memset(&fs,0,sizeof(fs));}
b1_status b1_init(b1_sector_read read,void *ctx,uint64_t media) {
 uint8_t b[512];uint64_t part_end;uint32_t base=0;
 if(fs.ready)return B1_STATE;
 if(!read || !media || media>UINT64_C(0x100000000))return B1_ARGUMENT;
 b1_shutdown();fs.read=read;fs.ctx=ctx;fs.media=media;
 b1_status s=raw(0,b);if(s!=B1_OK)goto fail;
 if(le16(b+SIGNATURE_POSITION)!=SIGNATURE_VALUE){s=B1_METADATA;goto fail;}
 part_end=media;
 /* Superfloppy or first primary FAT partition. No EBR/GPT traversal. */
 if(le16(b+BPB_BYTSPERSEC)!=512){
  uint8_t type=b[PARTITION1_TYPECODE_LOCATION];
  if(type!=4 && type!=6 && type!=0xe && type!=0xb && type!=0xc){s=B1_UNSUPPORTED;goto fail;}
  base=le32(b+PARTITION1_LBA_BEGIN_LOCATION);
  uint32_t len=le32(b+PARTITION1_SIZE_LOCATION);
  part_end=(uint64_t)base+len;
  if(!base||!len||part_end>media){s=B1_METADATA;goto fail;}
  s=raw(base,b);if(s!=B1_OK)goto fail;
 }
 if(le16(b+SIGNATURE_POSITION)!=SIGNATURE_VALUE || le16(b+BPB_BYTSPERSEC)!=512){s=B1_METADATA;goto fail;}
 uint32_t spc=b[BPB_SECPERCLUS],reserved=le16(b+BPB_RSVDSECCNT),nf=b[BPB_NUMFATS];
 uint32_t roots=le16(b+BPB_ROOTENTCNT),t16=le16(b+BPB_TOTSEC16),t32=le32(b+BPB_TOTSEC32);
 uint32_t total=t16?t16:t32,f16=le16(b+BPB_FATSZ16),fsize=f16?f16:le32(b+BPB_FAT32_FATSZ32);
 if(!spc || spc>128 || (spc&(spc-1)) || !reserved || !nf || nf>2 || !fsize || !total || (t16&&t32&&t16!=t32)) {s=B1_METADATA;goto fail;}
 uint64_t root_sectors=((uint64_t)roots*32+511)/512;
 uint64_t overhead=reserved+(uint64_t)nf*fsize+root_sectors;
 if(overhead>=total || (uint64_t)base+total>part_end){s=B1_METADATA;goto fail;}
 uint32_t clusters=(uint32_t)((total-overhead)/spc);
 if(clusters<4085){s=B1_UNSUPPORTED;goto fail;}
 uint8_t bits=clusters<65525?16:32;
 if((uint64_t)fsize*512/(bits/8U)<(uint64_t)clusters+2){s=B1_METADATA;goto fail;}
 uint32_t active=0;
 if(bits==16){
  if(!f16||!roots||roots%16){s=B1_METADATA;goto fail;}
 }else{
  uint16_t flags=le16(b+BPB_FAT32_EXTFLAGS);
  if(f16||roots||le16(b+BPB_FAT32_FSVER)||clusters>0xfffffeeU){s=B1_METADATA;goto fail;}
  if(flags&0x80)active=flags&15;
  if(active>=nf){s=B1_METADATA;goto fail;}
 }
 fs.base=base;fs.end=(uint64_t)base+total;fs.bits=bits;fs.spc=(uint8_t)spc;
 fs.fat=(uint32_t)((uint64_t)base+reserved+(uint64_t)active*fsize);fs.fat_sectors=fsize;
 fs.root_lba=(uint32_t)((uint64_t)base+reserved+(uint64_t)nf*fsize);fs.root_entries=roots;
 fs.data=(uint32_t)((uint64_t)fs.root_lba+root_sectors);fs.clusters=clusters;
 fs.root=bits==32?le32(b+BPB_FAT32_ROOTCLUS):0;
 if(bits==32&&!valid_cluster(fs.root)){s=B1_METADATA;goto fail;}
 fs.ready=1;return B1_OK;
fail:
 b1_shutdown();return s;
}
/* Bounded ASCII 8.3 paths. LFN entries skipped; existing short aliases usable. */
static b1_status name83(const char *p,size_t n,uint8_t out[11]) {
 size_t a=0,z=8;int dot=0;
 if(!n||n>12)return B1_ARGUMENT;
 memset(out,' ',11);
 for(size_t i=0;i<n;i++){
  unsigned char c=(unsigned char)p[i];
  if(c=='.'){if(dot||a==0||i+1==n)return B1_ARGUMENT;dot=1;continue;}
  if(c>='a'&&c<='z')c=(unsigned char)(c-'a'+'A');
  if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return B1_ARGUMENT;
  if(!dot){if(a==8)return B1_ARGUMENT;out[a++]=c;}
  else{if(z==11)return B1_ARGUMENT;out[z++]=c;}
 }
 return B1_OK;
}
struct entry {uint32_t start,size;uint8_t attr;};
static b1_status find(uint32_t dir,int fixed,const uint8_t name[11],struct entry *e) {
 uint8_t b[512];uint32_t nc=0;
 if(!fixed){b1_status s=chain(dir,fs.work,&nc);if(s!=B1_OK)return s;}
 uint32_t sectors=fixed?(fs.root_entries/16):nc*fs.spc;
 for(uint32_t i=0;i<sectors;i++){
  b1_status s=fixed?sector((uint64_t)fs.root_lba+i,b):cluster_sector(fs.work[i/fs.spc],i%fs.spc,b);
  if(s!=B1_OK)return s;
  for(unsigned j=0;j<512;j+=32){
   const uint8_t *p=b+j;uint8_t attr=p[11];
   if(p[0]==FILE_HEADER_BLANK)return B1_NOT_FOUND;
   if(p[0]==FILE_HEADER_DELETED||attr==FILE_ATTR_LFN_TEXT)continue;
   if(memcmp(p,name,11))continue;
   if(attr&0xc0)return B1_METADATA;
   if(attr&FILE_ATTR_VOLUME_ID)return B1_NOT_FOUND;
   uint32_t hi=le16(p+20);
   if((fs.bits==16&&hi)||(fs.bits==32&&(hi&0xf000)))return B1_METADATA;
   e->start=(hi<<16)|le16(p+26);e->size=le32(p+28);e->attr=attr;
   return B1_OK;
  }
 }
 return B1_NOT_FOUND;
}
static struct file *get(b1_handle h){
 if(!h||!fs.ready)return NULL;
 for(unsigned i=0;i<B1_MAX_HANDLES;i++)if(fs.files[i].token==h)return &fs.files[i];
 return NULL;
}
b1_status b1_open(const char *path,b1_handle *h) {
 if(!fs.ready)return B1_STATE;
 if(!path||!h)return B1_ARGUMENT;
 size_t len=0;while(len<B1_MAX_PATH&&path[len])len++;
 if(len<2||len==B1_MAX_PATH||path[0]!='/')return B1_ARGUMENT;
 /* Validate whole path before media access, including depth and final component. */
 unsigned depth=0;size_t start=1;uint8_t names[8][11];
 for(size_t i=1;i<=len;i++)if(path[i]=='/'||i==len){
  if(depth==8)return B1_LIMIT;
  b1_status s=name83(path+start,i-start,names[depth]);if(s!=B1_OK)return s;
  depth++;start=i+1;
 }
 struct file *f=NULL;
 for(unsigned i=0;i<B1_MAX_HANDLES;i++)if(!fs.files[i].token){f=&fs.files[i];break;}
 if(!f||serial==UINT64_MAX)return B1_RESOURCE;
 uint32_t dir=fs.root;int fixed=fs.bits==16;struct entry e={0,0,0};
 for(unsigned i=0;i<depth;i++){
  b1_status s=find(dir,fixed,names[i],&e);if(s!=B1_OK)return s;
  if(i+1<depth){if(!(e.attr&FILE_ATTR_DIRECTORY)||e.size)return B1_METADATA;dir=e.start;fixed=0;}
 }
 if(e.attr&FILE_ATTR_DIRECTORY)return B1_NOT_FILE;
 if(e.size>INT32_MAX)return B1_OVERFLOW;
 uint32_t nc=0;
 if(e.start){b1_status s=chain(e.start,f->chain,&nc);if(s!=B1_OK)return s;}
 if((uint64_t)nc*fs.spc*512<e.size)return B1_CHAIN;
 f->n=nc;f->size=e.size;f->pos=0;f->error=B1_OK;f->token=++serial;*h=f->token;return B1_OK;
}
b1_status b1_read(b1_handle h,void *buf,size_t count,size_t *got) {
 struct file *f=get(h);
 if(!got)return B1_ARGUMENT;
 *got=0;if(!f)return B1_HANDLE;
 if(!count)return B1_OK;
 if(!buf)return B1_ARGUMENT;
 if(f->error!=B1_OK)return f->error;
 if(f->pos>=f->size)return B1_EOF;
 size_t wanted=count;if(wanted>f->size-f->pos)wanted=f->size-f->pos;
 uint8_t b[512];
 while(*got<wanted){
  uint32_t index=f->pos/(512U*fs.spc),in_cluster=(f->pos/512)%fs.spc;
  if(index>=f->n){f->error=B1_CHAIN;return f->error;}
  b1_status s=cluster_sector(f->chain[index],in_cluster,b);
  if(s!=B1_OK){f->error=s;return s;}
  size_t n=512-f->pos%512;if(n>wanted-*got)n=wanted-*got;
  memcpy((uint8_t *)buf+*got,b+f->pos%512,n);f->pos+=(uint32_t)n;*got+=n;
 }
 return *got<count?B1_EOF:B1_OK;
}
b1_status b1_seek(b1_handle h,int64_t delta,b1_whence w) {
 struct file *f=get(h);if(!f)return B1_HANDLE;
 int64_t base;
 switch(w){case B1_SET:base=0;break;case B1_CUR:base=f->pos;break;case B1_END:base=f->size;break;default:return B1_SEEK;}
 if(delta>INT64_MAX-base)return B1_OVERFLOW;
 /* base >= 0, so adding a negative delta cannot underflow int64_t. */
 int64_t pos=base+delta;if(pos<0)return B1_SEEK;
 if(pos>INT32_MAX)return B1_OVERFLOW;
 f->pos=(uint32_t)pos;f->error=B1_OK;return B1_OK;
}
b1_status b1_info(b1_handle h,uint32_t *pos,uint32_t *size) {
 struct file *f=get(h);if(!f)return B1_HANDLE;
 if(!pos||!size)return B1_ARGUMENT;
 *pos=f->pos;*size=f->size;return B1_OK;
}
b1_status b1_close(b1_handle h){struct file *f=get(h);if(!f)return B1_HANDLE;memset(f,0,sizeof(*f));return B1_OK;}
