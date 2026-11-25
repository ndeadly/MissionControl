"""One-time offline legacy MKV migration (no plaintext temporary output).

Run with --confirm after backups are reviewed. Existing plain files are removed
only after full authenticated verification of their encrypted replacement.
SSD snapshots/backups may retain deleted plaintext; address separately.
"""
import argparse
from pathlib import Path
from encrypted_media import encrypt_file, decrypt_chunks, KeyProvider, VaultError

def migrate(folder,confirm=False,provider=None):
    provider=provider or KeyProvider()
    provider.master()  # avoid deletion if keychain inaccessible
    paths=sorted(Path(folder).expanduser().glob('*.mkv'))
    if not confirm:return {'pending':[p.name for p in paths],'converted':[]}
    completed=[]
    for source in paths:
        if source.is_symlink() or not source.is_file():continue
        dest=source.with_suffix('.dnvr')
        if dest.exists():raise FileExistsError(dest)
        encrypt_file(source,dest,provider)
        try:
            with open(dest,'rb') as f:
                digest=__import__('hashlib').sha256()
                for chunk in decrypt_chunks(f,provider):digest.update(chunk)
            with open(source,'rb') as original_file:
                original=__import__('hashlib').file_digest(original_file,'sha256').digest()
            if digest.digest()!=original:raise VaultError('Verifizierung fehlgeschlagen')
            source.unlink()
            completed.append(dest.name)
        except Exception:
            dest.unlink(missing_ok=True)
            raise
    return {'pending':[],'converted':completed}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('folder');p.add_argument('--confirm',action='store_true')
    args=p.parse_args()
    print(migrate(args.folder,args.confirm))
