#include "runtime.h"
#include "diagnostic.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <reent.h>
static volatile unsigned bss_probe;
static volatile unsigned data_probe = 0x1234abcd;
static volatile unsigned ctor_probe;
static void __attribute__((constructor)) construct(void) { ctor_probe = 0x6789; }
static void __attribute__((destructor)) destruct(void) { puts("M31A destructor"); }
static void goodbye(void) { puts("M31A atexit"); }
int main(void) {
    unsigned char *p;
    void *before;
    M31_MARK('M');
    if (bss_probe || data_probe != 0x1234abcd || ctor_probe != 0x6789) return 10;
    if (atexit(goodbye)) return 11;
    M31_MARK('O');
    printf("M31A top=%08lx stack=%08lx heap=%08lx..%08lx\n",
           (unsigned long)m31_memory.top, (unsigned long)m31_memory.stack_bottom,
           (unsigned long)m31_memory.heap_start, (unsigned long)m31_memory.heap_end);
    M31_MARK('F');
    p = malloc(513);
    if (!p) return 12;
    memset(p, 0xa5, 513);
    if (p[0] != 0xa5 || p[512] != 0xa5) return 13;
    free(p);
    before = _sbrk_r(_REENT, 0);
    errno = 0;
    if (_sbrk_r(_REENT, PTRDIFF_MAX) != (void *)-1 || errno != ENOMEM ||
        _sbrk_r(_REENT, 0) != before) return 14;
    puts("M31A software checks complete; returning via exit");
    return 0;
}
