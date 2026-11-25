import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'nvr'))
from backend import delete_unlocked, playback_file_lock, cleanup_storage

class RetentionLockTests(unittest.TestCase):
    def test_locked_recording_not_deleted(self):
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/'archive.dnvr'
            file.write_bytes(b'cryptotext')
            with playback_file_lock(file):
                self.assertFalse(delete_unlocked(file))
                self.assertTrue(file.exists())
            self.assertTrue(delete_unlocked(file))
            self.assertFalse(file.exists())
    def test_cleanup_skips_in_use_file(self):
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/'archive.dnvr';file.write_bytes(b'cryptotext')
            past=time.time()-86400*31
            os.utime(file,(past,past))
            with playback_file_lock(file):
                self.assertEqual(cleanup_storage(d,14,0,minimum_age_seconds=0),[])
                self.assertTrue(file.exists())
            self.assertEqual(len(cleanup_storage(d,14,0,minimum_age_seconds=0)),1)
