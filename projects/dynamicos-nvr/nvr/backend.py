"""Local camera configuration, recording engine and SQLite events for dynamicOS NVR."""
from __future__ import annotations
import contextlib
import datetime as dt
import hashlib
import hmac
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import sqlite3
import subprocess
import sys
import threading
import time
try:
    import fcntl
except ImportError:
    fcntl = None
from urllib.parse import urlsplit, urlunsplit
from encrypted_media import encrypt_stream, decrypt_chunks, KeyProvider, VaultError
try:
    import appliance_keyring as keyring
except ImportError:
    keyring = None

APP_DIR = Path(os.environ.get('DYNAMICOS_STATE_DIR', str(Path.home() / '.local' / 'share' / 'dynamicOS-NVR'))) 
CONFIG = APP_DIR / 'settings.json'
DB = APP_DIR / 'events.sqlite3'
DEFAULT_RECORD_DIR = Path(os.environ.get('DYNAMICOS_RECORD_DIR', str(Path.home() / 'Videos' / 'dynamicOS-NVR'))) 


def ensure_private(path):
    path.mkdir(parents=True, exist_ok=True)
    with contextlib.suppress(OSError): os.chmod(path, 0o700)


def defaults():
    return {'version': 2, 'auth': None, 'cameras': [], 'record_dir': str(DEFAULT_RECORD_DIR),
            'retention_days': 14, 'retention_hours': 336, 'max_storage_gb': 0, 'record_mode': 'continuous',
            'schedule_start': '00:00', 'schedule_end': '23:59', 'motion_enabled': False,
            'motion_sensitivity': 18, 'snapshot_dir': str(DEFAULT_RECORD_DIR / 'Snapshots')}


def load_config():
    ensure_private(APP_DIR)
    cfg = defaults()
    if CONFIG.exists():
        try:
            loaded = json.loads(CONFIG.read_text(encoding='utf8'))
            if isinstance(loaded, dict): cfg.update(loaded)
        except (ValueError, OSError): pass
    if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1':
        cfg['record_dir'] = str(DEFAULT_RECORD_DIR)
        cfg['snapshot_dir'] = str(DEFAULT_RECORD_DIR / 'Snapshots')
    if not isinstance(cfg.get('cameras'), list): cfg['cameras'] = []
    for cam in cfg['cameras']:
        if 'id' not in cam: cam['id'] = secrets.token_hex(8)
        cam.setdefault('enabled', True)
        if cam.get('credential_ref'):
            if keyring is None:
                cam['enabled'] = False  # never fall back to plaintext credentials
                continue
            try:
                secret = keyring.get_password('dynamicOS NVR Camera', cam['credential_ref'])
                if secret: cam['url'] = secret
                else: cam['enabled'] = False
            except Exception: cam['enabled'] = False
    return cfg


def save_config(cfg):
    from security_policy import validate_appliance_config
    validate_appliance_config(cfg)
    ensure_private(APP_DIR)
    temp = CONFIG.with_suffix('.tmp')
    persisted = dict(cfg)
    persisted['cameras'] = []
    for cam in cfg.get('cameras', []):
        entry = dict(cam)
        url = entry.get('url', '')
        if url:
            parts = urlsplit(url)
            if parts.username is not None or parts.password is not None:
                if keyring is None:
                    raise RuntimeError('Linux-Schlüsselablage nicht verfügbar; Kameradaten nicht ungeschützt speichern')
                reference = entry.get('credential_ref') or entry['id']
                keyring.set_password('dynamicOS NVR Camera', reference, url)
                entry['credential_ref'] = reference
                host = parts.hostname or ''
                if ':' in host: host = '['+host+']'
                if parts.port: host += ':'+str(parts.port)
                entry['url'] = urlunsplit((parts.scheme,host,parts.path,parts.query,parts.fragment))
        persisted['cameras'].append(entry)
    with open(temp, 'w', encoding='utf8') as f:
        json.dump(persisted, f, indent=2, ensure_ascii=False)
        f.flush(); os.fsync(f.fileno())
    with contextlib.suppress(OSError): os.chmod(temp, 0o600)
    os.replace(temp, CONFIG)


def password_hash(password, salt):
    return hashlib.pbkdf2_hmac('sha256', password.encode('utf8'), bytes.fromhex(salt), 600000).hex()


def verify_password(auth, username, password):
    if not isinstance(auth, dict): return False
    try:
        return hmac.compare_digest(username, auth['user']) and hmac.compare_digest(
            hashlib.pbkdf2_hmac('sha256', password.encode('utf8'), bytes.fromhex(auth['salt']), int(auth.get('iterations',350000))).hex(), auth['hash'])
    except (KeyError, ValueError): return False


def new_auth(username, password):
    if not username.strip() or len(password) < 12:
        raise ValueError('Benutzername und mindestens 12 Zeichen Passwort erforderlich')
    salt = secrets.token_hex(16)
    return {'user': username.strip(), 'salt': salt, 'hash': password_hash(password, salt), 'iterations': 600000}


def valid_stream_url(raw):
    try:
        parts = urlsplit(raw.strip())
        return parts.scheme.lower() in ('rtsp', 'rtsps', 'http', 'https') and bool(parts.hostname)
    except ValueError: return False


def safe_name(name):
    result = re.sub(r'[^\w-]+', '_', name, flags=re.UNICODE).strip('_')
    return result[:64] or 'camera'


def ffmpeg_path():
    embedded = Path(getattr(sys, '_MEIPASS', '')) / 'ffmpeg' if getattr(sys, 'frozen', False) else None
    if embedded and embedded.is_file(): return str(embedded)
    return shutil.which('ffmpeg')


def ffprobe_path():
    embedded = Path(getattr(sys, '_MEIPASS', '')) / 'ffprobe' if getattr(sys, 'frozen', False) else None
    if embedded and embedded.is_file(): return str(embedded)
    return shutil.which('ffprobe')


def probe_camera_profile(url, *, timeout=12, probe_binary=None):
    """Inspect actual encoded main stream; never pretend upscaled video is Full-HD."""
    if not valid_stream_url(url):
        raise ValueError('Ungültiger Kamerastream')
    cmd = probe_binary or ffprobe_path()
    if not cmd:
        raise RuntimeError('FFprobe fehlt')
    args=[cmd,'-v','error']
    if url.lower().startswith('rtsp://'):
        args+=['-rtsp_transport','tcp']
    args+=['-select_streams','v:0','-show_entries',
           'stream=width,height,codec_name,avg_frame_rate', '-of','json',url]
    try:
        result=subprocess.run(args,stdin=subprocess.DEVNULL,capture_output=True,
                              timeout=timeout,check=False)
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError('Kameradiagnose: Zeitüberschreitung') from exc
    if result.returncode:
        raise RuntimeError('RTSP-Videostream nicht verfügbar: Kamera/URL prüfen')
    try:
        data=json.loads(result.stdout)
        stream=data['streams'][0]
        width=int(stream['width']);height=int(stream['height'])
        if width < 1 or height < 1:raise ValueError()
    except (ValueError,KeyError,TypeError,IndexError) as exc:
        raise RuntimeError('Kein gültiger Videostream erkennbar') from exc
    return {'width':width,'height':height,'codec':str(stream.get('codec_name','')),
            'frame_rate':str(stream.get('avg_frame_rate','')),
            'full_hd':width>=1920 and height>=1080}

class EventStore:
    def __init__(self, path=DB):
        ensure_private(Path(path).parent)
        self.path = path
        self.lock = threading.RLock()
        with self._conn() as conn:
            conn.execute('CREATE TABLE IF NOT EXISTS events (id INTEGER PRIMARY KEY, ts TEXT NOT NULL, camera TEXT, category TEXT NOT NULL, details TEXT NOT NULL)')
            conn.execute('CREATE INDEX IF NOT EXISTS idx_events_ts ON events(ts DESC)')

    @contextlib.contextmanager
    def _conn(self):
        conn = sqlite3.connect(self.path, timeout=10)
        try:
            with contextlib.suppress(OSError): os.chmod(self.path, 0o600)
            with conn:
                yield conn
        finally:
            conn.close()

    def log(self, category, camera='', details=''):
        with self.lock, self._conn() as conn:
            conn.execute('INSERT INTO events(ts,camera,category,details) VALUES(?,?,?,?)',
                         (dt.datetime.now().astimezone().isoformat(timespec='seconds'),camera,category,details))

    def recent(self, limit=250):
        with self.lock, self._conn() as conn:
            return conn.execute('SELECT ts,camera,category,details FROM events ORDER BY id DESC LIMIT ?', (int(limit),)).fetchall()

    def purge(self, days):
        cutoff = (dt.datetime.now().astimezone() - dt.timedelta(days=int(days))).isoformat()
        with self.lock, self._conn() as conn:
            conn.execute('DELETE FROM events WHERE ts < ?', (cutoff,))


def in_schedule(now, start, end):
    """Local time inclusive schedule; correctly handles midnight crossing."""
    try:
        a = dt.time.fromisoformat(start); b = dt.time.fromisoformat(end)
    except ValueError: return False
    current = now.time().replace(tzinfo=None)
    if a <= b: return a <= current <= b
    return current >= a or current <= b


def recording_files(folder):
    base = Path(folder).expanduser()
    if not base.exists(): return []
    return sorted((p for p in base.glob('*.dnvr') if p.is_file() and not p.is_symlink()), key=lambda p: p.stat().st_mtime, reverse=True)


@contextlib.contextmanager
def playback_file_lock(path, *, exclusive=False, nonblocking=False):
    """Cross-process lifetime lock: daemon cleanup cannot remove a playing file.

    On Linux uses advisory flock on the recording itself, so it requires no
    new state or lock files in a possibly locked vault.
    """
    stream = open(path, 'rb')
    try:
        if fcntl is not None:
            how = fcntl.LOCK_EX if exclusive else fcntl.LOCK_SH
            if nonblocking: how |= fcntl.LOCK_NB
            fcntl.flock(stream.fileno(), how)
        yield stream
    finally:
        stream.close()


def delete_unlocked(path):
    """Remove finalized file only when no local/remote reader holds a lock."""
    try:
        with playback_file_lock(path, exclusive=True, nonblocking=True):
            Path(path).unlink()
        return True
    except (OSError, BlockingIOError):
        return False


def cleanup_storage(folder, retention_days, max_storage_gb, active_paths=(), *, retention_hours=None, protected_paths=(), minimum_age_seconds=3600):
    """Delete only finalized encrypted recordings; never follow symlinks."""
    active = {str(Path(p).resolve()) for p in (*active_paths, *protected_paths)}
    if retention_hours is None:
        retention_hours = float(retention_days) * 24
    if not 0 <= float(retention_hours) <= 24 * 3650:
        raise ValueError('Ungültige Aufbewahrungsfrist')
    if not 0 <= float(max_storage_gb):
        raise ValueError('Ungültiges Speicherlimit')
    files = recording_files(folder)
    now = time.time()
    removed = []
    for p in files:
        if p.is_symlink() or str(p.resolve()) in active or now - p.stat().st_mtime < minimum_age_seconds: continue
        if retention_hours > 0 and now - p.stat().st_mtime > retention_hours * 3600:
            if delete_unlocked(p): removed.append(str(p))
    remaining = [p for p in recording_files(folder) if str(p.resolve()) not in active and not p.is_symlink() and now - p.stat().st_mtime >= minimum_age_seconds]
    max_bytes = max(0.0, float(max_storage_gb)) * (1024 ** 3)
    if max_bytes:
        all_recordings = recording_files(folder)
        total = sum(p.stat().st_size for p in all_recordings)
        for p in reversed(remaining):
            if total <= max_bytes: break
            size = p.stat().st_size
            if delete_unlocked(p): removed.append(str(p)); total -= size
    return removed


class Recorder:
    """Encrypted-at-source recorder: FFmpeg stdout goes directly to AES-GCM vault.

    A fixed-duration FFmpeg process produces one encrypted file. The manager
    starts the next segment on its next tick. No plaintext recording file is created.
    """
    def __init__(self, cam, folder, events, segment_seconds=900, key_provider=None):
        self.cam=cam;self.folder=Path(folder);self.events=events
        self.segment_seconds=segment_seconds;self.keys=key_provider or KeyProvider()
        self.proc=None;self.started_at=None;self.last_error=''
        self._lock=threading.RLock();self._worker=None;self._path=None
        self._stop_requested=False

    def _encrypt_worker(self,proc,path):
        temp=path.with_suffix('.dnvr.part')
        try:
            # Exclusive create prevents accidental overwrite; temp never contains plaintext.
            with open(temp,'xb') as out:
                os.chmod(temp,0o600)
                encrypt_stream(proc.stdout,out,self.keys)
                out.flush();os.fsync(out.fileno())
            if proc.stdout:proc.stdout.close()
            code=proc.wait(timeout=10)
            if code != 0 or self._stop_requested and temp.stat().st_size <= 80:
                raise VaultError('Kamera lieferte keinen gültigen Aufnahmestrom')
            temp.replace(path)
            self.events.log('Verschlüsselte Aufzeichnung gespeichert',self.cam['name'],path.name)
        except Exception as exc:
            self.last_error=str(exc)
            with contextlib.suppress(OSError):temp.unlink()
            self.events.log('Aufnahmefehler',self.cam['name'],'Verschlüsselung/Aufzeichnung fehlgeschlagen')
        finally:
            with contextlib.suppress(Exception):
                if proc.poll() is None:proc.kill();proc.wait(timeout=3)

    def start(self):
        with self._lock:
            if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1':
                from security_policy import require_vault
                try: require_vault(self.folder)
                except (RuntimeError, OSError, subprocess.SubprocessError) as exc:
                    self.last_error='Sicherheitsvault nicht verfügbar'; return False
            if self.running:return True
            if self._worker is not None and self._worker.is_alive():return False
            binary=ffmpeg_path()
            if not binary:self.last_error='FFmpeg nicht gefunden';return False
            if not valid_stream_url(self.cam['url']):self.last_error='Ungültige Stream-URL';return False
            # Fail CLOSED before recording starts if Keychain is unavailable.
            try:self.keys.master()
            except VaultError as exc:self.last_error=str(exc);return False
            ensure_private(self.folder)
            prefix=safe_name(self.cam['name'])+'_'+self.cam['id']+'_'
            path=self.folder/(prefix+dt.datetime.now().strftime('%Y%m%d_%H%M%S')+'_'+secrets.token_hex(4)+'.dnvr')
            args=[binary,'-nostdin','-hide_banner','-loglevel','error']
            if self.cam['url'].lower().startswith('rtsp://'):
                args.extend(['-rtsp_transport','tcp'])
            args.extend(['-i',self.cam['url'],'-map','0:v:0','-an','-c:v','copy',
                         '-t',str(self.segment_seconds),'-f','matroska','pipe:1'])
            try:
                proc=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,
                                     stderr=subprocess.DEVNULL,start_new_session=True)
            except OSError as exc:self.last_error=str(exc);return False
            self.proc=proc;self._path=path;self._stop_requested=False
            self._worker=threading.Thread(target=self._encrypt_worker,args=(proc,path),daemon=True)
            self._worker.start();self.started_at=time.time()
            self.events.log('Verschlüsselte Aufzeichnung gestartet',self.cam['name'])
            return True

    @property
    def running(self):
        return self.proc is not None and self.proc.poll() is None

    def stop(self):
        with self._lock:
            if self.proc is None:return
            proc=self.proc;self._stop_requested=True
            if proc.poll() is None:
                proc.terminate()
                try:proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill();proc.wait(timeout=3)
            if self._worker is not None:self._worker.join(timeout=10)
            self.proc=None
            self.events.log('Aufzeichnung gestoppt',self.cam['name'])


class RecordingManager:
    def __init__(self, config, events):
        self.cfg = config; self.events = events; self.recorders = {}
        self.manual_ids = set(); self.last_cleanup = 0
        self.protected_recordings = set()
        self._protected_lock = threading.RLock()

    def sync_cameras(self):
        current = {c['id']: c for c in self.cfg['cameras'] if c.get('enabled', True)}
        for key in list(self.recorders):
            rec = self.recorders[key]
            if key not in current or rec.cam['url'] != current[key]['url'] or rec.folder != Path(self.cfg['record_dir']):
                rec.stop(); del self.recorders[key]; self.manual_ids.discard(key)
        for key, cam in current.items():
            if key not in self.recorders:
                self.recorders[key] = Recorder(cam, self.cfg['record_dir'], self.events)
            else: self.recorders[key].cam = cam

    def toggle_manual(self, cam_id):
        self.sync_cameras()
        if cam_id in self.manual_ids:
            self.manual_ids.remove(cam_id)
            rec = self.recorders.get(cam_id)
            if rec and self.cfg['record_mode'] == 'manual': rec.stop()
            return False
        self.manual_ids.add(cam_id)
        return True

    @contextlib.contextmanager
    def protect_playback(self, path):
        # Playback and retention cleanup coordinate on the same lock.
        resolved = str(Path(path).resolve())
        with playback_file_lock(path):
            with self._protected_lock:
                self.protected_recordings.add(resolved)
            try:
                yield
            finally:
                with self._protected_lock:
                    self.protected_recordings.discard(resolved)

    def tick(self):
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1':
            from security_policy import require_vault
            try: require_vault(self.cfg['record_dir'])
            except Exception:
                for rec in self.recorders.values(): rec.stop()
                self.events.log('Sicherheitsalarm', '', 'Vault nicht verifiziert; Aufzeichnung gestoppt')
                return
        self.sync_cameras()
        mode = self.cfg.get('record_mode', 'continuous')
        scheduled = in_schedule(dt.datetime.now(), self.cfg.get('schedule_start','00:00'), self.cfg.get('schedule_end','23:59'))
        for key, rec in self.recorders.items():
            wanted = key in self.manual_ids or mode == 'continuous' or (mode == 'scheduled' and scheduled)
            if wanted and not rec.running:
                # Retry on next tick for disconnected camera.
                if not rec.start(): self.events.log('Aufnahmefehler', rec.cam['name'], rec.last_error)
            if not wanted and rec.running: rec.stop()
        if time.monotonic() - self.last_cleanup >= 300:
            self.last_cleanup = time.monotonic()
            with self._protected_lock:
                deleted = cleanup_storage(self.cfg['record_dir'], int(self.cfg.get('retention_days',14)),
                                          float(self.cfg.get('max_storage_gb',0)),
                                          retention_hours=float(self.cfg.get('retention_hours', int(self.cfg.get('retention_days',14))*24)),
                                          protected_paths=tuple(self.protected_recordings))
            if deleted:
                for file in deleted: self.events.log('Aufzeichnung automatisch gelöscht','',Path(file).name)
            self.events.purge(max(30, int(self.cfg.get('retention_days', 14))))

    def shutdown(self):
        for rec in self.recorders.values(): rec.stop()
