# Separate API sandbox – opt-in only

`dynamicos-api.service` is **disabled** by default. It binds only to 127.0.0.1 and is NOT exposed to Android unless explicitly configured and firewall-reviewed. Do not run the GUI embedded HTTPS listener and this systemd service on the same address/port.

For headless mobile access, set `/etc/dynamicos-nvr/api.env` to `DYNAMICOS_API_HOST=192.168.x.x` and `DYNAMICOS_ALLOWED_NETWORKS=192.168.x.0/24`, with root-only permissions. Review nftables/host firewall and VLAN first. The Android client must confirm both SAN and SHA-256 pinned certificate. Certificate regeneration/re-pairing needs a managed maintenance window. The GUI and separate API now share an atomically updated, cross-process locked registry under DYNAMICOS_STATE_DIR. Pairing codes, token creation and revocations are synchronized. **Important:** the GUI still embeds its own TLS server; do not activate the LAN API in both GUI and systemd simultaneously. The dedicated service remains opt-in until its network access, startup order and authentication are verified on the target device.

## Managed service ownership (v18)

The GUI does not bind a port in appliance mode. Only `dynamicos-api.service`
serves Android. The API defaults to `127.0.0.1` and is not enabled by install.
Configure `/etc/dynamicos-nvr/api.env` as root with a static LAN IP in
`DYNAMICOS_API_HOST` and an explicitly approved CIDR in
`DYNAMICOS_ALLOWED_NETWORKS`. Configure a default-deny firewall rule for port
8443 and verify it from an authorized Android device before enabling the service.

Changing the interface address with existing paired clients requires an
**explicit TLS certificate rotation and device re-pairing**. The API now rejects
certificates whose SAN lacks the configured address rather than silently serving
a mismatched certificate.
