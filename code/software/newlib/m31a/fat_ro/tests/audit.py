#!/usr/bin/env python3
"""Host object/link audit only; no filesystem mutation interfaces are accepted."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

def require(ok, message):
    if not ok:
        raise ValueError(message)

def main():
    root=Path(__file__).resolve().parents[1]
    build=Path(sys.argv[1]).resolve()
    obj=build/'fat_ro.o'
    nm=subprocess.check_output(['nm','-g','--defined-only',str(obj)],text=True)
    expected={'b1_init','b1_open','b1_read','b1_seek','b1_info','b1_close','b1_shutdown'}
    require({l.split()[-1] for l in nm.splitlines()}==expected,'unexpected public symbols')
    undefined=subprocess.check_output(['nm','-u',str(obj)],text=True)
    require({l.split()[-1] for l in undefined.splitlines()} <= {'memcpy','memset','memcmp','__stack_chk_fail'},'unexpected external dependency')
    config=subprocess.check_output(['cc','-E','-dM','-I'+str(root/'include'),'-include','fat_opts.h','-'],input='',text=True)
    for name in ['FATFS_INC_WRITE_SUPPORT','FATFS_INC_FORMAT_SUPPORT']:
        require('#define '+name+' 0' in config,'mutation configuration enabled')
    # Verify exact source ownership and dependencies emitted by the host compiler.
    deps=subprocess.check_output(['cc','-MM','-I'+str(root/'include'),str(root/'src/fat_ro.c')],text=True)
    require('reference/' not in deps and 'libs/src/sdfat/' not in deps,'uncontrolled original source dependency')
    require('fat_write' not in deps and 'fat_format' not in deps,'mutation implementation input')
    syms=subprocess.check_output(['nm',str(obj)],text=True)
    for token in ['SD_', 'fatfs_sector_write','fl_fwrite','fl_format','malloc','calloc','realloc','_open_r','_write_r']:
        require(token not in syms,'forbidden symbol '+token)
    dis=subprocess.check_output(['objdump','-dr',str(obj)],text=True)
    require('write' not in dis.lower(),'write path in backend object')
    provenance=json.loads((root/'PROVENANCE.json').read_text())
    for row in provenance['files']:
        require(hashlib.sha256((root/row['reference']).read_bytes()).hexdigest()==row['sha256'],'reference hash mismatch')
        require(hashlib.sha256((root.parents[4]/row['original']).read_bytes()).hexdigest()==row['sha256'],'original changed')
    result={'result':'PASS','public_symbols':sorted(expected),'undefined':undefined.splitlines(),'configuration':'write=0 format=0',
            'sources':['src/fat_ro.c'],'dependencies':deps,'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest(),
            'limitations':['Static host audit, not target or physical write protection.','Dynamic write counter is corroborating evidence: B1 exposes no write callback.']}
    (build/'audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__':
    try: main()
    except (ValueError,OSError,subprocess.SubprocessError,KeyError) as e:
        print('AUDIT FAIL:',e,file=sys.stderr)
        raise SystemExit(1)
