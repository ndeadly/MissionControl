#!/bin/bash
# Builds and runs the host test (ini layer + UI logic), then converts the screenshots to PNG.
set -euo pipefail
cd "$(dirname "$0")/.."
gcc -std=gnu11 -g -O1 -Wall -Wextra -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    source/ini.c source/gfx.c source/ui.c source/remap.c test/host_test.c -lm -o /tmp/ds4color_host_test
/tmp/ds4color_host_test
