/* Kilo68k, BSD-2-Clause; see LICENSE. */
#ifndef KILO68K_TERMINAL_H
#define KILO68K_TERMINAL_H

enum terminal_key {
    KEY_NONE = 256, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
    KEY_HOME, KEY_END, KEY_BACKSPACE, KEY_DELETE, KEY_QUIT
};

/* Call close even after open fails. No Unix file descriptors are used. */
int terminal_open(void);
int terminal_close(int failed);
/* One timed byte read: 0 = no event, 1 = key, -1 = I/O error.
 * Printable ASCII is returned as its byte value; other keys are above 255. */
int terminal_read_key(int *key);
int terminal_write(const char *data, unsigned int count);

#endif
