# Known limitations and compatibility notes

This is a limited POSIX-like layer, **not** a complete implementation of POSIX termios.

1. **UART speed:** actual 38400 baud 8N1 configuration is owned by firmware. `B38400` is reported and enforced in software, not applied to UART hardware.
2. **Handles:** four internal handles (`0..3`) bound to firmware devices; not general Unix file descriptors.
3. **Canonical input:** 256-byte fixed line buffer (255 ordinary characters plus LF). Excess ordinary characters are dropped. EOF handling and editing are software-defined.
4. **Signals:** `ISIG`, Ctrl+C signal generation, job control, terminal sessions and process groups are not implemented.
5. **Unsupported flags:** some POSIX-like constants are declared in the header but rejected by `tcsetattr`. `IEXTEN` is accepted as an inert compatibility flag. Hardware framing is fixed at 8N1.
6. **Unsupported actions:** `TCSADRAIN`, `TCSAFLUSH`, `tcdrain`, `TCOFLUSH`, `TCIOFLUSH` are unsupported; they return an error. `ENOTSUP` falls back to `EINVAL` if the platform does not define it.
7. **RX flush:** `TCIFLUSH` drains only bytes currently visible to the firmware device and clears the software line buffer; not atomic relative to arriving data.
8. **Concurrency:** global descriptor table has no synchronization or thread-safety guarantee.
9. **Input translation:** `ICRNL`, `IGNCR`, `INLCR` are implemented only in canonical mode; noncanonical mode rejects these flags.
10. **Raw-mode read:** `(VMIN>0,VTIME=0)` returns once the minimum count is reached rather than attempting to fill the entire supplied buffer. No general async I/O or nonblocking descriptor integration.
11. **Error handling:** standard `errno` behavior is provided for explicit validation failures, but successful operations generally do not reset `errno`; inspect the return value first.
12. **Portability:** depends on rosco_m68k firmware character-device API and 100 Hz timer; no generic POSIX host compatibility is claimed.
