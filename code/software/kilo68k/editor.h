/* Kilo68k, BSD-2-Clause; see LICENSE. */
#ifndef KILO68K_EDITOR_H
#define KILO68K_EDITOR_H

#include <stdint.h>

#define EDITOR_MAX_ROWS 64U
#define EDITOR_MAX_COLS 256U
#define EDITOR_COLS 80U
#define EDITOR_TEXT_ROWS 23U

enum editor_diagnostic {
    EDITOR_DIAG_READY = 0,
    EDITOR_DIAG_LINE_FULL,
    EDITOR_DIAG_DOCUMENT_FULL
};

struct editor_row {
    uint16_t length;
    char text[EDITOR_MAX_COLS + 1U];
};

struct editor {
    struct editor_row rows[EDITOR_MAX_ROWS];
    uint16_t row_count;
    uint16_t cx;
    uint16_t cy;
    uint16_t coloff;
    uint16_t rowoff;
    uint8_t diagnostic;
};

void editor_init(struct editor *e);
/* -1 = invalid position, 1 = display changed, 0 = unchanged.
 * Quit is handled by main. Up/Down clamp cx without a preferred column. */
int editor_key(struct editor *e, int key);
int editor_render(const struct editor *e);

#endif
