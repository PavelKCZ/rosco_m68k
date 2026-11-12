/* Kilo68k, BSD-2-Clause; see LICENSE. */
#ifndef KILO68K_EDITOR_H
#define KILO68K_EDITOR_H

#define EDITOR_MAX 256U
#define EDITOR_COLS 80U

struct editor {
    char text[EDITOR_MAX + 1];
    unsigned int length, cursor, offset;
    int full;
};

void editor_init(struct editor *e);
/* 1 = display changed, 0 = unchanged. Quit is handled by main. */
int editor_key(struct editor *e, int key);
int editor_render(const struct editor *e);

#endif
