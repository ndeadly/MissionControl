import tempfile, unittest
from pathlib import Path
from security_policy import require_vault

class VaultPolicyTests(unittest.TestCase):
    def test_mounted_crypt_inside(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)/'vault';rec=root/'recordings';rec.mkdir(parents=True)
            self.assertEqual(require_vault(rec,vault=root,mount_source='/dev/mapper/x',device_type='crypt'),rec)
    def test_plaintext_mount_denied(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)/'vault';rec=root/'recordings';rec.mkdir(parents=True)
            with self.assertRaises(RuntimeError):require_vault(rec,vault=root,mount_source='/dev/sda3',device_type='part')
    def test_outside_path_denied(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)/'vault';root.mkdir();other=Path(d)/'other';other.mkdir()
            with self.assertRaises(RuntimeError):require_vault(other,vault=root,mount_source='/dev/mapper/x',device_type='crypt')
    def test_symlink_escape_denied(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)/'vault';root.mkdir();other=Path(d)/'other';other.mkdir()
            (root/'recordings').symlink_to(other,target_is_directory=True)
            with self.assertRaises(RuntimeError):require_vault(root/'recordings',vault=root,mount_source='/dev/mapper/x',device_type='crypt')
