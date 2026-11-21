#ifndef M31A_RUNTIME_H
#define M31A_RUNTIME_H
#include "memory.h"
extern struct m31_layout m31_memory;
int m31_console_init(void);
void m31_heap_init(const struct m31_layout *);
void m31_runtime_start(unsigned long top, unsigned long load) __attribute__((noreturn));
#endif
