import importlib.util
import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
def mod(file):
    spec=importlib.util.spec_from_file_location(file.stem.replace('-','_'),file)
    obj=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj

class SandboxTests(unittest.TestCase):
    def test_recorder_baseline(self):
        audit=mod(ROOT/'appliance'/'audit-sandbox.py')
        self.assertEqual(audit.verify(ROOT/'appliance'/'dynamicos-recorder.service'),[])
    def test_fail_missing_protection(self):
        audit=mod(ROOT/'appliance'/'audit-sandbox.py')
        import tempfile
        data=(ROOT/'appliance'/'dynamicos-recorder.service').read_text().replace('NoNewPrivileges=yes','NoNewPrivileges=no')
        with tempfile.TemporaryDirectory() as p:
            f=pathlib.Path(p)/'fake.service';f.write_text(data)
            self.assertIn('NoNewPrivileges missing or incorrect',audit.verify(f))
    def test_mount_unit_name_matches_systemd_path(self):
        expected="run-dynamicos\\x2dnvr-hls.mount"
        self.assertTrue((ROOT/"appliance"/expected).exists())
        self.assertIn(expected,(ROOT/"appliance"/"dynamicos-recorder.service").read_text())
    def test_firewall_rejects_public_and_wide(self):
        fw=mod(ROOT/'appliance'/'firewall-policy.py')
        for value in ('0.0.0.0/0','8.8.8.0/24','192.168.0.0/8'):
            with self.assertRaises(ValueError):fw.generate([value],[])
        with self.assertRaises(ValueError):fw.generate(['192.168.2.0/24'],['8.8.8.8'])
    def test_firewall_only_explicit_inputs(self):
        fw=mod(ROOT/'appliance'/'firewall-policy.py')
        result=fw.generate(['192.168.2.0/24'],['192.168.30.5'])
        self.assertIn('policy drop',result)
        self.assertNotIn('flush ruleset',result)
        self.assertIn('192.168.30.5',result)
        self.assertIn('tcp dport 8443',result)
        self.assertNotIn('tcp dport 22 accept',result)

if __name__=='__main__':unittest.main()
