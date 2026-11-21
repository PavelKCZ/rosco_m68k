#include "runtime.h"
#include "diagnostic.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
extern unsigned char _end[], _image_size[];
extern void __libc_init_array(void), __libc_fini_array(void);
extern void (*_ctors_start[])(void), (*_ctors_end[])(void);
extern void (*_dtors_start[])(void), (*_dtors_end[])(void);
extern int main(void);
struct m31_layout m31_memory;
/* No crti/crtn or second startup owner. Newlib array walkers call these. */
void _init(void) {}
void _fini(void) {}
static void finish(void) {
    void (**p)(void);
    for (p = _dtors_start; p != _dtors_end; ++p) (*p)();
    __libc_fini_array();
}
void m31_run_application(void) __attribute__((noreturn));
/* Testable C lifecycle; hardware metadata/assembly entry remain separate. */
void m31_run_application(void) {
    void (**p)(void);
    m31_heap_init(&m31_memory);
    M31_MARK('P');
    M31_MARK('B');
    if (m31_console_init()) { M31_ERROR(101); _exit(101); }
    M31_MARK('T');
    if (atexit(finish)) { M31_ERROR(102); _exit(102); }
    M31_MARK('A');
    __libc_init_array();
    M31_MARK('I');
    for (p = _ctors_end; p != _ctors_start;) (*--p)();
    M31_MARK('K');
    exit(main());
}
void m31_runtime_start(unsigned long top, unsigned long load) {
    M31_MARK('C');
    if (m31_plan(*(volatile uint32_t *)0x414,
                 *(volatile uint32_t *)0, (uint32_t)(uintptr_t)_end,
                 load, (uint32_t)(uintptr_t)_image_size, &m31_memory) ||
        m31_memory.top != top) { M31_ERROR(100); _exit(100); }
    m31_run_application();
}
