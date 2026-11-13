/* Independent ANSI screen interpreter for native tests only.
 * BSD-2-Clause; see ../LICENSE. Not linked into the target application. */
#include <string.h>
#include "ansi.h"

void ansi_init(struct ansi_screen *s, char fill) {
    memset(s, 0, sizeof(*s));
    memset(s->cells, fill, sizeof(s->cells));
    s->row = s->col = 1;
    s->visible = 1;
}

static int decimal(const char **p, unsigned int *value) {
    unsigned int digits = 0;
    *value = 0;
    while (**p >= '0' && **p <= '9') {
        if (++digits > 2) return -1;
        *value = *value * 10U + (unsigned int)(*(*p)++ - '0');
    }
    return digits ? 0 : -1;
}

static int csi(struct ansi_screen *s) {
    const char *p = s->sequence;
    unsigned int r, c;
    if (strcmp(p, "2K") == 0) {
        memset(s->cells[s->row - 1], ' ', 80);
    } else if (strcmp(p, "2J") == 0) {
        memset(s->cells, ' ', sizeof(s->cells));
    } else if (strcmp(p, "0m") == 0) {
        /* No colored/attributed cells in this renderer's format. */
    } else if (strcmp(p, "?25h") == 0) {
        s->visible = 1;
    } else if (strcmp(p, "?25l") == 0) {
        s->visible = 0;
    } else {
        if (decimal(&p, &r) < 0 || *p++ != ';' || decimal(&p, &c) < 0 ||
            *p++ != 'H' || *p != '\0' || r < 1 || r > 25 || c < 1 || c > 80)
            return -1;
        s->row = r; s->col = c;
        s->pending_wrap = 0; /* CUP cancels VT pending auto-wrap. */
    }
    return 0;
}

int ansi_feed(struct ansi_screen *s, const char *data, unsigned int count) {
    if (s->error) return -1;
    for (unsigned int i = 0; i < count; ++i) {
        unsigned char c = (unsigned char)data[i];
        if (c == 27) {
            /* A new ESC can abort an interrupted CSI during cleanup. */
            if (s->state) ++s->aborted_sequences;
            s->state = 1; s->used = 0;
        } else if (s->state == 1) {
            if (c != '[') goto invalid;
            s->state = 2;
        } else if (s->state == 2) {
            if (s->used >= sizeof(s->sequence) - 1 || c < 0x20 || c > 0x7e)
                goto invalid;
            s->sequence[s->used++] = (char)c;
            if (c >= 0x40) {
                s->sequence[s->used] = '\0';
                if (csi(s) < 0) goto invalid;
                s->state = 0;
            }
        } else if (c == '\r') {
            /* Cleanup uses CR; frame tests independently prohibit CR/LF. */
            s->col = 1; s->pending_wrap = 0;
        } else if (c >= 32 && c <= 126) {
            if (s->pending_wrap) {
                ++s->wraps;
                s->col = 1;
                s->pending_wrap = 0;
                if (s->row < 25) ++s->row;
                else {
                    ++s->scrolls;
                    for (unsigned int r = 0; r < 24; ++r)
                        memcpy(s->cells[r], s->cells[r + 1], 80);
                    memset(s->cells[24], ' ', 80);
                }
            }
            if (s->row == 25 && s->col == 80) ++s->bottom_right_writes;
            s->cells[s->row - 1][s->col - 1] = (char)c;
            if (s->col == 80) s->pending_wrap = 1;
            else ++s->col;
        } else goto invalid;
    }
    return 0;
invalid:
    s->error = 1;
    return -1;
}
