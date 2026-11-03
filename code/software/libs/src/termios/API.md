# Public API and behavior

Header: `src/termios/include/termios.h`.

- `rosco_termios_bind(int fd, uint8_t firmware_device_index)`: bind one of four software handles (`0..3`) to a firmware character device; initializes defaults.
- `rosco_termios_bind_device(int fd, const void *char_device)`: bind a handle to a supplied firmware `CharDevice` structure, copied internally; caller must supply a valid structure.
- `rosco_termios_unbind(int fd)`: release a handle.
- `tcgetattr`, `tcsetattr`: read/update software terminal settings. Only `TCSANOW` is accepted. `tcsetattr` rejects unsupported flags, framing and speed.
- `cfgetispeed`, `cfgetospeed`, `cfsetispeed`, `cfsetospeed`: read or set speed fields in the struct. The setters do not configure hardware; `tcsetattr` enforces `B38400`.
- `cfmakeraw`: change the supplied settings structure to a raw-style configuration; apply using `tcsetattr`.
- `rosco_termios_read`, `rosco_termios_write`: firmware-device I/O with software terminal processing. Return byte count on success, `-1` on error. These are **not** Unix `read`/`write` functions.
- `tcflush(fd, TCIFLUSH)`: discard internal canonical line and drain currently available firmware RX data on a best-effort basis. Other flush selectors are unsupported.
- `tcdrain`: unsupported; returns `-1` with `ENOTSUP` where available.

## Defaults on bind

`ICRNL`; `OPOST|ONLCR`; `CREAD|CS8|CLOCAL`; `ICANON|ECHO|ECHOE|ECHOK|IEXTEN`; erase=DEL, kill=Ctrl+U, EOF=Ctrl+D; `VMIN=1`, `VTIME=0`; speeds `B38400`.

## Input

Canonical mode: CR/LF translation according to accepted input flags, line editing with VERASE and VKILL, VEOF, and echo. Canonical input is buffered to a fixed 256-byte line array, with 255 ordinary bytes reserved before LF; excess ordinary input is discarded. A completed line can be returned over multiple reads.

Noncanonical mode: `(VMIN,VTIME)=(0,0)` polling; `(0,>0)` total timeout; `(>0,0)` blocking minimum; `(>0,>0)` inter-byte timeout after first byte. `VTIME` units are tenths of a second based on firmware 100 Hz ticks.

Output: `OPOST|ONLCR` converts LF to CR+LF. `rosco_termios_write` returns count after issuing firmware sends; it does not guarantee hardware TX completion.
