/* Host-only ABI shim. Target compilation always uses installed Newlib reent.h. */
#ifndef M31_TEST_REENT
#define M31_TEST_REENT
#include <stddef.h>
#include <sys/types.h>
struct _reent { int _errno; };
#define _REENT_ERRNO(r) ((r)->_errno)
typedef ssize_t _ssize_t;
typedef off_t _off_t;
#endif
