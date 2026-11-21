#ifndef B3_MOCK_H
#define B3_MOCK_H
#include "b3_integration.h"
#include "private.h"
#include "fat_fixture.h"
typedef struct {
 uint32_t support,init_status,type,csd_result;
 int initialized,partial_csd;
 unsigned calls[5],forbidden,sequence[4096],count;
 uint8_t csd[16];uint32_t last_lba;
} mock_state;
extern mock_state mock;
void mock_setup(int,int,int,int);
void check(int,const char *,int);
#define CHECK(x) check(!!(x),#x,__LINE__)
#endif
