/* termios.c includes legacy fdlibm.h but uses no math interfaces.
 * Isolated include shim avoids importing legacy math/errno headers.
 * The unchanged termios.c receives Newlib errno.h and its __errno(). */
#ifndef M31A_FDLIBM_COMPAT_H
#define M31A_FDLIBM_COMPAT_H
#endif
