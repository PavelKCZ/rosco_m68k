/*
 * rosco_m68k termios compatibility layer.
 *
 * This is a deliberately small POSIX-like API for programs such as Kilo.
 * It does not provide Unix file descriptors or modify the firmware ROM.
 */
#ifndef ROSCO_M68K_TERMIOS_H
#define ROSCO_M68K_TERMIOS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long tcflag_t;
typedef unsigned char cc_t;
typedef unsigned long speed_t;

#define NCCS  8
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF  4
#define VTIME 5
#define VMIN  6
#define VSTART 7

/* Input flags (declaration does not imply implementation; tcsetattr rejects unsupported flags) */
#define IGNBRK  0x00000001UL
#define BRKINT  0x00000002UL
#define IGNPAR  0x00000004UL
#define PARMRK  0x00000008UL
#define INPCK   0x00000010UL
#define ISTRIP  0x00000020UL
#define INLCR   0x00000040UL
#define IGNCR   0x00000080UL
#define ICRNL   0x00000100UL
#define IXON    0x00000400UL

/* Output flags */
#define OPOST   0x00000001UL
#define ONLCR   0x00000002UL

/* Control flags (reported for compatibility; hardware framing is fixed) */
#define CSIZE   0x00000030UL
#define CS5     0x00000000UL
#define CS6     0x00000010UL
#define CS7     0x00000020UL
#define CS8     0x00000030UL
#define CSTOPB  0x00000040UL
#define CREAD   0x00000080UL
#define PARENB  0x00000100UL
#define PARODD  0x00000200UL
#define HUPCL   0x00000400UL
#define CLOCAL  0x00000800UL

/* Local flags */
#define ISIG    0x00000001UL
#define ICANON  0x00000002UL
#define ECHO    0x00000008UL
#define ECHOE   0x00000010UL
#define ECHOK   0x00000020UL
#define ECHONL  0x00000040UL
#define NOFLSH  0x00000080UL
#define IEXTEN  0x00000100UL

/* tcsetattr actions */
#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

/* Speed constants. This build accepts only B38400 for both input/output.
 * The library reports the verified board profile but does not program UART. */
#define B0       0UL
#define B50      50UL
#define B75      75UL
#define B110     110UL
#define B300     300UL
#define B600     600UL
#define B1200    1200UL
#define B2400    2400UL
#define B4800    4800UL
#define B9600    9600UL
#define B19200   19200UL
#define B38400   38400UL
#define B57600   57600UL
#define B115200  115200UL

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

/*
 * rosco-specific setup: bind a small integer handle (fd) to a firmware
 * character-device slot. Do this before tcgetattr/tcsetattr/read/write.
 * The CharDevice structure is copied internally.
 */
int rosco_termios_bind(int fd, uint8_t firmware_device_index);
int rosco_termios_bind_device(int fd, const void *char_device);
int rosco_termios_unbind(int fd);

/* These I/O functions are used until rosco has a POSIX read/write layer.
 * They honour the software termios flags and operate on the bound device. */
int rosco_termios_read(int fd, void *buffer, unsigned int count);
int rosco_termios_write(int fd, const void *buffer, unsigned int count);

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
void cfmakeraw(struct termios *termios_p);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);

#define TCIFLUSH  0
#define TCOFLUSH  1
#define TCIOFLUSH 2

#ifdef __cplusplus
}
#endif
#endif
