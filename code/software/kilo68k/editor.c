/* Bounded-row adaptation of antirez Kilo editing concepts.
 * Copyright (c) 2016, Salvatore Sanfilippo <antirez at gmail dot com>
 * BSD-2-Clause: copyright, conditions and disclaimer retained in LICENSE.
 */
#include "editor.h"
#include "terminal.h"

/* Static bounded frame; never place a screen-sized buffer on the stack. */
static char frame[4096];
static unsigned int frame_length;

void editor_init(struct editor *e) {
    unsigned int i;
    for (i = 0; i < EDITOR_MAX_ROWS; ++i) {
        e->rows[i].length = 0;
        e->rows[i].text[0] = '\0';
    }
    e->row_count = 1;
    e->cx = e->cy = e->coloff = e->rowoff = 0;
    e->diagnostic = EDITOR_DIAG_READY;
}

static int position_valid(const struct editor *e) {
    return e->row_count >= 1 && e->row_count <= EDITOR_MAX_ROWS &&
        e->cy < e->row_count && e->rows[e->cy].length <= EDITOR_MAX_COLS &&
        e->cx <= e->rows[e->cy].length && e->coloff <= e->cx &&
        (unsigned int)e->cx - e->coloff < EDITOR_COLS && e->rowoff <= e->cy &&
        (unsigned int)e->cy - e->rowoff < EDITOR_TEXT_ROWS;
}

static void update_offsets(struct editor *e) {
    if (e->cx < e->coloff) e->coloff = e->cx;
    else if ((unsigned int)e->cx - e->coloff >= EDITOR_COLS)
        e->coloff = e->cx - EDITOR_COLS + 1;
    /* Keep the logical cursor inside the 23x80 document viewport. */
    if (e->cy < e->rowoff) e->rowoff = e->cy;
    else if ((unsigned int)e->cy - e->rowoff >= EDITOR_TEXT_ROWS)
        e->rowoff = e->cy - EDITOR_TEXT_ROWS + 1;
}

static int reject(struct editor *e, uint8_t diagnostic) {
    int changed = e->diagnostic != diagnostic;
    e->diagnostic = diagnostic;
    return changed;
}

static int split_line(struct editor *e) {
    unsigned int i, j;
    struct editor_row *row = &e->rows[e->cy];
    struct editor_row *next;
    if (e->row_count == EDITOR_MAX_ROWS)
        return reject(e, EDITOR_DIAG_DOCUMENT_FULL);
    /* Destination never exceeds slot 63. Copy only initialized text + NUL,
     * backwards across rows, without a row-sized automatic temporary. */
    for (i = e->row_count; i > e->cy + 1U; --i) {
        e->rows[i].length = e->rows[i - 1].length;
        for (j = 0; j <= e->rows[i - 1].length; ++j)
            e->rows[i].text[j] = e->rows[i - 1].text[j];
    }
    next = &e->rows[e->cy + 1U];
    next->length = row->length - e->cx;
    for (j = 0; j <= next->length; ++j)
        next->text[j] = row->text[e->cx + j];
    row->length = e->cx;
    row->text[e->cx] = '\0';
    ++e->row_count;
    ++e->cy;
    e->cx = 0;
    e->diagnostic = EDITOR_DIAG_READY;
    update_offsets(e);
    return 1;
}

int editor_key(struct editor *e, int key) {
    unsigned int i;
    int changed = 0;
    struct editor_row *row;
    if (!position_valid(e)) return -1;
    row = &e->rows[e->cy];
    if (key == KEY_ENTER) return split_line(e);
    if (key >= 32 && key <= 126) {
        if (row->length == EDITOR_MAX_COLS)
            return reject(e, EDITOR_DIAG_LINE_FULL);
        /* Copy backwards, including NUL. Highest write is text[256]. */
        for (i = row->length + 1U; i > e->cx; --i)
            row->text[i] = row->text[i - 1];
        row->text[e->cx++] = (char)key;
        ++row->length;
        changed = 1;
    } else {
        switch (key) {
        case KEY_UP:
            if (e->cy > 0) {
                --e->cy;
                if (e->cx > e->rows[e->cy].length) e->cx = e->rows[e->cy].length;
                changed = 1;
            }
            break;
        case KEY_DOWN:
            if (e->cy + 1U < e->row_count) {
                ++e->cy;
                if (e->cx > e->rows[e->cy].length) e->cx = e->rows[e->cy].length;
                changed = 1;
            }
            break;
        case KEY_LEFT:
            if (e->cx) { --e->cx; changed = 1; }
            break;
        case KEY_RIGHT:
            if (e->cx < row->length) { ++e->cx; changed = 1; }
            break;
        case KEY_HOME:
            changed = e->cx != 0; e->cx = 0; break;
        case KEY_END:
            changed = e->cx != row->length; e->cx = row->length; break;
        case KEY_BACKSPACE:
            if (!e->cx) break;
            --e->cx;
            /* fall through */
        case KEY_DELETE:
            if (e->cx == row->length) break;
            for (i = e->cx; i < row->length; ++i)
                row->text[i] = row->text[i + 1];
            --row->length;
            changed = 1;
            break;
        default: break;
        }
    }
    if (changed) {
        e->diagnostic = EDITOR_DIAG_READY;
        update_offsets(e);
    }
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

/* CUP coordinates are 1..25 and 1..80: one or two decimal digits. */
static int coordinate(unsigned int n) {
    char digits[2];
    if (n < 1 || n > 80) return -1;
    if (n >= 10) {
        digits[0] = (char)('0' + n / 10);
        digits[1] = (char)('0' + n % 10);
        return append(digits, 2);
    }
    digits[0] = (char)('0' + n);
    return append(digits, 1);
}

static int cup(unsigned int row, unsigned int column) {
    if (row < 1 || row > 25 || column < 1 || column > EDITOR_COLS) return -1;
    if (literal("\033[") < 0 || coordinate(row) < 0 || literal(";") < 0 ||
        coordinate(column) < 0 || literal("H") < 0) return -1;
    return 0;
}

int editor_render(const struct editor *e) {
    const struct editor_row *current;
    unsigned int s, screen_row, screen_column;
    if (!position_valid(e) || e->diagnostic > EDITOR_DIAG_DOCUMENT_FULL) return -1;
    current = &e->rows[e->cy];
    screen_row = (unsigned int)e->cy - e->rowoff + 1U;
    screen_column = (unsigned int)e->cx - e->coloff + 1U;
    if (screen_row < 1 || screen_row > EDITOR_TEXT_ROWS ||
        screen_column < 1 || screen_column > EDITOR_COLS) return -1;
    if (terminal_service_rx() < 0) return -1;
    frame_length = 0;
    if (literal("\033[0m\033[?25l") < 0) return -1;
    for (s = 1; s <= EDITOR_TEXT_ROWS; ++s) {
        unsigned int document_row = e->rowoff + s - 1U;
        if (terminal_service_rx() < 0) return -1;
        if (cup(s, 1) < 0 || literal("\033[2K") < 0) return -1;
        if (document_row < e->row_count) {
            const struct editor_row *row = &e->rows[document_row];
            unsigned int i, visible;
            if (row->length > EDITOR_MAX_COLS || row->text[row->length] != '\0')
                return -1;
            /* A short neighbouring row may end before coloff. Do not form
             * its text+coloff pointer until the range is known to exist. */
            if (row->length <= e->coloff) continue;
            visible = row->length - e->coloff;
            if (visible > EDITOR_COLS) visible = EDITOR_COLS;
            /* At most 80 visible bytes, copied in <=16-byte blocks. RX
             * service cannot mutate the document or assembled frame. */
            for (i = 0; i < visible;) {
                unsigned int j, count = visible - i;
                if (count > 16U) count = 16U;
                for (j = 0; j < count; ++j) {
                    unsigned char c = (unsigned char)row->text[e->coloff + i + j];
                    if (c < 32 || c > 126) return -1;
                }
                if (append(row->text + e->coloff + i, count) < 0 ||
                    terminal_service_rx() < 0) return -1;
                i += count;
            }
        }
    }
    if (terminal_service_rx() < 0) return -1;
    if (cup(24, 1) < 0 || literal("\033[2KRow ") < 0 || number(e->cy + 1U) < 0 ||
        literal("/") < 0 || number(e->row_count) < 0 ||
        literal(" Len ") < 0 || number(current->length) < 0 ||
        literal("/256 Col ") < 0 || number(e->cx) < 0 ||
        literal(" Off ") < 0 || number(e->rowoff) < 0 ||
        literal(",") < 0 || number(e->coloff) < 0 ||
        literal(" | ") < 0 ||
        literal(e->diagnostic == EDITOR_DIAG_LINE_FULL ? "LINE FULL"
                : e->diagnostic == EDITOR_DIAG_DOCUMENT_FULL ? "DOCUMENT FULL"
                : "READY") < 0) return -1;
    /* One service covers both after status and before warning. */
    if (terminal_service_rx() < 0) return -1;
    if (cup(25, 1) < 0 || literal("\033[2K") < 0 ||
        literal("RAM ONLY - Ctrl-Q exits immediately and discards all text.") < 0)
        return -1;
    if (terminal_service_rx() < 0) return -1;
    if (cup(screen_row, screen_column) < 0 || literal("\033[?25h") < 0) return -1;
    /* Bound for this exact format: 10 + 2084 + 70 + 69 + 14 = 2247.
     * No ED/CR/LF, padding, colors, or variable user messages are emitted.
     * Assemble everything before issuing any write, including on errors. */
    if (frame_length > 2247U) return -1;
    if (terminal_service_rx() < 0) return -1;
    return terminal_write(frame, frame_length);
}
