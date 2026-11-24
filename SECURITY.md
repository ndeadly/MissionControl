# dynamicOS NVR v15 — Security boundaries

## Defender architecture
1. Private ZOSI camera network, ideally switch VLAN without public Internet route; restrict NVR outbound connections to camera subnet.
2. Capture engine in a dedicated unprivileged systemd service with filtered capabilities, namespaces, systemcalls, writable paths and IP address families. The GUI crash must not stop capture.
3. LUKS2 mounted data vault + authenticated per-recording AES-256-GCM streams; fail closed when the data vault is absent. Root/unlocked-vault compromise is out of scope of storage encryption.
4. Live HLS files only in bounded RAM tmpfs; authorized media endpoints over TLS. Android stores its token via Android Keystore.
5. Unattended distribution-security patches, with controlled maintenance reboot and verified recovery required.

## Deliberate limits / deployment blockers
- The GUI account can still access unlocked vault contents; privilege separation of GUI, API, and archive-decrypt worker remains an open hardening objective.
- Kiosk autologin is not a secure console login; implement authenticated lock and physical-access controls in a production installation.
- Application doesn't establish end-to-end encryption from ZOSI camera where RTSP itself is plaintext.
- Host firewall policy generator defaults to manual review; it is **not** automatically installed. Never replace existing firewall rules over remote SSH.
- Secure boot, signed immutable OS images, TPM-bound keys, recovery key lifecycle and independent security certification are NOT complete.
- Full HD performance depends on cameras and CPU/codec availability and requires hardware testing.
- Swap, backups, database snapshots and hibernation must be considered separately when configuring LUKS. No claim of secure SSD erasure.
- For installation, use a maintained Debian distribution and audited system services. Updating only application code cannot remediate a compromised kernel.
