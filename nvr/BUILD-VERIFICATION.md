# Build verification

GitHub Actions is triggered by this committed source-tree verification marker.

Requested checks: Python unit tests, Android debug APK, Debian ISO. Artifacts are development builds; hardware validation and product approval remain separate.

## Release build execution

Build chain: source verification → Android debug APK → Debian 13 test ISO. ISO must be verified with xorriso and SHA-256; hardware approval is separate.

