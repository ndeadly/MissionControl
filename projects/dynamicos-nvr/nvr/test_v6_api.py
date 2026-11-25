"""Regression tests for newly introduced Android-compatible endpoints."""
import tempfile
import unittest
from pathlib import Path
from fastapi.testclient import TestClient
from secure_server import NVRService, DeviceStore, LiveRelay, create_app

class MobileAccessTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.folder=Path(self.tmp.name)
        self.cfg={'cameras':[{'id':'cam12345','name':'LAN Eingang','url':'rtsp://192.168.2.4/video1','enabled':True}],
                  'record_dir':str(self.folder),'record_mode':'manual','retention_days':14,'max_storage_gb':0}
        self.s=NVRService(self.cfg,devices=DeviceStore(self.folder/'devices.json'),relay=LiveRelay(self.folder/'relay'))
        self.client=TestClient(create_app(self.s))
        code=self.s.devices.create_pairing()
        r=self.client.post('/api/v1/pair',json={'code':code,'name':'Android'})
        self.auth={'Authorization':'Bearer '+r.json()['token']}
    def tearDown(self):
        self.s.relay.stop()
        self.tmp.cleanup()
    def test_status_requires_auth_and_has_no_private_url(self):
        self.assertEqual(self.client.get('/api/v1/status').status_code,401)
        result=self.client.get('/api/v1/status',headers=self.auth)
        self.assertEqual(result.status_code,200)
        self.assertEqual(result.json()['cameras'],1)
        self.assertNotIn('192.168.2.4',result.text)
    def test_snapshot_requires_auth(self):
        self.assertEqual(self.client.get('/api/v1/cameras/cam12345/snapshot.jpg').status_code,401)
        self.assertEqual(self.client.get('/api/v1/cameras/unknown/snapshot.jpg',headers=self.auth).status_code,404)
    def test_mp4_playback_requires_auth_and_valid_recording(self):
        self.assertEqual(self.client.get('/api/v1/recordings/private.mkv/play.mp4').status_code,401)
        self.assertEqual(self.client.get('/api/v1/recordings/private.mkv/play.mp4',headers=self.auth).status_code,404)
    def test_revocation_applies_to_media_status(self):
        device=self.s.devices.authenticated(self.auth['Authorization'][7:])
        self.s.devices.revoke(device)
        self.assertEqual(self.client.get('/api/v1/status',headers=self.auth).status_code,401)

if __name__=='__main__':unittest.main()
