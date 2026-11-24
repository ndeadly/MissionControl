"""v8 regression suite: strict LAN admission and no public camera relay."""
import tempfile
import unittest
from pathlib import Path
from fastapi.testclient import TestClient
from secure_server import safe_camera_source, checked_allowed_networks, DeviceStore, LiveRelay, NVRService, create_app

class SecurityV8(unittest.TestCase):
    def test_camera_source_allowlist(self):
        self.assertTrue(safe_camera_source('rtsp://user:password@192.168.2.25:554/video1'))
        self.assertTrue(safe_camera_source('rtsp://10.5.1.9:554/live'))
        for target in ['rtsp://8.8.8.8/live','http://example.com/stream',
                       'rtsp://192.0.0.9/a','rtsp://127.0.0.1:554/a','rtsp://169.254.4.5/a',
                       'file:///etc/passwd','rtsp://localhost/live','rtsp://224.0.0.1/video']:
            with self.subTest(target=target):self.assertFalse(safe_camera_source(target))
    def test_lan_networks(self):
        self.assertEqual(checked_allowed_networks(['192.168.2.41/24']),('192.168.2.0/24',))
        for net in ['0.0.0.0/0','8.8.8.0/24','10.0.0.0/8','172.0.0.0/8','169.254.0.0/16','::/0']:
            with self.subTest(net=net):
                with self.assertRaises(ValueError):checked_allowed_networks([net])
    def test_secretless_api_and_bad_camera(self):
        with tempfile.TemporaryDirectory() as td:
            cfg={'cameras':[{'id':'c1','name':'Bad endpoint','url':'rtsp://8.8.8.8/a','enabled':True}],
                 'record_dir':td,'record_mode':'manual','schedule_start':'00:00','schedule_end':'23:59','retention_days':14,'max_storage_gb':0}
            svc=NVRService(cfg,devices=DeviceStore(Path(td)/'devices.json'),relay=LiveRelay(Path(td)/'hls'))
            with TestClient(create_app(svc)) as client:
                code=svc.devices.create_pairing()
                token=client.post('/api/v1/pair',json={'code':code,'name':'QA'}).json()['token']
                headers={'Authorization':'Bearer '+token}
                self.assertEqual(client.get('/api/v1/cameras/c1/live/index.m3u8',headers=headers).status_code,503)
                self.assertEqual(client.get('/api/v1/cameras/c1/snapshot.jpg',headers=headers).status_code,503)
                self.assertEqual(client.get('/api/v1/security').status_code,401)
                security=client.get('/api/v1/security',headers=headers).json()
                self.assertNotIn(token,str(security))
                self.assertNotIn('8.8.8.8',str(security))
            svc.relay.stop()

if __name__=='__main__':unittest.main()
