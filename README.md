# dynamicOS NVR v16 – hardening build candidate (Intel x86-64)

**Development release. NOT production-ready and NOT an installable finished operating system.**

Hardware: Mac mini late 2012, 4-core Intel Core i7, 16 GiB RAM; Debian 13 amd64, no conventional desktop. ZOSI network cameras connect directly over LAN without ZOSI DVR. The encrypted recorder is independent of the display session. Android code is present but no signed APK has been built in this environment.

## Changes from v15

- Native Android TLS now requires *both* exact SHA-256 certificate pin and conventional hostname/SAN verification; the prior insecure hostname-verifier override was removed.
- Server initialization rejects a missing encrypted LUKS recording vault in appliance mode.
- Health monitoring fails closed when no enabled cameras exist, instead of reporting an empty system as healthy.
- Additional sandboxed mobile API service definition is available as **opt-in**, disabled by default pending device pairing synchronization integration.
- Automated regression tests are included under `tests/` and `nvr/`.

## Run checks (development host)

```sh
python3 -m unittest discover -s tests -p 'test_*.py'
PYTHONPATH=nvr python3 -m unittest discover -s nvr -p 'test_*.py'
python3 appliance/audit-sandbox.py
```

## Pending hard release blockers

- A completed and *tested* fully encrypted disk installer/boot ISO for Mac mini EFI.
- A successful Android Gradle build, APK signing and device-run validation.
- Camera codec/RTSP interoperability and 1080p live + continuous 72-hour burn-in.
- Independently audited secret custody, API privilege separation / persistent pairing synchronization, threat model and backup recovery drills.
- On-target unit and LUKS mounting checks, firewall and local Linux authenticated kiosk escape tests.

Do not install this build as an unattended security system. See RELEASE-GATES.md.


## Integration update – v17 shared device identity

Cross-process synchronized pairing, token registry and revocation are now implemented using an atomic state file protected by a Linux advisory file lock. The API and GUI require the **same** `DYNAMICOS_STATE_DIR` and Unix account privileges. The dedicated API service remains disabled until the endpoint collision with the GUI's embedded server is resolved and hardware deployment is verified. New tests cover separate processes, restarts and pairing attempt exhaustion. This is **not** an approved production release.
