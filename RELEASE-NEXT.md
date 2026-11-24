# dynamicOS NVR – remaining release actions (November 2026)

## Implemented and verified in this source package
- Shared persistent, atomic device-pairing registry, with process-wide locking.
- Revoked Android token denied by a separately instantiated API service without restart.
- Pairing attempt limits persist across API restarts; pairing code is one-time.
- 73 automated backend/appliance tests; compile and shell syntax checks in the developer environment.
- GitHub Actions source-verification workflow included but not executed against this commit.

## Mandatory before deployment
1. **Fix GUI/API server ownership**: only ONE mobile HTTPS listener must run in production, as an isolated managed service. GUI must exclusively manage pairing and configuration and not bind its own TCP server. Validate runtime user permissions and config synchronization between processes.
2. **ISO builder**: replace current Debian live/installer skeleton with a reproducible, automated, encrypted, UEFI-booted and recovery-tested install process. Existing build-iso.sh does NOT produce a configured appliance after installation.
3. **On-device security checks**: proper vault unlock/recovery, isolate camera network, nftables deny-by-default, login/authentication policy, secure boot constraints on 2012 Mac mini, audit and verified backup restores.
4. **Android**: compile and sign production APK, perform on-device TLS/SAN/pinning/Media3 playback tests and revoke during active streams.
5. **ZOSI**: confirm actual per-camera stream URLs, 1080p resolution and simultaneous 72h capture/playback at expected bitrates, power/network outage recovery.
6. **Defense-in-depth**: move plaintext temporary frame generation into sandboxed service; test untrusted video parser isolation; implement signed updates/rollback and controlled secret rotation.

**Production safety:** no boot ISO or APK is validated here. Do not replace macOS or deploy as sole security camera recorder based on these unit tests.
