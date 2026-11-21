"""A.1 tests: native C lifecycle, actual target evidence rejection, dependency graph."""
import ctypes as C
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from test_runtime import lib, Reent

P=Path(__file__).resolve().parents[1]
TARGET=Path(os.environ['M31_TARGET_BUILD']).resolve()
spec=importlib.util.spec_from_file_location('m31_audit',P/'audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

class LifecycleTests(unittest.TestCase):
    def scenario(self,mode):
        r=subprocess.run([os.environ['M31_LIFECYCLE'],str(mode)],text=True,capture_output=True)
        self.assertEqual(r.returncode,0,r.stderr)
        result=json.loads(r.stdout)
        with (TARGET/'lifecycle-scenarios.jsonl').open('a') as f:f.write(json.dumps({'mode':mode,**result})+'\n')
        return result
    def test_full_c_lifecycle_order(self):
        r=self.scenario(0)
        self.assertEqual(r['status'],17)
        self.assertEqual(r['events'].split(','),['heap','console','register','arrays-init',
            'ctor-high','ctor-low','main','register','main-return','exit','app-atexit',
            'dtor-low','dtor-high','arrays-fini','low-exit'])
    def test_console_failure_aborts_before_constructors(self):
        self.assertEqual(self.scenario(1),{'status':101,'events':'heap,console,low-exit'})
    def test_atexit_registration_failure(self):
        self.assertEqual(self.scenario(2),{'status':102,'events':'heap,console,register,low-exit'})
    def test_real_console_bind_failure_and_retry(self):
        r=subprocess.run([os.environ['M31_BIND_PROBE']],capture_output=True)
        self.assertEqual(r.returncode,0,r.stderr);self.assertEqual(r.stdout,b'')
    def test_fstat_console_metadata_and_errors(self):
        lib.fake_reset();self.assertEqual(lib.rosco_termios_bind(0,0),0)
        self.assertEqual(lib.m31_console_init(),0);self.assertEqual(lib.m31_probe_fstat(),0)
    def test_unsupported_mutations(self):
        import errno
        r=Reent(0)
        self.assertEqual(lib._unlink_r(C.byref(r),b'/sd/a'),-1);self.assertEqual(r.error,errno.ENOSYS)
        for name in ['_link_r','_rename_r']:
            self.assertEqual(getattr(lib,name)(C.byref(r),b'/sd/a',b'/sd/b'),-1)
            self.assertEqual(r.error,errno.ENOSYS)

class AuditTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='audit-case-',dir=TARGET.parent)
        self.b=Path(self.tmp.name)
        for name in ['runtime-smoke.elf','runtime-smoke.bin','runtime-smoke.map',
                     'startup.o','syscalls.o','runtime.o','termios.o','machine.o']:
            shutil.copy2(TARGET/name,self.b/name)
        m=(self.b/'runtime-smoke.map').read_text().replace(str(TARGET),str(self.b))
        (self.b/'runtime-smoke.map').write_text(m)
    def tearDown(self):self.tmp.cleanup()
    def reject(self,text):
        with self.assertRaises(audit.AuditError) as cm:audit.audit(self.b)
        self.assertIn(text,str(cm.exception))
    def change_ph(self,typ,field,value):
        p=self.b/'runtime-smoke.elf';raw=bytearray(p.read_bytes())
        phoff=struct.unpack_from('>I',raw,28)[0];size,count=struct.unpack_from('>HH',raw,42)
        for n in range(count):
            off=phoff+n*size
            if struct.unpack_from('>I',raw,off)[0]==typ:
                struct.pack_into('>I',raw,off+field*4,value);p.write_bytes(raw);return
        self.fail('program header not found')
    def test_valid_audit_under_optimized_python(self):
        r=subprocess.run([sys.executable,'-O',str(P/'audit.py'),str(self.b)],text=True,capture_output=True)
        self.assertEqual(r.returncode,0,r.stdout+r.stderr)
        self.assertEqual(json.loads((self.b/'audit.json').read_text())['result'],'PASS')
    def test_rwx_load_rejected(self):
        self.change_ph(1,6,7);self.reject('LOAD permissions')
    def test_executable_stack_rejected(self):
        self.change_ph(0x6474e551,6,7);self.reject('GNU_STACK')
    def test_wrong_load_address(self):
        self.change_ph(1,3,0x40004);self.reject('load/runtime address')
    def test_wrong_map_owner(self):
        p=self.b/'runtime-smoke.map'
        p.write_text(re.sub(r'^(_sbrk_r\s+).*$',r'\g<1>'+str(self.b/'runtime.o'),p.read_text(),flags=re.M))
        self.reject('wrong map owner: _sbrk_r')
    def test_forbidden_fallback(self):
        p=self.b/'runtime-smoke.map';p.write_text(p.read_text()+'\nLOAD /tmp/libnosys.a\n')
        self.reject('forbidden map input')
    def test_missing_map_table(self):
        p=self.b/'runtime-smoke.map';p.write_text(p.read_text().split('Cross Reference Table')[0])
        self.reject('cross-reference table')
    def test_corrupt_bin(self):
        p=self.b/'runtime-smoke.bin';raw=bytearray(p.read_bytes());raw[0]^=1;p.write_bytes(raw)
        self.reject('ELF/BIN bytes differ')
    def test_missing_evidence_fails_closed_under_optimization(self):
        (self.b/'runtime-smoke.bin').unlink()
        (self.b/'audit.json').write_text('{"result":"PASS"}')
        r=subprocess.run([sys.executable,'-O',str(P/'audit.py'),str(self.b)],text=True,capture_output=True)
        self.assertEqual(r.returncode,2);self.assertEqual(json.loads((self.b/'audit.json').read_text())['result'],'FAIL')
    def test_truncated_elf_fails_closed(self):
        (self.b/'runtime-smoke.elf').write_bytes(b'\x7fELF')
        r=subprocess.run([sys.executable,str(P/'audit.py'),str(self.b)],capture_output=True)
        self.assertEqual(r.returncode,2);self.assertEqual(json.loads((self.b/'audit.json').read_text())['result'],'FAIL')
    def test_unresolved_symbol(self):
        p=self.b/'runtime-smoke.elf';raw=bytearray(p.read_bytes())
        shoff=struct.unpack_from('>I',raw,32)[0];sz,num=struct.unpack_from('>HH',raw,46)
        headers=[struct.unpack_from('>10I',raw,shoff+n*sz) for n in range(num)]
        found=False
        for h in headers:
            if h[1]!=2:continue
            strings=headers[h[6]];table=raw[strings[4]:strings[4]+strings[5]]
            for off in range(h[4],h[4]+h[5],h[9]):
                name=struct.unpack_from('>I',raw,off)[0]
                if table[name:table.find(b'\0',name)]==b'_write_r':
                    struct.pack_into('>I',raw,off+4,0);struct.pack_into('>H',raw,off+14,0);found=True
        self.assertTrue(found);p.write_bytes(raw);self.reject('unresolved ELF symbols')
    def test_bad_section_alignment(self):
        p=self.b/'runtime-smoke.elf';raw=bytearray(p.read_bytes())
        shoff=struct.unpack_from('>I',raw,32)[0];sz,num=struct.unpack_from('>HH',raw,46)
        for n in range(1,num):
            off=shoff+n*sz;flags=struct.unpack_from('>I',raw,off+8)[0]
            if flags&2:
                struct.pack_into('>I',raw,off+32,3);break
        p.write_bytes(raw)
        r=subprocess.run([sys.executable,str(P/'audit.py'),str(self.b)],capture_output=True)
        self.assertEqual(r.returncode,2);self.assertEqual(json.loads((self.b/'audit.json').read_text())['result'],'FAIL')

class DependencyTests(unittest.TestCase):
    def scheduled(self,header):
        # -W matches make's dependency spelling, not canonical inode identity.
        # Simulate a real mtime change for EVERY alias emitted by the compiler.
        aliases=set()
        for dep in TARGET.glob('*.d'):
            first=dep.read_text().replace('\\\n',' ').splitlines()[0]
            for name in first.split(':',1)[1].split():
                path=Path(name) if Path(name).is_absolute() else P/name
                if path.resolve()==Path(header).resolve():aliases.add(name)
        self.assertTrue(aliases,'header missing from generated dependencies')
        cmd=['make','-n','BUILD='+str(TARGET),'all']
        for alias in sorted(aliases):cmd.extend(['-W',alias])
        r=subprocess.run(cmd,cwd=P,text=True,capture_output=True)
        self.assertEqual(r.returncode,0,r.stderr)
        with (TARGET/'dependency-audit.jsonl').open('a') as f:
            f.write(json.dumps({'header':str(header),'command':cmd,'planned_rebuild':r.stdout})+'\n')
        return r.stdout
    def test_public_include_selection(self):
        record=json.loads((TARGET/'include-check.json').read_text())
        self.assertEqual(record['result'],'PASS')
        self.assertTrue(any(x.endswith('/libs/src/termios/include/termios.h') for x in record['selected_headers']))
        self.assertTrue(any('/m68k-elf-rosco/include/stdio.h' in x for x in record['selected_headers']))
    def test_termios_header_triggers_recompile(self):
        text=self.scheduled(P.parents[3]/'code/software/libs/src/termios/include/termios.h')
        for name in ['termios.o','syscalls.o','angle_include.o']:self.assertIn('-o '+str(TARGET/name),text)
    def test_machine_header_triggers_recompile(self):
        text=self.scheduled(P.parents[3]/'code/software/libs/src/machine/include/machine.h')
        self.assertIn('-o '+str(TARGET/'termios.o'),text)
    def test_runtime_header_triggers_recompile(self):
        self.assertIn('-o '+str(TARGET/'runtime.o'),self.scheduled(P/'include/runtime.h'))
    def test_newlib_header_triggers_recompile(self):
        records=json.loads((TARGET/'include-check.json').read_text())['selected_headers']
        header=next(x for x in records if x.endswith('/reent.h'))
        text=self.scheduled(header)
        for name in ['syscalls.o','smoke.o']:self.assertIn('-o '+str(TARGET/name),text)
