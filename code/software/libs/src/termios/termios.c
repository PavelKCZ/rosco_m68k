/*
 * Minimal POSIX-like termios layer for rosco_m68k.
 * Copyright (c) 2026, contributors.
 *
 * The ROM DEVICE_CTRL implementation is intentionally not modified or used
 * to change UART configuration. This RC1 describes the verified board profile as 38400 8N1;
 * actual UART configuration remains owned by firmware. Canonical input, echo and output processing are implemented in
 * software on top of the firmware character-device API.
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fdlibm.h>
#include "machine.h"
#include "termios.h"

#ifndef ENOTSUP
#define ENOTSUP EINVAL
#endif

#define ROSCO_TERMIOS_FDS 4
#define CANONICAL_LINE_SIZE 256

struct termios_fd {
    int bound;
    CharDevice device;
    struct termios settings;
    unsigned char line[CANONICAL_LINE_SIZE];
    unsigned int line_length;
    unsigned int line_offset;
    int line_ready;
};

static struct termios_fd fd_table[ROSCO_TERMIOS_FDS];

static int valid_fd(int fd) {
    if (fd < 0 || fd >= ROSCO_TERMIOS_FDS || !fd_table[fd].bound) {
        errno = EBADF;
        return 0;
    }
    return 1;
}

static void default_settings(struct termios *t) {
    unsigned int i;
    t->c_iflag = ICRNL;
    t->c_oflag = OPOST | ONLCR;
    t->c_cflag = CREAD | CS8 | CLOCAL;
    t->c_lflag = ICANON | ECHO | ECHOE | ECHOK | IEXTEN;
    for (i = 0; i < NCCS; ++i) t->c_cc[i] = 0;
    t->c_cc[VINTR] = 3;
    t->c_cc[VQUIT] = 28;
    t->c_cc[VERASE] = 127;
    t->c_cc[VKILL] = 21;
    t->c_cc[VEOF] = 4;
    t->c_cc[VTIME] = 0;
    t->c_cc[VMIN] = 1;
    t->c_cc[VSTART] = 17;
    t->c_ispeed = B38400;
    t->c_ospeed = B38400;
}

int rosco_termios_bind_device(int fd, const void *char_device) {
    struct termios_fd *slot;
    if (fd < 0 || fd >= ROSCO_TERMIOS_FDS || char_device == NULL) {
        errno = EINVAL;
        return -1;
    }
    slot = &fd_table[fd];
    memcpy(&slot->device, char_device, sizeof(CharDevice));
    slot->bound = 1;
    slot->line_length = 0;
    slot->line_offset = 0;
    slot->line_ready = 0;
    default_settings(&slot->settings);
    return 0;
}

int rosco_termios_bind(int fd, uint8_t firmware_device_index) {
    CharDevice device;
    if (!mcCheckDeviceSupport()) {
        errno = ENODEV;
        return -1;
    }
    if (firmware_device_index >= mcGetDeviceCount() ||
        !mcGetDevice(firmware_device_index, &device)) {
        errno = ENODEV;
        return -1;
    }
    return rosco_termios_bind_device(fd, &device);
}

int rosco_termios_unbind(int fd) {
    if (fd < 0 || fd >= ROSCO_TERMIOS_FDS) {
        errno = EBADF;
        return -1;
    }
    fd_table[fd].bound = 0;
    fd_table[fd].line_length = 0;
    fd_table[fd].line_offset = 0;
    fd_table[fd].line_ready = 0;
    return 0;
}

int tcgetattr(int fd, struct termios *termios_p) {
    if (!valid_fd(fd)) return -1;
    if (termios_p == NULL) {
        errno = EINVAL;
        return -1;
    }
    *termios_p = fd_table[fd].settings;
    return 0;
}

/* r2: fail explicitly where firmware cannot provide POSIX semantics. */
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
    struct termios_fd *slot;
    if (!valid_fd(fd)) return -1;
    if (termios_p == NULL || optional_actions < TCSANOW || optional_actions > TCSAFLUSH) {
        errno = EINVAL; return -1;
    }
    if (termios_p->c_ispeed != B38400 || termios_p->c_ospeed != B38400 ||
        (termios_p->c_cflag & CSIZE) != CS8 ||
        (termios_p->c_cflag & (PARENB | PARODD | CSTOPB))) {
        errno = ENOTSUP; return -1;
    }
    /* r4: reject flags whose behavior is not provided by this layer.
     * IEXTEN is retained as an inert compatibility flag (default is on).
     * CREAD and CLOCAL describe the firmware-fixed connection.
     */
    if ((termios_p->c_iflag & ~(ICRNL | IGNCR | INLCR)) ||
        (termios_p->c_oflag & ~(OPOST | ONLCR)) ||
        (termios_p->c_cflag & ~(CSIZE | CREAD | CLOCAL)) ||
        (termios_p->c_lflag & ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | IEXTEN))) {
        errno = ENOTSUP; return -1;
    }
    /* Input translation is currently implemented only in canonical mode. */
    if (!(termios_p->c_lflag & ICANON) &&
        (termios_p->c_iflag & (ICRNL | IGNCR | INLCR))) {
        errno = ENOTSUP; return -1;
    }
    /* Switching away from canonical mode must not silently drop an unread
     * completed line. Keep the old configuration until it is consumed or
     * explicitly discarded with TCIFLUSH. */
    slot = &fd_table[fd];
    if ((slot->settings.c_lflag & ICANON) &&
        !(termios_p->c_lflag & ICANON) && slot->line_ready) {
        errno = ENOTSUP; return -1;
    }
    /* TCSADRAIN/TCSAFLUSH require guaranteed TX completion; not exposed
       by the character-device firmware API. No settings are changed. */
    if (optional_actions != TCSANOW) { errno = ENOTSUP; return -1; }
    slot = &fd_table[fd];
    slot->settings = *termios_p;
    return 0;
}

speed_t cfgetispeed(const struct termios *termios_p) {
    return termios_p ? termios_p->c_ispeed : 0;
}

speed_t cfgetospeed(const struct termios *termios_p) {
    return termios_p ? termios_p->c_ospeed : 0;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
    if (!termios_p) { errno = EINVAL; return -1; }
    termios_p->c_ispeed = speed;
    return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
    if (!termios_p) { errno = EINVAL; return -1; }
    termios_p->c_ospeed = speed;
    return 0;
}

void cfmakeraw(struct termios *t) {
    if (!t) return;
    t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    t->c_oflag &= ~OPOST;
    t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(CSIZE | PARENB);
    t->c_cflag |= CS8 | CREAD | CLOCAL;
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}

static void send_processed(struct termios_fd *slot, unsigned char c) {
    if ((slot->settings.c_oflag & OPOST) &&
        (slot->settings.c_oflag & ONLCR) && c == '\n') {
        mcSendDevice('\r', &slot->device);
    }
    mcSendDevice((char)c, &slot->device);
}

static void echo_char(struct termios_fd *slot, unsigned char c) {
    if ((slot->settings.c_lflag & ECHO) ||
        (c == '\n' && (slot->settings.c_lflag & ECHONL))) send_processed(slot, c);
}

static int canonical_read(struct termios_fd *slot, unsigned char *out, unsigned int count) {
    unsigned char c;
    unsigned int n = 0;
    if (count == 0) return 0;

    if (!slot->line_ready) {
        slot->line_length = 0;
        slot->line_offset = 0;
        for (;;) {
            c = (unsigned char)mcReadDevice(&slot->device);
            if ((slot->settings.c_iflag & IGNCR) && c == '\r') continue;
            if ((slot->settings.c_iflag & ICRNL) && c == '\r') c = '\n';
            if ((slot->settings.c_iflag & INLCR) && c == '\n') c = '\r';

            if (c == slot->settings.c_cc[VEOF]) {
                if (slot->line_length == 0) return 0;
                break;
            }
            if (c == slot->settings.c_cc[VERASE]) {
                if (slot->line_length > 0) {
                    --slot->line_length;
                    if ((slot->settings.c_lflag & (ECHO | ECHOE)) == (ECHO | ECHOE)) {
                        mcSendDevice('\b', &slot->device);
                        mcSendDevice(' ', &slot->device);
                        mcSendDevice('\b', &slot->device);
                    } else if (slot->settings.c_lflag & ECHO) {
                        echo_char(slot, c);
                    }
                }
                continue;
            }
            /* VKILL removes the entire pending input line. The configured
             * control byte is only special while canonical mode is active.
             * ECHOK requests a newline after the erased line. */
            if (c == slot->settings.c_cc[VKILL]) {
                unsigned int i;
                if (slot->settings.c_lflag & ECHO) {
                    if (slot->settings.c_lflag & ECHOE) {
                        for (i = 0; i < slot->line_length; ++i) {
                            mcSendDevice('\b', &slot->device);
                            mcSendDevice(' ', &slot->device);
                            mcSendDevice('\b', &slot->device);
                        }
                    } else {
                        echo_char(slot, c);
                    }
                    if (slot->settings.c_lflag & ECHOK)
                        send_processed(slot, '\n');
                }
                slot->line_length = 0;
                continue;
            }
            if (c == '\n') {
                /* Reserve the final byte for LF even when the line is full.
                 * Extra ordinary input bytes are discarded, not written past
                 * the fixed-size buffer. */
                slot->line[slot->line_length++] = c;
                echo_char(slot, c);
                break;
            }
            if (slot->line_length < CANONICAL_LINE_SIZE - 1) {
                slot->line[slot->line_length++] = c;
                echo_char(slot, c);
            }
        }
        slot->line_ready = 1;
    }

    while (n < count && slot->line_offset < slot->line_length) {
        out[n++] = slot->line[slot->line_offset++];
    }
    if (slot->line_offset >= slot->line_length) {
        slot->line_ready = 0;
        slot->line_length = 0;
        slot->line_offset = 0;
    }
    return (int)n;
}

static int raw_read(struct termios_fd *slot, unsigned char *out, unsigned int count) {
    unsigned int n = 0;
    unsigned int min_count = slot->settings.c_cc[VMIN];
    unsigned int timeout = slot->settings.c_cc[VTIME];
    uint32_t last_char = 0;

    if (count == 0) return 0;
    if (min_count > count) min_count = count;

    /* VMIN=0, VTIME=0: return immediately with whatever is available. */
    if (min_count == 0 && timeout == 0) {
        while (n < count && mcCheckDevice(&slot->device)) {
            out[n++] = (unsigned char)mcReadDevice(&slot->device);
        }
        return (int)n;
    }

    /* VTIME=0 and VMIN>0: block until the requested minimum is received. */
    if (timeout == 0) {
        while (n < min_count) out[n++] = (unsigned char)mcReadDevice(&slot->device);
        return (int)n;
    }

    /* VTIME>0: with VMIN=0, timeout starts immediately. With VMIN>0,
       POSIX starts the inter-character timer only after the first byte. */
    {
        uint32_t start = _TIMER_100HZ;
        while (n < count) {
            if (mcCheckDevice(&slot->device)) {
                out[n++] = (unsigned char)mcReadDevice(&slot->device);
                last_char = _TIMER_100HZ;
                if (min_count == 0 || n >= min_count) break;
            } else {
                uint32_t now = _TIMER_100HZ;
                if (min_count == 0) {
                    if ((uint32_t)(now - start) >= ((uint32_t)timeout * 10U)) break;
                } else if (n > 0 &&
                           (uint32_t)(now - last_char) >= ((uint32_t)timeout * 10U)) {
                    break;
                }
            }
        }
    }
    return (int)n;
}

int rosco_termios_read(int fd, void *buffer, unsigned int count) {
    struct termios_fd *slot;
    if (!valid_fd(fd)) return -1;
    if (buffer == NULL && count != 0) { errno = EINVAL; return -1; }
    slot = &fd_table[fd];
    if (count == 0) return 0;
    if (slot->settings.c_lflag & ICANON)
        return canonical_read(slot, (unsigned char *)buffer, count);
    return raw_read(slot, (unsigned char *)buffer, count);
}

int rosco_termios_write(int fd, const void *buffer, unsigned int count) {
    struct termios_fd *slot;
    const unsigned char *p = (const unsigned char *)buffer;
    unsigned int i;
    if (!valid_fd(fd)) return -1;
    if (buffer == NULL && count != 0) { errno = EINVAL; return -1; }
    slot = &fd_table[fd];
    for (i = 0; i < count; ++i) send_processed(slot, p[i]);
    return (int)count;
}

int tcdrain(int fd) {
    if (!valid_fd(fd)) return -1;
    /* mcSendDevice() does not document a TX-complete guarantee. */
    errno = ENOTSUP;
    return -1;
}

int tcflush(int fd, int queue_selector) {
    struct termios_fd *slot;
    if (!valid_fd(fd)) return -1;
    if (queue_selector != TCIFLUSH && queue_selector != TCOFLUSH &&
        queue_selector != TCIOFLUSH) { errno = EINVAL; return -1; }
    /* No TX queue discard API. Fail atomically for mixed flush requests. */
    if (queue_selector != TCIFLUSH) { errno = ENOTSUP; return -1; }
    slot = &fd_table[fd];
    slot->line_length = 0;
    slot->line_offset = 0;
    slot->line_ready = 0;
    /* Drain bytes already observable in firmware's RX queue. This is a
       best-effort snapshot, not an atomic hardware FIFO flush. */
    while (mcCheckDevice(&slot->device)) {
        (void)mcReadDevice(&slot->device);
    }
    return 0;
}
