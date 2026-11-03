# Hardware validation report — libtermios 1.0

## Environment

Real rosco_m68k hardware, firmware UART at 38400 baud, 8N1; Windows COM6, Python 3.14, pyserial, Tera Term disconnected during automation. Firmware tick clock: 100 Hz.

## Results reported by operator

- Development revisions R1–R18: **73/73** planned hardware scenarios PASS, including canonical editing, VMIN/VTIME edges, integrity transfers, flush and recovery, long-running transfers, and mode switching.
- Release candidate RC1: **5/5** final test groups PASS. Combined tally: **78/78** groups/scenarios in the project's established counting convention.

| Test | Outcome | Key observations |
| --- | --- | --- |
| RC1.1 | PASS | Four canonical parts; counts 4,3,2,0; errors=0, errno=0 |
| RC1.2 | PASS | Four VMIN/VTIME cases; zero-data timeout 100 ticks; 16-byte cases error-free |
| RC1.3 | PASS | Unsupported drain/flush rejected; settings unchanged; 32 bytes, errors=0, ticks=8 |
| RC1.4 | PASS | 16 × 64-byte frames = 1024 bytes; alternating modes; errors=0; errno=0; 16–17 ticks per frame |
| RC1.5 | PASS | Input/output reported 38400; unsupported speeds rejected; unchanged settings; 32 bytes, errors=0, ticks=8 |

These results are transcribed from user-provided hardware test logs in the conversation. This package does **not** contain a machine-readable archive of every historical R1–R18 log. Successful testing is not a proof of full POSIX compliance or absence of defects.
