/* Kilo68k M2.1: standalone serial terminal probe; no filesystem or heap. */
#include <termios.h>

#define ESC 27
#define CTRL_Q 17
#define KEY_BYTES 16

enum key {
    K_BYTE, K_ESCAPE, K_UP, K_DOWN, K_RIGHT, K_LEFT,
    K_HOME, K_END, K_PAGEUP, K_PAGEDOWN, K_DELETE, K_BACKSPACE,
    K_UNKNOWN, K_INCOMPLETE, K_TOO_LONG, K_QUIT
};

struct event {
    enum key key;
    unsigned char bytes[KEY_BYTES];
    unsigned int count;
};

static int write_all(const char *data, unsigned int count) {
    while (count) {
        int n = rosco_termios_write(0, data, count);
        if (n <= 0 || (unsigned int)n > count) return -1;
        data += n;
        count -= (unsigned int)n;
    }
    return 0;
}

static int text(const char *s) {
    unsigned int n = 0;
    while (s[n]) ++n;
    return write_all(s, n);
}

static int move_to(unsigned int row, unsigned int col) {
    char pos[] = "\033[00;00H";
    pos[2] = (char)('0' + row / 10);
    pos[3] = (char)('0' + row % 10);
    pos[5] = (char)('0' + col / 10);
    pos[6] = (char)('0' + col % 10);
    return write_all(pos, sizeof(pos) - 1);
}

static enum key final_key(unsigned char c) {
    switch (c) {
    case 'A': return K_UP;
    case 'B': return K_DOWN;
    case 'C': return K_RIGHT;
    case 'D': return K_LEFT;
    case 'H': return K_HOME;
    case 'F': return K_END;
    default: return K_UNKNOWN;
    }
}

/* 0 = idle timeout, 1 = event, -1 = I/O error.
 * Each continuation read times out after VTIME=1. Even a continuous stream
 * without a final byte is bounded by KEY_BYTES (no discard-until-final loop).
 */
static int read_event(struct event *e) {
    unsigned char c;
    int n = rosco_termios_read(0, &c, 1);
    if (n <= 0) return n;
    e->count = 1;
    e->bytes[0] = c;
    e->key = K_BYTE;
    if (c == CTRL_Q) e->key = K_QUIT;
    else if (c == 8 || c == 127) e->key = K_BACKSPACE;
    else if (c == ESC) {
        e->key = K_ESCAPE;
        while (e->count < KEY_BYTES) {
            n = rosco_termios_read(0, &c, 1);
            if (n < 0) return -1;
            if (n == 0) {
                if (e->count > 1) e->key = K_INCOMPLETE;
                return 1;
            }
            e->bytes[e->count++] = c;
            if (c == CTRL_Q) { e->key = K_QUIT; return 1; }
            if (e->count == 2) {
                if (c == '[' || c == 'O') continue;
                e->key = K_UNKNOWN;
                return 1;
            }
            if (c >= 0x40 && c <= 0x7e) {
                e->key = K_UNKNOWN;
                if (e->count == 3) e->key = final_key(c);
                if (e->bytes[1] == '[' && e->count == 4 && c == '~') {
                    switch (e->bytes[2]) {
                    case '1': case '7': e->key = K_HOME; break;
                    case '4': case '8': e->key = K_END; break;
                    case '3': e->key = K_DELETE; break;
                    case '5': e->key = K_PAGEUP; break;
                    case '6': e->key = K_PAGEDOWN; break;
                    }
                }
                return 1;
            }
            if (c < 0x20 || c > 0x3f) {
                e->key = K_UNKNOWN;
                return 1;
            }
        }
        e->key = K_TOO_LONG;
    }
    return 1;
}

static int show_event(const struct event *e) {
    static const char *const names[] = {
        "Byte", "Escape", "Up", "Down", "Right", "Left", "Home", "End",
        "PageUp", "PageDown", "Delete", "Backspace", "Unknown escape",
        "Incomplete escape", "Escape length limit", "Ctrl-Q"
    };
    static const char hex[] = "0123456789ABCDEF";
    unsigned int i;
    if (text("\033[7;1H\033[2KKey: ") < 0 || text(names[e->key]) < 0)
        return -1;
    if (e->key == K_BYTE && e->bytes[0] >= 32 && e->bytes[0] <= 126) {
        char printable = (char)e->bytes[0];
        if (text(" '") < 0 || write_all(&printable, 1) < 0 || text("'") < 0)
            return -1;
    }
    if (text("\033[8;1H\033[2KBytes (hex): ") < 0) return -1;
    for (i = 0; i < e->count; ++i) {
        char pair[3] = {hex[e->bytes[i] >> 4], hex[e->bytes[i] & 15], ' '};
        if (write_all(pair, sizeof(pair)) < 0) return -1;
    }
    return 0;
}

static int run_test(void) {
    unsigned int row = 12, col = 40;
    if (text("\033[0m\033[?25l\033[2J\033[H"
             "Kilo68k M2.1 - MC68010 serial terminal test\r\n"
             "Fixed screen: 80 columns x 25 rows; raw VMIN=0 VTIME=1\r\n"
             "Ctrl-Q: exit and restore terminal settings\r\n"
             "Arrows: move X; Home/End: column 1/80; PgUp/PgDn: row 10/23\r\n"
             "Press ordinary keys, Esc, Delete or Backspace to inspect them.") < 0 ||
        move_to(row, col) < 0 || text("X") < 0) return -1;
    for (;;) {
        struct event e;
        int result = read_event(&e);
        if (result < 0) return -1;
        if (result == 0) continue;
        if (e.key == K_QUIT) return 0;
        if (move_to(row, col) < 0 || text(" ") < 0) return -1;
        switch (e.key) {
        case K_UP: if (row > 10) --row; break;
        case K_DOWN: if (row < 23) ++row; break;
        case K_LEFT: if (col > 1) --col; break;
        case K_RIGHT: if (col < 80) ++col; break;
        case K_HOME: col = 1; break;
        case K_END: col = 80; break;
        case K_PAGEUP: row = 10; break;
        case K_PAGEDOWN: row = 23; break;
        default: break;
        }
        if (show_event(&e) < 0 || move_to(row, col) < 0 || text("X") < 0)
            return -1;
    }
}

int kmain(void) {
    struct termios saved, raw;
    int result = 1;
    int restored;
    /* No alternate console API: binding failure can only return a status. */
    if (rosco_termios_bind(0, 0) < 0) return 1;
    if (tcgetattr(0, &saved) < 0) {
        if (text("Kilo68k: tcgetattr failed\r\n") < 0) result = 1;
        goto unbind;
    }
    raw = saved;
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(0, TCSANOW, &raw) == 0) result = run_test() < 0;
    /* Attempt every cleanup step, including on setup/output/read errors. */
    if (text("\033[0m\033[?25h\033[25;1H\033[2K") < 0) result = 1;
    restored = tcsetattr(0, TCSANOW, &saved);
    if (restored < 0) result = 1;
    if (text(restored < 0 ? "Kilo68k: terminal restore FAILED"
                         : result ? "Kilo68k: error; terminal attributes restored"
                                  : "Kilo68k: terminal attributes restored") < 0)
        result = 1;
    /* CR only: avoid ONLCR double translation and bottom-row scrolling. */
    if (text("\r") < 0) result = 1;
unbind:
    if (rosco_termios_unbind(0) < 0) result = 1;
    return result;
}
