# dynamicOS NVR macOS

See [../README.md](../README.md) for architecture, pairing and security limitations.

Start desktop: `python nvr.py` after installing dependencies.

The desktop automatically starts an HTTPS API server on TCP 8443. Create and revoke Android pairing in the macOS desktop interface. The direct camera transport stays within the LAN. RTSP support depends on the exact ZOSI camera firmware and codec.

Camera usernames/passwords embedded in RTSP URLs are migrated to macOS Keychain when saving the configuration. The application's recording directory contains unencrypted video; enable FileVault and keep physical storage and user account protected.

Run the backend suite with `python -m unittest discover -v`.
