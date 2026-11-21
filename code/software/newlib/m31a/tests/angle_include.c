/* Compile only; never executed. Newlib standard headers and project termios. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <reent.h>
#include <termios.h>
_Static_assert(NCCS == 8, "wrong termios header");
_Static_assert(sizeof(tcflag_t) == 4, "wrong target termios ABI");
_Static_assert(sizeof(FILE) > sizeof(void *), "legacy stdio header selected");
int m31_angle_include(struct termios *t) {
    cfmakeraw(t);
    return tcgetattr(0, t);
}
