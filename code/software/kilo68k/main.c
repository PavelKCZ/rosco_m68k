/* Kilo68k M2.2-B1.3a, BSD-2-Clause; see LICENSE. */
#include "editor.h"
#include "terminal.h"

#define INPUT_BATCH_BYTES 64U

static struct editor document;
#ifdef KILO_RX_DIAGNOSTICS
static struct terminal_rx_diagnostics snapshot;
#endif

int kmain(void) {
    int failed = 1;
#ifdef KILO_RX_DIAGNOSTICS
    int showing_snapshot = 0;
#endif
    editor_init(&document);
    if (terminal_open() < 0) goto cleanup;
    if (editor_render(&document) < 0) goto cleanup;
    for (;;) {
        int key;
        int dirty = 0;
        unsigned int consumed = 0;
        enum terminal_input_result input = terminal_read_input(&key);
        if (input == TERMINAL_INPUT_ERROR) goto cleanup;
        if (input == TERMINAL_INPUT_EMPTY) continue;
        for (;;) {
            /* Both PROGRESS and EVENT consume exactly one parser byte. */
            ++consumed;
            if (input == TERMINAL_INPUT_EVENT) {
                int result;
                if (key == KEY_QUIT) { failed = 0; goto cleanup; }
#ifdef KILO_RX_DIAGNOSTICS
                /* Commands are ordered barriers, with no prefetch before
                 * reset eligibility or diagnostic snapshot evaluation. */
                if (key == 18) {
                    if (document.row_count == 1 && document.rows[0].length == 0 &&
                        terminal_diag_reset() == 0) {
                        showing_snapshot = 0;
                    }
                    break; /* Preserve any earlier dirty editor state. */
                }
                if (key == 20) {
                    if (!showing_snapshot) {
                        terminal_diag_snapshot(&snapshot, 1);
                        snapshot.row_length = document.rows[document.cy].length;
                        snapshot.cx = document.cx; snapshot.cy = document.cy;
                        showing_snapshot = 1;
                    }
                    if (terminal_diag_report(&snapshot) < 0) goto cleanup;
                    dirty = 0; /* Never overwrite the frozen report. */
                    break;
                }
                if (showing_snapshot) result = 0;
                else if (key >= 32 && key <= 126) {
                    uint16_t row = document.cy;
                    uint16_t length = document.rows[row].length;
                    terminal_diag_delivered();
                    result = editor_key(&document, key);
                    if (result >= 0 && document.rows[row].length == length + 1U)
                        terminal_diag_accepted();
                } else result = editor_key(&document, key);
#else
                result = editor_key(&document, key);
#endif
                if (result < 0) goto cleanup;
                dirty = dirty || (result > 0);
            }
            if (consumed == INPUT_BATCH_BYTES) break;
            /* Keep draining hardware RX while consuming the software FIFO.
             * Commands above are barriers: never prefetch after recognition.
             * The renderer services RX itself at a dirty batch boundary. */
            if (terminal_service_rx() < 0) goto cleanup;
            input = terminal_poll_input(&key);
            if (input == TERMINAL_INPUT_ERROR) goto cleanup;
            if (input == TERMINAL_INPUT_EMPTY) break;
        }
        if (dirty) {
            if (editor_render(&document) < 0) goto cleanup;
            dirty = 0;
        }
    }
cleanup:
    return terminal_close(failed);
}
