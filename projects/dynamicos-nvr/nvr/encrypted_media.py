"""dynamicOS authenticated media vault.

A single master key lives in a restricted secret store on an unlocked LUKS filesystem.
Each recording gets an HKDF-derived independent AES-256-GCM key and fresh salt.
64 KiB records are authenticated individually with nonce and sequence bound as AAD.
A final authenticated empty record detects truncation, including block-aligned truncation.
Plaintext never needs to be materialized on the filesystem.
"""
from __future__ import annotations
import base64
import hashlib
import hmac
import os
from pathlib import Path
import secrets
import struct

from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes

MAGIC=b'DYNAMICNVR-ENC-1\n'
CHUNK=64*1024
MAX_CIPHER=CHUNK+16
KEYRING_SERVICE='dynamicOS NVR Recording Master Key v1'
KEYRING_ACCOUNT='local-vault'

class VaultError(Exception): pass

class KeyProvider:
    def __init__(self, *, key=None, keyring_module=None):
        self._key=key
        self._ring=keyring_module
    def master(self):
        if self._key is not None:
            if not isinstance(self._key,bytes) or len(self._key)!=32:raise VaultError('Ungültiger Testschlüssel')
            return self._key
        if self._ring is None:
            try:import appliance_keyring as keyring
            except ImportError as exc:raise VaultError('verschlüsselte Linux-Schlüsselablage nicht verfügbar') from exc
            self._ring=keyring
        # No insecure fallback to plaintext key files or environment variables.
        try:
            existing=self._ring.get_password(KEYRING_SERVICE,KEYRING_ACCOUNT)
            if existing:
                key=base64.b64decode(existing,validate=True)
                if len(key)!=32:raise VaultError('Ungültiger Schlüssel im verschlüsselte Linux-Schlüsselablage')
                return key
            key=secrets.token_bytes(32)
            self._ring.set_password(KEYRING_SERVICE,KEYRING_ACCOUNT,base64.b64encode(key).decode('ascii'))
            check=self._ring.get_password(KEYRING_SERVICE,KEYRING_ACCOUNT)
            if not check or not hmac.compare_digest(check,base64.b64encode(key).decode('ascii')):
                raise VaultError('Schlüsselbund-Schreibzugriff konnte nicht bestätigt werden')
            return key
        except VaultError:raise
        except Exception as exc:raise VaultError('Schlüsselbund ist gesperrt oder nicht verfügbar') from exc

def _key(master,salt):
    return HKDF(algorithm=hashes.SHA256(),length=32,salt=salt,info=b'dynamicOS recording vault v1').derive(master)

def _aad(header,seq,final):return header+struct.pack('>Q',seq)+(b'F' if final else b'D')

def encrypt_stream(source,dest,key_provider=None):
    """Stream source.read into an already-open output stream, without plaintext temp files."""
    master=(key_provider or KeyProvider()).master()
    salt=secrets.token_bytes(16);prefix=secrets.token_bytes(4)
    header=MAGIC+salt+prefix
    dest.write(header)
    aes=AESGCM(_key(master,salt))
    seq=0
    while True:
        data=source.read(CHUNK)
        if not data:break
        if seq >= 2**64-1:raise VaultError('Aufzeichnung zu lang')
        cipher=aes.encrypt(prefix+struct.pack('>Q',seq),data,_aad(header,seq,False))
        dest.write(struct.pack('>I',len(cipher)));dest.write(cipher)
        seq+=1
    final=aes.encrypt(prefix+struct.pack('>Q',seq),b'',_aad(header,seq,True))
    dest.write(struct.pack('>I',len(final)));dest.write(final)
    dest.flush()

def _read_exact(stream,n):
    v=stream.read(n)
    if len(v)!=n:raise VaultError('Aufzeichnung abgeschnitten oder beschädigt')
    return v

def decrypt_chunks(source,key_provider=None):
    """Yield verified plaintext; authentication errors never become partial success."""
    master=(key_provider or KeyProvider()).master()
    header=_read_exact(source,len(MAGIC)+20)
    if not hmac.compare_digest(header[:len(MAGIC)],MAGIC):raise VaultError('Kein dynamicOS verschlüsseltes Video')
    salt=header[len(MAGIC):len(MAGIC)+16]
    prefix=header[-4:]
    aes=AESGCM(_key(master,salt));seq=0
    while True:
        raw=_read_exact(source,4)
        n=struct.unpack('>I',raw)[0]
        if n<16 or n>MAX_CIPHER:raise VaultError('Ungültiger Datenblock')
        encrypted=_read_exact(source,n)
        nonce=prefix+struct.pack('>Q',seq)
        if n==16:
            try:aes.decrypt(nonce,encrypted,_aad(header,seq,True))
            except Exception as exc:raise VaultError('Ungültiger Abschlussblock') from exc
            if source.read(1):raise VaultError('Unerwartete Daten nach Abschluss')
            return
        try:data=aes.decrypt(nonce,encrypted,_aad(header,seq,False))
        except Exception as exc:raise VaultError('Aufzeichnung manipuliert oder falscher Schlüssel') from exc
        yield data
        seq+=1

def encrypt_file(source_path,dest_path,provider=None):
    with open(source_path,'rb') as inp,open(dest_path,'xb') as out:
        os.chmod(dest_path,0o600)
        encrypt_stream(inp,out,provider)
        out.flush();os.fsync(out.fileno())
