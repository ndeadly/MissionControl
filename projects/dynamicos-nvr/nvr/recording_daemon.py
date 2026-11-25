"""Headless 24/7 recorder; systemd, not the Qt session, owns capture lifetime."""
import json
import logging
import os
from pathlib import Path
import signal
import time

from backend import APP_DIR, CONFIG, EventStore, RecordingManager, load_config, ensure_private
from security_policy import require_vault

RUN = True

def shutdown(*_):
    global RUN
    RUN = False

def atomic_status(status):
    ensure_private(APP_DIR)
    dst = APP_DIR / 'recorder-status.json'
    tmp = dst.with_suffix('.tmp')
    with open(tmp, 'w', encoding='utf-8') as stream:
        json.dump(status, stream)
        stream.flush()
        os.fsync(stream.fileno())
    os.chmod(tmp, 0o600)
    os.replace(tmp, dst)

def main():
    if os.environ.get('DYNAMICOS_APPLIANCE_MODE') != '1':
        raise RuntimeError('Recording daemon must run in sealed appliance mode')
    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)
    events=EventStore()
    cfg=load_config()
    require_vault(cfg['record_dir'])
    manager=RecordingManager(cfg,events)
    last_conf=CONFIG.stat().st_mtime_ns if CONFIG.exists() else 0
    try:
        while RUN:
            try:
                require_vault(cfg['record_dir'])
                new_conf=CONFIG.stat().st_mtime_ns if CONFIG.exists() else 0
                if new_conf != last_conf:
                    updated=load_config()
                    require_vault(updated['record_dir'])
                    manager.shutdown()
                    manager=RecordingManager(updated,events)
                    cfg=updated
                    last_conf=new_conf
                manager.tick()
                atomic_status({'state':'recording', 'running':sum(r.running for r in manager.recorders.values()),
                               'configured':len([c for c in cfg['cameras'] if c.get('enabled',True)]),
                               'epoch':int(time.time())})
            except Exception as exc:
                manager.shutdown()
                atomic_status({'state':'degraded','running':0,'epoch':int(time.time()),'error':type(exc).__name__})
                logging.exception('Recorder error (fail-closed)')
            for _ in range(20):
                if not RUN:break
                time.sleep(0.5)
    finally:
        manager.shutdown()
        atomic_status({'state':'stopped','running':0,'epoch':int(time.time())})

if __name__=='__main__': main()
