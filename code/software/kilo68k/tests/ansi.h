/* Deterministic native-test terminal model; BSD-2-Clause, see ../LICENSE. */
#ifndef KILO_TEST_ANSI_H
#define KILO_TEST_ANSI_H

struct ansi_screen {
    char cells[25][80];
    unsigned int row, col;
    unsigned int wraps, scrolls, bottom_right_writes, aborted_sequences;
    int visible, pending_wrap, state, error;
    char sequence[20];
    unsigned int used;
};

void ansi_init(struct ansi_screen *s, char fill);
/* Streaming: retains CSI state across arbitrarily split writes. */
int ansi_feed(struct ansi_screen *s, const char *data, unsigned int count);

#endif
