/* Native-only substitutes for direct firmware diagnostics. No stdio recursion. */
static unsigned marks[64], tx_at_mark[64], used;
extern unsigned fake_tx_count(void);
void m31_diag_mark(unsigned code) { if(used<64) { tx_at_mark[used]=fake_tx_count(); marks[used++]=code; } }
void m31_diag_error(unsigned code) { if(used<64) marks[used++]=0x10000+code; }
unsigned m31_fake_mark_count(void) { return used; }
unsigned m31_fake_mark_at(unsigned n) { return n<used ? marks[n] : 0; }

unsigned m31_fake_tx_at(unsigned n) { return n<used ? tx_at_mark[n] : 0; }
