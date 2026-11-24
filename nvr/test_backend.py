import datetime as dt
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from backend import (valid_stream_url, safe_name, in_schedule, new_auth,
                     verify_password, EventStore, cleanup_storage)

class BackendTests(unittest.TestCase):
    def test_urls(self):
        self.assertTrue(valid_stream_url('rtsp://user:pass@192.168.1.20:554/stream'))
        self.assertTrue(valid_stream_url('https://example.org/video'))
        self.assertFalse(valid_stream_url('file:///etc/passwd'))
        self.assertFalse(valid_stream_url('hello'))
    def test_safe_file(self):
        self.assertEqual(safe_name('../../foo bar'), 'foo_bar')
    def test_schedule_midnight(self):
        self.assertTrue(in_schedule(dt.datetime(2026,1,1,23,0),'22:00','06:00'))
        self.assertTrue(in_schedule(dt.datetime(2026,1,1,2,0),'22:00','06:00'))
        self.assertFalse(in_schedule(dt.datetime(2026,1,1,13,0),'22:00','06:00'))
    def test_password(self):
        auth = new_auth('admin','a very long secret')
        self.assertTrue(verify_password(auth,'admin','a very long secret'))
        self.assertFalse(verify_password(auth,'admin','other'))
    def test_events(self):
        with TemporaryDirectory() as temp:
            ev=EventStore(Path(temp)/'db.sqlite')
            ev.log('Test','Kamera','OK')
            self.assertEqual(ev.recent()[0][2],'Test')
    def test_cleanup(self):
        with TemporaryDirectory() as temp:
            p=Path(temp)/'old.dnvr';p.write_bytes(b'encrypted-bytes')
            old=dt.datetime.now().timestamp()-86400*3
            import os
            os.utime(p,(old,old))
            txt=Path(temp)/'important.txt';txt.write_text('keep')
            cleanup_storage(temp,1,0)
            self.assertFalse(p.exists());self.assertTrue(txt.exists())

if __name__=='__main__':unittest.main()
