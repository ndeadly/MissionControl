# dynamicOS NVR v18 integration gate

This is **source code, not an installable production appliance**.

## Verified in development container
- GUI does not open any HTTPS listener in appliance mode.
- The separately managed mobile API binds to localhost by default and remains disabled on first install.
- Camera configuration refresh is performed in the appliance API without starting a recording loop.
- TLS certificate reuse fails closed if the configured IP address is missing from its SAN.
- API device revocation state is shared with GUI on a mounted protected state volume.
- 76 Python regression tests (45 backend, 31 appliance) passed on Linux development host.

## Not verified – no release allowed
- EFI boot on late-2012 Mac mini, fully encrypted installer and LUKS recovery.
- Audio/video RTSP 1080p on actual ZOSI LAN cameras, 72h endurance, live streaming and archival playback.
- Android APK Gradle build, signature and physical device testing.
- User-authenticated boot console (LightDM autologin remains in current source).
- Android stream revocation under ongoing media sessions.
- Independent security penetration testing, backup restoration and cryptographic audit.

**Do not install as sole security system or destroy an existing Mac installation.**
