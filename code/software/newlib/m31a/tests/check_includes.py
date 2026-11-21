"""Inspect compiler-produced dependencies; no preprocessing assumptions."""
from pathlib import Path
import json
import subprocess
import sys
b, root = map(lambda p: Path(p).resolve(), sys.argv[1:])
dep = (b/'angle_include.d').read_text().replace('\\\n',' ')
project = root/'code/software/libs/src/termios/include/termios.h'
libdir = Path(subprocess.check_output(['m68k-elf-rosco-gcc','-print-file-name=libc.a'],text=True).strip()).parent
inc = libdir.parent/'include'
required = [project, inc/'stdio.h', inc/'stdlib.h', inc/'errno.h', inc/'reent.h']
# The forwarding include may occur with '..'; resolve each emitted dependency.
tokens = dep.split(':',1)[1].split()
selected = {str(Path(t).resolve()) for t in tokens if t != ':' and not t.endswith(':')}
missing = [str(p.resolve()) for p in required if str(p.resolve()) not in selected]
if missing or str((inc/'termios.h').resolve()) in selected:
    raise SystemExit('include selection FAIL: '+repr(missing))
result={'result':'PASS','selected_headers':[str(p.resolve()) for p in required]}
(b/'include-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
