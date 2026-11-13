/* Kilo68k M2.2-B1.3a, BSD-2-Clause; see LICENSE. */
#include "editor.h"
#include "terminal.h"

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
        int result = terminal_read_key(&key);
        if (result < 0) goto cleanup;
        if (result == 0) continue;
        if (key == KEY_QUIT) { failed = 0; break; }
#ifdef KILO_RX_DIAGNOSTICS
        /* Ctrl-R/Ctrl-T were previously ignored controls. Interpret only
         * raw events returned by the parser, never sequence fragments. */
        if (key == 18) {
            if (document.row_count == 1 && document.rows[0].length == 0 &&
                terminal_diag_reset() == 0) {
                showing_snapshot = 0;
            }
            continue;
        }
        if (key == 20) {
            if (!showing_snapshot) {
                terminal_diag_snapshot(&snapshot, 1);
                snapshot.row_length = document.rows[document.cy].length;
                snapshot.cx = document.cx; snapshot.cy = document.cy;
                showing_snapshot = 1;
            }
            if (terminal_diag_report(&snapshot) < 0) goto cleanup;
            continue;
        }
        if (showing_snapshot) continue; /* Frozen report; Ctrl-Q still exits. */
        if (key >= 32 && key <= 126) {
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
        if (result < 0 || (result > 0 && editor_render(&document) < 0))
            goto cleanup;
    }
cleanup:
    return terminal_close(failed);
}
