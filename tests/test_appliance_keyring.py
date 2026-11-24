import os, tempfile, unittest, sys
from unittest.mock import patch
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'nvr'))
import appliance_keyring
class TestKeyring(unittest.TestCase):
 def test_denies_unencrypted_store(self):
  with tempfile.TemporaryDirectory() as d:
   old=os.environ.get('DYNAMICOS_STATE_DIR')
   os.environ['DYNAMICOS_STATE_DIR']=d
   try:
    with self.assertRaises((RuntimeError,Exception)):
     appliance_keyring.set_password('service','user','pw')
   finally:
    if old is None:os.environ.pop('DYNAMICOS_STATE_DIR',None)
    else:os.environ['DYNAMICOS_STATE_DIR']=old
 def test_atomic_restricted_file_on_mock_encrypted(self):
  with tempfile.TemporaryDirectory() as d:
   old=os.environ.get('DYNAMICOS_STATE_DIR')
   os.environ['DYNAMICOS_STATE_DIR']=d
   try:
    with patch.object(appliance_keyring,'_store',return_value=Path(d)/'sealed-secrets.json'):
     appliance_keyring.set_password('vault','key','sample-secret')
     self.assertEqual(appliance_keyring.get_password('vault','key'),'sample-secret')
    self.assertEqual(Path(d,'sealed-secrets.json').stat().st_mode&0o777,0o600)
   finally:
    for k,v in [('DYNAMICOS_STATE_DIR',old)]:
     if v is None:os.environ.pop(k,None)
     else:os.environ[k]=v
if __name__=='__main__':unittest.main()
