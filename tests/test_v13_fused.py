import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'nvr'))
from security_policy import require_ram_cache
from recording_daemon import atomic_status

class IntegratedApplianceTests(unittest.TestCase):
    def test_tmpfs_cache_ok(self):
        self.assertEqual(require_ram_cache('/run/dynamicos-nvr/hls',fs_type='tmpfs'), Path('/run/dynamicos-nvr/hls'))
    def test_no_persistent_cache(self):
        with self.assertRaises(RuntimeError):require_ram_cache('/var/lib/dynamicos-nvr/hls',fs_type='tmpfs')
    def test_not_tmpfs(self):
        with self.assertRaises(RuntimeError):require_ram_cache('/run/dynamicos-nvr/hls',fs_type='ext4')
    def test_recorder_status_atomic(self):
        with tempfile.TemporaryDirectory() as d:
            with patch('recording_daemon.APP_DIR',Path(d)):
                atomic_status({'state':'recording','running':3,'epoch':123})
                self.assertEqual((Path(d)/'recorder-status.json').read_text().count('recording'),1)
                self.assertFalse((Path(d)/'recorder-status.tmp').exists())
