import datetime as dt
import json
import os
from pathlib import Path
import subprocess
import tempfile
import threading
import time
import unittest
from unittest.mock import patch
from backend import cleanup_storage, defaults, probe_camera_profile, RecordingManager, EventStore

class LuxurySecurityTests(unittest.TestCase):
    def older(self,path,seconds):
        path.write_bytes(b'cipher')
        t=time.time()-seconds
        os.utime(path,(t,t))
    def test_default_continuous(self):
        self.assertEqual(defaults()['record_mode'],'continuous')
        self.assertEqual(defaults()['retention_hours'],336)
    def test_hourly_retention(self):
        with tempfile.TemporaryDirectory() as temp:
            old=Path(temp)/'expired.dnvr'; new=Path(temp)/'fresh.dnvr'
            self.older(old,7200);self.older(new,3900)
            removed=cleanup_storage(temp,14,0,retention_hours=1,protected_paths=[new])
            self.assertEqual(removed,[str(old)])
            self.assertFalse(old.exists());self.assertTrue(new.exists())
    def test_retention_active_protection(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'viewing.dnvr';self.older(p,7200)
            manager=RecordingManager({'record_dir':temp,'cameras':[]},EventStore(Path(temp)/'events.sqlite'))
            with manager.protect_playback(p):
                cleanup_storage(temp,1,0,retention_hours=1,protected_paths=manager.protected_recordings)
                self.assertTrue(p.exists())
            cleanup_storage(temp,1,0,retention_hours=1)
            self.assertFalse(p.exists())
    def test_never_delete_unfinalized_file(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'unfinished.dnvr.part';self.older(p,999999)
            cleanup_storage(temp,1,1,retention_hours=1)
            self.assertTrue(p.exists())
    def test_reject_negative_retention(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaises(ValueError):cleanup_storage(temp,0,0,retention_hours=-1)
    def test_probe_full_hd(self):
        response=subprocess.CompletedProcess([],0,stdout=json.dumps({'streams':[{'width':1920,'height':1080,'codec_name':'h264','avg_frame_rate':'25/1'}]}).encode())
        with patch('backend.subprocess.run',return_value=response):
            result=probe_camera_profile('rtsp://192.168.2.4:554/video1',probe_binary='ffprobe')
        self.assertTrue(result['full_hd'])
        self.assertEqual(result['frame_rate'],'25/1')
    def test_probe_no_upscale(self):
        response=subprocess.CompletedProcess([],0,stdout=b'{"streams":[{"width":640,"height":360}]}')
        with patch('backend.subprocess.run',return_value=response):
            result=probe_camera_profile('rtsp://192.168.2.4:554/video1',probe_binary='ffprobe')
        self.assertFalse(result['full_hd'])
    def test_probe_error_no_credentials_exposed(self):
        response=subprocess.CompletedProcess([],1,stdout=b'',stderr=b'rtsp://secret:secret@ip')
        with patch('backend.subprocess.run',return_value=response):
            with self.assertRaisesRegex(RuntimeError,'RTSP-Videostream nicht verfügbar') as err:
                probe_camera_profile('rtsp://secret:secret@192.168.2.4/video1',probe_binary='ffprobe')
        self.assertNotIn('secret',str(err.exception))

if __name__=='__main__': unittest.main()
