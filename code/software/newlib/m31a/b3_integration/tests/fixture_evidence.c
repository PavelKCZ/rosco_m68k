/* Evidence helper: exports only a deterministic in-memory prefix to stdout.
 * Sparse capacity beyond this prefix is zero-filled by fixture_read(). */
#include "fat_fixture.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
 if(argc!=5)return 2;
 int bits=atoi(argv[1]);if(bits!=16&&bits!=32)return 2;
 fixture_create(bits,atoi(argv[2]),atoi(argv[3]),atoi(argv[4]));
 size_t bytes=(size_t)disk.stored*512;
 int ok=fwrite(disk.image,1,bytes,stdout)==bytes;
 fixture_destroy();return ok?0:1;
}
