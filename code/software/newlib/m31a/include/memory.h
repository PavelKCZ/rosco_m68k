#ifndef M31A_MEMORY_H
#define M31A_MEMORY_H
/* Shared with startup.S. Addresses are 32-bit firmware addresses, not host pointers. */
#define M31_RAM_MIN 0x00080000
#define M31_RAM_MAX 0x00E00000
#define M31_RUN 0x00002000
#define M31_LOAD 0x00040000
#define M31_STACK 0x00008000
#ifndef __ASSEMBLER__
#include <stdint.h>
#include <stddef.h>
struct m31_layout { uint32_t top, stack_bottom, heap_start, heap_end; };
struct m31_heap { uint32_t start, end, current; };
int m31_plan(uint32_t ram, uint32_t reset,
             uint32_t program_end, uint32_t load, uint32_t image_size,
             struct m31_layout *out);
int m31_heap_move(struct m31_heap *, int64_t increment, uint32_t *previous);
#endif
#endif
