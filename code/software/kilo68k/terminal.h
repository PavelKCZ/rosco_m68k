/* Kilo68k, BSD-2-Clause; see LICENSE. */
#ifndef KILO68K_TERMINAL_H
#define KILO68K_TERMINAL_H

enum terminal_key {
    KEY_NONE = 256, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
    KEY_HOME, KEY_END, KEY_BACKSPACE, KEY_DELETE, KEY_QUIT, KEY_ENTER
};

enum terminal_input_result {
    TERMINAL_INPUT_ERROR = -1,
    TERMINAL_INPUT_EMPTY = 0,
    TERMINAL_INPUT_PROGRESS = 1,
    TERMINAL_INPUT_EVENT = 2
};

/* Call close even after open fails. No Unix file descriptors are used. */
int terminal_open(void);
int terminal_close(int failed);
/* All input operations require a valid non-NULL key pointer. They consume
 * at most one byte, preferring FIFO data. Only EVENT sets a key; all other
 * results set KEY_NONE. ERROR latches transport failure until a new open.
 * PROGRESS means one byte consumed without an event; EMPTY means no byte.
 * An ERROR may follow successful RX if restoring timed mode failed; that
 * byte remains accounted for in diagnostics but is not parsed/dispatched. */
/* FIFO first, otherwise timed input. A genuine timeout applies the existing
 * parser timeout transition and returns EMPTY. */
enum terminal_input_result terminal_read_input(int *key);
/* FIFO first without mode changes; otherwise a single nonblocking read in
 * temporary polling mode, always attempting timed-mode restoration.
 * EMPTY polling never applies a parser timeout. No pumping or prefetch. */
enum terminal_input_result terminal_poll_input(int *key);
/* Transport failures latch until a new open; close must still be called.
 * Reads consume the bounded RX FIFO before performing a timed read.
 * Compatibility wrapper: ERROR -> -1, EMPTY/PROGRESS -> 0, EVENT -> 1.
 * Printable ASCII is returned as its byte value; other keys are above 255. */
int terminal_read_key(int *key);
/* Sends single-byte segments while polling RX into a 512-byte FIFO.
 * Overflow is fatal. Restores timed raw mode even after output errors.
 * A zero count is a no-op on a healthy open transport. */
int terminal_write(const char *data, unsigned int count);
/* Bounded nonblocking RX during frame construction or between batch inputs,
 * outside terminal_write. Call only after handling any command barrier.
 * No TX, parsing, dispatch or document changes. 0 success, -1 latched error.
 * Attempts timed-mode restoration on every mode-switch/pump error path. */
int terminal_service_rx(void);

#ifdef KILO_RX_DIAGNOSTICS
#include <stdint.h>
struct terminal_rx_diagnostics {
    uint32_t rx_direct, rx_poll, enqueued, overflows;
    uint32_t delivered, accepted;
    uint16_t fifo_peak, fifo_pending;
    uint16_t row_length, cx, cy;
    uint8_t saturated, parser_state, transport_error, ctrl_t_included;
};
/* Reset never reads/flushes input. Snapshot freezes until the next reset.
 * triggered is true only for a Ctrl-T returned by a terminal input operation. */
int terminal_diag_reset(void);
void terminal_diag_snapshot(struct terminal_rx_diagnostics *out, int triggered);
void terminal_diag_delivered(void);
void terminal_diag_accepted(void);
int terminal_diag_report(const struct terminal_rx_diagnostics *snapshot);
/* Shared saturating primitive; exposed to verify the boundary in host tests. */
void terminal_diag_increment(uint32_t *counter, uint8_t *saturated);
#endif

#endif
