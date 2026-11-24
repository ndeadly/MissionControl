#!/bin/sh
set -eu
export DYNAMICOS_STATE_DIR=/var/lib/dynamicos-nvr/state
export DYNAMICOS_RECORD_DIR=/var/lib/dynamicos-nvr/recordings
export DYNAMICOS_APPLIANCE_MODE=1
export DYNAMICOS_EXTERNAL_RECORDER=1
export DYNAMICOS_HLS_DIR=/run/dynamicos-nvr/hls
umask 077
export PYTHONUNBUFFERED=1
export PYTHONPATH=/opt/dynamicos-nvr/nvr
/opt/dynamicos-nvr/venv/bin/python -c 'from security_policy import require_vault;require_vault("/var/lib/dynamicos-nvr/recordings")' || { echo 'SECURITY: encrypted recording vault unavailable' >&2; exit 41; }
exec /opt/dynamicos-nvr/venv/bin/python /opt/dynamicos-nvr/nvr/appliance_main.py
