"""D2: native instrumentation paths + static target contracts, never target execution."""
import ctypes as C
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import unittest

P=Path(__file__).resolve().parents[1]
NORMAL=Path(os.environ['M31_TARGET_BUILD'])
DIAG=Path(os.environ['M31_DIAG_TARGET'])
spec=importlib.util.spec_from_file_location('diagnostic_audit',P/'audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

class DiagnosticTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.elf=DIAG/'runtime-smoke-diag.elf'
        cls.dis=subprocess.check_output(['m68k-elf-rosco-objdump','-d',str(cls.elf)],text=True)
        cls.nm=subprocess.check_output(['m68k-elf-rosco-nm','-n',str(cls.elf)],text=True)
        cls.symbols={x[2]:int(x[0],16) for l in cls.nm.splitlines() if len(x:=l.split())==3}
    def block(self,name):
        m=re.search(r'^[0-9a-f]+ <'+re.escape(name)+r'>:\n.*?(?=\n[0-9a-f]+ <|\Z)',self.dis,re.M|re.S)
        self.assertIsNotNone(m,name);return m.group(0)
    def lifecycle(self,mode):
        r=subprocess.run([os.environ['M31_DIAG_LIFECYCLE'],str(mode)],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        result=json.loads(r.stdout)
        with (DIAG/'host-checkpoints.jsonl').open('a') as f:f.write(json.dumps({'mode':mode,**result})+'\n')
        return result
    def test_default_profile_is_not_diagnostic(self):
        r=subprocess.run(['make','-n','BUILD='+str(NORMAL.parent/'unused-normal'),'all'],cwd=P,text=True,capture_output=True)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertNotIn('-DM31_DIAGNOSTIC',r.stdout);self.assertNotIn('runtime-smoke-diag',r.stdout)
    def test_original_reference_bin_is_preserved(self):
        value=os.environ.get('M31_HISTORICAL_RUNTIME_BIN')
        self.assertTrue(value, 'BLOCKED: M31_HISTORICAL_RUNTIME_BIN required')
        fixture=Path(value)
        self.assertTrue(fixture.is_file(), 'BLOCKED: historical runtime BIN missing')
        self.assertEqual(fixture.stat().st_size,44980)
        self.assertEqual(hashlib.sha256(fixture.read_bytes()).hexdigest(),
            '066e667d849623ac682c8153542af1f688209f28c9f09a0bedaafba6943ebeb5')
    def test_normal_has_no_diagnostic_symbols(self):
        nm=subprocess.check_output(['m68k-elf-rosco-nm',str(NORMAL/'runtime-smoke.elf')],text=True)
        self.assertNotIn('m31_diag_',nm)
    def test_both_profiles_pass_strict_audit(self):
        self.assertEqual(audit.audit(NORMAL)['profile'],'normal')
        self.assertEqual(audit.audit(DIAG,True)['profile'],'diagnostic')
    def test_diagnostic_cannot_use_normal_audit_exception(self):
        # Explicit selection is necessary; default audit must not accept early stack calls.
        with self.assertRaises((audit.AuditError,OSError)):
            audit.audit(DIAG)
    def test_firmware_putchar_is_exact_preserving_primitive(self):
        image=(DIAG/'runtime-smoke-diag.bin').read_bytes()
        off=self.symbols['m31_diag_putchar']-0x2000
        self.assertEqual(image[off:off+18],bytes.fromhex('48e7c0c040e772044e4e44df4cdf03034e75'))
        # Includes a restore to CCR, then flag-neutral MOVEM/RTS.
    def test_entry_snapshots_and_original_stack_frame(self):
        b=self.block('_start')
        self.assertIn('moveml %d0-%fp,%sp@-',b);self.assertIn('movew %sr,%sp@-',b)
        self.assertIn('lea %sp@(62),%a0',b)
        self.assertLess(b.index('m31_diag_putchar'),b.index('movel 400'))
        for x in ['movel 400','movel 414','movel 0 <','%sp@(10)','%sp@(18)','%sp@(22)','movew %sp@+,%ccr','moveml %sp@+,%d0-%fp']:
            self.assertIn(x,b)
    def test_pre_relocation_helpers_do_not_call_libc(self):
        boot=subprocess.check_output(['m68k-elf-rosco-objdump','-d','-j','.boot',str(self.elf)],text=True)
        self.assertNotIn('jsr',boot)
        for line in boot.splitlines():
            if 'bsr' in line:self.assertIn('<m31_diag_',line)
        for n in ['m31_diag_mark','m31_diag_error','m31_diag_hex8','m31_diag_putchar']:
            self.assertLess(self.symbols[n],0x2000+self.symbols['_boot_size'])
    def test_rejection_code_mapping(self):
        validation=self.block('m31_diag_validate')
        for number,count in [(2,2),(3,2),(4,1),(5,2),(6,2),(7,2)]:
            self.assertEqual(validation.count('<m31_diag_reject%02d>'%number),count)
            b=self.block('m31_diag_reject%02d'%number)
            self.assertIn('moveq #48,%d0',b)
            self.assertIn('moveq #%d,%%d0'%(48+number),b)
            self.assertIn('m31_diag_stop',b)
    def test_assembly_marker_order_and_preservation(self):
        validation=self.block('m31_diag_validate')
        self.assertLess(validation.index('moveq #86,%d0'),validation.index('moveal %d4,%sp'))
        self.assertLess(validation.index('moveal %d4,%sp'),validation.index('moveq #83,%d0'))
        post=self.block('_relocated')
        self.assertLess(post.index('moveq #82,%d0'),post.index('moveb %a0@+,%a1@+'))
        self.assertLess(post.index('clrb %a0@+'),post.index('moveq #90,%d0'))
        mark=self.block('m31_diag_mark')
        for text in ['moveml %d0-%fp,%sp@-','movew %sr,%sp@-','%sp@(66)','movew %sp@+,%ccr','moveml %sp@+,%d0-%fp']:
            self.assertIn(text,mark)
    def test_diagnostic_c_lifecycle_order(self):
        r=self.lifecycle(0);self.assertEqual(r['status'],17)
        self.assertEqual(r['events'].split(','),['heap','diag-P','diag-B','console','diag-T','register','diag-A',
            'arrays-init','diag-I','ctor-high','ctor-low','diag-K','main','register','main-return','exit',
            'app-atexit','dtor-low','dtor-high','arrays-fini','low-exit'])
    def test_diagnostic_console_error_is_reported_before_exit(self):
        self.assertEqual(self.lifecycle(1),{'status':101,'events':'heap,diag-P,diag-B,console,error-101,low-exit'})
    def test_diagnostic_atexit_error_is_reported_before_exit(self):
        self.assertEqual(self.lifecycle(2),{'status':102,'events':'heap,diag-P,diag-B,console,diag-T,register,error-102,low-exit'})
    def test_first_write_marker_once_without_stdio_recursion(self):
        lib=C.CDLL(os.environ['M31_DIAG_LIB'],use_errno=True)
        class R(C.Structure):_fields_=[('error',C.c_int)]
        lib._write_r.argtypes=[C.POINTER(R),C.c_int,C.c_void_p,C.c_size_t]
        lib._write_r.restype=C.c_ssize_t
        lib.fake_reset();self.assertEqual(lib.m31_console_init(),0);r=R(0)
        self.assertEqual(lib._write_r(C.byref(r),1,b'A',1),1)
        self.assertEqual(lib._write_r(C.byref(r),2,b'B',1),1)
        self.assertEqual(lib.m31_fake_mark_count(),1);self.assertEqual(lib.m31_fake_mark_at(0),ord('W'))
        self.assertEqual(lib.m31_fake_tx_at(0),0)
        lib.fake_tx.restype=C.c_void_p
        self.assertEqual(C.string_at(lib.fake_tx(),lib.fake_tx_count()),b'AB')
    def test_main_and_write_target_checkpoint_order(self):
        main=self.block('main')
        self.assertLess(main.index('pea 4d'),main.index('<printf>'))  # M
        self.assertLess(main.index('pea 4f'),main.index('<printf>'))  # O
        self.assertGreater(main.index('pea 46'),main.index('<printf>'))  # F
        write=self.block('_write_r')
        # GCC places the first-write branch AFTER the common TX block. Follow
        # the actual branch target instead of assuming textual block ordering.
        ins=[]
        for line in write.splitlines():
            m=re.match(r'\s*([0-9a-f]+):\s+(?:[0-9a-f]{4}\s+)+(.*)',line)
            if m:ins.append((int(m[1],16),m[2]))
        i=next(i for i,x in enumerate(ins) if 'tstl' in x[1] and '<diagnostic_write_seen>' in x[1])
        branch=re.search(r'beq\w* ([0-9a-f]+)',ins[i+1][1]);self.assertIsNotNone(branch)
        target=int(branch[1],16)
        call=next(a for a,t in ins if '<m31_diag_mark>' in t)
        tx=next(a for a,t in ins if '<rosco_termios_write>' in t)
        self.assertLessEqual(target,call)
        back=next(t for a,t in ins if a>call and re.search(r'bra\w* ',t))
        resume=int(re.search(r'bra\w* ([0-9a-f]+)',back)[1],16)
        self.assertLess(resume,tx);self.assertLess(tx,target)
        self.assertNotIn('<printf>',write)
