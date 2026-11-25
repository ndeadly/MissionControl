import io
import os
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from encrypted_media import encrypt_stream,decrypt_chunks,KeyProvider,VaultError,encrypt_file
from migrate_legacy import migrate
from backend import recording_files,cleanup_storage

KEY=KeyProvider(key=b'k'*32)
class EncryptedVaultTests(unittest.TestCase):
    def enc(self,data):
        out=io.BytesIO();encrypt_stream(io.BytesIO(data),out,KEY);return out.getvalue()
    def dec(self,enc,provider=KEY):return b''.join(decrypt_chunks(io.BytesIO(enc),provider))
    def test_roundtrip_empty(self):self.assertEqual(self.dec(self.enc(b'')),b'')
    def test_roundtrip_multichunk(self):
        original=os.urandom(205000);crypted=self.enc(original)
        self.assertEqual(self.dec(crypted),original)
        self.assertNotIn(original[:200],crypted)
    def test_random_iv(self):self.assertNotEqual(self.enc(b'hello'),self.enc(b'hello'))
    def test_wrong_key(self):
        with self.assertRaises(VaultError):self.dec(self.enc(b'secret'),KeyProvider(key=b'a'*32))
    def test_corrupted_chunk(self):
        v=bytearray(self.enc(b'secret'));v[-18]^=1
        with self.assertRaises(VaultError):self.dec(v)
    def test_truncated(self):
        v=self.enc(b'hello world')
        for n in (1,5,16,32):
            with self.subTest(n=n),self.assertRaises(VaultError):self.dec(v[:-n])
    def test_appended(self):
        with self.assertRaises(VaultError):self.dec(self.enc(b'hello')+b'junk')
    def test_encrypted_files_and_cleanup(self):
        with TemporaryDirectory() as temp:
            p=Path(temp)/'camera_123_2026.dnvr'
            encrypt_file.__name__
            p.write_bytes(self.enc(b'secret'))
            self.assertEqual(recording_files(temp),[p])
            self.assertNotIn(b'secret',p.read_bytes())
    def test_migrate(self):
        with TemporaryDirectory() as temp:
            p=Path(temp)/'camera.mkv';p.write_bytes(b'test mkv bytes')
            self.assertEqual(len(migrate(temp,provider=KEY)['pending']),1)
            result=migrate(temp,confirm=True,provider=KEY)
            self.assertFalse(p.exists())
            self.assertEqual(len(result['converted']),1)
            with open(p.with_suffix('.dnvr'),'rb') as f:self.assertEqual(b''.join(decrypt_chunks(f,KEY)),b'test mkv bytes')
if __name__=='__main__':unittest.main()
