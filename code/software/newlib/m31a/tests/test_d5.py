"""D5 ABI-slot independence, verified on actual target instructions + native C."""
import ctypes as C
import os
from pathlib import Path
import re
import subprocess
import unittest

class Layout(C.Structure):
    _fields_=[(x,C.c_uint32) for x in ('top','stack_bottom','heap_start','heap_end')]

class D5Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib=C.CDLL(os.environ['M31_HOST_LIB'])
        cls.lib.m31_plan.argtypes=[C.c_uint32]*5+[C.POINTER(Layout)]
        cls.build=Path(os.environ['M31_TARGET_BUILD'])
        cls.boot={}
        for profile,rel in [('normal','runtime-smoke.elf'),('diagnostic','diagnostic/runtime-smoke-diag.elf')]:
            cls.boot[profile]=subprocess.check_output(['m68k-elf-rosco-objdump','-d','-j','.boot',str(cls.build/rel)],text=True)
    def test_native_plan_needs_only_memory_inputs(self):
        x=Layout()
        self.assertEqual(self.lib.m31_plan(0x500000,0x500000,0x18000,0x40000,0x10000,C.byref(x)),0)
        self.assertEqual((x.top,x.heap_start,x.heap_end),(0x500000,0x18000,0x4f8000))
    def test_raw_values_cannot_influence_diagnostic_validation(self):
        b=self.boot['diagnostic'].split('<m31_diag_validate>:',1)[1].split('<m31_diag_stop>:',1)[0]
        # D_ENTRY leaves the measured raw slot in d2. No instruction reads it,
        # regardless of whether its bits are boot magic, a pointer, or garbage.
        instructions=[l for l in b.splitlines() if re.match(r'\s*[0-9a-f]+:',l)]
        self.assertTrue(instructions)
        for slot in [0xb105d47a,0x1df8,0,0xffffffff,0xdeadbeef]:
            with self.subTest(raw_slot=slot):
                self.assertFalse(any('%d2' in l for l in instructions))
                self.assertFalse(any(re.search(r'\b400(?:\s|,)',l) for l in instructions))
        self.assertIn('cmpil #524288,%d4',instructions[0])
        self.assertIn('moveq #86,%d0',b) # V still follows safety checks
        self.assertNotIn('m31_diag_reject01',self.boot['diagnostic'])
    def test_normal_never_reads_raw_slot(self):
        self.assertNotRegex(self.boot['normal'],r'\b400(?:\s|,)')
        self.assertNotIn('b105d47a',self.boot['normal'].lower())
    def test_diagnostic_raw_slot_is_one_direct_read_not_pointer_follow(self):
        boot=self.boot['diagnostic']
        lines=[l for l in boot.splitlines() if re.search(r'\b400(?:\s|,)',l)]
        self.assertEqual(len(lines),1)
        self.assertRegex(lines[0],r'movel 400(?: <[^>]+>)?,%d2')
        image=(self.build/'diagnostic/runtime-smoke-diag.bin').read_bytes()
        self.assertIn(b' ABI_SLOT=\0',image)
        self.assertNotIn(b' MAGIC=',image)
    def test_c_runtime_and_planner_do_not_read_slot_or_compare_magic(self):
        for profile,rel in [('normal','runtime-smoke.elf'),('diagnostic','diagnostic/runtime-smoke-diag.elf')]:
            for name in ['m31_runtime_start','m31_plan']:
                with self.subTest(profile=profile,function=name):
                    d=subprocess.check_output(['m68k-elf-rosco-objdump','-d','--disassemble='+name,str(self.build/rel)],text=True)
                    self.assertIn('<'+name+'>:',d)
                    self.assertNotRegex(d,r'\b400(?:\s|,)')
                    self.assertNotIn('b105d47a',d.lower())
