#!/bin/bash
set -Eeuo pipefail
# dynamicOS NVR Debian 13 amd64 live/installer image.
# Debian live-installer installs the live filesystem; this does NOT silently partition disks.
[[ "$(uname -m)" == x86_64 ]] || { echo "ISO-Build erfordert x86_64 Linux" >&2; exit 1; }
command -v lb >/dev/null || { echo "Debian-Paket live-build fehlt" >&2; exit 1; }
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD=${1:-"$ROOT/build/iso"}
mkdir -p "$BUILD"
cd "$BUILD"

# Clean stale live-build configuration from previous attempts.
lb clean --purge >/dev/null 2>&1 || true
lb config --distribution trixie --architectures amd64 --binary-images iso-hybrid \
  --debian-installer live --archive-areas 'main contrib non-free non-free-firmware' \
  --bootloaders 'grub-efi,syslinux' --image-name 'dynamicOS-NVR-amd64' \
  --apt-recommends false

mkdir -p config/package-lists \
  config/includes.chroot/opt/dynamicos-nvr \
  config/includes.chroot/usr/local/share/dynamicos-nvr \
  config/hooks/live

# Offline-capable networking and a working fallback X11 login are deliberately
# installed in the image itself, not downloaded after the system was installed.
cat > config/package-lists/dynamicos.list.chroot <<'PACKAGES'
linux-image-amd64
live-boot
live-config
firmware-linux-free
firmware-misc-nonfree
firmware-brcm80211
firmware-iwlwifi
firmware-atheros
firmware-realtek
wireless-regdb
wpasupplicant
network-manager
iproute2
iputils-ping
isc-dhcp-client
rfkill
pciutils
usbutils
sudo
curl
git
ca-certificates
python3
python3-venv
python3-pip
ffmpeg
xserver-xorg
xserver-xorg-core
xserver-xorg-input-libinput
xinit
lightdm
openbox
xterm
dbus-x11
cryptsetup
unattended-upgrades
nftables
libegl1
libgl1
libxkbcommon-x11-0
libxcb-cursor0
libxcb-xinerama0
fonts-dejavu-core
PACKAGES

cp -a "$ROOT/nvr" "$ROOT/appliance" config/includes.chroot/opt/dynamicos-nvr/

# Create an actual, selectable graphical session and enable network management
# on live media and systems installed by Debian live-installer.
cat > config/hooks/live/0200-dynamicos-base.hook.chroot <<'HOOK'
#!/bin/sh
set -eu
install -d /usr/share/xsessions /etc/lightdm/lightdm.conf.d
cat > /usr/share/xsessions/dynamicos-recovery.desktop <<'DESKTOP'
[Desktop Entry]
Name=dynamicOS Recovery Desktop
Comment=Lightweight desktop for network and NVR setup
Exec=openbox-session
Type=Application
DESKTOP
cat > /etc/lightdm/lightdm.conf.d/40-dynamicos-recovery.conf <<'LIGHTDM'
[Seat:*]
user-session=dynamicos-recovery
allow-guest=false
LIGHTDM
if [ -x /usr/bin/update-alternatives ] && [ -x /usr/bin/openbox-session ]; then
  update-alternatives --set x-session-manager /usr/bin/openbox-session || true
fi
if command -v systemctl >/dev/null 2>&1; then
  systemctl enable NetworkManager.service lightdm.service || true
fi
HOOK
chmod +x config/hooks/live/0200-dynamicos-base.hook.chroot

cat > config/includes.chroot/usr/local/share/dynamicos-nvr/INSTALL.txt <<'DOC'
dynamicOS NVR — Debian 13 test/installer image
==============================================
This image includes networking firmware, NetworkManager, LightDM and a
working Openbox recovery desktop so that login does not depend on an
uninstalled session manager.

IMPORTANT: This image is NOT a verified turnkey production appliance.
Use Debian live-installer's manual partitioning to prepare an encrypted
LUKS data volume. Installing to disk can erase existing data.
Once installed and booted, mount a LUKS encrypted volume at
/var/lib/dynamicos-nvr, then run as root:
  /opt/dynamicos-nvr/appliance/install-on-debian.sh
This protected installer validates the encrypted volume before enabling
the dynamicOS NVR application and kiosk session.

If networking is missing, verify:
  ip -br addr
  systemctl status NetworkManager
  nmcli device status
  journalctl -b -u NetworkManager --no-pager
The Wi-Fi chipset/firmware must still be validated on the target Mac mini.
Never use automatic disk partitioning on a machine with valuable data.
DOC

# Guard against accidental release of an incomplete filesystem.
test -f config/includes.chroot/opt/dynamicos-nvr/appliance/install-on-debian.sh
test -f config/includes.chroot/opt/dynamicos-nvr/appliance/start-nvr.sh
test -f config/hooks/live/0200-dynamicos-base.hook.chroot
lb build
