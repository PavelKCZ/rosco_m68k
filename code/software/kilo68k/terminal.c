/* Terminal lifecycle adapted from Kilo68k M2.1; BSD-2-Clause, see LICENSE. */
#include <termios.h>
#include "terminal.h"

enum parser_state { INPUT, ESCAPE, CSI, SS3, DISCARD, QUIET };
static enum parser_state state;
static unsigned char sequence[16];
static unsigned int used;
static int bound, have_saved;
static struct termios saved;

int terminal_write(const char *data, unsigned int count) {
    while (count) {
        int n = rosco_termios_write(0, data, count);
        if (n <= 0 || (unsigned int)n > count) return -1;
        data += n;
        count -= (unsigned int)n;
    }
    return 0;
}

static int message(const char *s) {
    unsigned int n = 0;
    while (s[n]) ++n;
    return terminal_write(s, n);
}

int terminal_open(void) {
    struct termios raw;
    state = INPUT;
    used = 0;
    bound = have_saved = 0;
    if (rosco_termios_bind(0, 0) < 0) return -1;
    bound = 1;
    if (tcgetattr(0, &saved) < 0) return -1;
    have_saved = 1;
    raw = saved;
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(0, TCSANOW, &raw) < 0) return -1;
    return message("\033[0m\033[2J\033[H\033[?25h"
                   "Kilo68k M2.2-A - single-line RAM editor"
                   "\033[3;1HEdit row 5; ASCII only, maximum 256 characters."
                   "\033[24;1HLeft/Right Home/End | BS: before cursor | Delete: at cursor"
                   "\033[25;1HRAM ONLY - Ctrl-Q exits immediately and discards all text.");
}

int terminal_close(int failed) {
    int result = failed != 0;
    int restored = 0;
    if (bound) {
        /* Try every cleanup step even when a preceding write failed. */
        if (message("\033[0m\033[?25h\033[25;1H\033[2K") < 0) result = 1;
        if (have_saved && tcsetattr(0, TCSANOW, &saved) < 0) {
            restored = -1;
            result = 1;
        }
        if (message(restored < 0 ? "Kilo68k: terminal restore FAILED"
                    : result ? "Kilo68k: error; RAM text discarded"
                             : "Kilo68k: RAM text discarded") < 0) result = 1;
        if (message("\r") < 0) result = 1;
        if (rosco_termios_unbind(0) < 0) result = 1;
    }
    bound = have_saved = 0;
    return result;
}

static int decode(void) {
    if (used == 1) {
        switch (sequence[0]) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        case 'H': return KEY_HOME;
        case 'F': return KEY_END;
        }
    }
    if (state == CSI && used == 2 && sequence[1] == '~') {
        switch (sequence[0]) {
        case '1': case '7': return KEY_HOME;
        case '4': case '8': return KEY_END;
        case '3': return KEY_DELETE;
        }
    }
    return KEY_NONE;
}

int terminal_read_key(int *key) {
    unsigned char c;
    int n = rosco_termios_read(0, &c, 1);
    *key = KEY_NONE;
    if (n < 0 || n > 1) return -1;
    if (n == 0) {
        if (state == ESCAPE || state == QUIET) state = INPUT;
        else if (state == CSI || state == SS3) state = DISCARD;
        /* Incomplete CSI/SS3 stays quarantined until a final byte or ESC.
         * Each call still returns on timeout; there is no waiting loop. */
        return 0;
    }
    if (c == 17) { *key = KEY_QUIT; return 1; }
    if (c == 27) { state = ESCAPE; used = 0; return 0; }
    if (state == INPUT) {
        if (c == 8 || c == 127) *key = KEY_BACKSPACE;
        else if (c >= 32 && c <= 126) *key = c;
    } else if (state == ESCAPE) {
        if (c == '[') state = CSI;
        else if (c == 'O') state = SS3;
        else state = QUIET; /* Unknown ESC family: discard until idle. */
    } else if (state == CSI || state == SS3 || state == DISCARD) {
        if (state != DISCARD) {
            if (used < sizeof(sequence)) sequence[used++] = c;
            else state = DISCARD;
        }
        if (c >= 0x40 && c <= 0x7e) {
            if (state != DISCARD) *key = decode();
            state = INPUT;
        } else if (c < 0x20 || c > 0x3f) {
            state = DISCARD;
        }
    }
    return *key == KEY_NONE ? 0 : 1;
}
