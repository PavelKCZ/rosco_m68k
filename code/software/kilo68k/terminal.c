/* Terminal lifecycle adapted from Kilo68k M2.1; BSD-2-Clause, see LICENSE. */
#include <termios.h>
#include "terminal.h"

enum parser_state { INPUT, ESCAPE, CSI, SS3, DISCARD, QUIET };
static enum parser_state state;
static unsigned char sequence[16];
static unsigned int used;
static int bound, have_saved;
static struct termios saved, timed_raw, polling_raw;
#define RX_CAPACITY 512U
#define RX_PUMP_LIMIT 16U
static unsigned char rx_fifo[RX_CAPACITY];
static unsigned int rx_head, rx_count;
static int transport_error, ready;

#ifdef KILO_RX_DIAGNOSTICS
static struct terminal_rx_diagnostics diag, frozen;
static int diag_active, diag_frozen;
void terminal_diag_increment(uint32_t *counter, uint8_t *saturated) {
    if (*counter == UINT32_MAX) *saturated = 1;
    else ++*counter;
}
#define DIAG_COUNT(field) do { if (diag_active) \
    terminal_diag_increment(&diag.field, &diag.saturated); } while (0)
int terminal_diag_reset(void) {
    if (!ready || transport_error || state != INPUT || rx_count) return -1;
    diag = (struct terminal_rx_diagnostics){0};
    diag_active = 1;
    diag_frozen = 0;
    return 0;
}
void terminal_diag_snapshot(struct terminal_rx_diagnostics *out, int triggered) {
    if (!diag_frozen) {
        frozen = diag;
        frozen.fifo_pending = (uint16_t)rx_count;
        frozen.parser_state = (uint8_t)state;
        frozen.transport_error = (uint8_t)transport_error;
        frozen.ctrl_t_included = (uint8_t)(triggered && diag_active);
        diag_frozen = 1;
        diag_active = 0;
    }
    *out = frozen;
}
void terminal_diag_delivered(void) { DIAG_COUNT(delivered); }
void terminal_diag_accepted(void) { DIAG_COUNT(accepted); }
#else
#define DIAG_COUNT(field) ((void)0)
#endif

/* This is the only unpumped output path, also used after mode restoration
 * during cleanup. Never recurse through terminal_write from cleanup. */
static int direct_write(const char *data, unsigned int count) {
    while (count) {
        int n = rosco_termios_write(0, data, count);
        if (n <= 0 || (unsigned int)n > count) return -1;
        data += n;
        count -= (unsigned int)n;
    }
    return 0;
}

/* Poll only with VMIN=VTIME=0. A zero result is not a parser timeout.
 * The fixed budget guarantees TX progress even under continuous RX. */
static int pump_rx(void) {
    unsigned int i;
    for (i = 0; i < RX_PUMP_LIMIT; ++i) {
        unsigned char c;
        int n = rosco_termios_read(0, &c, 1);
        if (n == 0) return 0;
        if (n != 1) return -1;
        DIAG_COUNT(rx_poll);
        if (rx_count == RX_CAPACITY) {
            DIAG_COUNT(overflows);
            return -1;
        }
        rx_fifo[(rx_head + rx_count) % RX_CAPACITY] = c;
        ++rx_count;
        DIAG_COUNT(enqueued);
#ifdef KILO_RX_DIAGNOSTICS
        if (diag_active && rx_count > diag.fifo_peak)
            diag.fifo_peak = (uint16_t)rx_count;
#endif
    }
    return 0;
}

int terminal_service_rx(void) {
    int failed = 0;
    /* Called by the frame builder and between batch inputs, never inside
     * TX or cleanup. All public successful operations leave timed raw mode. */
    if (!ready || transport_error) return -1;
    if (tcsetattr(0, TCSANOW, &polling_raw) < 0) failed = 1;
    if (!failed && pump_rx() < 0) failed = 1;
    /* Restore even if entering polling or pumping failed. A failed restore
     * permanently blocks subsequent reads, services and writes. */
    if (tcsetattr(0, TCSANOW, &timed_raw) < 0) failed = 1;
    if (failed) transport_error = 1;
    return failed ? -1 : 0;
}

int terminal_write(const char *data, unsigned int count) {
    int failed = 0;
    if (!ready || transport_error) return -1;
    if (count == 0) return 0;
    if (tcsetattr(0, TCSANOW, &polling_raw) < 0) failed = 1;
    if (!failed && pump_rx() < 0) failed = 1;
    while (!failed && count) {
        /* A single-byte request cannot have a positive short result. */
        int n = rosco_termios_write(0, data, 1);
        if (n != 1) { failed = 1; break; }
        ++data;
        --count;
        if (pump_rx() < 0) failed = 1;
    }
    /* Attempt restoration even if switching to polling or I/O failed. */
    if (tcsetattr(0, TCSANOW, &timed_raw) < 0) failed = 1;
    if (failed) transport_error = 1;
    return failed ? -1 : 0;
}

static int message(const char *s) {
    unsigned int n = 0;
    while (s[n]) ++n;
    return direct_write(s, n);
}

int terminal_open(void) {
    state = INPUT;
    used = 0;
    bound = have_saved = ready = transport_error = 0;
    rx_head = rx_count = 0;
#ifdef KILO_RX_DIAGNOSTICS
    diag = (struct terminal_rx_diagnostics){0};
    diag_active = diag_frozen = 0;
#endif
    if (rosco_termios_bind(0, 0) < 0) return -1;
    bound = 1;
    if (tcgetattr(0, &saved) < 0) return -1;
    have_saved = 1;
    timed_raw = saved;
    cfmakeraw(&timed_raw);
    timed_raw.c_cc[VMIN] = 0;
    timed_raw.c_cc[VTIME] = 1;
    polling_raw = timed_raw;
    polling_raw.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &timed_raw) < 0) return -1;
    ready = 1;
    /* The initial editor_render uses the same full layout as every redraw.
     * No preview text or separate initialization frame over document rows. */
    return 0;
}

int terminal_close(int failed) {
    int result = failed != 0 || transport_error;
    ready = 0;
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

static void parser_timeout(void) {
    if (state == ESCAPE || state == QUIET) state = INPUT;
    else if (state == CSI || state == SS3) state = DISCARD;
    /* Incomplete CSI/SS3 stays quarantined until a final byte or ESC. */
}

static enum terminal_input_result parse_byte(unsigned char c, int *key) {
    *key = KEY_NONE;
    if (c == 17) { *key = KEY_QUIT; return TERMINAL_INPUT_EVENT; }
    if (c == 27) {
        state = ESCAPE;
        used = 0;
        return TERMINAL_INPUT_PROGRESS;
    }
    if (state == INPUT) {
#ifdef KILO_RX_DIAGNOSTICS
        /* Raw control events only; command policy belongs to main.c. */
        if (c == 18 || c == 20) { *key = c; return TERMINAL_INPUT_EVENT; }
#endif
        if (c == 13) *key = KEY_ENTER; /* Raw CR; LF is always ignored. */
        else if (c == 8 || c == 127) *key = KEY_BACKSPACE;
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
    return *key == KEY_NONE ? TERMINAL_INPUT_PROGRESS : TERMINAL_INPUT_EVENT;
}

/* The mode flag affects direct RX and EMPTY handling only. FIFO input uses
 * the same parser without touching termios or counting RX a second time. */
static enum terminal_input_result read_input(int *key, int polling) {
    unsigned char c;
    int n = 0;
    *key = KEY_NONE;
    if (!ready || transport_error) return TERMINAL_INPUT_ERROR;
    if (rx_count) {
        c = rx_fifo[rx_head];
        rx_head = (rx_head + 1U) % RX_CAPACITY;
        --rx_count;
        n = 1;
    } else if (polling) {
        int failed = 0;
        if (tcsetattr(0, TCSANOW, &polling_raw) < 0) failed = 1;
        if (!failed) {
            n = rosco_termios_read(0, &c, 1);
            if (n == 1) { DIAG_COUNT(rx_direct); }
            if (n < 0 || n > 1) failed = 1;
        }
        /* Never parse a received byte if restoration failed. Keep any
         * earlier failure even if timed-mode restoration succeeds. */
        if (tcsetattr(0, TCSANOW, &timed_raw) < 0) failed = 1;
        if (failed) {
            transport_error = 1;
            return TERMINAL_INPUT_ERROR;
        }
    } else {
        n = rosco_termios_read(0, &c, 1);
        if (n == 1) { DIAG_COUNT(rx_direct); }
        if (n < 0 || n > 1) {
            transport_error = 1;
            return TERMINAL_INPUT_ERROR;
        }
    }
    if (n == 0) {
        if (!polling) parser_timeout();
        return TERMINAL_INPUT_EMPTY;
    }
    return parse_byte(c, key);
}

enum terminal_input_result terminal_read_input(int *key) {
    return read_input(key, 0);
}

enum terminal_input_result terminal_poll_input(int *key) {
    return read_input(key, 1);
}

int terminal_read_key(int *key) {
    enum terminal_input_result result = terminal_read_input(key);
    if (result == TERMINAL_INPUT_ERROR) return -1;
    return result == TERMINAL_INPUT_EVENT ? 1 : 0;
}

#ifdef KILO_RX_DIAGNOSTICS
/* Fixed format: one label (<=16), ten digits and CUP/EL per row.
 * This small buffer is static; no formatter or heap dependency. */
static char report_line[64];
static int report_value(unsigned int row, const char *label, uint32_t value) {
    unsigned int n = 0, i;
    uint32_t divisor = 1000000000UL;
    report_line[n++] = 27; report_line[n++] = '[';
    if (row >= 10) report_line[n++] = (char)('0' + row / 10);
    report_line[n++] = (char)('0' + row % 10);
    report_line[n++] = ';'; report_line[n++] = '1'; report_line[n++] = 'H';
    report_line[n++] = 27; report_line[n++] = '[';
    report_line[n++] = '2'; report_line[n++] = 'K';
    for (i = 0; label[i]; ++i) {
        if (n >= sizeof(report_line) - 10U) return -1;
        report_line[n++] = label[i];
    }
    for (i = 0; i < 10; ++i) {
        if (n == sizeof(report_line)) return -1;
        report_line[n++] = (char)('0' + value / divisor);
        value %= divisor;
        divisor /= 10;
    }
    return terminal_write(report_line, n);
}
int terminal_diag_report(const struct terminal_rx_diagnostics *s) {
    static const char title[] = "\033[0m\033[2J\033[1;1HRX DIAGNOSTICS - FROZEN; Ctrl-Q exits\033[?25h";
    static const char note[] = "\033[18;1HCtrl-T included=1: RX counts include snapshot command byte.";
    uint32_t total = UINT32_MAX;
    uint8_t total_saturated = s->saturated;
    if (UINT32_MAX - s->rx_direct >= s->rx_poll)
        total = s->rx_direct + s->rx_poll;
    else total_saturated = 1;
    if (terminal_write(title, sizeof(title) - 1U) < 0 ||
        report_value(2, "RX direct: ", s->rx_direct) < 0 ||
        report_value(3, "RX pump: ", s->rx_poll) < 0 ||
        report_value(4, "RX total: ", total) < 0 ||
        report_value(5, "FIFO enqueued: ", s->enqueued) < 0 ||
        report_value(6, "FIFO peak: ", s->fifo_peak) < 0 ||
        report_value(7, "FIFO pending: ", s->fifo_pending) < 0 ||
        report_value(8, "FIFO overflow: ", s->overflows) < 0 ||
        report_value(9, "Delivered: ", s->delivered) < 0 ||
        report_value(10, "Accepted: ", s->accepted) < 0 ||
        report_value(11, "Row length: ", s->row_length) < 0 ||
        report_value(12, "Cursor column: ", s->cx) < 0 ||
        report_value(13, "Cursor row(0): ", s->cy) < 0 ||
        report_value(14, "Parser state: ", s->parser_state) < 0 ||
        report_value(15, "Transport error: ", s->transport_error) < 0 ||
        report_value(16, "Saturated: ", total_saturated) < 0 ||
        report_value(17, "Ctrl-T included: ", s->ctrl_t_included) < 0 ||
        terminal_write(note, sizeof(note) - 1U) < 0) return -1;
    return 0;
}
#endif
