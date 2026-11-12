/* Kilo68k M2.2-A, BSD-2-Clause; see LICENSE. */
#include "editor.h"
#include "terminal.h"

static struct editor document;

int kmain(void) {
    int failed = 1;
    editor_init(&document);
    if (terminal_open() < 0) goto cleanup;
    if (editor_render(&document) < 0) goto cleanup;
    for (;;) {
        int key;
        int result = terminal_read_key(&key);
        if (result < 0) goto cleanup;
        if (result == 0) continue;
        if (key == KEY_QUIT) { failed = 0; break; }
        if (editor_key(&document, key) && editor_render(&document) < 0)
            goto cleanup;
    }
cleanup:
    return terminal_close(failed);
}
