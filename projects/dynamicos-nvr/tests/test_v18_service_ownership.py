import os
from pathlib import Path
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]

class ServiceOwnership(unittest.TestCase):
    def test_appliance_gui_never_launches_api(self):
        code=(ROOT/'nvr/nvr.py').read_text()
        self.assertIn("if os.environ.get('DYNAMICOS_APPLIANCE_MODE') != '1':\n            self.launch_https()",code)
        self.assertIn('The GUI must never start an additional network listener.',code)

    def test_api_lan_is_opt_in(self):
        installer=(ROOT/'appliance/install-on-debian.sh').read_text()
        service=(ROOT/'appliance/dynamicos-api.service').read_text()
        self.assertIn('DYNAMICOS_API_HOST=127.0.0.1',installer)
        self.assertNotIn('dynamicos-api.service lightdm.service',installer)
        self.assertIn('ExecStart=',service)

    def test_tls_ip_san_mismatch_fails_closed(self):
        import sys,tempfile
        sys.path.insert(0,str(ROOT/'nvr'))
        import secure_server
        with tempfile.TemporaryDirectory() as tmp:
            cert=Path(tmp)/'cert.pem';key=Path(tmp)/'key.pem'
            with patch.object(secure_server,'CERT_FILE',cert),patch.object(secure_server,'KEY_FILE',key):
                secure_server.generate_certificate(('127.0.0.1','192.168.1.20'))
                secure_server.generate_certificate(('127.0.0.1','192.168.1.20'))
                with self.assertRaisesRegex(RuntimeError,'configured API address'):
                    secure_server.generate_certificate(('127.0.0.1','192.168.1.21'))

if __name__=='__main__':unittest.main()
