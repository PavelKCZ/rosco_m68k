#!/usr/bin/env python3
"""I1 host object and assembly-SOURCE audit. Never builds/executes target code."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

def need(ok,message):
    if not ok:raise ValueError(message)

def main():
    root=Path(__file__).resolve().parents[1];build=Path(sys.argv[1]).resolve()
    exports={'sd_ro':{'sdro_init','sdro_capacity','sdro_card_type','sdro_context','sdro_read','sdro_diagnostics','sdro_shutdown'},
             'csd_decode':{'sdro_decode_csd','sdro_validate_capacity','sdro_check_lba'}}
    allowed={'memset','memcpy','__stack_chk_fail','sdro_fw_support','sdro_fw_init','sdro_fw_csd','sdro_fw_read','sdro_decode_csd','sdro_check_lba'}
    objects={}
    for stem,names in exports.items():
        obj=build/(stem+'.o')
        defined=subprocess.check_output(['nm','-g','--defined-only',str(obj)],text=True)
        undef=subprocess.check_output(['nm','-u',str(obj)],text=True)
        need({l.split()[-1] for l in defined.splitlines()}==names,'unexpected export set '+stem)
        need({l.split()[-1] for l in undef.splitlines()}<=allowed,'unexpected object imports '+stem)
        deps=subprocess.check_output(['cc','-MM','-I'+str(root/'include'),'-I'+str(root/'src'),str(root/'src'/(stem+'.c'))],text=True)
        for bad in ['fat_ro','libs/src/','librosco','syscalls','sdcard_asm']:
            need(bad not in deps,'legacy/runtime dependency '+bad)
        objects[stem]={'defined':defined.splitlines(),'undefined':undef.splitlines(),'deps':deps,'sha256':hashlib.sha256(obj.read_bytes()).hexdigest()}
    # Exact instruction-source contracts. This does NOT establish emitted opcodes.
    s=(root/'src/sd_ro_traps.S').read_text()
    s=re.sub(r'/\*.*?\*/','',s,flags=re.S)
    names=['sdro_fw_support','sdro_fw_init','sdro_fw_read','sdro_fw_csd']
    bodies={}
    for i,n in enumerate(names):
        body=s.split(n+':',1)[1].split('.section',1)[0]
        bodies[n]=[l.strip() for l in body.splitlines() if l.strip()]
    expected={
      'sdro_fw_support':['moveq #0,%d0','trap #13','rts'],
      'sdro_fw_init':['movem.l %a0-%a1,-(%sp)','move.l 12(%sp),%a1','moveq #1,%d0','trap #13','movem.l (%sp)+,%a0-%a1','rts'],
      'sdro_fw_read':['movem.l %d1/%a0-%a2,-(%sp)','move.l 20(%sp),%a1','move.l 24(%sp),%d1','move.l 28(%sp),%a2','moveq #2,%d0','trap #13','movem.l (%sp)+,%d1/%a0-%a2','rts'],
      'sdro_fw_csd':['movem.l %d1/%a0-%a2,-(%sp)','move.l 20(%sp),%a1','move.l 24(%sp),%a2','moveq #9,%d1','moveq #4,%d0','trap #13','movem.l (%sp)+,%d1/%a0-%a2','rts']}
    need(bodies==expected,'assembly source ABI/selector change')
    need(s.count('trap #13')==4,'unexpected trap entry')
    need(set(re.findall(r'moveq #(\d+),%d0',s))=={'0','1','2','4'},'forbidden selector')
    make=(root/'Makefile').read_text()
    need('CORE := src/sd_ro.c src/csd_decode.c' in make,'host core source list')
    need('TEST := tests/mock_firmware.c tests/test_adapter.c tests/test_csd.c' in make,'host mock source list')
    need(not re.search(r'^\s*\$\(CC\).*sd_ro_traps',make,re.M),'target source compiled by host profile')
    need(not any(t in make for t in ['-lrosco','sdcard.o','sdcard_asm.o','fat_ro.c','syscalls.o']),'forbidden link input')
    report={'result':'PASS','objects':objects,'assembly_source_only':True,'selectors':[0,1,2,4],'register_command':9,
      'target_assembled':False,'hardware_executed':False,'limitations':['Assembly source checked, not assembled, linked or executed.','CRC integrity and active ROM ABI require later validation.']}
    (build/'audit.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':
    try:main()
    except (ValueError,OSError,KeyError,subprocess.SubprocessError) as e:
        print('AUDIT FAIL:',e,file=sys.stderr);raise SystemExit(1)
