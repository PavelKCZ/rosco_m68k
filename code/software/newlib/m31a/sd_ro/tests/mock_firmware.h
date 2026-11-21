#ifndef SDRO_MOCK_H
#define SDRO_MOCK_H
#include "private.h"
typedef struct {
 uint32_t support,init_result,type,reg_result,read_result;
 uint8_t initialized,csd[16];
 unsigned calls[5],trace[64],used;
 uint32_t last_lba;unsigned last_cmd;
 int partial_csd,partial_read,init_descriptor_zero,csd_aligned,read_aligned;
 uint8_t *last_descriptor,*last_csd,*last_bounce;
} mock_state;
extern mock_state mock;
void mock_reset(void);
void mock_csd_v1(uint32_t size,unsigned mult);
void mock_csd_v2(uint32_t size);
void test_check(int,const char *,int);
void test_done(const char *);
void test_decoder(void);
extern unsigned test_count;
#define CHECK(x) test_check(!!(x),#x,__LINE__)
#endif
