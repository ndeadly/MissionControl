# Agent instructions — dynamicOS NVR

## Scope
Work only on the isolated branch `dynamicos-nvr-automation` or a short-lived feature branch based on it. This repository also contains unrelated Nintendo Switch code. **Do not modify, merge into, or deploy from the default branch without explicit human approval.**

## Target
- Debian 13 (trixie), amd64, Intel Mac mini Late 2012, optional headless NVR.
- Android companion (Gradle / Android API 36 compileSdk, minSdk 26).
- LAN-only ZOSI IP camera streaming, secure recorder vault and Android pairing.
- Current app is a development build. ISO is NOT a production, fully encrypted automated installer.

## Required autonomous CI repair loop
1. Inspect GitHub Actions runs for `continuous-nvr.yml` and `build-android.yml`, latest commit, artifacts and failing step logs.
2. Reproduce failure in an appropriate disposable CI environment. Identify a **specific root cause** before changing source.
3. Keep fixes minimal, never weaken TLS verification, secrets protection, login, encrypted-media checks or service isolation just to make CI green.
4. Run Python unittest discovery for `nvr/` and `tests/`, Python compileall and shell syntax checks.
5. Build Android debug APK with API 36; verify manifest/packaging and upload checksummed artifact.
6. Build Debian 13 amd64 iso-hybrid EFI *test image* with live-build; verify ISO structure, report_el_torito and SHA-256. Never label as an encrypted production installer without actual proof.
7. Open a reviewable pull request with modified files, root cause, test transcript, SHA-256 and remaining release gates; leave the production branch and hardware untouched.
8. On failure, preserve logs and surface concrete blockers; never claim a successful artifact without inspecting the Actions conclusion and uploaded artifact list.

## Environment safeguards
- Use temporary and isolated runners, no persistent secrets in repository or artifact.
- Do not automatically flash disks, repartition storage, modify boot firmware, expose cameras on the public internet or install on Mac mini.
- Do not turn off test failures, disable signature/hostname checking or skip certificate validation.
- Any mount-enabled privileged container is confined to ephemeral ISO compilation; never use privileged containers to handle production vault keys or live camera credentials.
- Mark unsigned debug APK and ISO as development/test-only.
- Require human review for changes to LUKS handling, encryption keys, authentication, systemd sandboxing, firewall rules, upgrades or retention/destructive deletion.

## Release gates
Real Intel Mac mini EFI/network/graphics/storage boot, LUKS2 setup/recovery, verified encrypted swap and backups, real ZOSI RTSP DESCRIBE profiles, 72h 1080p capture, concurrent Android live/seek/playback, power-fail recovery, offline token revocation, no uncontrolled WAN access and security review are all **unverified** until independently demonstrated.

## Current external blocker
The GitHub ISO run `45514598924` remained `in_progress` on 2026-11-25 while building image; investigate completion/timeout before launching repeated unnecessary builds.
