#!/bin/bash
set -Eeuo pipefail
# Non-destructive installation of the software layer ONLY.
# Debian must already be installed and /var/lib/dynamicos-nvr must be a mounted LUKS volume.
[[ $(id -u) -eq 0 ]] || { echo 'Als root ausführen' >&2;exit 1; }
REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
[[ -d "$REPO/nvr" ]] || { echo 'Quellcode fehlt' >&2;exit 1; }
if ! mountpoint -q /var/lib/dynamicos-nvr;then
 echo 'ABBRUCH: LUKS-Ziel /var/lib/dynamicos-nvr ist kein eigener Mountpoint.' >&2;exit 2
fi
src=$(findmnt -n -o SOURCE --target /var/lib/dynamicos-nvr)
[[ "$(lsblk -n -o TYPE "$src" | head -n1)" == crypt ]] || { echo 'ABBRUCH: Volume nicht dm-crypt/LUKS' >&2;exit 2; }
if [[ "$(findmnt -n -o FSTYPE --target /run)" != tmpfs ]]; then
 echo 'ABBRUCH: /run ist nicht tmpfs' >&2; exit 2
fi
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends python3 python3-venv python3-pip ffmpeg \
 xserver-xorg-core xserver-xorg-input-libinput xinit lightdm dbus-x11 \
 ca-certificates cryptsetup network-manager unattended-upgrades \
 nftables openssh-client pciutils util-linux procps libegl1 libgl1 libxkbcommon-x11-0 libxcb-cursor0 libxcb-xinerama0 fonts-dejavu-core
id dynamicos >/dev/null 2>&1 || useradd --create-home --shell /bin/bash --user-group dynamicos
# Password-login disabled for dedicated graphical user; key-based SSH access must be denied by separate SSH policy.
passwd -l dynamicos >/dev/null 2>&1 || true
install -d -o dynamicos -g dynamicos -m 0700 /var/lib/dynamicos-nvr/state /var/lib/dynamicos-nvr/recordings
# /run is tmpfs on Debian with systemd; no HLS segments on persistent disk.
[[ "$(findmnt -n -o FSTYPE --target /run)" == 'tmpfs' ]] || { echo 'ABBRUCH: /run nicht tmpfs' >&2;exit 2; }
install -d -m 0755 /opt/dynamicos-nvr
cp -a "$REPO/nvr" "$REPO/appliance" /opt/dynamicos-nvr/
# Code is root-owned and cannot be modified by the recorder or kiosk account.
chown -R root:root /opt/dynamicos-nvr
chmod -R go-w /opt/dynamicos-nvr
python3 /opt/dynamicos-nvr/appliance/audit-sandbox.py
python3 -m venv /opt/dynamicos-nvr/venv
/opt/dynamicos-nvr/venv/bin/pip install --no-cache-dir -r /opt/dynamicos-nvr/appliance/requirements-linux.txt
install -m 0644 /opt/dynamicos-nvr/appliance/dynamicos.desktop /usr/share/xsessions/dynamicos.desktop
install -d -m 0755 /etc/lightdm/lightdm.conf.d
install -m 0644 /opt/dynamicos-nvr/appliance/lightdm.conf /etc/lightdm/lightdm.conf.d/50-dynamicos.conf
install -m 0644 /opt/dynamicos-nvr/appliance/30-security.conf /etc/apt/apt.conf.d/30-dynamicos-security
install -m 0644 /opt/dynamicos-nvr/appliance/dynamicos-recorder.service /etc/systemd/system/
install -m 0644 /opt/dynamicos-nvr/appliance/dynamicos-api.service /etc/systemd/system/
install -m 0644 /opt/dynamicos-nvr/appliance/run-dynamicos\x2dnvr-hls.mount /etc/systemd/system/
install -m 0644 /opt/dynamicos-nvr/appliance/dynamicos-tmpfiles.conf /etc/tmpfiles.d/dynamicos-nvr.conf
systemd-tmpfiles --create /etc/tmpfiles.d/dynamicos-nvr.conf
install -m 0644 /opt/dynamicos-nvr/appliance/dynamicos-health.service /etc/systemd/system/
install -m 0644 /opt/dynamicos-nvr/appliance/dynamicos-health.timer /etc/systemd/system/
install -m 0755 /opt/dynamicos-nvr/appliance/dynamicos-health /usr/local/sbin/dynamicos-health
install -d -m 0750 -o root -g dynamicos /etc/dynamicos-nvr
if [[ ! -e /etc/dynamicos-nvr/api.env ]]; then
    cat > /etc/dynamicos-nvr/api.env <<'EOF'
# API is localhost-only until administrator selects and authorizes a private LAN IP.
DYNAMICOS_API_HOST=127.0.0.1
DYNAMICOS_ALLOWED_NETWORKS=
EOF
    chown root:dynamicos /etc/dynamicos-nvr/api.env
    chmod 0640 /etc/dynamicos-nvr/api.env
fi
systemctl daemon-reload
# Validate before enabling; no silent acceptance of an invalid unit.
systemd-analyze verify /etc/systemd/system/dynamicos-recorder.service /etc/systemd/system/dynamicos-api.service
systemctl enable NetworkManager.service unattended-upgrades.service run-dynamicos\x2dnvr-hls.mount dynamicos-health.timer dynamicos-recorder.service lightdm.service
# Do not reboot, overwrite partitions, enable insecure inbound ports or claim success prematurely.
echo 'Software installiert. LUKS-Backup, Netzwerkisolation und Login vor Produktivbetrieb prüfen.'
