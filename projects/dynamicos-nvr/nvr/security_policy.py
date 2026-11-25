"""Fail-closed storage policy for a dedicated NVR appliance."""
import os
from pathlib import Path
import subprocess

VAULT=Path('/var/lib/dynamicos-nvr')

def _is_beneath(path,root):
    try: path.relative_to(root);return True
    except ValueError:return False

def require_vault(directory, *, vault=None, mount_source=None, device_type=None):
    """Verify actual dm-crypt mount and prohibit accidental recording elsewhere.
    Injected mount data is solely for offline tests, never the normal caller.
    """
    root=Path(vault or VAULT).resolve(strict=True)
    dest=Path(directory).resolve(strict=True)
    if not _is_beneath(dest,root) or dest==root:
        raise RuntimeError('Aufzeichnungsverzeichnis liegt außerhalb des Sicherheitsvaults')
    if mount_source is None:
        mount_source=subprocess.check_output(['findmnt','-n','-o','SOURCE','--mountpoint',str(root)],text=True,timeout=4).strip()
    if not mount_source or not mount_source.startswith('/dev/'):
        raise RuntimeError('LUKS-Vault nicht eingehängt')
    if device_type is None:
        device_type=subprocess.check_output(['lsblk','-n','-o','TYPE',mount_source],text=True,timeout=4).strip().splitlines()[0]
    if device_type != 'crypt':
        raise RuntimeError('Vault ist kein dm-crypt-Gerät')
    return dest

def validate_appliance_config(cfg):
    if os.environ.get('DYNAMICOS_APPLIANCE_MODE')!='1':return
    require_vault(cfg['record_dir'])
    require_vault(cfg.get('snapshot_dir',cfg['record_dir']))


def require_ram_cache(directory, *, fs_type=None, runtime_root=Path('/run/dynamicos-nvr')):
    """Fail closed if HLS files could land on the persistent root filesystem."""
    path=Path(directory).resolve(strict=False)
    root=Path(runtime_root).resolve(strict=False)
    if path != root/'hls':
        raise RuntimeError('Livecache liegt nicht im ausschließlich zugelassenen Laufzeitverzeichnis')
    if fs_type is None:
        fs_type=subprocess.check_output(['findmnt','-n','-o','FSTYPE','--mountpoint',str(path)],text=True,timeout=4).strip()
    if fs_type != 'tmpfs':
        raise RuntimeError('Livecache benötigt tmpfs/RAM statt unverschlüsselter Festplatte')
    return path
