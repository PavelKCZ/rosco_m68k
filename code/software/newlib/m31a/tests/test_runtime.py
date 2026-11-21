import ctypes as C
import errno
import os
import unittest

class Layout(C.Structure):
    _fields_ = [(x, C.c_uint32) for x in ('top','stack_bottom','heap_start','heap_end')]
class Heap(C.Structure):
    _fields_ = [(x, C.c_uint32) for x in ('start','end','current')]
class Reent(C.Structure):
    _fields_ = [('error',C.c_int)]
class Termios(C.Structure):
    _fields_ = [('iflag',C.c_ulong),('oflag',C.c_ulong),('cflag',C.c_ulong),('lflag',C.c_ulong),
                ('cc',C.c_ubyte*8),('ispeed',C.c_ulong),('ospeed',C.c_ulong)]
lib=C.CDLL(os.environ['M31_HOST_LIB'],use_errno=True)
lib.m31_plan.argtypes=[C.c_uint32]*5+[C.POINTER(Layout)]
lib.m31_heap_move.argtypes=[C.POINTER(Heap),C.c_int64,C.POINTER(C.c_uint32)]
lib._sbrk_r.argtypes=[C.POINTER(Reent),C.c_ssize_t];lib._sbrk_r.restype=C.c_void_p
lib._read_r.argtypes=[C.POINTER(Reent),C.c_int,C.c_void_p,C.c_size_t];lib._read_r.restype=C.c_ssize_t
lib._write_r.argtypes=lib._read_r.argtypes;lib._write_r.restype=C.c_ssize_t
lib._lseek_r.restype=C.c_long
lib.fake_tx.restype=C.c_void_p

class MemoryTests(unittest.TestCase):
    def plan(self,ram=0x500000,reset=0x500000,end=0x18000,load=0x40000,size=0x10000):
        out=Layout(1,2,3,4)
        rc=lib.m31_plan(ram,reset,end,load,size,C.byref(out))
        if rc: self.assertEqual(tuple(getattr(out,x) for x,_ in out._fields_),(1,2,3,4))
        return rc,out
    def test_five_mib(self):
        rc,x=self.plan();self.assertEqual(rc,0);self.assertEqual((x.top,x.stack_bottom,x.heap_start),(0x500000,0x4f8000,0x18000))
    def test_one_mib(self):
        rc,x=self.plan(ram=0x100000,reset=0x100000);self.assertEqual(rc,0);self.assertEqual(x.heap_end,0xf8000)
    def test_resident_reservation(self):
        rc,x=self.plan(reset=0x4ff800);self.assertEqual(rc,0);self.assertEqual(x.heap_end,0x4f7800)
    def test_smaller_sdb_wins(self):
        rc,x=self.plan(ram=0x100000);self.assertEqual(rc,0);self.assertEqual(x.top,0x100000)
    def test_null_plan_output(self):
        self.assertEqual(lib.m31_plan(0x500000,0x500000,0x18000,0x40000,0x10000,None),-1)
    def test_bad_ram_ranges(self):
        for ram in [0,0x7fffc,0xe00004,0xffffffff]:
            with self.subTest(ram=ram): self.assertEqual(self.plan(ram=ram)[0],-1)
    def test_bad_reset_ranges(self):
        for reset in [0,0x7fffc,0xe00004,0xffffffff]:
            with self.subTest(reset=reset): self.assertEqual(self.plan(reset=reset)[0],-1)
    def test_metadata_alignment(self):
        self.assertEqual(self.plan(ram=0x500002)[0],-1);self.assertEqual(self.plan(reset=0x500001)[0],-1)
    def test_heap_alignment(self):
        rc,x=self.plan(end=0x18001);self.assertEqual(rc,0);self.assertEqual(x.heap_start,0x18008)
    def test_program_collision(self): self.assertEqual(self.plan(end=0x4f8000)[0],-1)
    def test_program_overflow(self): self.assertEqual(self.plan(end=0xffffffff)[0],-1)
    def test_source_overflow(self): self.assertEqual(self.plan(load=0xfffffff0)[0],-1)
    def test_source_stack_collision(self): self.assertEqual(self.plan(load=0x4f0000)[0],-1)
    def test_source_destination_overlap(self): self.assertEqual(self.plan(load=0x10000)[0],-1)
    def test_invalid_image(self):
        for size in [0,3,0xffffffff,0xffffe000]:
            with self.subTest(size=size): self.assertEqual(self.plan(size=size)[0],-1)
    def test_program_end_before_image_end(self): self.assertEqual(self.plan(end=0x10000)[0],-1)
    def test_heap_lifetime(self):
        h=Heap(0x1000,0x2000,0x1000);old=C.c_uint32()
        for inc,previous,current in [(0,0x1000,0x1000),(0x1000,0x1000,0x2000),(-1,0x2000,0x1fff),(-0xfff,0x1fff,0x1000)]:
            self.assertEqual(lib.m31_heap_move(C.byref(h),inc,C.byref(old)),0);self.assertEqual((old.value,h.current),(previous,current))
    def test_heap_rejections_do_not_mutate(self):
        for inc in [-1,0x1001,2**63-1,-2**63]:
            with self.subTest(inc=inc):
                h=Heap(0x1000,0x2000,0x1000);old=C.c_uint32(123)
                self.assertEqual(lib.m31_heap_move(C.byref(h),inc,C.byref(old)),-1)
                self.assertEqual((h.current,old.value),(0x1000,123))
    def test_invalid_heap(self):
        for h in [Heap(2,1,2),Heap(1,3,0),Heap(1,3,4)]:self.assertEqual(lib.m31_heap_move(C.byref(h),0,C.byref(C.c_uint32())),-1)
    def test_sbrk_errno_ownership(self):
        x=Layout(0,0,0x1000,0x2000);lib.m31_heap_init(C.byref(x));r=Reent(0);C.set_errno(777)
        self.assertEqual(lib._sbrk_r(C.byref(r),0x1001),C.c_void_p(-1).value)
        self.assertEqual((r.error,C.get_errno()),(errno.ENOMEM,777))
        self.assertEqual(lib._sbrk_r(C.byref(r),0),0x1000)

class ConsoleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        lib.fake_reset();assert lib.m31_console_init()==0
    def setUp(self):
        lib.fake_reset();self.assertEqual(lib.rosco_termios_bind(0,0),0);self.r=Reent(0)
    def output(self):return C.string_at(lib.fake_tx(),lib.fake_tx_count())
    def test_single_newline_conversion(self):
        self.assertEqual(lib._write_r(C.byref(self.r),1,b'A\n',2),2);self.assertEqual(self.output(),b'A\r\n')
    def test_stderr_same_console(self):
        self.assertEqual(lib._write_r(C.byref(self.r),2,b'B\n',2),2);self.assertEqual(self.output(),b'B\r\n')
    def test_canonical_input_echo_once(self):
        lib.fake_input(b'X\r',2);buf=C.create_string_buffer(8)
        self.assertEqual(lib._read_r(C.byref(self.r),0,buf,8),2);self.assertEqual(buf.raw[:2],b'X\n');self.assertEqual(self.output(),b'X\r\n')
    def test_shared_settings(self):
        t=Termios();self.assertEqual(lib.tcgetattr(2,C.byref(t)),0)
        lib.cfmakeraw(C.byref(t));t.cc[6]=0;t.cc[5]=0
        self.assertEqual(lib.tcsetattr(1,0,C.byref(t)),0)
        lib.fake_input(b'\r',1);buf=C.create_string_buffer(8)
        self.assertEqual(lib._read_r(C.byref(self.r),0,buf,8),1);self.assertEqual(buf.raw[:1],b'\r');self.assertEqual(self.output(),b'')
        self.assertEqual(lib._write_r(C.byref(self.r),1,b'\n',1),1);self.assertEqual(self.output(),b'\n')
    def test_wrong_direction(self):
        for fd in [1,2,-1,32]:
            self.assertEqual(lib._read_r(C.byref(self.r),fd,None,0),-1);self.assertEqual(self.r.error,errno.EBADF)
        for fd in [0,-1,32]:self.assertEqual(lib._write_r(C.byref(self.r),fd,None,0),-1)
        self.assertEqual(self.output(),b'')
    def test_zero_count(self):
        self.assertEqual(lib._read_r(C.byref(self.r),0,None,0),0);self.assertEqual(lib._write_r(C.byref(self.r),1,None,0),0)
    def test_null_buffer(self):
        self.assertEqual(lib._write_r(C.byref(self.r),1,None,1),-1);self.assertEqual(self.r.error,errno.EINVAL)
    def test_large_count(self):
        self.assertEqual(lib._write_r(C.byref(self.r),1,None,2**31),-1);self.assertEqual(self.r.error,errno.EINVAL)
    def test_filesystem_explicitly_unavailable(self):
        self.assertEqual(lib._open_r(C.byref(self.r),b'/sd/x',0,0),-1);self.assertEqual(self.r.error,errno.ENOSYS)
        self.assertEqual(lib._stat_r(C.byref(self.r),b'/sd/x',None),-1);self.assertEqual(self.r.error,errno.ENOSYS)
        self.assertEqual(self.output(),b'');self.assertEqual(lib.fake_rx_count(),0)
    def test_close_and_seek(self):
        self.assertEqual(lib._close_r(C.byref(self.r),1),-1);self.assertEqual(self.r.error,errno.ENOTSUP)
        self.assertEqual(lib._lseek_r(C.byref(self.r),0,0,0),-1);self.assertEqual(self.r.error,errno.ESPIPE)
    def test_isatty(self):
        for fd in range(3): self.assertEqual(lib._isatty_r(C.byref(self.r),fd),1)
        self.assertEqual(lib._isatty_r(C.byref(self.r),32),0);self.assertEqual(self.r.error,errno.EBADF)
    def test_invalid_termios_fd(self):
        self.assertEqual(lib.tcgetattr(32,C.byref(Termios())),-1);self.assertEqual(C.get_errno(),errno.EBADF)
    def test_no_hidden_io(self):
        self.assertEqual(lib.m31_console_init(),0);self.assertEqual(self.output(),b'');self.assertEqual(lib.fake_rx_count(),0)
