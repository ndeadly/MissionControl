"""Cross-process registry tests: UI, API and restarts must share identity state."""
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
NVR=ROOT/'nvr'
sys.path.insert(0,str(NVR))
from secure_server import DeviceStore

class SharedIdentityTests(unittest.TestCase):
    def test_pairing_created_by_gui_consumed_by_api(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'clients.json'
            gui=DeviceStore(path)
            code=gui.create_pairing()
            api=DeviceStore(path)
            result=api.pair(code,'Android')
            self.assertIsNotNone(result)
            self.assertEqual(gui.authenticated(result['token']),result['device_id'])
            self.assertIsNone(gui.pair(code,'replay'))

    def test_revocation_visible_to_running_api_and_after_restart(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'clients.json'
            gui=DeviceStore(path)
            token=gui.pair(gui.create_pairing(),'S24')['token']
            api=DeviceStore(path)
            device=api.authenticated(token)
            self.assertIsNotNone(device)
            self.assertTrue(gui.revoke(device))
            self.assertIsNone(api.authenticated(token))
            self.assertIsNone(DeviceStore(path).authenticated(token))

    def test_pair_code_works_between_python_processes(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'clients.json'
            gui=DeviceStore(path)
            code=gui.create_pairing()
            script='from secure_server import DeviceStore;import sys; print(DeviceStore(sys.argv[1]).pair(sys.argv[2],"Mobile")["token"])'
            env=os.environ.copy();env['PYTHONPATH']=str(NVR)
            result=subprocess.run([sys.executable,'-c',script,str(path),code],env=env,text=True,capture_output=True,timeout=15,check=True)
            self.assertIsNotNone(gui.authenticated(result.stdout.strip()))

    def test_attempt_limit_persists_restarts(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'clients.json'
            first=DeviceStore(path);code=first.create_pairing()
            for _ in range(4):self.assertIsNone(DeviceStore(path).pair('000000','attacker'))
            self.assertIsNone(DeviceStore(path).pair('000000','attacker'))
            self.assertIsNone(DeviceStore(path).pair(code,'valid'))

if __name__=='__main__':unittest.main()
