# dynamicOS NVR v2 — clean build

A fresh build branch with no inherited NVR CI workflow definitions. The original source files are mounted in `projects/dynamicos-nvr/` using the exact original Git objects.

## Layout
- `projects/dynamicos-nvr/nvr/` — Python NVR backend and unit tests
- `projects/dynamicos-nvr/tests/` — appliance/security tests
- `projects/dynamicos-nvr/android/` — Android client
- `projects/dynamicos-nvr/appliance/` — Debian image and installer scripts
- `.github/workflows/dynamicos-nvr-v2.yml` — isolated CI

The ISO is a Debian live/test and manual-installer medium. It is not yet an approved LUKS-encrypted, hardware-tested production system. The Mac mini must not be overwritten until EFI boot, network, vault, power-loss recovery and camera integration have been verified on real hardware.

The build is triggered by commits to this branch or manual workflow dispatch. A `queued` GitHub run without any allocated jobs is not evidence of a compiling system. ISO builds can take significant time.
