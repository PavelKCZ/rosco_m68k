/* Additional host probes call the actual syscall and unchanged termios code. */
#include <reent.h>
#include <errno.h>
#include <sys/stat.h>
#include "runtime.h"
#include "termios.h"
extern int _fstat_r(struct _reent *,int,struct stat *);
extern void fake_available(int);
int m31_probe_fstat(void) {
    struct _reent r={0};struct stat s;
    if (_fstat_r(&r,0,&s) || !S_ISCHR(s.st_mode) || s.st_size || s.st_uid || s.st_gid || s.st_nlink!=1) return 1;
    if (_fstat_r(&r,2,&s) || !S_ISCHR(s.st_mode)) return 2;
    if (_fstat_r(&r,32,&s)!=-1 || r._errno!=EBADF) return 3;
    if (_fstat_r(&r,0,0)!=-1 || r._errno!=EFAULT) return 4;
    return 0;
}
#ifdef M31_BIND_PROBE
int main(void) {
    fake_available(0);
    if (m31_console_init()!=-1 || errno!=ENODEV) return 1;
    /* Failure must not latch successful initialization; retry can bind. */
    fake_available(1);
    if (m31_console_init()) return 2;
    return 0;
}
#endif
