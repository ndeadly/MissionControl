#!/bin/bash
set -Eeuo pipefail
[[ "$(uname -m)" == x86_64 ]] || { echo 'ISO-Build erfordert x86_64 Linux';exit 1; }
command -v lb >/dev/null || { echo 'Bitte Debian-Paket live-build installieren';exit 1; }
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD=${1:-"$ROOT/build/iso"}
mkdir -p "$BUILD"
cd "$BUILD"
lb config --distribution trixie --architectures amd64 --binary-images iso-hybrid \
 --debian-installer live --archive-areas 'main non-free-firmware' \
 --bootloaders 'grub-efi,syslinux' --image-name 'dynamicOS-NVR-amd64' \
 --apt-recommends false
mkdir -p config/package-lists config/includes.chroot/opt/dynamicos-nvr config/includes.chroot/usr/local/share/dynamicos-nvr
cat >config/package-lists/dynamicos.list.chroot <<'EOF'
linux-image-amd64
live-boot
live-config
firmware-linux-free
firmware-misc-nonfree
python3
python3-venv
python3-pip
ffmpeg
xserver-xorg-core
xserver-xorg-input-libinput
xinit
lightdm
dbus-x11
network-manager
cryptsetup
unattended-upgrades
ca-certificates
nftables
libegl1
libgl1
libxkbcommon-x11-0
libxcb-cursor0
libxcb-xinerama0
fonts-dejavu-core
EOF
cp -a "$ROOT/nvr" "$ROOT/appliance" config/includes.chroot/opt/dynamicos-nvr/
cat > config/includes.chroot/usr/local/share/dynamicos-nvr/INSTALL.txt <<'EOF'
This ISO is an installer/live test medium, NOT an automatically encrypted production appliance.
Install Debian manually with LUKS, then use /opt/dynamicos-nvr/appliance/install-on-debian.sh
from the installed system. The installer does not automatically configure LUKS or the app.
EOF
lb build
