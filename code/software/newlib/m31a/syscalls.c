#include "runtime.h"
#include "diagnostic.h"
#include <reent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include "termios.h"
static struct m31_heap heap;
static int console_ready;
#ifdef M31_DIAGNOSTIC
static int diagnostic_write_seen; /* zeroed with application BSS */
#endif
/* Private names are compile-time aliases only in the separate termios object. */
extern int m31_termios_tcgetattr(int, struct termios *);
extern int m31_termios_tcsetattr(int, int, const struct termios *);
extern int m31_termios_tcdrain(int);
extern int m31_termios_tcflush(int, int);
static int fail(struct _reent *r, int code) { _REENT_ERRNO(r) = code; return -1; }
int m31_console_init(void) {
    if (console_ready) return 0;
    if (rosco_termios_bind(0, 0)) return -1;
    console_ready = 1;
    return 0;
}
void m31_heap_init(const struct m31_layout *x) {
    heap.start = heap.current = x->heap_start;
    heap.end = x->heap_end;
}
void *_sbrk_r(struct _reent *r, ptrdiff_t increment) {
    uint32_t previous;
    if (m31_heap_move(&heap, increment, &previous)) {
        fail(r, ENOMEM); return (void *)-1;
    }
    return (void *)(uintptr_t)previous;
}
static int console_fd(int fd) {
    if (!console_ready || fd < 0 || fd > 2) { errno = EBADF; return -1; }
    return 0;
}
int tcgetattr(int fd, struct termios *t) {
    return console_fd(fd) ? -1 : m31_termios_tcgetattr(0, t);
}
int tcsetattr(int fd, int action, const struct termios *t) {
    return console_fd(fd) ? -1 : m31_termios_tcsetattr(0, action, t);
}
int tcdrain(int fd) { return console_fd(fd) ? -1 : m31_termios_tcdrain(0); }
int tcflush(int fd, int q) { return console_fd(fd) ? -1 : m31_termios_tcflush(0, q); }
_ssize_t _read_r(struct _reent *r, int fd, void *buffer, size_t count) {
    int n;
    if (!console_ready || fd != 0) return fail(r, EBADF);
    if (count > INT_MAX) return fail(r, EINVAL);
    n = rosco_termios_read(0, buffer, (unsigned)count);
    return n < 0 ? fail(r, errno) : n;
}
_ssize_t _write_r(struct _reent *r, int fd, const void *buffer, size_t count) {
    int n;
#ifdef M31_DIAGNOSTIC
    if (!diagnostic_write_seen) { diagnostic_write_seen = 1; M31_MARK('W'); }
#endif
    if (!console_ready || (fd != 1 && fd != 2)) return fail(r, EBADF);
    if (count > INT_MAX) return fail(r, EINVAL);
    n = rosco_termios_write(0, buffer, (unsigned)count);
    return n < 0 ? fail(r, errno) : n;
}
int _fstat_r(struct _reent *r, int fd, struct stat *s) {
    if (!console_ready || fd < 0 || fd > 2) return fail(r, EBADF);
    if (!s) return fail(r, EFAULT);
    memset(s, 0, sizeof(*s)); s->st_mode = S_IFCHR; s->st_nlink = 1;
    return 0;
}
int _isatty_r(struct _reent *r, int fd) {
    if (console_ready && fd >= 0 && fd <= 2) return 1;
    fail(r, EBADF);
    return 0;
}
int _close_r(struct _reent *r, int fd) {
    return fail(r, fd >= 0 && fd <= 2 ? ENOTSUP : EBADF);
}
_off_t _lseek_r(struct _reent *r, int fd, _off_t off, int whence) {
    (void)off; (void)whence;
    return fail(r, fd >= 0 && fd <= 2 ? ESPIPE : EBADF);
}
/* No file namespace, SD initialization, or backend in M3.1-A. */
int _open_r(struct _reent *r, const char *p, int flags, int mode) {
    (void)p; (void)flags; (void)mode; return fail(r, ENOSYS);
}
int _stat_r(struct _reent *r, const char *p, struct stat *s) {
    (void)p; (void)s; return fail(r, ENOSYS);
}
int _unlink_r(struct _reent *r, const char *p) { (void)p; return fail(r, ENOSYS); }
int _link_r(struct _reent *r, const char *a, const char *b) {
    (void)a; (void)b; return fail(r, ENOSYS);
}
int _rename_r(struct _reent *r, const char *a, const char *b) {
    (void)a; (void)b; return fail(r, ENOSYS);
}
/* Newlib abort/signal support may reference these even in a single application. */
int _kill_r(struct _reent *r, int pid, int sig) {
    (void)pid; (void)sig; return fail(r, ENOSYS);
}
int _getpid_r(struct _reent *r) { (void)r; return 1; }
