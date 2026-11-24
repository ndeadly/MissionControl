"""Direct LAN integration for ZOSI IP cameras; no ZOSI recorder required.

ONVIF WS-Discovery discovers only cameras that announce themselves.
RTSP path candidates are model-dependent and must be checked before saving.
"""
from __future__ import annotations
from concurrent.futures import ThreadPoolExecutor, as_completed
import ipaddress
import re
import socket
import subprocess
import uuid
from urllib.parse import quote, urlsplit, urlunsplit
import xml.etree.ElementTree as ET

MULTICAST = ('239.255.255.250', 3702)
PROBE = '''<?xml version="1.0" encoding="UTF-8"?>
<e:Envelope xmlns:e="http://www.w3.org/2003/05/soap-envelope"
xmlns:w="http://schemas.xmlsoap.org/ws/2004/08/addressing"
xmlns:d="http://schemas.xmlsoap.org/ws/2005/04/discovery">
<e:Header><w:MessageID>urn:uuid:{id}</w:MessageID><w:To e:mustUnderstand="true">urn:schemas-xmlsoap-org:ws:2005:04:discovery</w:To>
<w:Action e:mustUnderstand="true">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</w:Action></e:Header>
<e:Body><d:Probe><d:Types>dn:NetworkVideoTransmitter</d:Types></d:Probe></e:Body></e:Envelope>'''


def private_ip(host):
    try:
        ip = ipaddress.ip_address(host)
        return isinstance(ip, ipaddress.IPv4Address) and ip.is_private and not (ip.is_loopback or ip.is_link_local or ip.is_multicast)
    except ValueError:
        return False


def discover_onvif(timeout=3.0):
    """Multicast WS-Discovery. No authentication and no external network traffic."""
    found = {}
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    try:
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
        sock.settimeout(min(max(float(timeout), 0.25), 8))
        sock.sendto(PROBE.format(id=uuid.uuid4()).encode(), MULTICAST)
        import time
        until = time.monotonic() + min(max(float(timeout), 0.25), 8)
        while time.monotonic() < until:
            sock.settimeout(max(0.05, until - time.monotonic()))
            try: data, src = sock.recvfrom(65535)
            except socket.timeout: break
            if not private_ip(src[0]): continue
            try:
                root = ET.fromstring(data)
                xaddrs = [x.text or '' for x in root.iter() if x.tag.endswith('XAddrs')]
                scopes = [x.text or '' for x in root.iter() if x.tag.endswith('Scopes')]
            except ET.ParseError: continue
            found[src[0]] = {'ip': src[0], 'onvif_addresses': xaddrs, 'scopes': scopes}
    finally: sock.close()
    return sorted(found.values(), key=lambda c: tuple(int(i) for i in c['ip'].split('.')))


def port_open(ip, port=554, timeout=0.35):
    if not private_ip(ip): return False
    try:
        with socket.create_connection((ip, int(port)), timeout=timeout): return True
    except OSError: return False


def scan_local_subnet(cidr, ports=(554, 80, 8899), max_hosts=254):
    """Explicitly scan only one private IPv4 LAN /24 or smaller, not the Internet."""
    network = ipaddress.ip_network(cidr, strict=False)
    if network.version != 4 or not network.is_private or network.num_addresses > max_hosts + 2:
        raise ValueError('Nur ein privates LAN mit höchstens 254 Hosts ist erlaubt')
    hosts = [str(x) for x in network.hosts()]
    output = []
    def check(host):
        opened = [p for p in ports if port_open(host, p)]
        return {'ip': host, 'ports': opened} if opened else None
    with ThreadPoolExecutor(max_workers=32) as pool:
        futures = [pool.submit(check, host) for host in hosts]
        for future in as_completed(futures):
            item = future.result()
            if item: output.append(item)
    return sorted(output, key=lambda c: tuple(int(i) for i in c['ip'].split('.')))


def rtsp_candidates(ip, username='', password='', port=554):
    """Known ZOSI candidate /video1 plus alternatives requiring on-device verification."""
    if not private_ip(ip): raise ValueError('Eine lokale IPv4-Kameraadresse ist erforderlich')
    credentials = ''
    if username: credentials = quote(username, safe='') + ':' + quote(password, safe='') + '@'
    authority = f'{credentials}{ip}:{int(port)}'
    return [f'rtsp://{authority}{path}' for path in ('/video1', '/video2', '/stream1', '/stream2')]


def redact_url(url):
    try:
        u = urlsplit(url)
        host = u.hostname or ''
        authority = ('***:***@' if u.username else '') + host + (f':{u.port}' if u.port else '')
        return urlunsplit((u.scheme, authority, u.path, u.query, u.fragment))
    except ValueError: return '<ungültige URL>'


def probe_rtsp(url, ffprobe='ffprobe', timeout=7):
    """Real stream test via FFprobe: return metadata, never fabricate success."""
    u = urlsplit(url)
    if u.scheme != 'rtsp' or not private_ip(u.hostname or ''):
        return {'ok': False, 'error': 'Nur RTSP im lokalen privaten LAN erlaubt'}
    command = [ffprobe, '-v', 'error', '-rtsp_transport', 'tcp', '-show_entries',
               'stream=codec_name,width,height', '-select_streams', 'v:0', '-of', 'default=noprint_wrappers=1', url]
    try:
        p = subprocess.run(command, capture_output=True, text=True, timeout=max(2, min(timeout, 20)))
    except (OSError, subprocess.TimeoutExpired) as exc:
        return {'ok': False, 'error': type(exc).__name__}
    if p.returncode != 0 or 'codec_name=' not in p.stdout:
        return {'ok': False, 'error': 'Stream nicht erreichbar oder keine Video-Daten (Zugangsdaten/Pfad prüfen)'}
    return {'ok': True, 'details': p.stdout.strip()[:350]}
