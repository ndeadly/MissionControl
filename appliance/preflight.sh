#!/bin/sh
set -eu
printf 'dynamicOS Linux appliance preflight\n'
printf 'Architecture: ';uname -m
printf 'Firmware: ';test -d /sys/firmware/efi && echo EFI || echo BIOS
printf 'Disk mapping:\n';lsblk -o NAME,TYPE,SIZE,FSTYPE,MOUNTPOINTS
printf 'Network:\n';ip -br address || true
printf 'Hardware:\n';lspci | grep -E 'VGA|Ethernet|Network' || true
printf 'Vault:\n';findmnt /var/lib/dynamicos-nvr || true
