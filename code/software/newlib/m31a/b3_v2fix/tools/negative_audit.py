#!/usr/bin/env python3
"""Future mutations of copies of a VALID executable. NOT RUN in T2-I1.

Never mutates repository sources or the positive build. Requires a positive
machine-only audit, NOT fabricated manual acceptance. Every negative variant
uses the full default audit and must FAIL for its specific reason; CONDITIONAL
is never counted as a successful negative rejection.
"""
import sys
sys.dont_write_bytecode = True
import hashlib
import json
import re
import shutil
import struct
import subprocess
from pathlib import Path
from profile import HERE, PROGRAM
from build import external

def headers(raw):
    h = struct.unpack_from('>HHIIIIIHHHHHH', raw, 16)
    sh = [struct.unpack_from('>10I',raw,h[5]+i*h[10]) for i in range(h[11])]
    return h, sh

def symbol(raw, name):
    h, sh = headers(raw)
    for sec in sh:
        if sec[1] != 2: continue
        strings = sh[sec[6]]; names = raw[strings[4]:strings[4]+strings[5]]
        for offset in range(sec[4],sec[4]+sec[5],16):
            n, value, size, info, other, index = struct.unpack_from('>IIIBBH',raw,offset)
            if names[n:].split(b'\0',1)[0].decode() == name:
                return offset, value
    raise ValueError('negative fixture missing symbol '+name)

def file_offset(raw, address):
    _, sh = headers(raw)
    for sec in sh:
        if sec[1] != 8 and sec[2]&2 and sec[3] <= address < sec[3]+sec[5]:
            return sec[4]+address-sec[3]
    raise ValueError('negative fixture address outside image')

def main():
    if len(sys.argv) != 2: raise ValueError('usage: negative_audit.py BUILD')
    build = external(sys.argv[1]); audit = HERE/'tools/audit.py'
    def invoke(p, machine_only=False):
        argv = [sys.executable,str(audit),str(p)] + (['--machine-only'] if machine_only else [])
        done = subprocess.run(argv,text=True,capture_output=True)
        return done.returncode,json.loads(done.stdout)
    code, positive = invoke(build, True)
    if code or positive['result'] != 'MACHINE_PASS': raise ValueError('positive machine audit must PASS first')
    # A machine-only positive control is not a fabricated manual acceptance.
    # Every mutated case still invokes the FULL default audit, including manual
    # checks. A mere CONDITIONAL/manual warning never counts as rejection PASS.
    root = build/'negative-cases'
    if root.exists(): raise ValueError('negative output exists: use a fresh build directory')
    root.mkdir()
    results = []
    for case in json.loads((HERE/'tools/negative_cases.json').read_text()):
        p = root/case['name']
        shutil.copytree(build,p,ignore=shutil.ignore_patterns('negative-cases'))
        elf = p/(PROGRAM+'.elf'); raw = bytearray(elf.read_bytes()); h, _ = headers(raw)
        image = p/(PROGRAM+'.bin'); record_path = p/'build-record.json'
        record = json.loads(record_path.read_text()); mutation = case['mutation']
        if mutation == 'owner':
            path = p/(PROGRAM+'.map'); text = path.read_text()
            prefix, cross = text.split('Cross Reference Table',1)
            cross, count = re.subn(r'(?m)^(main\s+)\S+\s*$', r'\1/invalid/provider.o', cross, count=1)
            if count != 1: raise ValueError('negative main owner fixture missing')
            path.write_text(prefix+'Cross Reference Table'+cross)
        elif mutation == 'duplicate':
            shutil.copyfile(p/'syscalls.o',p/'duplicate_syscalls.o')
            record['objects']['duplicate_syscalls.o'] = record['objects']['syscalls.o']
            record_path.write_text(json.dumps(record))
        elif mutation == 'legacy':
            path = p/(PROGRAM+'.map'); path.write_text(path.read_text()+'\nLOAD /legacy/fat_filelib.o\n')
        elif mutation in ('selector','odd','image','unresolved'):
            name = {'selector':'sdro_fw_read','odd':'sdro_fw_read','image':'_image_size','unresolved':'main'}[mutation]
            offset,value = symbol(raw,name)
            if mutation == 'selector':
                index = file_offset(raw,value)+16
                if raw[index:index+2] != bytes.fromhex('7002'): raise ValueError('negative selector fixture changed')
                raw[index+1] = 3
                b = bytearray(image.read_bytes()); b[value-0x2000+17] = 3; image.write_bytes(b)
            elif mutation == 'odd': struct.pack_into('>I',raw,offset+4,value+1)
            elif mutation == 'image': struct.pack_into('>I',raw,offset+4,value+4)
            else:
                struct.pack_into('>I',raw,offset+4,0); struct.pack_into('>H',raw,offset+14,0)
            elf.write_bytes(raw)
        elif mutation in ('delta','rwx','stack'):
            done = False
            for i in range(h[9]):
                off = h[4]+i*h[8]; ph = struct.unpack_from('>8I',raw,off)
                if mutation == 'delta' and ph[0] == 1:
                    struct.pack_into('>I',raw,off+12,ph[3]+4); done = True
                elif mutation == 'rwx' and ph[0] == 1 and ph[6] == 5:
                    struct.pack_into('>I',raw,off+24,7); done = True
                elif mutation == 'stack' and ph[0] == 0x6474e551:
                    struct.pack_into('>I',raw,off+24,7); done = True
                if done: break
            if not done: raise ValueError('negative program header fixture absent')
            elf.write_bytes(raw)
        elif mutation == 'bin':
            b = bytearray(image.read_bytes()); b[0] ^= 1; image.write_bytes(b)
        elif mutation == 'missing_hash':
            del record['objects']['application.o']['sha256']; record_path.write_text(json.dumps(record))
        elif mutation == 'bad_hash':
            path = p/'application.o'; b = bytearray(path.read_bytes()); b[0] ^= 1; path.write_bytes(b)
        elif mutation == 'manual': (p/'manual-review.json').unlink()
        else: raise ValueError('unknown negative mutation')
        code,result = invoke(p)
        accepted = code != 0 and result['result'] == 'FAIL' and case['expected'] in result.get('error','')
        results.append({'case':case['name'],'rejected_as_expected':accepted,'result':result})
        if not accepted: raise ValueError('negative audit failure: '+json.dumps(results[-1]))
    print(json.dumps({'result':'PASS','unique_negative_cases':len(results),'cases':results},indent=2))

if __name__ == '__main__':
    try: main()
    except Exception as exc:
        print(json.dumps({'result':'FAIL','error':str(exc)}));sys.exit(2)
