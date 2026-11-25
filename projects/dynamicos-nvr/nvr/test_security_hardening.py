import datetime as dt
import tempfile
import unittest
from pathlib import Path
from fastapi.testclient import TestClient
from secure_server import DeviceStore, NVRService, LiveRelay, create_app, permitted_peer, PeerLimiter

class HardenedSecurityTests(unittest.TestCase):
    def test_private_network_deny_by_default(self):
        self.assertTrue(permitted_peer('127.0.0.1'))
        self.assertFalse(permitted_peer('192.168.1.20'))
        self.assertFalse(permitted_peer('8.8.8.8',['192.168.1.0/24']))
        self.assertTrue(permitted_peer('192.168.1.20',['192.168.1.0/24']))
        self.assertFalse(permitted_peer('192.168.2.20',['192.168.1.0/24']))
        self.assertFalse(permitted_peer('not-an-ip',['192.168.1.0/24']))

    def test_device_token_expiry(self):
        with tempfile.TemporaryDirectory() as td:
            store=DeviceStore(Path(td)/'devices.json')
            token=store.pair(store.create_pairing(),'Android')['token']
            dev=store.authenticated(token)
            self.assertIsNotNone(dev)
            with store._locked():
                store._reload()
                store._devices[dev]['expires_at']=(dt.datetime.now(dt.timezone.utc)-dt.timedelta(seconds=1)).isoformat()
                store._write_atomic(store.path,store._devices)
            self.assertIsNone(store.authenticated(token))
            with store._locked():
                store._reload()
                store._devices[dev].pop('expires_at')
                store._write_atomic(store.path,store._devices)
            self.assertIsNone(store.authenticated(token))

    def test_rate_limiter(self):
        limiter=PeerLimiter(limit=2,seconds=60)
        self.assertTrue(limiter.allowed('client'))
        self.assertTrue(limiter.allowed('client'))
        self.assertFalse(limiter.allowed('client'))
        self.assertTrue(limiter.allowed('other'))

    def test_security_headers_and_body_limit(self):
        with tempfile.TemporaryDirectory() as td:
            cfg={'cameras':[], 'record_dir':td,'record_mode':'manual','schedule_start':'00:00','schedule_end':'23:59','retention_days':14,'max_storage_gb':0}
            svc=NVRService(cfg,devices=DeviceStore(Path(td)/'devices.json'),relay=LiveRelay(Path(td)/'hls'))
            with TestClient(create_app(svc)) as c:
                r=c.get('/api/v1/cameras')
                self.assertEqual(r.status_code,401)
                self.assertEqual(r.headers['cache-control'],'no-store')
                self.assertEqual(r.headers['x-frame-options'],'DENY')
                self.assertEqual(c.post('/api/v1/pair',content=b'x'*5000).status_code,413)
            svc.relay.stop()

if __name__=='__main__': unittest.main()
