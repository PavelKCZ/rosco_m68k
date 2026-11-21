#!/usr/bin/env python3
"""Build/run native tests only. Fake machine functions cannot access hardware."""
from pathlib import Path
import os
import subprocess
import sys
import argparse
import hashlib
p=Path(__file__).resolve().parents[1]
r=p.parents[3]
parser=argparse.ArgumentParser()
parser.add_argument('build')
parser.add_argument('--historical-runtime-bin',required=True)
args=parser.parse_args()
fixture=Path(args.historical_runtime_bin).resolve()
if not fixture.is_file() or fixture.stat().st_size!=44980 or hashlib.sha256(fixture.read_bytes()).hexdigest()!='066e667d849623ac682c8153542af1f688209f28c9f09a0bedaafba6943ebeb5':
    parser.exit(2,'BLOCKED: missing or incorrect historical runtime BIN\n')
b=Path(args.build).resolve()/'host';b.mkdir(parents=True,exist_ok=True)
cc=['cc','-std=c11','-D_DEFAULT_SOURCE','-Wall','-Wextra','-Werror','-g','-fPIC',
    '-I'+str(p/'public'),'-I'+str(p/'tests/include'),'-I'+str(p/'include'),'-I'+str(p/'compat'),
    '-iquote',str(r/'code/software/libs/src/termios/include'),
    '-iquote',str(r/'code/software/libs/src/machine/include')]
objects=[]
for name,source,flags in [
    ('memory',p/'memory.c',[]),('syscalls',p/'syscalls.c',[]),
    ('fake',p/'tests/fake_machine.c',[]),('error_probe',p/'tests/error_probe.c',[]),
    ('termios',r/'code/software/libs/src/termios/termios.c',[
        '-Dtcgetattr=m31_termios_tcgetattr','-Dtcsetattr=m31_termios_tcsetattr',
        '-Dtcdrain=m31_termios_tcdrain','-Dtcflush=m31_termios_tcflush'])]:
    obj=b/(name+'.o');cmd=cc+flags+['-c',str(source),'-o',str(obj)]
    print('COMMAND', ' '.join(cmd),flush=True);subprocess.run(cmd,check=True);objects.append(str(obj))
lib=b/'runtime.so'
subprocess.run(['cc','-shared','-o',str(lib),*objects],check=True)
# Native lifecycle harness compiles the production runtime.c via inclusion.
lifecycle=b/'lifecycle'
subprocess.run(cc+['-fno-delete-null-pointer-checks',str(p/'tests/lifecycle.c'),str(p/'memory.c'),'-o',str(lifecycle)],check=True)
bind=b/'bind-probe'
subprocess.run(cc+['-DM31_BIND_PROBE',str(p/'tests/error_probe.c'),
    *[o for o in objects if not o.endswith('/error_probe.o')],'-o',str(bind)],check=True)
# Diagnostic C paths use a native recording primitive, never real TRAP code.
diag_lifecycle=b/'lifecycle-diag'
subprocess.run(cc+['-DM31_DIAGNOSTIC','-fno-delete-null-pointer-checks',
    str(p/'tests/lifecycle.c'),str(p/'memory.c'),'-o',str(diag_lifecycle)],check=True)
diag_sys=b/'syscalls-diag.o'
subprocess.run(cc+['-DM31_DIAGNOSTIC','-c',str(p/'syscalls.c'),'-o',str(diag_sys)],check=True)
diag_record=b/'diagnostic-record.o'
subprocess.run(cc+['-c',str(p/'tests/diagnostic_firmware.c'),'-o',str(diag_record)],check=True)
diag_lib=b/'runtime-diag.so'
subprocess.run(['cc','-shared','-o',str(diag_lib),str(diag_sys),str(diag_record),
    *[o for o in objects if not o.endswith('/syscalls.o')]],check=True)
env=os.environ.copy();env['M31_HISTORICAL_RUNTIME_BIN']=str(fixture);env['M31_TARGET_BUILD']=str(b.parent);env['M31_DIAG_TARGET']=str(b.parent/'diagnostic');env['M31_DIAG_LIFECYCLE']=str(diag_lifecycle);env['M31_DIAG_LIB']=str(diag_lib);env['M31_LIFECYCLE']=str(lifecycle);env['M31_BIND_PROBE']=str(bind);env['M31_HOST_LIB']=str(lib);env['PYTHONDONTWRITEBYTECODE']='1'
cmd=[sys.executable,'-m','unittest','discover','-s',str(p/'tests'),'-v']
print('COMMAND',' '.join(cmd),flush=True)
raise SystemExit(subprocess.run(cmd,env=env).returncode)
