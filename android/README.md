# dynamicOS NVR Android

Native Android secure client using Media3 HLS and OkHttp with exact certificate fingerprint matching. The Mac mini provides an authenticated HTTPS gateway. The client does not connect directly to ZOSI RTSP cameras and does not store camera passwords.

Pair via server URL, SHA-256 certificate fingerprint, and time-limited pairing code displayed by the Mac.

For Android SDK 35 / JDK17 use `gradle --no-daemon :app:assembleDebug`. Build on GitHub Actions is provided at repository root. No local SDK/Gradle compilation is claimed in this Linux session.
