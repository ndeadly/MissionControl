"""dynamicOS NVR: authenticated TLS API and HLS camera relay.

All Android live/recording traffic passes through the Mac; RTSP credentials never
leave the Mac. No public port mapping or third-party cloud services.
"""
from __future__ import annotations
import datetime as dt
import hashlib
import hmac
import ipaddress
import json
import os
import re
from collections import defaultdict, deque
from pathlib import Path
import secrets
import shutil
import subprocess
import threading
import time
from typing import Optional
from urllib.parse import urlsplit

from fastapi import Depends, FastAPI, Header, HTTPException, Request
from fastapi.responses import FileResponse, Response, StreamingResponse
from pydantic import BaseModel, Field
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.x509.oid import NameOID

from encrypted_media import decrypt_chunks, KeyProvider, VaultError
from backend import APP_DIR, ensure_private, ffmpeg_path, valid_stream_url, safe_name, load_config, save_config, EventStore, RecordingManager, probe_camera_profile

CERT_FILE = APP_DIR / 'nvr.crt'
KEY_FILE = APP_DIR / 'nvr.key'
CLIENT_FILE = APP_DIR / 'paired_devices.json'
HLS_ROOT = Path(os.environ.get('DYNAMICOS_HLS_DIR', '/run/dynamicos-nvr/hls' if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1' else str(APP_DIR / 'hls')))
PAIR_MAX_ATTEMPTS = 5
TOKEN_LIFETIME_DAYS = 30
CAMERA_ID_RE = re.compile(r"^[A-Za-z0-9_-]{1,80}$")


def permitted_peer(host: str, allowed_networks=None) -> bool:
    """Deny public peers, even with a valid token; never trust proxy headers."""
    if host in ("testclient", "localhost"):
        return True  # in-process tests / loopback
    try:
        ip = ipaddress.ip_address(host)
        if ip.is_loopback:
            return True
        networks = allowed_networks or ()
        return any(ip in ipaddress.ip_network(net, strict=False) for net in networks)
    except ValueError:
        return False


class PeerLimiter:
    """In-process request limiter; complements external firewall/DoS protections."""
    def __init__(self, limit=120, seconds=60):
        self.limit, self.seconds = limit, seconds
        self.lock = threading.Lock()
        self.tries = defaultdict(deque)

    def allowed(self, key):
        now = time.monotonic()
        with self.lock:
            q = self.tries[key]
            while q and q[0] < now - self.seconds:
                q.popleft()
            if len(q) >= self.limit:
                return False
            q.append(now)
            if len(self.tries) > 4096:
                for old in list(self.tries):
                    if not self.tries[old] or self.tries[old][-1] < now - self.seconds:
                        self.tries.pop(old, None)
            return True



def generate_certificate(hosts=('127.0.0.1',)):
    ensure_private(APP_DIR)
    if CERT_FILE.is_file() and KEY_FILE.is_file():
        # Never silently reuse a certificate for another IP address: Android
        # verifies both the SAN and the pinned certificate fingerprint.
        cert=x509.load_pem_x509_certificate(CERT_FILE.read_bytes())
        san=cert.extensions.get_extension_for_class(x509.SubjectAlternativeName).value
        existing_ips={str(ip) for ip in san.get_values_for_type(x509.IPAddress)}
        for host in hosts:
            try:
                if str(ipaddress.ip_address(host)) not in existing_ips:
                    raise RuntimeError('TLS certificate does not cover configured API address; rotate and re-pair devices explicitly')
            except ValueError:
                if host not in san.get_values_for_type(x509.DNSName):
                    raise RuntimeError('TLS certificate SAN mismatch; explicit key rotation required')
        return CERT_FILE, KEY_FILE
    key = ec.generate_private_key(ec.SECP256R1())
    now = dt.datetime.now(dt.timezone.utc)
    sans = [x509.DNSName('localhost')]
    for host in hosts:
        try: sans.append(x509.IPAddress(ipaddress.ip_address(host)))
        except ValueError: sans.append(x509.DNSName(host))
    cert = (x509.CertificateBuilder()
        .subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, 'dynamicOS NVR')]))
        .issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, 'dynamicOS NVR')]))
        .public_key(key.public_key()).serial_number(x509.random_serial_number())
        .not_valid_before(now - dt.timedelta(minutes=10))
        .not_valid_after(now + dt.timedelta(days=365))
        .add_extension(x509.SubjectAlternativeName(sans), critical=False)
        .add_extension(x509.BasicConstraints(ca=False,path_length=None), critical=True)
        .sign(key, hashes.SHA256()))
    for path, data in ((KEY_FILE, key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption())),
                       (CERT_FILE, cert.public_bytes(serialization.Encoding.PEM))):
        tmp = path.with_suffix('.tmp')
        tmp.write_bytes(data); os.chmod(tmp,0o600); os.replace(tmp,path)
    return CERT_FILE, KEY_FILE


def cert_fingerprint():
    cert=x509.load_pem_x509_certificate(CERT_FILE.read_bytes())
    return cert.fingerprint(hashes.SHA256()).hex().upper()


class PairRequest(BaseModel):
    code: str = Field(min_length=6, max_length=12)
    name: str = Field(min_length=1,max_length=80)

class DeviceStore:
    """Multiprocess-safe shared pairing and token registry.

    All reads and mutations use one file lock; revocation takes effect across
    GUI and API workers on the next request. One-use pairing codes and their
    failure counters survive an API process restart. Do not expose this state
    directory to unprivileged users.
    """
    def __init__(self, path=CLIENT_FILE):
        self.path=Path(path)
        ensure_private(self.path.parent)
        self._lockfile=self.path.with_name(self.path.name+'.lock')
        self._pairfile=self.path.with_name(self.path.name+'.pairing')
        self.lock=threading.RLock()
        self._devices={}
        self._reload()

    def _reload(self):
        try:
            data=json.loads(self.path.read_text())
            self._devices=data if isinstance(data,dict) else {}
        except (OSError,ValueError):
            self._devices={}

    @property
    def devices(self):
        # A copy, never the mutable internal dictionary.
        with self._locked():
            self._reload()
            return {k:v.copy() for k,v in self._devices.items()}

    def _read_pair(self):
        try:
            data=json.loads(self._pairfile.read_text())
            return data if isinstance(data,dict) else {}
        except (OSError,ValueError): return {}

    def _write_atomic(self,path,value):
        temp=path.with_name(path.name+'.'+secrets.token_hex(8)+'.tmp')
        try:
            with open(temp,'x',encoding='utf-8') as handle:
                os.chmod(temp,0o600)
                json.dump(value,handle,ensure_ascii=False)
                handle.flush();os.fsync(handle.fileno())
            os.replace(temp,path)
        finally:
            if temp.exists():temp.unlink()

    def _locked(self):
        from contextlib import contextmanager
        import fcntl
        @contextmanager
        def guard():
            with self.lock:
                fd=os.open(self._lockfile,os.O_CREAT|os.O_RDWR|os.O_NOFOLLOW,0o600)
                try:
                    with os.fdopen(fd,'rb') as handle:
                        fcntl.flock(handle.fileno(),fcntl.LOCK_EX)
                        yield
                finally:
                    pass
        return guard()

    def save(self):
        # Legacy public method: preserve the currently stored registry.
        with self._locked():
            self._reload()
            self._write_atomic(self.path,self._devices)

    def create_pairing(self,seconds=300):
        seconds=max(1,min(int(seconds),300))
        with self._locked():
            code=f'{secrets.randbelow(1000000):06d}'
            self._write_atomic(self._pairfile,{
                'hash':hashlib.sha256(code.encode()).hexdigest(),
                'expires':time.time()+seconds,'attempts':0})
            return code

    def pair(self,code,name):
        with self._locked():
            pairing=self._read_pair()
            if not pairing:return None
            attempts=int(pairing.get('attempts',0))+1
            if attempts>PAIR_MAX_ATTEMPTS or time.time()>float(pairing.get('expires',0)):
                self._write_atomic(self._pairfile,{})
                return None
            pairing['attempts']=attempts
            self._write_atomic(self._pairfile,pairing)
            if not hmac.compare_digest(hashlib.sha256(code.encode()).hexdigest(),str(pairing.get('hash',''))):
                return None
            self._write_atomic(self._pairfile,{})
            self._reload()
            token=secrets.token_urlsafe(48)
            device_id=secrets.token_hex(12)
            now=dt.datetime.now(dt.timezone.utc)
            self._devices[device_id]={
                'name':name,'token_hash':hashlib.sha256(token.encode()).hexdigest(),
                'paired':now.isoformat(),
                'expires_at':(now+dt.timedelta(days=TOKEN_LIFETIME_DAYS)).isoformat()}
            self._write_atomic(self.path,self._devices)
            return {'device_id':device_id,'token':token}

    def authenticated(self,token):
        if not isinstance(token,str) or not token:return None
        digest=hashlib.sha256(token.encode()).hexdigest()
        with self._locked():
            self._reload()
            now=dt.datetime.now(dt.timezone.utc)
            for ident,entry in self._devices.items():
                if not hmac.compare_digest(digest,entry.get('token_hash','')):continue
                try:
                    expires=dt.datetime.fromisoformat(entry['expires_at'])
                    if expires.tzinfo is None or expires<=now:return None
                except (ValueError,KeyError,TypeError):return None
                return ident
            return None

    def revoke(self,device_id):
        with self._locked():
            self._reload()
            if device_id not in self._devices:return False
            del self._devices[device_id]
            self._write_atomic(self.path,self._devices)
            return True

class LiveRelay:
    def __init__(self, base=HLS_ROOT):
        self.base=Path(base)
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1':
            from security_policy import require_ram_cache
            require_ram_cache(self.base)
        ensure_private(self.base)
        self.processes={}; self.lock=threading.RLock()
        self.failures={}  # camera_id -> (number_of_failures, retry_after_monotonic)

    def ensure(self, camera):
        cam_id=camera['id']
        with self.lock:
            record=self.processes.get(cam_id)
            if record and record['proc'].poll() is None:
                # A stalled but still-running FFmpeg process is terminated after 45 s
                # without a playlist; the next request can recover it.
                manifest=self.base/cam_id/'index.m3u8'
                if manifest.is_file() or time.monotonic()-record['started']<45:
                    return self.base/cam_id
                record['proc'].terminate()
                self.processes.pop(cam_id,None)
            failed=self.failures.get(cam_id)
            if failed and failed[1]>time.monotonic():
                raise RuntimeError('Kamera vorübergehend offline; Wiederverbindung läuft')
            if record and record['proc'].poll() is not None:
                attempts=self.failures.get(cam_id,(0,0))[0]+1
                self.failures[cam_id]=(min(attempts,7),time.monotonic()+min(60,2**attempts))
                self.processes.pop(cam_id,None)
                raise RuntimeError('Kamerastream offline; automatischer Neuversuch')
            if not safe_camera_source(camera['url']): raise ValueError('Nur private Kamera-IP-Adressen im LAN erlaubt')
            ffmpeg=ffmpeg_path()
            if not ffmpeg: raise RuntimeError('FFmpeg fehlt')
            folder=self.base/cam_id; ensure_private(folder)
            for file in folder.glob('*'):
                if file.is_file() and not file.is_symlink(): file.unlink()
            args=[ffmpeg,'-hide_banner','-nostdin','-loglevel','error']
            if camera['url'].lower().startswith('rtsp://'): args+=['-rtsp_transport','tcp','-timeout','8000000']
            args+=['-i',camera['url'],'-map','0:v:0','-an',
                    '-c:v','libx264','-preset','ultrafast','-tune','zerolatency','-pix_fmt','yuv420p','-f','hls','-hls_time','1','-hls_list_size','5',
                    '-hls_flags','delete_segments+omit_endlist+independent_segments+temp_file',
                    '-hls_segment_filename',str(folder/'seg_%06d.ts'),str(folder/'index.m3u8')]
            proc=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,start_new_session=True)
            self.processes[cam_id]={'proc':proc,'last_request':time.monotonic(),'started':time.monotonic()}
            self.failures.pop(cam_id,None)
            return folder

    def touch(self, camera_id):
        with self.lock:
            if camera_id in self.processes: self.processes[camera_id]['last_request']=time.monotonic()

    def reap(self, idle_seconds=120):
        with self.lock:
            for cid, record in list(self.processes.items()):
                if time.monotonic()-record['last_request']>idle_seconds or record['proc'].poll() is not None:
                    proc=record['proc']
                    if proc.poll() is None: proc.terminate()
                    del self.processes[cid]

    def stop(self):
        with self.lock:
            for r in self.processes.values():
                if r['proc'].poll() is None: r['proc'].terminate()
            self.processes.clear()

class NVRService:
    def __init__(self, config=None, devices=None, relay=None):
        self.cfg=config if config is not None else load_config()
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1':
            from security_policy import require_vault
            require_vault(self.cfg['record_dir'])
        self.devices=devices or DeviceStore()
        self.relay=relay or LiveRelay()
        self.events=EventStore()
        self.recording=RecordingManager(self.cfg,self.events)
        self.lock=threading.RLock()
        self._cfg_refresh_at=0

    def refresh_config(self):
        with self.lock:
            now=time.monotonic()
            if now-self._cfg_refresh_at>=2:
                updated=load_config()
                if os.environ.get('DYNAMICOS_APPLIANCE_MODE')=='1':
                    from security_policy import require_vault
                    require_vault(updated['record_dir'])
                self.cfg=updated
                self._cfg_refresh_at=now

    def camera(self,camera_id):
        return next((c for c in self.cfg['cameras'] if c.get('enabled',True) and c['id']==camera_id),None)

    def recordings(self):
        folder=Path(self.cfg['record_dir']).expanduser()
        output=[]
        if not folder.is_dir(): return output
        cameras={c['id']:c['name'] for c in self.cfg['cameras']}
        for file in folder.glob('*.dnvr'):
            if file.is_symlink() or not file.is_file(): continue
            cam_id=next((key for key in cameras if ('_'+key+'_') in file.name),None)
            if not cam_id:continue
            st=file.stat()
            output.append({'id':file.name,'camera_id':cam_id,'camera_name':cameras[cam_id],
                           'bytes':st.st_size,'timestamp':dt.datetime.fromtimestamp(st.st_mtime,dt.timezone.utc).isoformat()})
        return sorted(output,key=lambda x:x['timestamp'],reverse=True)

    def recording_path(self,record_id):
        if not record_id or len(record_id)>200 or Path(record_id).name!=record_id or not record_id.endswith('.dnvr'):
            return None
        if record_id not in {r['id'] for r in self.recordings()}:return None
        path=Path(self.cfg['record_dir']).expanduser()/record_id
        return path if path.is_file() and not path.is_symlink() else None


def safe_camera_source(url: str) -> bool:
    """Refuse public, loopback, DNS and link-local camera destinations (SSRF guard)."""
    if not valid_stream_url(url):
        return False
    try:
        parts=urlsplit(url)
        target=ipaddress.ip_address(parts.hostname or '')
        return (isinstance(target, ipaddress.IPv4Address)
                and target.is_private and not target.is_loopback
                and not target.is_link_local and not target.is_multicast
                and not target.is_unspecified and not target.is_reserved
                and any(target in block for block in (
                    ipaddress.ip_network('10.0.0.0/8'),
                    ipaddress.ip_network('172.16.0.0/12'),
                    ipaddress.ip_network('192.168.0.0/16')))
                and parts.port not in (0,))
    except ValueError:
        return False


def checked_allowed_networks(networks):
    """Accept only explicitly approved RFC1918 subnets, never 0.0.0.0/0."""
    result=[]
    for raw in networks:
        n=ipaddress.ip_network(raw, strict=False)
        if n.version!=4 or n.prefixlen<16 or not n.subnet_of(ipaddress.ip_network('10.0.0.0/8')) and not n.subnet_of(ipaddress.ip_network('172.16.0.0/12')) and not n.subnet_of(ipaddress.ip_network('192.168.0.0/16')):
            raise ValueError('Nur begrenzte lokale RFC1918-Netzbereiche sind erlaubt')
        result.append(str(n))
    return tuple(result)


def create_app(service:Optional[NVRService]=None, allowed_networks=None):
    svc=service or NVRService()
    app=FastAPI(title='dynamicOS NVR',docs_url=None,redoc_url=None,openapi_url=None)
    app.state.nvr=svc
    if allowed_networks is None:
        raw=os.environ.get('DYNAMICOS_ALLOWED_NETWORKS','')
        allowed_networks=[v.strip() for v in raw.split(',') if v.strip()]
    allowed_networks=checked_allowed_networks(allowed_networks)
    limiter=PeerLimiter(limit=180, seconds=60)
    pair_limiter=PeerLimiter(limit=8, seconds=600)

    def auth(authorization: str = Header(default='')):
        if not authorization.startswith('Bearer '): raise HTTPException(401,'Anmeldung erforderlich',headers={'WWW-Authenticate':'Bearer'})
        device=svc.devices.authenticated(authorization[7:])
        if not device:raise HTTPException(401,'Gerät nicht autorisiert')
        return device

    @app.middleware('http')
    async def security_headers(request:Request,call_next):
        peer=request.client.host if request.client else ''
        if not permitted_peer(peer,allowed_networks):
            return Response(status_code=403,content='Netzwerk nicht zugelassen')
        if not limiter.allowed(peer):
            return Response(status_code=429,content='Anfragelimit erreicht')
        if request.url.path == '/api/v1/pair' and request.method == 'POST' and not pair_limiter.allowed(peer):
            return Response(status_code=429,content='Kopplungsversuche begrenzt')
        if request.method in ('POST','PUT','PATCH'):
            length=request.headers.get('content-length','0')
            if not length.isdigit() or int(length)>4096:
                return Response(status_code=413,content='Anfrage zu groß')
        # Reflect local camera administration in the isolated API process.
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE')=='1' and request.url.path.startswith('/api/v1/'):
            try: svc.refresh_config()
            except Exception: return Response(status_code=503,content='Vault or configuration unavailable')
        response=await call_next(request)
        response.headers['Cache-Control']='no-store'
        response.headers['Pragma']='no-cache'
        response.headers['X-Frame-Options']='DENY'
        response.headers['Content-Security-Policy']="default-src 'none'; frame-ancestors 'none'"
        response.headers['Strict-Transport-Security']='max-age=31536000'
        response.headers['X-Content-Type-Options']='nosniff'
        response.headers['Referrer-Policy']='no-referrer'
        return response

    @app.get('/api/v1/health')
    def health(): return {'app':'dynamicOS NVR','status':'ready','tls_required':True}

    @app.post('/api/v1/pair')
    def pair(data:PairRequest,request:Request):
        result=svc.devices.pair(data.code,data.name)
        if not result: raise HTTPException(403,'Kopplungscode ungültig oder abgelaufen')
        svc.events.log('Android-Gerät gekoppelt',details=data.name)
        return result

    @app.get('/api/v1/cameras')
    def cameras(device=Depends(auth)):
        return [{'id':c['id'],'name':c.get('name','ZOSI'),'enabled':c.get('enabled',True)}
                for c in svc.cfg['cameras'] if c.get('enabled',True)]

    @app.get('/api/v1/cameras/{camera_id}/live/index.m3u8')
    def playlist(camera_id:str,device=Depends(auth)):
        if not CAMERA_ID_RE.fullmatch(camera_id):raise HTTPException(404)
        cam=svc.camera(camera_id)
        if not cam:raise HTTPException(404,'Kamera nicht gefunden')
        if not safe_camera_source(cam['url']):raise HTTPException(503,'Kameraziel im LAN nicht zugelassen')
        try: folder=svc.relay.ensure(cam)
        except (ValueError,RuntimeError,OSError) as e:raise HTTPException(503,str(e))
        svc.relay.touch(camera_id)
        path=folder/'index.m3u8'
        if not path.is_file():raise HTTPException(503,'Stream startet oder Kamera nicht erreichbar')
        return FileResponse(path,media_type='application/vnd.apple.mpegurl')

    @app.get('/api/v1/cameras/{camera_id}/live/{segment}')
    def segment(camera_id:str,segment:str,device=Depends(auth)):
        if not CAMERA_ID_RE.fullmatch(camera_id) or not svc.camera(camera_id):raise HTTPException(404)
        if not segment.startswith('seg_') or not segment.endswith('.ts') or not segment[4:-3].isdigit():raise HTTPException(404)
        svc.relay.touch(camera_id)
        file=svc.relay.base/camera_id/segment
        if not file.is_file() or file.is_symlink():raise HTTPException(404)
        return FileResponse(file,media_type='video/mp2t')

    @app.get('/api/v1/security')
    def security_status(device=Depends(auth)):
        # Never include camera secrets, certificate private key or token hashes.
        return {'internet_exposure':'not_configured_by_app',
                'lan_networks':list(allowed_networks),
                'server_encryption':'TLS', 'media_authentication':'bearer-token',
                'recordings_at_rest':'AES-256-GCM_HKDF_Keychain',
                'camera_transport':'RTSP_may_be_unencrypted',
                'paired_devices':len(svc.devices.devices)}

    @app.get('/api/v1/recordings')
    def recordings(device=Depends(auth)):return svc.recordings()

    @app.get('/api/v1/status')
    def status(device=Depends(auth)):
        return {'cameras':len([c for c in svc.cfg['cameras'] if c.get('enabled',True)]),
                'active_recordings':sum(1 for r in svc.recording.recorders.values() if r.running),
                'storage_bytes':sum(r['bytes'] for r in svc.recordings()),
                'record_mode':svc.cfg.get('record_mode','continuous'),
                'retention_hours':float(svc.cfg.get('retention_hours',int(svc.cfg.get('retention_days',14))*24)),
                'disk_free_bytes':shutil.disk_usage(svc.cfg['record_dir'] if Path(svc.cfg['record_dir']).exists() else str(Path.home())).free,
                'stream_protocol':'HLS/HTTPS', 'authentication':'paired-device'}

    @app.get('/api/v1/cameras/{camera_id}/diagnostics')
    def camera_diagnostics(camera_id:str,device=Depends(auth)):
        if not CAMERA_ID_RE.fullmatch(camera_id):raise HTTPException(404)
        cam=svc.camera(camera_id)
        if not cam:raise HTTPException(404)
        if not safe_camera_source(cam['url']):raise HTTPException(503,'Kameraziel nicht zugelassen')
        try:
            result=probe_camera_profile(cam['url'])
            result['camera_id']=camera_id
            result['target_1080p_met']=result['full_hd']
            return result
        except (RuntimeError,OSError):
            raise HTTPException(503,'Kamerastream nicht prüfbar')

    @app.get('/api/v1/cameras/{camera_id}/snapshot.jpg')
    def snapshot(camera_id:str,device=Depends(auth)):
        if not CAMERA_ID_RE.fullmatch(camera_id):raise HTTPException(404)
        cam=svc.camera(camera_id)
        if not cam:raise HTTPException(404,'Kamera nicht vorhanden')
        if not safe_camera_source(cam['url']):raise HTTPException(503,'Kameraziel im LAN nicht zugelassen')
        ffmpeg=ffmpeg_path()
        if not ffmpeg:raise HTTPException(503,'FFmpeg fehlt')
        args=[ffmpeg,'-hide_banner','-nostdin','-loglevel','error','-y']
        if cam['url'].lower().startswith(('rtsp://','rtsps://')):
            args+=['-rtsp_transport','tcp']
        args+=['-i',cam['url'],'-frames:v','1','-vf','scale=960:-2','-f','image2pipe','-vcodec','mjpeg','pipe:1']
        try:
            p=subprocess.run(args,stdin=subprocess.DEVNULL,capture_output=True,timeout=12,check=False)
        except (OSError,subprocess.TimeoutExpired):raise HTTPException(503,'Kamerabild derzeit nicht erreichbar')
        if p.returncode or not p.stdout or len(p.stdout)>4_000_000:
            raise HTTPException(503,'Kamerabild derzeit nicht erreichbar')
        return Response(content=p.stdout,media_type='image/jpeg')

    @app.get('/api/v1/recordings/{record_id}/play.mp4')
    def playback(record_id:str,device=Depends(auth)):
        path=svc.recording_path(record_id)
        if path is None:raise HTTPException(404,'Aufzeichnung nicht gefunden')
        ffmpeg=ffmpeg_path()
        if not ffmpeg:raise HTTPException(503,'FFmpeg fehlt')
        keys=KeyProvider()
        try:keys.master()  # fail closed if Keychain inaccessible
        except VaultError:raise HTTPException(503,'Aufzeichnungsschlüssel nicht verfügbar')
        # Never decrypt to disk. A worker decrypts into FFmpeg stdin, FFmpeg
        # streams fragmented MP4 over the existing authenticated TLS connection.
        args=[ffmpeg,'-hide_banner','-loglevel','error','-f','matroska','-i','pipe:0',
              '-map','0:v:0','-an','-c:v','libx264','-preset','veryfast','-crf','24',
              '-pix_fmt','yuv420p','-movflags','frag_keyframe+empty_moov+default_base_moof',
              '-f','mp4','pipe:1']
        try:proc=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL)
        except OSError:raise HTTPException(503,'Videokonvertierung nicht verfügbar')
        failures=[]
        def feed():
            try:
                from backend import playback_file_lock
                with playback_file_lock(path) as source:
                    for chunk in decrypt_chunks(source,keys):
                        proc.stdin.write(chunk)
            except (OSError,VaultError,BrokenPipeError) as exc:
                failures.append(type(exc).__name__)
            finally:
                try:proc.stdin.close()
                except OSError:pass
        worker=threading.Thread(target=feed,daemon=True);worker.start()
        def chunks():
            try:
                with svc.recording.protect_playback(path):
                    while True:
                        data=proc.stdout.read(64*1024)
                        if not data:break
                        yield data
            finally:
                if proc.poll() is None:
                    proc.terminate()
                    try:proc.wait(timeout=3)
                    except subprocess.TimeoutExpired:proc.kill();proc.wait()
                worker.join(timeout=3)
                if proc.stdout:proc.stdout.close()
        return StreamingResponse(chunks(),media_type='video/mp4',headers={
            'Cache-Control':'no-store, private','X-Content-Type-Options':'nosniff',
            'Content-Disposition':'inline'})

    @app.get('/api/v1/recordings/{record_id}')
    def recording(record_id:str,device=Depends(auth)):
        if svc.recording_path(record_id) is None:raise HTTPException(404)
        # No plaintext archive export: stream only through authenticated media endpoint.
        raise HTTPException(403,'Direkter Datei-Download aus Sicherheitsgründen deaktiviert')

    @app.get('/api/v1/events')
    def events(device=Depends(auth)):
        return [{'at':ts,'camera':cam,'type':category,'details':details} for ts,cam,category,details in svc.events.recent(100)]

    @app.get('/api/v1/devices')
    def devices(device=Depends(auth)):
        return [{'id':id,'name':d['name'],'paired':d['paired']} for id,d in svc.devices.devices.items()]

    @app.delete('/api/v1/devices/{device_id}')
    def revoke(device_id:str,device=Depends(auth)):
        if device_id!=device:raise HTTPException(403,'Andere Geräte nur lokal am Mac entkoppeln')
        svc.devices.revoke(device_id);return {'revoked':True}

    return app


if __name__=='__main__':
    import argparse
    import socket
    import uvicorn
    parser=argparse.ArgumentParser()
    parser.add_argument('--host',default='127.0.0.1', help='Standardmäßig nur lokal; LAN-IP explizit angeben')
    parser.add_argument('--allow-network',action='append',default=[],help='Zugelassenes CIDR, z.B. 192.168.2.0/24; mehrfach erlaubt')
    parser.add_argument('--port',type=int,default=8443)
    parser.add_argument('--pair',action='store_true')
    args=parser.parse_args()
    try: args.allow_network=list(checked_allowed_networks(args.allow_network))
    except ValueError as exc: parser.error(str(exc))
    if args.host not in ('127.0.0.1','::1','localhost') and not args.allow_network:
        parser.error('LAN-Bindung benötigt --allow-network: kein ungeschützter Netzwerkstart')
    generate_certificate(hosts=('127.0.0.1',args.host))
    svc=NVRService()
    if args.pair: print('KOPPLUNGSCODE:',svc.devices.create_pairing(),flush=True)
    print('TLS ZERTIFIKAT SHA256:',cert_fingerprint(),flush=True)
    print('API: https://<Mac-IP>:'+str(args.port),flush=True)
    stop=threading.Event()
    def ticker():
        while not stop.wait(5):
            try:svc.recording.tick();svc.relay.reap()
            except Exception as exc:print('NVR-Wartung:',type(exc).__name__,flush=True)
    threading.Thread(target=ticker,daemon=True).start()
    try: uvicorn.run(create_app(svc,allowed_networks=args.allow_network),host=args.host,port=args.port,
                 ssl_certfile=str(CERT_FILE),ssl_keyfile=str(KEY_FILE),server_header=False)
    finally:stop.set();svc.relay.stop();svc.recording.shutdown()
