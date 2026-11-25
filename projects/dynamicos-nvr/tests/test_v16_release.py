import ast
import importlib.util
import sys
import unittest
from pathlib import Path
P=Path(__file__).resolve().parents[1]

class V16Hardening(unittest.TestCase):
    def test_android_no_hostname_verifier_bypass(self):
        source=(P/'android/app/src/main/java/com/vonder3urg/dynamicnvr/NvrClient.java').read_text()
        self.assertNotIn('hostnameVerifier((host,session)->true)',source)
        self.assertIn('MessageDigest.isEqual',source)
    def test_api_is_separate_and_opt_in(self):
        src=(P/'appliance/dynamicos-api.service').read_text()
        self.assertIn('User=dynamicos',src)
        self.assertIn('NoNewPrivileges=yes',src)
        self.assertIn('ConditionPathIsMountPoint=/var/lib/dynamicos-nvr',src)
        self.assertIn('IPAddressDeny=any',src)
        self.assertNotIn('dynamicos-api.service lightdm.service', (P/'appliance/install-on-debian.sh').read_text())
        ast.parse((P/'nvr/api_daemon.py').read_text())
    def test_fail_closed_vault_api(self):
        src=(P/'nvr/secure_server.py').read_text()
        self.assertIn("require_vault(self.cfg['record_dir'])",src)
    def test_health_no_empty_success(self):
        self.assertIn("no active cameras configured",(P/'appliance/dynamicos-health').read_text())

if __name__=='__main__':unittest.main()
