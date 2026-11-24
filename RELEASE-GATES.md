# dynamicOS NVR – release gates (v16)

All conditions must be verified on the target Mac mini and Android hardware before production use.

- [ ] Reproducible, boot-tested x86-64 EFI installer that provisions LUKS2 without exposing master keys.
- [ ] API service privilege separation, protected GUI login, and target-host verification of cross-process pairing/revocation (cross-process registry tested in CI-style unit tests).
- [ ] Full disk, backups, recovery key handling and signed/rollback-capable update path reviewed.
- [ ] RTSP stream path verified for every ZOSI model, measured 1080p frame rate and bitrate.
- [ ] 72-hour continuous recording and concurrent mobile 1080p playback, including power failures and network loss.
- [ ] Android signed APK built and tested with pinned certificate + SAN matching, camera live and archive playback.
- [ ] Nftables policy deployed in isolated camera VLAN; no Internet port forwarding; independent security assessment.

**Never call the source-only build production-ready.**
