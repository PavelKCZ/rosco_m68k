"""Native profile tests. Writes only to caller-supplied external test output."""
import sys
sys.dont_write_bytecode = True
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import build
import os
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import json

ROOT=Path(os.environ['M31_PROFILE_TEST_OUTPUT']).resolve()
if Path('/tmp') not in ROOT.parents or build.REPO in ROOT.parents:
    raise RuntimeError('external test output required')
ROOT.mkdir(parents=True,exist_ok=False)

class ProfileTests(unittest.TestCase):
    def test_approved_profile(self):
        self.assertEqual(build.verify_sources()['objects'],build.OBJECTS)
    def test_reject_bad_pin(self):
        with tempfile.TemporaryDirectory(dir=ROOT) as d:
            p=Path(d)/'profile';shutil.copytree(build.HERE,p)
            v=json.loads((p/'PROVENANCE.json').read_text());v['protected_inputs']['code/software/newlib/m31a/startup.S']='0'*64
            (p/'PROVENANCE.json').write_text(json.dumps(v))
            (p/'SHA256SUMS').write_text(''.join(build.sha(x)+'  '+str(x.relative_to(p))+'\n' for x in sorted(p.rglob('*')) if x.is_file() and x.name!='SHA256SUMS'))
            with patch.object(build,'HERE',p),self.assertRaisesRegex(ValueError,'INPUT_HASH'):
                build.verify_sources()
    def test_invalid_output_paths(self):
        for p in ['relative','/tmp','/tmp/a b','/tmp/a,b',str(build.REPO/'output')]:
            with self.subTest(path=p),self.assertRaises(ValueError):build.external(p)
    def test_pin_traversal(self):
        with self.assertRaisesRegex(ValueError,'PIN_PATH'):build.pinned_file(build.REPO,'../escape','0'*64)
    def test_fixture_missing(self):
        with self.assertRaisesRegex(ValueError,'historical runtime BIN'):build.verify_historical_fixture(ROOT/'absent')
    def test_fixture_wrong(self):
        p=ROOT/'wrong.bin';p.write_bytes(bytes(44980))
        with self.assertRaisesRegex(ValueError,'historical runtime BIN'):build.verify_historical_fixture(p)
    def test_tool_identity(self):
        v=build.verify_sources()
        with self.assertRaisesRegex(ValueError,'TOOL_IDENTITY'):build.checked_tool(Path(__file__).resolve(),'gcc',v)
    def test_library_identity(self):
        v=build.verify_sources();fake={n:str(Path(__file__)) for n in v['archives']}
        with self.assertRaisesRegex(ValueError,'ARCHIVE_IDENTITY'):build.libraries_match(fake,v)
    def test_injected_environment(self):
        with patch.dict(os.environ,{'CPATH':'/unapproved'}),self.assertRaisesRegex(ValueError,'ENV_INJECTION'):build.clean_environment()
    def test_unapproved_dependency(self):
        p=ROOT/'foreign.h';p.write_text('/* not approved */\n')
        with self.assertRaisesRegex(ValueError,'UNAPPROVED_DEPENDENCY'):build.check_dependencies({str(p):build.sha(p)},{},build.verify_sources())
    def test_cwd_independent_help(self):
        p=subprocess.run([sys.executable,str(build.HERE/'tools/build.py'),'--help'],cwd=ROOT,capture_output=True,text=True)
        self.assertEqual(p.returncode,0,p.stderr);self.assertIn('--toolchain-bin',p.stdout)
    def test_runtime_runner_missing_argument(self):
        p=subprocess.run([sys.executable,str(build.RUNTIME/'tests/run.py'),str(ROOT/'should-not-exist')],cwd=ROOT,capture_output=True,text=True)
        self.assertEqual(p.returncode,2);self.assertIn('--historical-runtime-bin',p.stderr);self.assertFalse((ROOT/'should-not-exist').exists())
    def test_runtime_runner_wrong_fixture(self):
        p=subprocess.run([sys.executable,str(build.RUNTIME/'tests/run.py'),str(ROOT/'never-build'),'--historical-runtime-bin',str(ROOT/'absent')],cwd=ROOT,capture_output=True,text=True)
        self.assertEqual(p.returncode,2);self.assertIn('BLOCKED',p.stderr);self.assertFalse((ROOT/'never-build').exists())

if __name__=='__main__':unittest.main(verbosity=2)
