"""Minimal keyring facade. All key material stays within an already-unlocked LUKS volume.
Refuse all writes if state is outside a dm-crypt mapping. Never fall back to plaintext OS disk.
"""
import json, os, stat, subprocess, tempfile
from pathlib import Path
from threading import RLock
_lock=RLock()

def _store():
    state=Path(os.environ.get('DYNAMICOS_STATE_DIR',str(Path.home()/'.local/share/dynamicOS-NVR')))
    state.mkdir(parents=True,exist_ok=True,mode=0o700)
    os.chmod(state,0o700)
    try:
        src=subprocess.check_output(['findmnt','-n','-o','SOURCE','--target',str(state)],text=True,timeout=3).strip()
        kind=subprocess.check_output(['lsblk','-n','-o','TYPE',src],text=True,timeout=3,stderr=subprocess.DEVNULL).strip().splitlines()[0]
        if kind!='crypt':
            raise RuntimeError('Schlüsselablage muss auf einem LUKS/dm-crypt-Mount liegen')
    except (OSError,IndexError,subprocess.SubprocessError) as exc:
        raise RuntimeError('LUKS-Schlüsselablage nicht verifiziert') from exc
    return state/'sealed-secrets.json'

def _load(path):
    if not path.exists(): return {}
    if path.is_symlink():raise RuntimeError('Symlink als Schlüsselspeicher unzulässig')
    if stat.S_IMODE(path.stat().st_mode)&0o077:raise RuntimeError('Unsichere Schlüsseldateiberechtigung')
    return json.loads(path.read_text())

def get_password(service,username):
    with _lock:return _load(_store()).get(service,{}).get(username)

def set_password(service,username,password):
    with _lock:
        target=_store();data=_load(target)
        data.setdefault(service,{})[username]=password
        fd,tmp=tempfile.mkstemp(prefix='.secrets-',dir=target.parent)
        try:
            with os.fdopen(fd,'w') as f:
                os.fchmod(f.fileno(),0o600)
                json.dump(data,f);f.flush();os.fsync(f.fileno())
            os.replace(tmp,target)
        finally:
            if os.path.exists(tmp):os.unlink(tmp)
