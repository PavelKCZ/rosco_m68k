/* Native B1.1/B1.2/B1.3a tests against production sources, with termios mocks.
 * BSD-2-Clause; see ../LICENSE. No serial device or filesystem is used. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include "editor.h"
#include "terminal.h"
#include "ansi.h"

static struct editor e, before;
static char reference[EDITOR_MAX_COLS + 1U];
static char output[8192];
static unsigned int output_length;
static unsigned int largest_frame;
static struct ansi_screen screen;
static char expected_screen[25][80];
static int model_output;
static int input[512], input_length, input_at;
static int fault, writes, fail_write, partial, sets, restores, unbinds;
static struct termios original;
static int polling, fail_set, rx_failure, invalid_write;
static unsigned int poll_reads;
/* Bytes become available after a specified number of TX calls. */
static unsigned char arrivals[2048];
static int arrival_time[2048];
static unsigned int arrival_count, arrival_at;
static int arrival_service[2048], poll_sessions, fail_set_second;
/* Optional shared stream for direct API tests. -1 is an explicit timed
 * timeout: polling sees no byte and leaves that marker for the timed read.
 * Old tests retain their separate timed input / polling-arrival model. */
static int shared_input, shared_available;
static unsigned int read_calls, api_poll_sets, api_timed_sets, api_cases;
enum batch_case {
    B_SINGLE, B_MULTI, B_130, B_CONTINUOUS, B_PROGRESS,
    B_ESC_BOUNDARY, B_SS3_BOUNDARY, B_CSI_EMPTY, B_SS3_EMPTY,
    B_ESC_TIMEOUT, B_UNKNOWN, B_MALFORMED, B_DISCARD, B_QUIET,
    B_NOOPS, B_MOVES, B_FULL_LINE, B_FULL_DOC, B_QUIT,
    B_RX_ERROR, B_POLL_ENTRY, B_POLL_RESTORE, B_RENDER_ERROR, B_TX_ERROR,
    B_OVERFLOW, B_RESET, B_RESET_FIFO, B_RESET_EDIT, B_RESET_DIRTY,
    B_SNAPSHOT, B_REPORT_ERROR
};
static int batch_testing, batch_kind, batch_start, batch_first_end;
static int batch_target_frames, batch_release, batch_reset_pending;
static int batch_frames, batch_started, batch_reports, batch_frame_open;
static int batch_reset_sets, batch_generated, batch_progress_timed;
static unsigned int batch_prefix, batch_show, batch_report_prefix, batch_cases;
/* RX02 one-byte releases at successive service opportunities; no clock model. */
static int batch_stagger, batch_stagger_session;
static unsigned int batch_stagger_calls;
static unsigned int batch_render_prefix;
static int batch_render_failed;
static void batch_before_rx(int poll);
static void batch_after_rx(int byte, int poll);
static void batch_observe_tx(const void *data, unsigned int count);
static void arrive_service(unsigned char c, int session) {
    assert(arrival_count < sizeof(arrivals));
    arrivals[arrival_count] = c;
    arrival_time[arrival_count] = 0;
    arrival_service[arrival_count++] = session;
}
static void arrive(unsigned char c, int after_writes) {
    assert(arrival_count < sizeof(arrivals));
    arrivals[arrival_count] = c;
    arrival_service[arrival_count] = 0;
    arrival_time[arrival_count++] = after_writes;
}
int kmain(void);

static void reset_io(void) {
    output_length = 0;
    output[0] = '\0';
    input_length = input_at = fault = writes = fail_write = partial = 0;
    sets = restores = unbinds = 0;
    polling = fail_set = rx_failure = invalid_write = 0;
    poll_sessions = fail_set_second = 0;
    shared_input = shared_available = 0;
    read_calls = api_poll_sets = api_timed_sets = 0;
    batch_testing = 0;
    poll_reads = arrival_count = arrival_at = 0;
    model_output = 0;
    ansi_init(&screen, '#');
    memset(&original, 0, sizeof(original));
    original.c_lflag = ICANON | ECHO;
}

int rosco_termios_bind(int fd, uint8_t device) {
    assert(fd == 0 && device == 0);
    return fault == 1 ? -1 : 0;
}

int tcgetattr(int fd, struct termios *t) {
    assert(fd == 0);
    if (fault == 2) return -1;
    *t = original;
    return 0;
}

void cfmakeraw(struct termios *t) {
    t->c_iflag = t->c_oflag = t->c_lflag = 0;
}

int tcsetattr(int fd, int action, const struct termios *t) {
    assert(fd == 0 && action == TCSANOW);
    ++sets;
    if (memcmp(t, &original, sizeof(original)) == 0) {
        ++restores;
        return fault == 4 ? -1 : 0;
    }
    assert(t->c_cc[VMIN] == 0 && t->c_cc[VTIME] <= 1);
    assert(t->c_lflag == 0);
    if (shared_input) {
        if (t->c_cc[VTIME] == 0) ++api_poll_sets;
        else ++api_timed_sets;
    }
    if ((sets == 1 && fault == 3) || sets == fail_set || sets == fail_set_second) return -1;
    polling = t->c_cc[VTIME] == 0;
    if (polling) ++poll_sessions;
    return 0;
}

int rosco_termios_unbind(int fd) {
    assert(fd == 0);
    ++unbinds;
    return fault == 5 ? -1 : 0;
}

int rosco_termios_read(int fd, void *data, unsigned int count) {
    int c;
    assert(fd == 0 && count == 1);
    ++read_calls;
    if (shared_input) {
        if (batch_testing) batch_before_rx(polling);
        if (polling) ++poll_reads;
        if (rx_failure) return rx_failure;
        /* Controlled burst after the payload has been received, not parsed.
         * This is a FIFO overflow stimulus, not a UART timing model. */
        if (batch_testing && batch_kind == B_OVERFLOW && polling &&
            input_at == batch_start + 64 && batch_generated < 513) {
            ++batch_generated;
            *(unsigned char *)data = 'x';
            return 1;
        }
        if (input_at == shared_available) {
            assert(polling); /* Timed reads need an explicit timeout marker. */
            return 0;
        }
        c = input[input_at];
        if (c == -1 && polling) return 0;
        ++input_at;
        if (c == -1) return 0;
        if (c == -2) return -1;
        assert(c >= 0 && c <= 255);
        if (batch_testing) batch_after_rx(c, polling);
        *(unsigned char *)data = (unsigned char)c;
        return 1;
    }
    if (polling) {
        ++poll_reads;
        if (rx_failure) return rx_failure;
        if (arrival_at == arrival_count || arrival_time[arrival_at] > writes ||
            arrival_service[arrival_at] > poll_sessions)
            return 0;
        *(unsigned char *)data = arrivals[arrival_at++];
        return 1;
    }
    assert(input_at < input_length);
    c = input[input_at++];
    if (c == -1) return 0; /* timeout */
    if (c == -2) return -1;
    *(unsigned char *)data = (unsigned char)c;
    return 1;
}

int rosco_termios_write(int fd, const void *data, unsigned int count) {
    assert(fd == 0);
    ++writes;
    if (batch_testing && batch_kind == B_RENDER_ERROR && batch_frames == 1 &&
        polling && count == 1 && !batch_render_failed) {
        static const char prefix[] = "\033[0m\033";
        /* Production editor_render -> terminal_write sends this frame
         * prefix in single-byte polling TX. Input polls never write, and
         * cleanup uses direct writes after restoring timed mode. */
        assert(input_at == batch_start + 1 && !fail_set && !fail_write);
        assert(*(const char *)data == prefix[batch_render_prefix]);
        if (batch_render_prefix == sizeof(prefix) - 2U) {
            batch_render_failed = 1;
            return -1; /* Fail inside this renderer's TX, after a complete SGR. */
        }
        ++batch_render_prefix;
    }
    if (writes == fail_write) return -1;
    if (fault == 6) return 0;
    if (invalid_write) return (int)count + 1;
    if (partial && count > (unsigned int)partial) count = (unsigned int)partial;
    if (batch_testing) batch_observe_tx(data, count);
    else if (model_output) assert(ansi_feed(&screen, data, count) == 0);
    /* The screen interpreter streams indefinitely; keep only a bounded
     * recent capture for frame-size/byte checks and legacy diagnostics. */
    assert(count < sizeof(output));
    if (output_length + count >= sizeof(output)) output_length = 0;
    memcpy(output + output_length, data, count);
    output_length += count;
    output[output_length] = '\0';
    return (int)count;
}

static void invariants(void) {
    unsigned int i, j;
    assert(e.row_count >= 1 && e.row_count <= EDITOR_MAX_ROWS);
    assert(e.cy < e.row_count && e.cx <= e.rows[e.cy].length);
    assert(e.coloff <= e.cx && (unsigned int)e.cx - e.coloff < EDITOR_COLS);
    assert(e.rowoff <= e.cy && (unsigned int)e.cy - e.rowoff < EDITOR_TEXT_ROWS);
    for (i = 0; i < e.row_count; ++i) {
        assert(e.rows[i].length <= EDITOR_MAX_COLS);
        assert(e.rows[i].text[e.rows[i].length] == '\0');
        for (j = 0; j < e.rows[i].length; ++j)
            assert(e.rows[i].text[j] >= 32 && e.rows[i].text[j] <= 126);
    }
    assert(e.diagnostic <= EDITOR_DIAG_DOCUMENT_FULL);
}

static void check_editor_screen(void) {
    static const char warning[] = "RAM ONLY - Ctrl-Q exits immediately and discards all text.";
    char status[80];
    const char *diagnostic;
    int length;
    /* Compute every expected cell from document coordinates, independently
     * of ANSI serialization and the renderer's clipping/append helpers. */
    memset(expected_screen, ' ', sizeof(expected_screen));
    for (unsigned int y = 0; y < 23; ++y) {
        for (unsigned int x = 0; x < 80; ++x) {
            unsigned int doc_y = y + e.rowoff, doc_x = x + e.coloff;
            if (doc_y < e.row_count && doc_x < e.rows[doc_y].length)
                expected_screen[y][x] = e.rows[doc_y].text[doc_x];
        }
    }
    diagnostic = e.diagnostic == EDITOR_DIAG_READY ? "READY" :
        e.diagnostic == EDITOR_DIAG_LINE_FULL ? "LINE FULL" : "DOCUMENT FULL";
    length = snprintf(status, sizeof(status), "Row %03u/%03u Len %03u/256 Col %03u Off %03u,%03u | %s",
             (unsigned int)e.cy + 1U, (unsigned int)e.row_count,
             (unsigned int)e.rows[e.cy].length, (unsigned int)e.cx,
             (unsigned int)e.rowoff, (unsigned int)e.coloff, diagnostic);
    assert(length >= 0 && (unsigned int)length < sizeof(status) && length <= 80);
    memcpy(expected_screen[23], status, (unsigned int)length);
    assert(sizeof(warning) - 1 <= 79);
    memcpy(expected_screen[24], warning, sizeof(warning) - 1);
    assert(memcmp(screen.cells, expected_screen, sizeof(expected_screen)) == 0);
    assert(screen.row == (unsigned int)e.cy - e.rowoff + 1U);
    assert(screen.col == (unsigned int)e.cx - e.coloff + 1U);
    assert(screen.visible && !screen.pending_wrap && screen.state == 0 && !screen.error);
    assert(screen.wraps == 0 && screen.scrolls == 0 && screen.bottom_right_writes == 0);
}

static void render_check(void) {
    output_length = 0;
    model_output = 1;
    assert(editor_render(&e) == 0);
    model_output = 0;
    assert(output_length <= 2247 && output_length < 4096);
    if (output_length > largest_frame) largest_frame = output_length;
    check_editor_screen();
    assert(!strchr(output, '\n') && !strchr(output, '\r'));
}

static int key(int k) {
    int result = editor_key(&e, k);
    assert(result >= 0);
    invariants();
    render_check();
    return result;
}

static void init(void) {
    unsigned int i;
    editor_init(&e);
    assert(e.row_count == 1 && e.cx == 0 && e.cy == 0);
    assert(e.coloff == 0 && e.rowoff == 0 && e.diagnostic == EDITOR_DIAG_READY);
    for (i = 0; i < EDITOR_MAX_ROWS; ++i)
        assert(e.rows[i].length == 0 && e.rows[i].text[0] == '\0');
    invariants();
}

/* Set up an explicit valid host fixture, not an application navigation API. */
static void row(unsigned int index, const char *s) {
    size_t n = strlen(s);
    assert(index < EDITOR_MAX_ROWS && n <= EDITOR_MAX_COLS);
    memcpy(e.rows[index].text, s, n + 1);
    e.rows[index].length = (uint16_t)n;
}

static void cursor(unsigned int cy, unsigned int cx) {
    e.cy = (uint16_t)cy;
    e.cx = (uint16_t)cx;
    e.coloff = (uint16_t)(cx >= 80 ? cx - 79 : 0);
    e.rowoff = (uint16_t)(cy >= 23 ? cy - 22 : 0);
    invariants();
}

static void unchanged(uint8_t diagnostic) {
    before.diagnostic = diagnostic;
    assert(memcmp(&e, &before, sizeof(e)) == 0);
}

static void split_tests(void) {
    unsigned int n, p, i;
    init();
    key(KEY_ENTER);
    assert(e.row_count == 2 && e.cy == 1 && e.cx == 0);
    assert(e.rows[0].length == 0 && e.rows[1].length == 0);
    /* Every split position for every length, with following rows already
     * present. Includes lengths 0,1,79,80,81,255,256 and split start/end. */
    for (n = 0; n <= 256; ++n) {
        for (i = 0; i < n; ++i) reference[i] = (char)('a' + i % 26);
        reference[n] = '\0';
        for (p = 0; p <= n; ++p) {
            init();
            row(0, "before"); row(1, reference); row(2, "after1"); row(3, "after2");
            e.row_count = 4;
            cursor(1, p);
            assert(key(KEY_ENTER) == 1);
            assert(e.row_count == 5 && e.cy == 2 && e.cx == 0 && e.coloff == 0);
            assert(e.rows[1].length == p && e.rows[2].length == n - p);
            assert(memcmp(e.rows[1].text, reference, p) == 0);
            assert(strcmp(e.rows[2].text, reference + p) == 0);
            assert(strcmp(e.rows[0].text, "before") == 0);
            assert(strcmp(e.rows[3].text, "after1") == 0);
            assert(strcmp(e.rows[4].text, "after2") == 0);
        }
    }
    init();
    for (i = 1; i < 64; ++i) {
        key('a' + (int)(i % 26));
        key(KEY_ENTER);
    }
    assert(e.row_count == 64 && e.cy == 63 && e.rowoff == 41);
    /* Rejection at every row, not just at end. Preserve even unused bytes
     * and offsets; diagnostic is the only permitted state change. */
    for (i = 0; i < 64; ++i) {
        cursor(i, e.rows[i].length);
        e.diagnostic = EDITOR_DIAG_READY;
        before = e;
        assert(key(KEY_ENTER) == 1);
        unchanged(EDITOR_DIAG_DOCUMENT_FULL);
        assert(key(KEY_ENTER) == 0);
        unchanged(EDITOR_DIAG_DOCUMENT_FULL);
    }
    key('Q'); /* Document full does not prohibit edits within a row. */
    assert(e.rows[63].length == 1 && e.diagnostic == EDITOR_DIAG_READY);
    init();
    for (i = 0; i < 63; ++i) {
        char label[16];
        snprintf(label, sizeof(label), "row-%03u", i);
        row(i, label);
    }
    e.row_count = 63;
    cursor(0, 1);
    key(KEY_ENTER); /* Shift through the very last available slot. */
    assert(e.row_count == 64 && strcmp(e.rows[0].text, "r") == 0);
    assert(strcmp(e.rows[1].text, "ow-000") == 0);
    for (i = 2; i < 64; ++i) {
        char label[16];
        snprintf(label, sizeof(label), "row-%03u", i - 1);
        assert(strcmp(e.rows[i].text, label) == 0);
    }
    puts("PASS splits: all lengths/positions, subsequent order, 64-row boundary and atomic rejections");
}

static void editing_tests(void) {
    unsigned int n, p, i;
    for (n = 0; n <= 256; ++n) {
        for (p = 0; p <= n; ++p) {
            for (int op = 0; op < 3; ++op) {
                init();
                /* Exercise a nonzero current row and preserve its neighbours. */
                row(0, "left"); row(2, "right"); e.row_count = 3;
                for (i = 0; i < n; ++i) reference[i] = (char)('a' + i % 26);
                reference[n] = '\0'; row(1, reference); cursor(1, p);
                if (op == 0) {
                    before = e;
                    key('X');
                    if (n == 256) {
                        unchanged(EDITOR_DIAG_LINE_FULL);
                        assert(key('X') == 0);
                        unchanged(EDITOR_DIAG_LINE_FULL);
                    } else {
                        memmove(reference + p + 1, reference + p, n - p + 1);
                        reference[p] = 'X';
                        assert(e.cx == p + 1 && e.rows[1].length == n + 1);
                    }
                } else if (op == 1) {
                    key(KEY_BACKSPACE);
                    if (p) memmove(reference + p - 1, reference + p, n - p + 1);
                    assert(e.cx == (p ? p - 1 : 0));
                    assert(e.rows[1].length == n - (p != 0));
                } else {
                    key(KEY_DELETE);
                    if (p < n) memmove(reference + p, reference + p + 1, n - p);
                    assert(e.cx == p && e.rows[1].length == n - (p < n));
                }
                assert(strcmp(e.rows[1].text, reference) == 0);
                assert(strcmp(e.rows[0].text, "left") == 0);
                assert(strcmp(e.rows[2].text, "right") == 0);
            }
        }
    }
    init();
    for (i = 0; i < 256; ++i) key('a');
    assert(e.cx == 256 && e.coloff == 177);
    key('X'); key(KEY_BACKSPACE);
    assert(e.rows[0].length == 255 && e.cx == 255 && e.coloff == 177);
    key('X'); assert(e.cx == 256 && e.rows[0].text[255] == 'X');
    key(KEY_HOME); assert(e.cx == 0 && e.coloff == 0);
    before = e;
    assert(key(KEY_LEFT) == 0 && key(KEY_BACKSPACE) == 0);
    unchanged(EDITOR_DIAG_READY);
    for (i = 0; i < 256; ++i) key(KEY_RIGHT);
    assert(e.cx == 256 && e.coloff == 177);
    before = e;
    assert(key(KEY_RIGHT) == 0 && key(KEY_DELETE) == 0);
    unchanged(EDITOR_DIAG_READY);
    key(KEY_HOME); key(KEY_END); assert(e.cx == 256 && e.coloff == 177);
    key(KEY_ENTER); before = e;
    assert(key(KEY_LEFT) == 0 && key(KEY_BACKSPACE) == 0);
    assert(key(KEY_DOWN) == 0);
    unchanged(EDITOR_DIAG_READY);
    assert(key(KEY_UP) == 1 && e.cy == 0 && e.cx == 0);
    assert(key(KEY_DOWN) == 1 && e.cy == 1 && e.cx == 0);
    unchanged(EDITOR_DIAG_READY);
    key('q'); key('Q');
    assert(strcmp(e.rows[1].text, "qQ") == 0);
    for (i = 0; i < 256; ++i) {
        if (i >= 32 && i <= 126) continue;
        before = e; assert(key((int)i) == 0); unchanged(EDITOR_DIAG_READY);
    }
    puts("PASS editing: exhaustive insert/BS/Delete, full line, navigation, no joins or row crossing");
}

static void expect_position(unsigned int cy, unsigned int cx,
                            unsigned int rowoff, unsigned int coloff) {
    assert(e.cy == cy && e.cx == cx && e.rowoff == rowoff && e.coloff == coloff);
}

static void navigation(int k, unsigned int cy, unsigned int cx,
                       unsigned int rowoff, unsigned int coloff) {
    before = e;
    assert(key(k) == 1);
    expect_position(cy, cx, rowoff, coloff);
    assert(e.row_count == before.row_count);
    assert(memcmp(e.rows, before.rows, sizeof(e.rows)) == 0);
    assert(e.diagnostic == EDITOR_DIAG_READY);
}

static void navigation_noop(int k) {
    before = e;
    for (unsigned int i = 0; i < 10; ++i) {
        assert(key(k) == 0); /* Main must not redraw for this return value. */
        unchanged(before.diagnostic);
    }
}

static void navigation_tests(void) {
    unsigned int i, p, from, to;
    const unsigned int lengths[] = {0,1,79,80,81,255,256};
    init();
    navigation_noop(KEY_UP);
    navigation_noop(KEY_DOWN);
    e.diagnostic = EDITOR_DIAG_DOCUMENT_FULL;
    navigation_noop(KEY_UP); navigation_noop(KEY_DOWN);

    /* Equal lengths and diagnostic reset on an actual movement. */
    init(); row(0, "abcd"); row(1, "efgh"); e.row_count = 2;
    cursor(0, 3); e.diagnostic = EDITOR_DIAG_LINE_FULL;
    navigation(KEY_DOWN, 1, 3, 0, 0);
    navigation_noop(KEY_DOWN);
    navigation(KEY_UP, 0, 3, 0, 0);
    navigation_noop(KEY_UP);

    /* Long -> short -> empty -> short -> long: no preferred column. */
    init();
    for (i = 0; i < 256; ++i) reference[i] = 'a';
    reference[256] = '\0';
    row(0, reference); row(1, "xy"); row(2, ""); e.row_count = 3;
    cursor(0, 256);
    navigation(KEY_DOWN, 1, 2, 0, 2);
    navigation(KEY_DOWN, 2, 0, 0, 0);
    navigation(KEY_UP, 1, 0, 0, 0);
    assert(key(KEY_END) == 1); expect_position(1, 2, 0, 0);
    navigation(KEY_UP, 0, 2, 0, 0);
    assert(key(KEY_END) == 1); expect_position(0, 256, 0, 177);
    assert(key(KEY_HOME) == 1); expect_position(0, 0, 0, 0);
    navigation(KEY_DOWN, 1, 0, 0, 0);
    assert(key(KEY_END) == 1); expect_position(1, 2, 0, 0);
    assert(key(KEY_HOME) == 1); expect_position(1, 0, 0, 0);

    /* Every column at selected length boundaries, in both directions.
     * Check exact offsets and the entire unchanged document. */
    for (from = 0; from < sizeof(lengths)/sizeof(*lengths); ++from) {
        for (to = 0; to < sizeof(lengths)/sizeof(*lengths); ++to) {
            for (p = 0; p <= lengths[from]; ++p) {
                unsigned int x = p < lengths[to] ? p : lengths[to];
                unsigned int off = p >= 80 ? p - 79 : 0;
                unsigned int after_off = x < off ? x : off;
                init();
                reference[lengths[from]] = '\0';
                for (i = 0; i < lengths[from]; ++i) reference[i] = 'a';
                row(0, reference);
                reference[lengths[to]] = '\0';
                for (i = 0; i < lengths[to]; ++i) reference[i] = 'b';
                row(1, reference); e.row_count = 2;
                cursor(0, p);
                navigation(KEY_DOWN, 1, x, 0, after_off);
                navigation(KEY_UP, 0, x, 0, after_off);
                /* Up itself must clamp too, not only the Down case. */
                row(0, e.rows[1].text);
                reference[lengths[from]] = '\0';
                for (i = 0; i < lengths[from]; ++i) reference[i] = 'a';
                row(1, reference); cursor(1, p);
                navigation(KEY_UP, 0, x, 0, after_off);
            }
        }
    }

    init();
    for (i = 0; i < 64; ++i) row(i, "abc");
    e.row_count = 64; cursor(0, 2);
    for (i = 1; i < 64; ++i)
        navigation(KEY_DOWN, i, 2, i >= 23 ? i - 22 : 0, 0);
    expect_position(63, 2, 41, 0);
    e.diagnostic = EDITOR_DIAG_DOCUMENT_FULL;
    navigation_noop(KEY_DOWN);
    for (i = 63; i > 0; --i) {
        unsigned int y = i - 1;
        navigation(KEY_UP, y, 2, y < 41 ? y : 41, 0);
    }
    expect_position(0, 2, 0, 0);
    navigation_noop(KEY_UP);

    /* Split plus actual navigation, including long-line offset reset. */
    init(); for (i = 0; i < 85; ++i) key('a');
    key(KEY_ENTER); key('x'); key('y');
    navigation(KEY_UP, 0, 2, 0, 0);
    key(KEY_END); expect_position(0, 85, 0, 6);
    navigation(KEY_DOWN, 1, 2, 0, 2);
    key(KEY_ENTER); expect_position(2, 0, 0, 0);
    navigation(KEY_UP, 1, 0, 0, 0);
    key('X'); assert(strcmp(e.rows[1].text, "Xxy") == 0);
    /* Fill by splitting at a row with successors, then reject atomically. */
    while (e.row_count < 64) key(KEY_ENTER);
    before = e; assert(key(KEY_ENTER) == 1); unchanged(EDITOR_DIAG_DOCUMENT_FULL);
    assert(key(KEY_ENTER) == 0); unchanged(EDITOR_DIAG_DOCUMENT_FULL);
    puts("PASS B1.2 navigation: Up/Down, clamping, no preferred column, 23x80 viewport, boundaries and split regression");
}

static void interpreter_tests(void) {
    static struct ansi_screen m;
    const char *bad[] = {"\033[0;1H", "\033[26;1H", "\033[1;0H", "\033[1;81H",
        "\033[1H", "\033[;1H", "\033[1;;1H", "\033[999;1H", "\033[31m",
        "\033[1A", "\033[0K", "\033[3J", "\033[?7l", "\033X", "\n"};
    for (unsigned int i = 0; i < sizeof(bad)/sizeof(*bad); ++i) {
        ansi_init(&m, '#');
        assert(ansi_feed(&m, bad[i], (unsigned int)strlen(bad[i])) == -1);
    }
    ansi_init(&m, '#');
    assert(ansi_feed(&m, "\033[2J", 4) == 0);
    for (unsigned int y = 0; y < 25; ++y)
        for (unsigned int x = 0; x < 80; ++x) assert(m.cells[y][x] == ' ');
    assert(ansi_feed(&m, "\033[?25l", 6) == 0 && !m.visible);
    assert(ansi_feed(&m, "\033[?25h\033[0m", 10) == 0 && m.visible);
    assert(ansi_feed(&m, "\033[1;80H", 7) == 0);
    assert(ansi_feed(&m, "A", 1) == 0 && m.pending_wrap && !m.wraps);
    assert(ansi_feed(&m, "B", 1) == 0 && m.wraps == 1 && m.row == 2);
    assert(m.cells[0][79] == 'A' && m.cells[1][0] == 'B');
    assert(ansi_feed(&m, "\033[2;1H\033[2K", 10) == 0);
    assert(m.cells[1][0] == ' ' && m.cells[0][79] == 'A');
    assert(ansi_feed(&m, "\033[25;80HAB", 10) == 0);
    assert(m.scrolls == 1 && m.bottom_right_writes == 1);
    ansi_init(&m, '#');
    assert(ansi_feed(&m, "\033[", 2) == 0 && m.state == 2);
    assert(ansi_feed(&m, "X", 1) == -1); /* Unknown completed CSI. */
    puts("PASS ANSI model: strict grammar/ranges, ED/EL/SGR/visibility and pending-wrap/scroll detection");
}

static void full_fixture(void) {
    init();
    for (unsigned int r = 0; r < 64; ++r) {
        for (unsigned int x = 0; x < 256; ++x)
            reference[x] = (char)('!' + (r * 7U + x) % 94U);
        reference[256] = '\0';
        row(r, reference);
    }
    e.row_count = 64;
    cursor(22, 79); /* Final CUP has two digits in both coordinates. */
    e.diagnostic = EDITOR_DIAG_DOCUMENT_FULL;
}

static void renderer_tests(void) {
    const unsigned int counts[] = {1,5,23,24,64};
    const unsigned int lengths[] = {0,1,79,80,81,256};
    const unsigned int offsets[] = {0,1,6,177};
    const unsigned int row_offsets[] = {0,1,41};
    reset_io(); assert(terminal_open() == 0); init(); render_check();
    row(0, "short row"); cursor(0, 4); render_check();
    for (unsigned int a = 0; a < sizeof(counts)/sizeof(*counts); ++a) {
        init();
        for (unsigned int r = 0; r < counts[a]; ++r) {
            unsigned int length = lengths[r % 6];
            for (unsigned int x = 0; x < length; ++x)
                reference[x] = (char)('A' + (r + x) % 26);
            reference[length] = '\0'; row(r, reference);
        }
        e.row_count = (uint16_t)counts[a];
        for (unsigned int b = 0; b < 3; ++b) {
            if (row_offsets[b] >= counts[a]) continue;
            for (unsigned int edge = 0; edge < 2; ++edge) {
                unsigned int y = row_offsets[b];
                if (edge) {
                    y += 22;
                    if (y >= counts[a]) y = counts[a] - 1;
                }
                /* Current row must accommodate the requested cursor/offset;
                 * neighbours retain short, empty and long rows. */
                for (unsigned int x = 0; x < 256; ++x) reference[x] = 'x';
                reference[256] = '\0'; row(y, reference);
                for (unsigned int c = 0; c < 4; ++c) {
                    cursor(y, offsets[c] + 79);
                    e.coloff = (uint16_t)offsets[c];
                    e.rowoff = (uint16_t)row_offsets[b];
                    invariants(); render_check();
                }
            }
        }
    }
    full_fixture(); render_check();
    assert(output_length == 2247); /* Achievable exact upper bound. */
    /* All three diagnostics and shorter subsequent statuses clear stale text. */
    e.diagnostic = EDITOR_DIAG_LINE_FULL; render_check();
    e.diagnostic = EDITOR_DIAG_READY; render_check();
    for (int chunk = 1; chunk <= 3; ++chunk) {
        partial = chunk; render_check();
    }
    partial = 0;
    for (unsigned int i = 0; i < 100; ++i) render_check(); /* No accumulating capture overflow. */
    row(22, "abc"); cursor(22, 3); render_check();
    editor_init(&e); invariants(); render_check(); /* Previously occupied rows become blank. */
    memset(screen.cells[24], '!', 80); render_check(); /* Persistent warning restored. */
    /* Bad neighbouring rows are found after part of the frame is assembled,
     * yet no bytes of that frame may be transmitted. */
    full_fixture();
    e.rows[10].length = 257;
    output_length = 0; int old_writes = writes;
    assert(editor_render(&e) == -1 && writes == old_writes && output_length == 0);
    e.rows[10].length = 1; e.rows[10].text[0] = 27; e.rows[10].text[1] = '\0';
    assert(editor_render(&e) == -1 && writes == old_writes);
    e.rows[10].text[0] = 'a'; e.rows[10].text[1] = 'x';
    assert(editor_render(&e) == -1 && writes == old_writes);
    assert(terminal_close(0) == 0);
    puts("PASS B1.3a renderer: all 2000 cells, viewport boundaries, stale text, 2247-byte max, partial writes and atomic assembly errors");
}

static void renderer_failure_tests(void) {
    const int cutoffs[] = {1,2,8,12,25,2246,2247};
    full_fixture();
    for (unsigned int i = 0; i < sizeof(cutoffs)/sizeof(*cutoffs); ++i) {
        reset_io(); assert(terminal_open() == 0);
        model_output = 1; partial = 1; fail_write = cutoffs[i];
        assert(editor_render(&e) == -1);
        /* Cleanup's new ESC cancels a possibly interrupted CSI prefix. */
        assert(terminal_close(1) == 1 && restores == 1 && unbinds == 1);
        assert(screen.visible && screen.state == 0 && !screen.error);
        assert(!screen.wraps && !screen.scrolls && !screen.bottom_right_writes);
    }
    reset_io(); assert(terminal_open() == 0);
    fault = 6; screen.visible = 0; model_output = 1;
    assert(editor_render(&e) == -1);
    assert(terminal_close(1) == 1 && restores == 1 && unbinds == 1);
    assert(!screen.visible); /* Cannot guarantee visibility when every send fails. */
    assert(writes == 4); /* Failed frame plus all three cleanup write attempts. */
    puts("PASS renderer failures: interrupted CSI, zero writes, restore/unbind and best-effort cursor cleanup");
}

static void parser_case(const int *bytes, unsigned int count,
                        const int *expected, unsigned int events) {
    unsigned int i, at = 0;
    reset_io();
    assert(terminal_open() == 0);
    init();
    for (i = 0; i < count; ++i) input[input_length++] = bytes[i];
    for (i = 0; i < count; ++i) {
        int k, result = terminal_read_key(&k);
        assert(result >= 0);
        if (result) {
            assert(at < events && k == expected[at++]);
            if (k != KEY_QUIT) key(k);
        } else invariants();
    }
    assert(at == events);
    assert(terminal_close(0) == 0);
}

#define PARSE(bytes, expected) parser_case(bytes, sizeof(bytes)/sizeof(*(bytes)), \
                                         expected, sizeof(expected)/sizeof(*(expected)))

static void parser_tests(void) {
    const int cr[] = {13}, lf[] = {10}, crlf[] = {13,10};
    const int delayed[] = {13,-1,10}, twice[] = {13,10,13,10};
    const int enter[] = {KEY_ENTER}, two[] = {KEY_ENTER,KEY_ENTER};
    PARSE(cr, enter); assert(e.row_count == 2);
    parser_case(lf, 1, NULL, 0); assert(e.row_count == 1);
    PARSE(crlf, enter); assert(e.row_count == 2);
    PARSE(delayed, enter); assert(e.row_count == 2);
    PARSE(twice, two); assert(e.row_count == 3);
    const int two_cr[] = {13,13};
    PARSE(two_cr, two); assert(e.row_count == 3);
    const int cr_inside[] = {27,'[',13,'3','~',27,'O',13,'F',27,13,-1};
    parser_case(cr_inside, sizeof(cr_inside)/sizeof(*cr_inside), NULL, 0);
    assert(e.row_count == 1);
    const int normal[] = {27,'[','D',27,'O','C',27,'[','H',27,'O','F',
                          27,'[','3','~',27,'[','7','~',27,'[','8','~',8,127,28,9};
    const int keys[] = {KEY_LEFT,KEY_RIGHT,KEY_HOME,KEY_END,KEY_DELETE,
                        KEY_HOME,KEY_END,KEY_BACKSPACE,KEY_BACKSPACE};
    PARSE(normal, keys);
    const int recovery[] = {27,27,'[','D',27,-1,'x',27,'[','9','~','y',
                            27,'[',-1,13,'3','~','z',27,']','a','b',10,-1,'v'};
    const int recovered[] = {KEY_LEFT,'x','y','z','v'};
    PARSE(recovery, recovered);
    assert(e.row_count == 1 && strcmp(e.rows[0].text,"xyzv") == 0);
    int long_sequence[40];
    long_sequence[0] = 27; long_sequence[1] = '[';
    for (int i = 2; i < 35; ++i) long_sequence[i] = '1';
    long_sequence[35] = 13; long_sequence[36] = '~'; long_sequence[37] = 'x';
    const int x[] = {'x'};
    parser_case(long_sequence, 38, x, 1); assert(e.row_count == 1);
    const int quit[] = {KEY_QUIT};
    const int prefixes[][4] = {{0,0,0,0},{27,0,0,0},{27,'[',0,0},
        {27,'O',0,0},{27,'[',-1,0},{27,']',0,0}};
    const unsigned int lengths[] = {0,1,2,2,3,2};
    for (unsigned int i = 0; i < 6; ++i) {
        int bytes[5];
        for (unsigned int j = 0; j < lengths[i]; ++j) bytes[j] = prefixes[i][j];
        bytes[lengths[i]] = 17;
        parser_case(bytes, lengths[i] + 1, quit, 1);
    }
    puts("PASS parser: CR/LF/CRLF, timeout, sequence recovery and Ctrl-Q priority");
}

static void lifecycle_tests(void) {
    int normal_writes;
    reset_io(); input[input_length++] = 17;
    assert(kmain() == 0 && restores == 1 && unbinds == 1);
    normal_writes = writes;
    for (int i = 1; i <= normal_writes; ++i) {
        reset_io(); fail_write = i; input[input_length++] = 17;
        assert(kmain() == 1 && restores == 1 && unbinds == 1);
    }
    for (int f = 1; f <= 6; ++f) {
        reset_io(); fault = f; input[input_length++] = 17;
        assert(kmain() == 1 && unbinds == (f != 1));
        if (f >= 3) assert(restores == 1);
    }
    reset_io(); input[input_length++] = -2;
    assert(kmain() == 1 && restores == 1 && unbinds == 1);
    reset_io(); input[input_length++] = -1; input[input_length++] = 17;
    assert(kmain() == 0 && writes == normal_writes);
    reset_io();
    input[input_length++] = 27; input[input_length++] = '['; input[input_length++] = 'A';
    input[input_length++] = 27; input[input_length++] = '['; input[input_length++] = 'B';
    input[input_length++] = 17;
    assert(kmain() == 0 && writes == normal_writes); /* Actual no-op: no redraw. */
    reset_io(); partial = 1; input[input_length++] = 17;
    assert(kmain() == 0 && restores == 1 && unbinds == 1);
    /* Failure on the redraw after an Enter, not just on initial output. */
    reset_io(); fail_write = 2; input[input_length++] = 13;
    assert(kmain() == 1 && restores == 1 && unbinds == 1);
    reset_io();
    input[input_length++] = 'a'; input[input_length++] = 'b';
    input[input_length++] = 13; input[input_length++] = 10;
    input[input_length++] = 'x'; input[input_length++] = 17;
    assert(kmain() == 0 && restores == 1 && unbinds == 1);
    assert(strstr(output, "Row 002/002 Len 001/256 Col 001"));
    puts("PASS lifecycle: I/O errors, partial writes, timeout without redraw and cleanup");
}

static void read_expected(int expected) {
    int k = KEY_NONE;
    assert(terminal_read_key(&k) == 1 && k == expected);
}

static void transport_tests(void) {
    int k;
    /* Pre-TX input, plus one fragment per write, followed by real A/B. */
    reset_io(); assert(terminal_open() == 0);
    arrive('x', 0);
    for (int i = 0; i < 90; ++i) {
        arrive(27, 1 + i * 3); arrive('[', 2 + i * 3);
        arrive((unsigned char)(i % 2 ? 'B' : 'A'), 3 + i * 3);
    }
    arrive('A', 271); arrive('B', 272);
    for (int i = 0; i < 272; ++i) assert(terminal_write("t", 1) == 0);
    assert(!polling && arrival_at == arrival_count);
    read_expected('x');
    for (int i = 0; i < 90; ++i) {
        assert(terminal_read_key(&k) == 0 && k == KEY_NONE);
        /* Empty polls between fragments must not expire ESC/CSI. */
        assert(terminal_write("", 0) == 0);
        assert(terminal_write("t", 1) == 0);
        assert(terminal_read_key(&k) == 0);
        read_expected(i % 2 ? KEY_DOWN : KEY_UP);
    }
    read_expected('A'); read_expected('B');
    assert(terminal_close(0) == 0);

    /* A prefix consumed by the parser survives empty polling during TX;
     * receive each continuation in a later write, not a timed read. */
    reset_io(); assert(terminal_open() == 0);
    input[input_length++] = 27;
    assert(terminal_read_key(&k) == 0);
    assert(terminal_write("idle", 4) == 0);
    arrive('[', writes + 1);
    assert(terminal_write("t", 1) == 0);
    assert(terminal_read_key(&k) == 0);
    assert(terminal_write("idle", 4) == 0);
    arrive('A', writes + 1);
    assert(terminal_write("t", 1) == 0);
    read_expected(KEY_UP);
    assert(terminal_close(0) == 0);

    /* Actual frame bytes remain identical while RX arrives inside ANSI
     * sequences. Use static captures; no additional target allocation. */
    {
        static char frame_copy[2247];
        unsigned int length;
        reset_io(); assert(terminal_open() == 0); full_fixture();
        assert(editor_render(&e) == 0);
        length = output_length; assert(length == sizeof(frame_copy));
        memcpy(frame_copy, output, length);
        output_length = 0;
        arrive(27, writes + 1); arrive('[', writes + 2); arrive('B', writes + 3);
        assert(editor_render(&e) == 0);
        assert(output_length == length && memcmp(frame_copy, output, length) == 0);
        assert(terminal_read_key(&k) == 0);
        assert(terminal_read_key(&k) == 0); read_expected(KEY_DOWN);
        assert(terminal_close(0) == 0);
    }

    /* Exactly 512 queued bytes, wrap-around, then FIFO before fresh RX. */
    reset_io(); assert(terminal_open() == 0);
    for (int i = 0; i < 512; ++i) arrive((unsigned char)(' ' + i % 95), 0);
    assert(terminal_write("abcdefghijklmnopqrstuvwxyz0123456789", 36) == 0);
    for (int i = 0; i < 300; ++i) read_expected(' ' + i % 95);
    for (int i = 0; i < 300; ++i) arrive((unsigned char)(' ' + i % 95), writes);
    assert(terminal_write("abcdefghijklmnopqrst", 20) == 0);
    for (int i = 300; i < 512; ++i) read_expected(' ' + i % 95);
    for (int i = 0; i < 300; ++i) read_expected(' ' + i % 95);
    input[input_length++] = 'z'; read_expected('z');
    assert(terminal_close(0) == 0);

    /* Overflow across an escape prefix is fatal, including queued Ctrl-Q. */
    reset_io(); assert(terminal_open() == 0);
    for (int i = 0; i < 510; ++i) arrive('x', 0);
    arrive(27, 0); arrive('[', 0); arrive('A', 0); arrive(17, 0);
    assert(terminal_write("abcdefghijklmnopqrstuvwxyz0123456789", 36) == -1);
    assert(arrival_at == 513 && writes == 32); /* 16-byte pump budget. */
    assert(terminal_read_key(&k) == -1 && k == KEY_NONE);
    int old_writes = writes;
    assert(terminal_write("x", 1) == -1 && writes == old_writes);
    assert(terminal_close(0) == 1 && restores == 1 && unbinds == 1);
    assert(strstr(output, "\033[?25h") && !polling);

    /* Fail polling entry and timed restoration separately. Both attempt
     * timed restoration; no input may be dispatched after either failure. */
    for (int failure = 2; failure <= 3; ++failure) {
        reset_io(); assert(terminal_open() == 0); fail_set = failure;
        assert(terminal_write("x", 1) == -1 && sets == 3);
        assert(writes == (failure == 2 ? 0 : 1));
        assert(terminal_read_key(&k) == -1);
        assert(terminal_close(0) == 1 && restores == 1 && unbinds == 1);
    }
    for (int failure = 0; failure < 4; ++failure) {
        reset_io(); assert(terminal_open() == 0);
        if (failure == 0) rx_failure = -1;
        if (failure == 1) rx_failure = 2;
        if (failure == 2) invalid_write = 1;
        if (failure == 3) fault = 6;
        assert(terminal_write("x", 1) == -1 && !polling);
        assert(terminal_read_key(&k) == -1);
        assert(terminal_close(0) == 1 && restores == 1 && unbinds == 1);
    }
    /* A frame is byte-identical under one-byte TX; cleanup direct writes
     * still handle positive short writes. All 2000 cells checked elsewhere. */
    reset_io(); assert(terminal_open() == 0); partial = 1;
    assert(terminal_write("\033[2;3Habc\033[?25h", 15) == 0);
    assert(output_length == 15 && memcmp(output, "\033[2;3Habc\033[?25h", 15) == 0);
    assert(terminal_close(0) == 0 && restores == 1);
    puts("PASS H07-F1: RX during TX, FIFO order/capacity/wrap/overflow, fragmented autorepeat, mode failures and cleanup");
}

static void rx_service_tests(void) {
    static char expected_frame[2247];
    int k;
    /* Public calls never nest inside TX. Every successful service restores
     * timed mode, does not transmit, and has exactly one 16-read budget. */
    reset_io(); assert(terminal_open() == 0);
    unsigned int polls = poll_reads;
    assert(terminal_service_rx() == 0 && !polling && writes == 0);
    assert(poll_reads == polls + 1 && sets == 3);
    for (int i = 0; i < 17; ++i) arrive((unsigned char)('a' + i), 0);
    polls = poll_reads;
    assert(terminal_service_rx() == 0 && poll_reads == polls + 16);
    assert(arrival_at == 16 && writes == 0);
    assert(terminal_service_rx() == 0 && arrival_at == 17);
    for (int i = 0; i < 17; ++i) read_expected('a' + i);
    assert(terminal_close(0) == 0);
    assert(terminal_service_rx() == -1);

    /* 28 fixed services plus ceil(visible/16) per rendered document row.
     * All 143 construction sessions precede the single F1 TX session. */
    reset_io(); assert(terminal_open() == 0); full_fixture();
    before = e;
    assert(editor_render(&e) == 0 && output_length == sizeof(expected_frame));
    assert(poll_sessions == 144 && sets == 289);
    memcpy(expected_frame, output, sizeof(expected_frame));
    reset_io(); assert(terminal_open() == 0);
    for (int session = 1; session <= 143; ++session)
        arrive_service((unsigned char)(' ' + (session - 1) % 95), session);
    assert(editor_render(&e) == 0 && arrival_at == 143);
    assert(output_length == sizeof(expected_frame));
    assert(memcmp(output, expected_frame, sizeof(expected_frame)) == 0);
    assert(memcmp(&e, &before, sizeof(e)) == 0);
    for (int i = 0; i < 143; ++i) read_expected(' ' + i % 95);
    assert(terminal_close(0) == 0);

    /* Zero/80/256-character row cases have 28/33/33 services. The viewport
     * clips the 256-character row before chunk construction. */
    const unsigned int lengths[] = {0,80,256};
    for (unsigned int t = 0; t < 3; ++t) {
        reset_io(); assert(terminal_open() == 0); init();
        for (unsigned int i = 0; i < lengths[t]; ++i) reference[i] = 'a';
        reference[lengths[t]] = 0; row(0, reference);
        int sessions = t ? 33 : 28;
        assert(editor_render(&e) == 0 && poll_sessions == sessions + 1);
        assert(sets == 1 + 2 * (sessions + 1));
        assert(terminal_close(0) == 0);
    }

    /* Fragmented sequence prefix is already in the production parser.
     * Continuations and commands stay ordered without renderer dispatch. */
    reset_io(); assert(terminal_open() == 0); full_fixture(); before = e;
    input[input_length++] = 27; assert(terminal_read_key(&k) == 0);
    arrive_service('[', 3); arrive_service('A', 7);
    arrive_service(27, 134); arrive_service('[', 140); arrive_service('B', 141);
    arrive_service(17, 142);
#ifdef KILO_RX_DIAGNOSTICS
    arrive_service(18, 143); arrive_service(20, 143);
#endif
    assert(editor_render(&e) == 0 && memcmp(&e, &before, sizeof(e)) == 0);
    assert(terminal_read_key(&k) == 0); read_expected(KEY_UP);
    assert(terminal_read_key(&k) == 0); assert(terminal_read_key(&k) == 0);
    read_expected(KEY_DOWN); read_expected(KEY_QUIT);
#ifdef KILO_RX_DIAGNOSTICS
    read_expected(18); read_expected(20);
#endif
    assert(terminal_close(0) == 0);

    /* Full capacity is legal until a further byte is actually available. */
    reset_io(); assert(terminal_open() == 0); init();
    for (int i = 0; i < 512; ++i) arrive_service('x', 1);
    assert(editor_render(&e) == 0 && arrival_at == 512);
    for (int i = 0; i < 512; ++i) read_expected('x');
    assert(terminal_close(0) == 0);
    reset_io(); assert(terminal_open() == 0); full_fixture(); before = e;
    for (int i = 0; i < 513; ++i) arrive_service('x', 1);
    assert(editor_render(&e) == -1 && writes == 0 && output_length == 0);
    assert(memcmp(&e, &before, sizeof(e)) == 0);
    assert(terminal_read_key(&k) == -1 && !polling);
    assert(terminal_service_rx() == -1);
    assert(terminal_close(1) == 1 && restores == 1 && unbinds == 1);

    /* Entry/restore failures at EVERY full-frame construction service.
     * Not even a frame prefix may be transmitted. */
    for (int session = 1; session <= 143; ++session) {
        for (int restore = 0; restore <= 1; ++restore) {
            reset_io(); assert(terminal_open() == 0); full_fixture(); before = e;
            fail_set = 2 * session + restore;
            assert(editor_render(&e) == -1 && writes == 0 && output_length == 0);
            assert(sets == 2 * session + 1);
            assert(memcmp(&e, &before, sizeof(e)) == 0);
            assert(terminal_read_key(&k) == -1 && terminal_service_rx() == -1);
            assert(terminal_close(1) == 1 && restores == 1 && unbinds == 1);
        }
    }
    reset_io(); assert(terminal_open() == 0); init(); rx_failure = -1;
    assert(editor_render(&e) == -1 && writes == 0 && !polling && sets == 3);
    assert(terminal_close(1) == 1 && restores == 1 && unbinds == 1);
    reset_io(); assert(terminal_open() == 0); init();
    fail_set = 2; fail_set_second = 3;
    assert(editor_render(&e) == -1 && writes == 0 && poll_reads == 0);
    assert(terminal_read_key(&k) == -1);
    fault = 4; fail_write = 1;
    assert(terminal_close(1) == 1 && restores == 1 && unbinds == 1);
    puts("PASS F3: all construction services, <=16 RX budget, no TX, ordered keys, FIFO boundaries, immutable document/frame and all switch failures");
}

#ifdef KILO_RX_DIAGNOSTICS
static void diagnostic_tests(void) {
    static struct terminal_rx_diagnostics snap, again;
    int k;
    uint32_t value = UINT32_MAX - 1U;
    uint8_t saturated = 0;
    terminal_diag_increment(&value, &saturated);
    assert(value == UINT32_MAX && saturated == 0);
    terminal_diag_increment(&value, &saturated);
    assert(value == UINT32_MAX && saturated == 1);

    reset_io(); assert(terminal_open() == 0);
    int old_writes = writes;
    unsigned int old_polls = poll_reads;
    assert(terminal_diag_reset() == 0);
    assert(writes == old_writes && poll_reads == old_polls);
    input[input_length++] = -1; /* True timeout does not count as a byte. */
    assert(terminal_read_key(&k) == 0);
    input[input_length++] = 'a'; read_expected('a');
    arrive('b', writes + 1); arrive('c', writes + 2);
    assert(terminal_write("xy", 2) == 0);
    assert(terminal_diag_reset() == -1); /* Pending FIFO cannot be reset. */
    read_expected('b'); read_expected('c');
    terminal_diag_delivered(); terminal_diag_accepted();
    terminal_diag_snapshot(&snap, 0);
    assert(snap.rx_direct == 1 && snap.rx_poll == 2 && snap.enqueued == 2);
    assert(snap.fifo_peak == 2 && snap.fifo_pending == 0 && snap.overflows == 0);
    assert(snap.delivered == 1 && snap.accepted == 1 && !snap.ctrl_t_included);
    arrive('z', writes + 1);
    model_output = 1;
    assert(terminal_diag_report(&snap) == 0); /* Polling during output. */
    model_output = 0;
    assert(!screen.error && screen.visible && !screen.wraps && !screen.scrolls);
    assert(screen.bottom_right_writes == 0);
    terminal_diag_snapshot(&again, 1);
    assert(memcmp(&snap, &again, sizeof(snap)) == 0);
    assert(terminal_close(0) == 0);

    reset_io(); assert(terminal_open() == 0); assert(terminal_diag_reset() == 0);
    for (int i = 0; i < 512; ++i) arrive('a', 0);
    assert(terminal_write("abcdefghijklmnopqrstuvwxyz0123456789", 36) == 0);
    terminal_diag_snapshot(&snap, 0);
    assert(snap.rx_poll == 512 && snap.enqueued == 512 && snap.fifo_peak == 512);
    assert(snap.fifo_pending == 512 && snap.overflows == 0);
    assert(terminal_close(0) == 0);

    reset_io(); assert(terminal_open() == 0); assert(terminal_diag_reset() == 0);
    for (int i = 0; i < 513; ++i) arrive('a', 0);
    assert(terminal_write("abcdefghijklmnopqrstuvwxyz0123456789", 36) == -1);
    terminal_diag_snapshot(&snap, 0);
    assert(snap.rx_poll == 513 && snap.enqueued == 512 && snap.fifo_peak == 512);
    assert(snap.overflows == 1 && snap.transport_error == 1);
    assert(terminal_read_key(&k) == -1);
    assert(terminal_close(1) == 1);

    reset_io(); assert(terminal_open() == 0); assert(terminal_diag_reset() == 0);
    rx_failure = -1;
    assert(terminal_write("x", 1) == -1);
    terminal_diag_snapshot(&snap, 0);
    assert(snap.rx_poll == 0 && snap.overflows == 0 && snap.transport_error);
    assert(terminal_close(1) == 1);

    reset_io(); assert(terminal_open() == 0); assert(terminal_diag_reset() == 0);
    input[input_length++] = 27; input[input_length++] = '[';
    input[input_length++] = 18; input[input_length++] = 20;
    for (int i = 0; i < 4; ++i) assert(terminal_read_key(&k) == 0);
    assert(terminal_diag_reset() == -1); /* No command in incomplete CSI. */
    assert(terminal_close(0) == 0);

    /* Application command integration: 100 payload bytes plus Ctrl-T.
     * The reset command was received before the measurement interval. */
    reset_io();
    input[input_length++] = 18;
    for (int i = 0; i < 100; ++i) input[input_length++] = '0' + i % 10;
    input[input_length++] = 20; input[input_length++] = 17;
    assert(kmain() == 0);
    terminal_diag_snapshot(&snap, 0);
    assert(snap.rx_direct == 101 && snap.rx_poll == 0);
    assert(snap.delivered == 100 && snap.accepted == 100 && snap.ctrl_t_included);
    assert(strstr(output, "RX total: 0000000101"));
    assert(strstr(output, "Row length: 0000000100"));
    assert(strstr(output, "Cursor column: 0000000100"));
    assert(strstr(output, "Ctrl-T included: 0000000001"));

    reset_io(); input[input_length++] = 18;
    for (int i = 0; i < 257; ++i) input[input_length++] = 'a';
    input[input_length++] = 20; input[input_length++] = 17;
    assert(kmain() == 0);
    terminal_diag_snapshot(&snap, 0);
    assert(snap.delivered == 257 && snap.accepted == 256 && snap.rx_direct == 258);
    assert(strstr(output, "Row length: 0000000256"));

    reset_io(); input[input_length++] = 18;
    input[input_length++] = 'a'; input[input_length++] = 13;
    input[input_length++] = 'b'; input[input_length++] = 'c';
    input[input_length++] = 20; input[input_length++] = 'z';
    input[input_length++] = 20; input[input_length++] = 17;
    assert(kmain() == 0);
    terminal_diag_snapshot(&snap, 0);
    assert(snap.delivered == 3 && snap.accepted == 3 && snap.rx_direct == 5);
    assert(strstr(output, "Row length: 0000000002"));
    assert(strstr(output, "Cursor row(0): 0000000001"));
    assert(restores == 1 && unbinds == 1);
    puts("PASS D2 diagnostics: counts, saturation, reset guards, immutable snapshot, commands, full/second-row acceptance and cleanup");
}
#endif

/* Direct API checks use one explicitly available stream in both RX modes.
 * No wall-clock model or hardware throughput claim is involved. */
#define API_NO_BYTE (-3)

static void api_begin(void) {
    reset_io();
    assert(terminal_open() == 0);
#ifdef KILO_RX_DIAGNOSTICS
    assert(terminal_diag_reset() == 0);
#endif
    shared_input = 1;
    ++api_cases;
}

static void api_offer(int byte) {
    assert(input_length < (int)(sizeof(input) / sizeof(*input)));
    input[input_length++] = byte;
    shared_available = input_length;
}

static void api_expect(int poll, int byte, enum terminal_input_result expected,
                       int expected_key) {
    int k = -12345, at = input_at, old_writes = writes;
    unsigned int reads = read_calls;
    if (byte != API_NO_BYTE) api_offer(byte);
    enum terminal_input_result result = poll ? terminal_poll_input(&k)
                                            : terminal_read_input(&k);
    assert(result == expected);
    assert(k == (expected == TERMINAL_INPUT_EVENT ? expected_key : KEY_NONE));
    assert(input_at - at >= 0 && input_at - at <= 1);
    assert(read_calls - reads <= 1); /* No bulk pump/prefetch through API. */
    assert(writes == old_writes);    /* Neither input operation transmits. */
    if (expected != TERMINAL_INPUT_ERROR) assert(!polling);
}

static void api_end(int failed) {
    assert(terminal_close(failed) == failed);
    assert(restores == 1 && unbinds == 1);
}

#ifdef KILO_RX_DIAGNOSTICS
static struct terminal_rx_diagnostics api_snapshot;
static void api_counts(unsigned int direct, unsigned int pump,
                       unsigned int enqueued, unsigned int peak,
                       unsigned int pending, unsigned int overflows,
                       unsigned int failed) {
    terminal_diag_snapshot(&api_snapshot, 0);
    assert(api_snapshot.rx_direct == direct && api_snapshot.rx_poll == pump);
    assert(api_snapshot.enqueued == enqueued && api_snapshot.fifo_peak == peak);
    assert(api_snapshot.fifo_pending == pending && api_snapshot.overflows == overflows);
    assert(api_snapshot.transport_error == failed);
    assert(api_snapshot.delivered == 0 && api_snapshot.accepted == 0);
    assert(!api_snapshot.saturated && !api_snapshot.ctrl_t_included);
}
#define API_COUNTS(...) api_counts(__VA_ARGS__)
#else
#define API_COUNTS(...) ((void)0)
#endif

struct api_step {
    int poll, byte;
    enum terminal_input_result result;
    int key;
};
#define AP(b, r, k) {1, b, TERMINAL_INPUT_##r, k}
#define AT(b, r, k) {0, b, TERMINAL_INPUT_##r, k}

static void api_sequence(const struct api_step *steps, unsigned int count) {
    unsigned int direct = 0;
    api_begin();
    for (unsigned int i = 0; i < count; ++i) {
        api_expect(steps[i].poll, steps[i].byte, steps[i].result, steps[i].key);
        if (steps[i].byte >= 0) ++direct;
    }
    /* direct is also used in non-diagnostic builds. */
    assert((unsigned int)input_at >= direct);
    API_COUNTS(direct, 0, 0, 0, 0, 0, 0);
    api_end(0);
}
#define API_SEQUENCE(a) api_sequence(a, sizeof(a) / sizeof(*(a)))

static void api_timed_and_poll_tests(void) {
    static const struct api_step timed[] = {
        AT(-1, EMPTY, KEY_NONE), AT('a', EVENT, 'a'),
        AT(27, PROGRESS, KEY_NONE), AT(-1, EMPTY, KEY_NONE),
        AT('[', EVENT, '['), AT('A', EVENT, 'A')
    };
    API_SEQUENCE(timed); /* Original late-ESC limitation is preserved. */
    static const struct api_step poll[] = {
        AP(API_NO_BYTE, EMPTY, KEY_NONE), AP('a', EVENT, 'a'),
        AP(27, PROGRESS, KEY_NONE), AP(API_NO_BYTE, EMPTY, KEY_NONE),
        AP('[', PROGRESS, KEY_NONE), AP(API_NO_BYTE, EMPTY, KEY_NONE),
        AP('A', EVENT, KEY_UP)
    };
    API_SEQUENCE(poll);

    /* More bytes may exist, but they are not available until released. */
    api_begin(); input[input_length++] = 'x';
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EMPTY, KEY_NONE);
    assert(input_at == 0 && api_poll_sets == 1 && api_timed_sets == 1);
    shared_available = input_length;
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'x');
    assert(input_at == 1 && api_poll_sets == 2 && api_timed_sets == 2);
    API_COUNTS(1, 0, 0, 0, 0, 0, 0); api_end(0);

    /* An EMPTY poll cannot consume the explicit timeout of a later timed
     * read. Only that timed read resets the pending ESC. */
    static const struct api_step timeout[] = {
        AP(27, PROGRESS, KEY_NONE), AP(-1, EMPTY, KEY_NONE),
        AT(API_NO_BYTE, EMPTY, KEY_NONE), AP('x', EVENT, 'x')
    };
    API_SEQUENCE(timeout);

    api_begin(); api_offer('a'); api_offer('b'); api_offer('c');
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'a');
    assert(input_at == 1 && read_calls == 1);
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'b');
    assert(input_at == 2 && read_calls == 2);
    api_expect(0, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'c');
    assert(input_at == 3 && read_calls == 3);
    assert(api_poll_sets == 2 && api_timed_sets == 2);
    API_COUNTS(3, 0, 0, 0, 0, 0, 0); api_end(0);

    /* Shared RX source: F3 enqueues f/g, while d remains directly available.
     * Both timed and polling APIs must prefer the queued bytes. */
    api_begin(); api_offer('f'); api_offer('g');
    assert(terminal_service_rx() == 0 && input_at == 2);
    api_offer('d');
    int old_sets = sets;
    unsigned int reads = read_calls;
    api_expect(0, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'f');
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'g');
    assert(sets == old_sets && read_calls == reads && input_at == 2);
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'd');
    assert(sets == old_sets + 2 && read_calls == reads + 1);
    API_COUNTS(1, 2, 2, 2, 0, 0, 0); api_end(0);
}

static void api_parser_tests(void) {
    for (unsigned int family = 0; family < 2; ++family) {
        int intro = family ? 'O' : '[';
        int final = family ? 'H' : 'A';
        int event = family ? KEY_HOME : KEY_UP;
        api_begin();
        api_expect(0, 27, TERMINAL_INPUT_PROGRESS, KEY_NONE);
        api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EMPTY, KEY_NONE);
        api_expect(1, intro, TERMINAL_INPUT_PROGRESS, KEY_NONE);
        api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EMPTY, KEY_NONE);
        api_expect(1, final, TERMINAL_INPUT_EVENT, event);
        API_COUNTS(3, 0, 0, 0, 0, 0, 0); api_end(0);

        api_begin();
        api_expect(1, 27, TERMINAL_INPUT_PROGRESS, KEY_NONE);
        api_expect(0, intro, TERMINAL_INPUT_PROGRESS, KEY_NONE);
        api_expect(0, -1, TERMINAL_INPUT_EMPTY, KEY_NONE);
        api_expect(1, final, TERMINAL_INPUT_PROGRESS, KEY_NONE); /* DISCARD. */
        api_expect(1, 'x', TERMINAL_INPUT_EVENT, 'x');
        API_COUNTS(4, 0, 0, 0, 0, 0, 0); api_end(0);
    }
    static const struct api_step unknown[] = {
        AP(27, PROGRESS, KEY_NONE), AP('[', PROGRESS, KEY_NONE),
        AP('9', PROGRESS, KEY_NONE), AP('~', PROGRESS, KEY_NONE),
        AP('x', EVENT, 'x')
    };
    API_SEQUENCE(unknown);
    static const struct api_step malformed[] = {
        AP(27, PROGRESS, KEY_NONE), AP('[', PROGRESS, KEY_NONE),
        AP(1, PROGRESS, KEY_NONE), AP(API_NO_BYTE, EMPTY, KEY_NONE),
        AT(-1, EMPTY, KEY_NONE), AP('A', PROGRESS, KEY_NONE),
        AP('b', EVENT, 'b')
    };
    API_SEQUENCE(malformed);
    static const struct api_step quiet[] = {
        AP(27, PROGRESS, KEY_NONE), AP(']', PROGRESS, KEY_NONE),
        AP('a', PROGRESS, KEY_NONE), AP(API_NO_BYTE, EMPTY, KEY_NONE),
        AP('b', PROGRESS, KEY_NONE), AT(-1, EMPTY, KEY_NONE), AP('x', EVENT, 'x')
    };
    API_SEQUENCE(quiet);
    static const struct api_step consecutive[] = {
        AP(27, PROGRESS, KEY_NONE), AP('[', PROGRESS, KEY_NONE), AP('A', EVENT, KEY_UP),
        AP(27, PROGRESS, KEY_NONE), AP('[', PROGRESS, KEY_NONE), AP('B', EVENT, KEY_DOWN),
        AP(27, PROGRESS, KEY_NONE), AP(27, PROGRESS, KEY_NONE),
        AP('[', PROGRESS, KEY_NONE), AP('D', EVENT, KEY_LEFT)
    };
    API_SEQUENCE(consecutive);
    static const struct api_step legacy_limit[] = {
        AP(27, PROGRESS, KEY_NONE), AP('[', PROGRESS, KEY_NONE),
        AP('[', PROGRESS, KEY_NONE), AP('A', EVENT, 'A')
    };
    API_SEQUENCE(legacy_limit);
    api_begin();
    api_expect(1, 27, TERMINAL_INPUT_PROGRESS, KEY_NONE);
    api_expect(1, '[', TERMINAL_INPUT_PROGRESS, KEY_NONE);
    for (int i = 0; i < 17; ++i)
        api_expect(1, '1', TERMINAL_INPUT_PROGRESS, KEY_NONE);
    api_expect(1, '~', TERMINAL_INPUT_PROGRESS, KEY_NONE);
    api_expect(1, 'x', TERMINAL_INPUT_EVENT, 'x');
    API_COUNTS(21, 0, 0, 0, 0, 0, 0); api_end(0);

    /* Quit in every parser state; R/T only in INPUT in diagnostic builds.
     * Quit does not reset the parser, matching the original implementation. */
    const int prefixes[][3] = {{0,0,0},{27,0,0},{27,'[',0},
                             {27,'O',0},{27,'[',1},{27,']',0}};
    const unsigned int lengths[] = {0,1,2,2,3,2};
    for (unsigned int i = 0; i < 6; ++i) {
        api_begin();
        for (unsigned int j = 0; j < lengths[i]; ++j)
            api_expect(1, prefixes[i][j], TERMINAL_INPUT_PROGRESS, KEY_NONE);
        api_expect(1, 17, TERMINAL_INPUT_EVENT, KEY_QUIT);
        for (int c = 18; c <= 20; c += 2) {
#ifdef KILO_RX_DIAGNOSTICS
            if (i == 0) api_expect(1, c, TERMINAL_INPUT_EVENT, c);
            else
#endif
                api_expect(1, c, TERMINAL_INPUT_PROGRESS, KEY_NONE);
        }
        api_expect(0, -1, TERMINAL_INPUT_EMPTY, KEY_NONE);
        if (i >= 2 && i <= 4)
            api_expect(1, '~', TERMINAL_INPUT_PROGRESS, KEY_NONE);
        api_expect(1, 'x', TERMINAL_INPUT_EVENT, 'x');
        API_COUNTS(lengths[i] + 4U + (i >= 2 && i <= 4), 0, 0, 0, 0, 0, 0);
        api_end(0);
    }
}

static void api_latched(void) {
    unsigned int reads = read_calls;
    int old_sets = sets, at = input_at, k = -12345;
    api_expect(0, API_NO_BYTE, TERMINAL_INPUT_ERROR, KEY_NONE);
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_ERROR, KEY_NONE);
    assert(terminal_read_key(&k) == -1 && k == KEY_NONE);
    assert(read_calls == reads && sets == old_sets && input_at == at);
}

static void api_error_and_wrapper_tests(void) {
    for (int failure = 0; failure < 8; ++failure) {
        api_begin();
        if (failure == 0) api_offer(-2); /* Timed RX failure. */
        if (failure == 1 || failure == 6) fail_set = sets + 1;
        if (failure >= 3 && failure <= 5) fail_set = sets + 2;
        if (failure == 2) api_offer(-2); /* Polling RX failure. */
        if (failure == 4) api_offer('x');
        if (failure == 5) api_offer(27); /* Must not change parser on restore error. */
        if (failure == 6) fail_set_second = sets + 2;
        if (failure == 7) rx_failure = 2; /* Invalid RX count. */
        api_expect(failure != 0, API_NO_BYTE, TERMINAL_INPUT_ERROR, KEY_NONE);
        if (failure == 0) assert(api_poll_sets == 0 && api_timed_sets == 0);
        else assert(api_poll_sets == 1 && api_timed_sets == 1);
        if (failure == 1 || failure == 6) assert(read_calls == 0);
        else assert(read_calls == 1);
        api_latched();
        API_COUNTS(failure == 4 || failure == 5, 0, 0, 0, 0, 0, 1);
#ifdef KILO_RX_DIAGNOSTICS
        assert(api_snapshot.parser_state == 0); /* In particular, failed ESC restore. */
#endif
        api_end(1);
    }

    /* F1 enqueues at the exact boundary; new APIs must not double-count
     * dequeues or poll past queued data, even with fresh RX available. */
    api_begin(); shared_input = 0;
    for (int i = 0; i < 512; ++i) arrive('a', 0);
    assert(terminal_write("abcdefghijklmnopqrstuvwxyz0123456789", 36) == 0);
    shared_input = 1; api_offer('z');
    int old_sets = sets;
    unsigned int reads = read_calls;
    for (int i = 0; i < 512; ++i)
        api_expect(i % 2, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'a');
    assert(sets == old_sets && read_calls == reads && input_at == 0);
    api_expect(1, API_NO_BYTE, TERMINAL_INPUT_EVENT, 'z');
    API_COUNTS(1, 512, 512, 512, 0, 0, 0); api_end(0);

    api_begin(); shared_input = 0;
    for (int i = 0; i < 513; ++i) arrive('a', 0);
    assert(terminal_write("abcdefghijklmnopqrstuvwxyz0123456789", 36) == -1);
    api_latched();
    API_COUNTS(0, 513, 512, 512, 512, 1, 1); api_end(1);

    api_begin();
    const int bytes[] = {-1,27,'[','A','x',-2};
    const int results[] = {0,0,0,1,1,-1};
    const int keys[] = {KEY_NONE,KEY_NONE,KEY_NONE,KEY_UP,'x',KEY_NONE};
    for (unsigned int i = 0; i < sizeof(bytes) / sizeof(*bytes); ++i) {
        int k = -12345;
        api_offer(bytes[i]);
        assert(terminal_read_key(&k) == results[i] && k == keys[i]);
    }
    api_latched();
    API_COUNTS(4, 0, 0, 0, 0, 0, 1); api_end(1);
}

static void input_api_tests(void) {
    api_timed_and_poll_tests();
    api_parser_tests();
    api_error_and_wrapper_tests();
    printf("PASS F2-I1a direct input API: %u cases; timed/poll/FIFO/parser, mode errors, latch, wrapper and diagnostic accounting\n", api_cases);
}

static void batch_add(int byte) {
    assert(input_length < (int)(sizeof(input) / sizeof(*input)));
    input[input_length++] = byte;
}

static void batch_arrow(int intro, int final) {
    batch_add(27); batch_add(intro); batch_add(final);
}

static void batch_before_rx(int poll) {
    if (!poll && batch_stagger && batch_kind == B_PROGRESS &&
        input_at == shared_available && shared_available < batch_first_end)
        ++shared_available; /* Next bounded progress batch starts with data. */
    if (poll && batch_stagger && !batch_reset_pending &&
        input_at >= batch_start + 1 && shared_available < batch_first_end &&
        batch_stagger_session != poll_sessions) {
        batch_stagger_session = poll_sessions;
        ++shared_available;
        ++batch_stagger_calls;
    }
    if (!poll && batch_reset_pending) {
        if (input_at == 0) shared_available = 1; /* Ctrl-R alone. */
        else {
            assert(input_at == batch_start && sets == batch_reset_sets);
            shared_available = batch_stagger ? batch_start + 1 : batch_first_end;
            batch_reset_pending = batch_release = 0;
        }
    } else if (!poll && batch_release) {
        shared_available = batch_stagger ? batch_start + 1 : batch_first_end;
        batch_release = 0;
    }
    if (!poll && !batch_reset_pending &&
        (batch_kind == B_ESC_TIMEOUT || batch_kind == B_DISCARD || batch_kind == B_QUIET) &&
        input_at == batch_first_end && shared_available == batch_first_end)
        shared_available = input_length - 1; /* Explicit timeout + continuation. */
    if (!poll && batch_kind == B_PROGRESS && input_at >= batch_start) {
        assert(input_at - batch_start == 64 * batch_progress_timed);
        ++batch_progress_timed;
    }
}

static void batch_after_rx(int byte, int poll) {
    if (batch_reset_pending && input_at == batch_start && byte == 18)
        batch_reset_sets = sets;
    if (!poll && input_at == batch_start + 1) {
        if (batch_kind == B_POLL_ENTRY) fail_set = sets + 1;
        if (batch_kind == B_POLL_RESTORE) fail_set = sets + 2;
        if (batch_kind == B_TX_ERROR || batch_kind == B_REPORT_ERROR)
            fail_write = writes + 1;
    }
}

static void batch_model_key(int k) {
    assert(editor_key(&e, k) >= 0);
    invariants();
}

static void batch_frame_complete(void) {
    unsigned int count = 0;
    editor_init(&e); /* Independent expected editor fixture; not main's document. */
    if (batch_frames > 1) {
        switch (batch_kind) {
        case B_SINGLE: batch_model_key('a'); break;
        case B_MULTI:
            batch_model_key('a'); batch_model_key('b'); batch_model_key('c'); break;
        case B_130: case B_CONTINUOUS:
            count = (unsigned int)(batch_frames - 1) * 64U;
            if (count > (batch_kind == B_130 ? 130U : 192U))
                count = batch_kind == B_130 ? 130U : 192U;
            for (unsigned int i = 0; i < count; ++i) batch_model_key('0' + i % 10);
            break;
        case B_ESC_BOUNDARY: case B_SS3_BOUNDARY:
            for (unsigned int i = 0; i < 63; ++i) batch_model_key('a');
            if (batch_frames == 3) batch_model_key(KEY_LEFT);
            break;
        case B_CSI_EMPTY: case B_SS3_EMPTY: case B_MOVES:
            batch_model_key('a'); batch_model_key('b'); batch_model_key('c');
            if (batch_kind == B_MOVES || batch_frames == 3) batch_model_key(KEY_LEFT);
            break;
        case B_MALFORMED: batch_model_key('b'); break;
        case B_FULL_LINE:
            count = (unsigned int)(batch_frames - 1) * 64U;
            if (count > 256) count = 256;
            for (unsigned int i = 0; i < count; ++i) batch_model_key('a');
            if (batch_frames == 6) batch_model_key('X');
            break;
        case B_FULL_DOC:
            for (unsigned int i = 0; i < 64; ++i) batch_model_key(KEY_ENTER);
            break;
        case B_RESET_EDIT: break; /* a, Backspace => empty, still dirty. */
        case B_RESET_DIRTY:
            batch_model_key('a');
            if (batch_frames == 3) batch_model_key('b');
            break;
        default: batch_model_key('x'); break;
        }
    }
    check_editor_screen(); /* Every cell, cursor and pending wrap checked. */
    assert(batch_frames <= batch_target_frames);
    if (batch_frames == 2 &&
        (batch_kind == B_ESC_BOUNDARY || batch_kind == B_SS3_BOUNDARY))
        shared_available = batch_start + 66;
    if (batch_frames == 2 &&
        (batch_kind == B_CSI_EMPTY || batch_kind == B_SS3_EMPTY))
        shared_available = batch_start + 6;
    if (batch_frames == 2 && batch_kind == B_RESET_DIRTY)
        shared_available = batch_start + 3;
    if (batch_frames > 1 && batch_frames == batch_target_frames)
        shared_available = input_length; /* Quit only after dirty is displayed. */
}

static void batch_observe_tx(const void *data, unsigned int count) {
    static const char begin[] = "\033[0m\033[?25l";
    static const char show[] = "\033[?25h";
    static const char report[] = "\033[0m\033[2J";
    const char *p = data;
    for (unsigned int i = 0; i < count; ++i) {
        char c = p[i];
        assert(ansi_feed(&screen, &c, 1) == 0);
        batch_prefix = c == begin[batch_prefix] ? batch_prefix + 1U : (c == 27 ? 1U : 0U);
        if (batch_prefix == sizeof(begin) - 1U) {
            batch_prefix = 0;
            assert(!batch_frame_open);
            batch_frame_open = 1;
            ++batch_started;
        }
        batch_report_prefix = c == report[batch_report_prefix] ? batch_report_prefix + 1U
                                                             : (c == 27 ? 1U : 0U);
        if (batch_report_prefix == sizeof(report) - 1U) {
            batch_report_prefix = 0;
            assert(!batch_frame_open);
            ++batch_reports;
        }
        batch_show = c == show[batch_show] ? batch_show + 1U : (c == 27 ? 1U : 0U);
        if (batch_show == sizeof(show) - 1U) {
            batch_show = 0;
            if (batch_frame_open) {
                batch_frame_open = 0;
                ++batch_frames;
                batch_frame_complete();
            }
        }
    }
}

static void batch_prepare(enum batch_case kind) {
    reset_io();
    shared_input = batch_testing = 1;
    batch_kind = kind;
    batch_frames = batch_started = batch_reports = batch_frame_open = 0;
    batch_prefix = batch_show = batch_report_prefix = 0;
    batch_reset_sets = batch_generated = batch_progress_timed = 0;
    batch_stagger = batch_stagger_session = 0;
    batch_stagger_calls = 0;
    batch_render_prefix = 0;
    batch_render_failed = 0;
    batch_start = batch_reset_pending = 0;
    batch_release = 1;
    batch_target_frames = 2;
    ++batch_cases;
#ifdef KILO_RX_DIAGNOSTICS
    if (kind != B_RESET_FIFO) {
        batch_add(18);
        batch_start = batch_reset_pending = 1;
    }
#endif
    switch (kind) {
    case B_SINGLE: batch_add('a'); break;
    case B_MULTI: batch_add('a'); batch_add('b'); batch_add('c'); break;
    case B_130: case B_CONTINUOUS:
        for (int i = 0; i < (kind == B_130 ? 130 : 192); ++i) batch_add('0' + i % 10);
        batch_target_frames = 4;
        break;
    case B_PROGRESS:
        for (int i = 0; i < 128; ++i) batch_add(10);
        batch_target_frames = 1;
        break;
    case B_ESC_BOUNDARY: case B_SS3_BOUNDARY:
        for (int i = 0; i < 63; ++i) batch_add('a');
        batch_arrow(kind == B_ESC_BOUNDARY ? '[' : 'O', 'D');
        batch_target_frames = 3;
        break;
    case B_CSI_EMPTY: case B_SS3_EMPTY:
        batch_add('a'); batch_add('b'); batch_add('c');
        batch_arrow(kind == B_CSI_EMPTY ? '[' : 'O', 'D');
        batch_target_frames = 3;
        break;
    case B_ESC_TIMEOUT: batch_add(27); batch_add(-1); batch_add('x'); break;
    case B_UNKNOWN: batch_arrow('[', '9'); batch_add('~'); batch_add('x'); break;
    case B_MALFORMED: batch_arrow('[', 1); batch_add('A'); batch_add('b'); break;
    case B_DISCARD: batch_arrow('[', -1); batch_add('A'); batch_add('x'); break;
    case B_QUIET:
        batch_add(27); batch_add(']'); batch_add('a'); batch_add('b');
        batch_add(-1); batch_add('x'); break;
    case B_NOOPS:
        batch_arrow('[', 'D'); batch_arrow('[', 'H');
        batch_arrow('[', 'A'); batch_arrow('[', 'B');
        batch_target_frames = 1; break;
    case B_MOVES:
        batch_add('a'); batch_add('b'); batch_add('c');
        batch_arrow('[', 'D'); batch_arrow('[', 'D'); batch_arrow('[', 'C'); break;
    case B_FULL_LINE:
        for (int i = 0; i < 256; ++i) batch_add('a');
        batch_add('X'); batch_add('X'); batch_add('X');
        batch_target_frames = 6; break;
    case B_FULL_DOC:
        for (int i = 0; i < 65; ++i) batch_add(13);
        break;
    case B_QUIT:
        batch_add('a'); batch_add('b'); batch_add(17); batch_add('z');
        batch_target_frames = 1; break;
    case B_RX_ERROR: batch_add('a'); batch_add(-2); batch_target_frames = 1; break;
    case B_POLL_ENTRY: case B_POLL_RESTORE:
        batch_add('a'); batch_add('b'); batch_target_frames = 1; break;
    case B_RENDER_ERROR: case B_TX_ERROR:
        batch_add('a'); batch_target_frames = 1; break;
    case B_OVERFLOW:
        for (int i = 0; i < 64; ++i) batch_add('a');
        batch_target_frames = 1; break;
    case B_RESET: batch_add('x'); batch_add(20); batch_target_frames = 1; break;
    case B_RESET_FIFO: batch_add(18); batch_add('x'); break;
    case B_RESET_EDIT: batch_add('a'); batch_add(8); batch_add(18); break;
    case B_RESET_DIRTY: batch_add('a'); batch_add(18); batch_add('b');
        batch_target_frames = 3; break;
    case B_SNAPSHOT:
        batch_add('a'); batch_add('b'); batch_add('c'); batch_add(20);
        batch_add('z'); batch_add(20); batch_target_frames = 1; break;
    case B_REPORT_ERROR:
        batch_add('a'); batch_add(20); batch_target_frames = 1; break;
    }
    int error = (kind >= B_RX_ERROR && kind <= B_OVERFLOW) || kind == B_REPORT_ERROR;
    if (!error && kind != B_QUIT) batch_add(17);
    batch_first_end = input_length - (!error && kind != B_QUIT ? 1 : 0);
    if (kind == B_PROGRESS || kind == B_NOOPS || kind == B_SNAPSHOT || kind == B_RESET)
        batch_first_end = input_length; /* Quit/report is part of this stream. */
    if (kind == B_ESC_BOUNDARY || kind == B_SS3_BOUNDARY || kind == B_FULL_DOC)
        batch_first_end = batch_start + 64;
    if (kind == B_CSI_EMPTY || kind == B_SS3_EMPTY) batch_first_end = batch_start + 5;
    if (kind == B_ESC_TIMEOUT) batch_first_end = batch_start + 1;
    if (kind == B_DISCARD) batch_first_end = batch_start + 2;
    if (kind == B_QUIET) batch_first_end = batch_start + 4;
    if (kind == B_FULL_LINE) batch_first_end = batch_start + 258;
    if (kind == B_RESET_DIRTY) batch_first_end = batch_start + 2;
    if (kind == B_RESET_FIFO) {
        batch_release = 0;
        shared_available = 2; /* F3 queues R/x before main handles Ctrl-R. */
    }
}

static void batch_run(enum batch_case kind) {
    batch_prepare(kind);
    int failed = (kind >= B_RX_ERROR && kind <= B_OVERFLOW) || kind == B_REPORT_ERROR;
    assert(kmain() == failed);
    assert(batch_frames == batch_target_frames && batch_started == batch_target_frames);
    assert(!batch_frame_open && !screen.error && screen.visible);
    assert(restores == 1 && unbinds == 1);
    if (kind == B_RENDER_ERROR) {
        assert(batch_render_failed && batch_render_prefix == 4);
        assert(batch_frames == 1 && batch_started == 1 && !batch_frame_open);
        puts("PASS B_RENDER_ERROR: subsequent editor frame reached TX, injected failure and cleanup verified");
    }
    if (kind == B_PROGRESS) assert(batch_progress_timed >= 1);
    /* z may be prefetched, but must not be dispatched after Ctrl-Q. */
    if (kind == B_QUIT) assert(input_at >= batch_start + 3);
    if (kind == B_OVERFLOW) assert(batch_generated > 0 && batch_generated <= 513);
    if (failed && kind != B_REPORT_ERROR) assert(batch_reports == 0);
#ifdef KILO_RX_DIAGNOSTICS
    static struct terminal_rx_diagnostics result;
    terminal_diag_snapshot(&result, 0);
    if (kind == B_130) {
        assert(result.delivered == 130 && result.accepted == 130);
        assert(result.rx_direct + result.rx_poll == 131);
        assert(result.enqueued == result.rx_poll && result.fifo_peak <= 512);
        assert(result.fifo_pending == 0 && !result.overflows && !result.transport_error);
    }
    if (kind == B_QUIT || kind == B_RX_ERROR || kind == B_POLL_ENTRY ||
        kind == B_POLL_RESTORE || kind == B_RENDER_ERROR || kind == B_TX_ERROR) {
        unsigned int n = kind == B_QUIT ? 2U : 1U;
        assert(result.delivered == n && result.accepted == n);
    }
    if (kind == B_OVERFLOW) {
        /* Services interleave enqueue and dequeue, so overflow can occur
         * before the 64-byte boundary. Check conservation, not old timing. */
        assert(result.delivered > 0 && result.delivered < 64);
        assert(result.accepted == result.delivered && result.rx_direct == 1);
        assert(result.rx_poll == 63U + (unsigned int)batch_generated);
        assert(result.rx_poll == result.enqueued + 1 && result.overflows == 1);
        assert(result.enqueued == 512U + result.delivered - 1U);
        assert(result.fifo_peak == 512 && result.transport_error);
    }
    if (kind == B_FULL_LINE) assert(result.delivered == 259 && result.accepted == 256);
    if (kind == B_RESET || kind == B_SNAPSHOT || kind == B_REPORT_ERROR) {
        unsigned int n = kind == B_SNAPSHOT ? 3U : 1U;
        assert(result.delivered == n && result.accepted == n);
        /* RX counts include bytes prefetched beyond Ctrl-T, while delivered
         * and accepted stop at the frozen snapshot command boundary. */
        unsigned int trailing = kind == B_SNAPSHOT ? 3U : kind == B_RESET ? 1U : 0U;
        assert(result.rx_direct + result.rx_poll == n + 1U + trailing);
        assert(result.fifo_pending == trailing && result.ctrl_t_included);
        if (kind != B_REPORT_ERROR) {
            assert(batch_reports == (kind == B_SNAPSHOT ? 2 : 1));
            assert(strstr(output, kind == B_SNAPSHOT ? "Row length: 0000000003"
                                                   : "Row length: 0000000001"));
        }
    }
    if (kind == B_RESET_FIFO || kind == B_RESET_EDIT)
        assert(result.delivered == 0 && result.accepted == 0);
    if (kind == B_RESET_DIRTY) assert(result.delivered == 2 && result.accepted == 2);
#endif
    batch_testing = 0;
}

static void batching_tests(void) {
    for (int kind = B_SINGLE; kind <= B_OVERFLOW; ++kind)
        batch_run((enum batch_case)kind);
#ifdef KILO_RX_DIAGNOSTICS
    for (int kind = B_RESET; kind <= B_REPORT_ERROR; ++kind)
        batch_run((enum batch_case)kind);
#endif
    printf("PASS F2 batching: %u scenarios; raw 64-byte boundaries, coalesced frames, ordered events/barriers, full screen, errors and FIFO\n", batch_cases);
}

static void batch_rx_tests(void) {
    static const enum batch_case cases[] = {
        B_MULTI, B_MOVES, B_NOOPS, B_130, B_CONTINUOUS, B_PROGRESS
    };
    for (unsigned int i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
        batch_prepare(cases[i]);
        batch_stagger = 1;
        assert(kmain() == 0);
        assert(batch_frames == batch_target_frames && !screen.error && screen.visible);
        assert(batch_stagger_calls > 0 && restores == 1 && unbinds == 1);
        batch_testing = 0;
    }
    /* A failed service after a dirty event must never transmit that frame. */
    for (int failure = 0; failure < 3; ++failure) {
        batch_prepare(B_RX_ERROR);
        /* Entry/restore failures and a read error in the newly added service. */
        if (failure == 0) batch_kind = B_POLL_ENTRY;
        if (failure == 1) batch_kind = B_POLL_RESTORE;
        assert(kmain() == 1);
        assert(batch_frames == 1 && batch_started == 1 && !screen.error);
        assert(restores == 1 && unbinds == 1 && screen.visible);
        batch_testing = 0;
    }
#ifdef KILO_RX_DIAGNOSTICS
    /* LF leaves an empty document, then service prefetches R/x/T/Q. R must
     * reject reset because FIFO contains trailing data; never flush it. */
    batch_prepare(B_RESET);
    input_length = batch_start;
    batch_add(10); batch_add(18); batch_add('x'); batch_add(20); batch_add(17);
    batch_first_end = input_length;
    assert(kmain() == 0 && batch_frames == 1 && batch_reports == 1);
    static struct terminal_rx_diagnostics result;
    terminal_diag_snapshot(&result, 0);
    assert(result.rx_direct + result.rx_poll == 5);
    assert(result.delivered == 1 && result.accepted == 1);
    /* main adds document fields to its report copy, not terminal's frozen counters. */
    assert(strstr(output, "Row length: 0000000001"));
    assert(result.fifo_pending == 1 && result.ctrl_t_included);
    assert(!result.overflows && !result.transport_error);
    assert(restores == 1 && unbinds == 1 && screen.visible && !screen.error);
    batch_testing = 0;
    puts("PASS RX02 D2: prefetched Ctrl-R rejected with empty document and trailing FIFO input");
#endif
    puts("PASS RX02: 6 staged-arrival streams and 3 batch-service failures; ordered input, coalescing and cleanup");
}

int main(void) {
    reset_io(); assert(terminal_open() == 0); interpreter_tests();
    split_tests(); editing_tests(); navigation_tests();
    renderer_tests(); renderer_failure_tests(); parser_tests(); lifecycle_tests(); transport_tests();
    rx_service_tests();
#ifdef KILO_RX_DIAGNOSTICS
    diagnostic_tests();
#endif
    input_api_tests();
    batching_tests();
    batch_rx_tests();
    printf("Host sizeof(row)=%zu sizeof(editor)=%zu (not target ABI measurements)\n",
           sizeof(struct editor_row), sizeof(struct editor));
    printf("Largest complete production frame observed: %u bytes (bound 2247, capacity 4096)\n", largest_frame);
    puts("All B1.1/B1.2/B1.3a/H07-F1 host checks passed; no hardware execution.");
    return 0;
}
