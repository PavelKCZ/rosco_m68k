#!/usr/bin/env python3
"""Explicit host-only validation; all generated evidence below a fresh /tmp path."""
import sys
sys.dont_write_bytecode=True
from pathlib import Path
import subprocess,json,os
HERE=Path(__file__).resolve().parents[1];R=HERE.parent
out=Path(sys.argv[1]).resolve()
if Path('/tmp') not in out.parents or out.exists():raise SystemExit('require NEW /tmp host directory')
out.mkdir(parents=True)
commands=[]
def run(name,args,env=None):
 p=subprocess.run(list(map(str,args)),capture_output=True,text=True,env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',**(env or {})))
 (out/(name+'.log')).write_text(p.stdout+p.stderr)
 commands.append({'name':name,'argv':list(map(str,args)),'exit':p.returncode})
 (out/'commands.json').write_text(json.dumps(commands,indent=2))
 if p.returncode:raise SystemExit('STOP '+name+' exit '+str(p.returncode))
 return p.stdout
flags=['-std=c11','-g','-Wall','-Wextra','-Werror','-Wpedantic']
inc=sum((['-I'+str(R/p)] for p in ['include','sd_ro/include','sd_ro/src','sd_ro/tests','fat_ro/include','b3_integration/include']),[])
diag=['-DSDRO_CSD_DIAGNOSTIC'];san=['-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
core=[R/'sd_ro/src/sd_ro.c',R/'sd_ro/src/csd_decode.c']
extra=[HERE/'tests/test_diagnostics.c',R/'sd_ro/tests/mock_firmware.c',core[0],R/'fat_ro/src/fat_ro.c',R/'b3_integration/src/b3_integration.c']
results={}
for mode,opts in [('normal',['-O2']),('sanitized',san)]:
 env={'ASAN_OPTIONS':'detect_leaks=0:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'} if mode=='sanitized' else {}
 for variant,defs in [('off',[]),('on',diag)]:
  name='diagnostics-'+variant+'-'+mode
  run(name+'-compile',['cc',*flags,*inc,*defs,*opts,*extra,'-o',out/name])
  results[name]=run(name,[out/name],env)
 for variant,defs in [('off',[]),('on',diag)]:
  name='b2-'+variant+'-'+mode
  run(name+'-compile',['cc',*flags,*inc,*defs,*opts,*core,R/'sd_ro/tests/mock_firmware.c',R/'sd_ro/tests/test_adapter.c',R/'sd_ro/tests/test_csd.c','-o',out/name])
  results[name]=run(name,[out/name],env)
 for variant,defs in [('off',[]),('on',diag)]:
  name='b3-'+variant+'-'+mode
  src=[R/'fat_ro/src/fat_ro.c',*core,R/'b3_integration/src/b3_integration.c',R/'b3_integration/tests/mock_firmware.c',R/'b3_integration/tests/fat_fixture.c',R/'b3_integration/tests/test_integration.c']
  run(name+'-compile',['cc',*flags,*inc,'-I'+str(R/'b3_integration/tests'),*defs,*opts,*src,'-o',out/name])
  results[name]=run(name,[out/name],env)
 for variant,defs in [('off',[]),('on',diag)]:
  name='application-'+variant+'-'+mode
  src=R/'b3_executable/tests/test_smoke.c' if variant=='off' else HERE/'tests/test_application.c'
  run(name+'-compile',['cc',*flags,*inc,'-I'+str(R/'public'),'-I'+str(R/'compat'),'-DM31_DIAGNOSTIC',*defs,*opts,src,'-o',out/name])
  results[name]=run(name,[out/name],env)
for mode in ['normal','sanitized']:
 a=[x for x in results['diagnostics-off-'+mode].splitlines() if x.startswith('COMPAT ')]
 b=[x for x in results['diagnostics-on-'+mode].splitlines() if x.startswith('COMPAT ') and x.split()[1] in {y.split()[1] for y in a}]
 if a!=b:raise SystemExit('diagnostic on/off status/call sequence divergence')
summary={k:[x for x in v.splitlines() if x.startswith(('SUMMARY','LAYOUT'))] for k,v in results.items()}
(out/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
