import tempfile
from pathlib import Path
import unittest
from fastapi.testclient import TestClient
from secure_server import DeviceStore, LiveRelay, NVRService, create_app

class ServerTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory()
  self.dir=Path(self.tmp.name)
  cfg={'cameras':[{'id':'a1b2c3d4','name':'ZOSI Eingang','url':'rtsp://192.168.1.45:554/video1','enabled':True}],
       'record_dir':str(self.dir),'record_mode':'manual','schedule_start':'00:00','schedule_end':'23:59','retention_days':14,'max_storage_gb':0}
  self.s=NVRService(cfg,devices=DeviceStore(self.dir/'devices.json'),relay=LiveRelay(self.dir/'hls'))
  self.c=TestClient(create_app(self.s))
 def tearDown(self): self.s.relay.stop();self.tmp.cleanup()
 def pairing(self):
  code=self.s.devices.create_pairing()
  r=self.c.post('/api/v1/pair',json={'code':code,'name':'S24 Ultra'})
  self.assertEqual(r.status_code,200)
  return {'Authorization':'Bearer '+r.json()['token']}
 def test_pairing_expired_and_bad(self):
  self.assertEqual(self.c.post('/api/v1/pair',json={'code':'000000','name':'x'}).status_code,403)
  code=self.s.devices.create_pairing()
  self.assertEqual(self.c.post('/api/v1/pair',json={'code':code,'name':'Android'}).status_code,200)
  self.assertEqual(self.c.post('/api/v1/pair',json={'code':code,'name':'Android'}).status_code,403)
 def test_private_api(self):
  self.assertEqual(self.c.get('/api/v1/cameras').status_code,401)
  self.assertEqual(self.c.get('/api/v1/recordings').status_code,401)
  headers=self.pairing()
  r=self.c.get('/api/v1/cameras',headers=headers)
  self.assertEqual(r.status_code,200)
  self.assertNotIn('rtsp',r.text)
  self.assertNotIn('192.168.',r.text)
 def test_recordings_are_scoped_and_authenticated(self):
  path=self.dir/'ZOSI_Eingang_a1b2c3d4_20261124_220000.dnvr';path.write_bytes(b'encrypted-test-fixture')
  (self.dir/'other_secret.dnvr').write_bytes(b'no')
  headers=self.pairing()
  found=self.c.get('/api/v1/recordings',headers=headers).json()
  self.assertEqual(len(found),1)
  self.assertEqual(self.c.get('/api/v1/recordings/'+path.name,headers=headers).status_code,403)
  self.assertEqual(self.c.get('/api/v1/recordings/other_secret.dnvr',headers=headers).status_code,404)
  self.assertEqual(self.c.get('/api/v1/recordings/'+path.name).status_code,401)
 def test_hls_rejects_traversal_and_unauthorized(self):
  headers=self.pairing()
  self.assertEqual(self.c.get('/api/v1/cameras/a1b2c3d4/live/seg_../../x.ts',headers=headers).status_code,404)
  self.assertEqual(self.c.get('/api/v1/cameras/a1b2c3d4/live/seg_123.ts',headers=headers).status_code,404)
  self.assertEqual(self.c.get('/api/v1/cameras/a1b2c3d4/live/seg_123.ts').status_code,401)
 def test_revoke(self):
  headers=self.pairing()
  dev=self.c.get('/api/v1/devices',headers=headers).json()[0]['id']
  self.assertEqual(self.c.delete('/api/v1/devices/'+dev,headers=headers).status_code,200)
  self.assertEqual(self.c.get('/api/v1/cameras',headers=headers).status_code,401)
 def test_wrong_device_cannot_revoke_other(self):
  a=self.pairing();b=self.pairing()
  dev=self.c.get('/api/v1/devices',headers=a).json()[0]['id']
  # Pick another device's ID intentionally
  own=self.s.devices.authenticated(b['Authorization'][7:])
  foreign=next(x for x in self.s.devices.devices if x!=own)
  self.assertEqual(self.c.delete('/api/v1/devices/'+foreign,headers=b).status_code,403)

if __name__=='__main__':unittest.main()
