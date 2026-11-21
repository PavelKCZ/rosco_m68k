#include "memory.h"
#include <limits.h>
int m31_plan(uint32_t ram, uint32_t reset,
             uint32_t program_end, uint32_t load, uint32_t image_size,
             struct m31_layout *out) {
    struct m31_layout x;
    if (!out || ram < M31_RAM_MIN || ram > M31_RAM_MAX ||
        reset < M31_RAM_MIN || reset > M31_RAM_MAX || ((ram | reset) & 3)) return -1;
    x.top = ram < reset ? ram : reset;
    x.stack_bottom = x.top - M31_STACK;
    if (!image_size || (image_size & 3) || image_size > UINT32_MAX - M31_RUN ||
        program_end < M31_RUN + image_size || program_end > UINT32_MAX - 7) return -1;
    x.heap_start = (program_end + 7) & ~7U;
    x.heap_end = x.stack_bottom;
    /* Require a nonempty heap and disjoint initialized source/destination. */
    if (x.heap_start >= x.heap_end || (load & 3) || load < M31_RUN + image_size ||
        load > x.heap_end || image_size > x.heap_end - load) return -1;
    *out = x;
    return 0;
}
int m31_heap_move(struct m31_heap *h, int64_t increment, uint32_t *previous) {
    uint32_t next;
    if (!h || !previous || h->start > h->end || h->current < h->start ||
        h->current > h->end) return -1;
    if (increment >= 0) {
        if ((uint64_t)increment > (uint64_t)(h->end - h->current)) return -1;
        next = h->current + (uint32_t)increment;
    } else {
        /* Also safe for INT64_MIN. */
        uint64_t decrease = (uint64_t)(-(increment + 1)) + 1;
        if (decrease > h->current - h->start) return -1;
        next = h->current - (uint32_t)decrease;
    }
    *previous = h->current;
    h->current = next;
    return 0;
}
