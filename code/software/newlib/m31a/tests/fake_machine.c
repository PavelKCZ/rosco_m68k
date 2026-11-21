#include "machine.h"
#include <string.h>
volatile uint32_t _TIMER_100HZ;
static unsigned char rx[1024], tx[2048];
static unsigned rn, ri, tn;
static int available = 1;
void fake_reset(void) { rn = ri = tn = 0; available = 1; }
void fake_available(int n) { available = n; }
void fake_input(const unsigned char *s, unsigned n) { if(n <= sizeof(rx)) { memcpy(rx,s,n); rn=n; ri=0; } }
unsigned fake_tx_count(void) { return tn; }
unsigned fake_rx_count(void) { return ri; }
const unsigned char *fake_tx(void) { return tx; }
bool mcCheckDeviceSupport(void) { return available != 0; }
uint8_t mcGetDeviceCount(void) { return 1; }
bool mcGetDevice(uint8_t n, CharDevice *d) { if(n) return false; memset(d,0,sizeof(*d)); return true; }
bool mcCheckDevice(CharDevice *d) { (void)d; return ri < rn; }
char mcReadDevice(CharDevice *d) { (void)d; return ri < rn ? rx[ri++] : 0; }
void mcSendDevice(char c, void *d) { (void)d; if(tn < sizeof(tx)) tx[tn++]=(unsigned char)c; }
