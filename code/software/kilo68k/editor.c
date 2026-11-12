/* Single-row adaptation of antirez Kilo editing concepts.
 * Copyright (c) 2016, Salvatore Sanfilippo <antirez at gmail dot com>
 * BSD-2-Clause: copyright, conditions and disclaimer retained in LICENSE.
 */
#include "editor.h"
#include "terminal.h"

/* Static bounded frame; never place a screen-sized buffer on the stack. */
static char frame[384];
static unsigned int frame_length;

void editor_init(struct editor *e) {
    e->text[0] = '\0';
    e->length = e->cursor = e->offset = 0;
    e->full = 0;
}

int editor_key(struct editor *e, int key) {
    unsigned int i;
    int changed = 0;
    if (key >= 32 && key <= 126) {
        if (e->length == EDITOR_MAX) {
            changed = !e->full;
            e->full = 1;
            return changed;
        }
        /* Copy backwards, including NUL. Highest write is text[256]. */
        for (i = e->length + 1; i > e->cursor; --i)
            e->text[i] = e->text[i - 1];
        e->text[e->cursor++] = (char)key;
        ++e->length;
        changed = 1;
    } else {
        switch (key) {
        case KEY_LEFT:
            if (e->cursor) { --e->cursor; changed = 1; }
            break;
        case KEY_RIGHT:
            if (e->cursor < e->length) { ++e->cursor; changed = 1; }
            break;
        case KEY_HOME:
            changed = e->cursor != 0; e->cursor = 0; break;
        case KEY_END:
            changed = e->cursor != e->length; e->cursor = e->length; break;
        case KEY_BACKSPACE:
            if (!e->cursor) break;
            --e->cursor;
            /* fall through */
        case KEY_DELETE:
            if (e->cursor == e->length) break;
            for (i = e->cursor; i < e->length; ++i)
                e->text[i] = e->text[i + 1];
            --e->length;
            changed = 1;
            break;
        default: break;
        }
    }
    if (changed && e->full) e->full = 0;
    if (e->cursor < e->offset) e->offset = e->cursor;
    if (e->cursor - e->offset >= EDITOR_COLS)
        e->offset = e->cursor - EDITOR_COLS + 1;
    return changed;
}

static int append(const char *data, unsigned int count) {
    unsigned int i;
    if (count > sizeof(frame) - frame_length) return -1;
    for (i = 0; i < count; ++i) frame[frame_length++] = data[i];
    return 0;
}

static int literal(const char *s) {
    unsigned int n = 0;
    while (s[n]) ++n;
    return append(s, n);
}

static int number(unsigned int n) {
    char digits[3];
    if (n > 999) return -1;
    digits[0] = (char)('0' + n / 100);
    digits[1] = (char)('0' + n / 10 % 10);
    digits[2] = (char)('0' + n % 10);
    return append(digits, sizeof(digits));
}

int editor_render(const struct editor *e) {
    unsigned int visible = e->length - e->offset;
    unsigned int column = e->cursor - e->offset + 1;
    char pos[] = "\033[5;00H\033[?25h";
    if (visible > EDITOR_COLS) visible = EDITOR_COLS;
    if (column < 1 || column > EDITOR_COLS) return -1;
    pos[4] = (char)('0' + column / 10);
    pos[5] = (char)('0' + column % 10);
    frame_length = 0;
    if (literal("\033[?25l\033[5;1H\033[2K") < 0 ||
        append(e->text + e->offset, visible) < 0 ||
        literal("\033[7;1H\033[2KLength ") < 0 || number(e->length) < 0 ||
        literal("/256  Cursor ") < 0 || number(e->cursor) < 0 ||
        literal("  View offset ") < 0 || number(e->offset) < 0 ||
        literal("\033[8;1H\033[2K") < 0 ||
        literal(e->full ? "FULL - insertion rejected; text and cursor unchanged."
                        : "Enter, Tab, Up/Down and other controls are ignored.") < 0 ||
        append(pos, sizeof(pos) - 1) < 0) return -1;
    return terminal_write(frame, frame_length);
}
