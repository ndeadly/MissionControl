# About this fork

Fork of [ndeadly/MissionControl](https://github.com/ndeadly/MissionControl) with extra Dualshock 4 features.
Branch `ds4-lightbar-remap-livereload`, based on upstream commit `71bbe2d` ("Update for 23.0.0").
Licensed GPL-2.0-only like upstream (see `LICENSE`). Read [AI_NOTICE.md](AI_NOTICE.md) first.

## What it adds

- `dualshock4_lightbar_colour=RRGGBB`, with per-player `dualshock4_lightbar_colour_1` to `_8`.
- `dualshock4_map_<input>=<button>[+<button>]` to remap every DS4 input (including the three touchpad zones) to any
  Switch button, combos, or `none`. Keys and values are documented in `mc_mitm/config.ini`.
- The ini is re-read about four times a second (`PollConfigurationReload`) and applied without a reboot. A file that
  is mid-write is read twice 30 ms apart and only applied if it is unchanged. Lightbar colour, brightness and polling
  rate are re-sent to connected controllers when the config changes.
- `ds4color-setup/`: a homebrew app (`.nro`) to change all of this from the Switch. See its README.

Not hot-reloaded: `[bluetooth] host_name`/`host_address`, and settings that other controller types only read when they
connect. Analog stick remapping is not supported.

## Build

```bash
git clone --recurse-submodules <this fork>
cd MissionControl
git checkout ds4-lightbar-remap-livereload
podman run --rm -v "$PWD":/src:Z -w /src docker.io/devkitpro/devkita64 \
  sh -c "make -C lib/libnx -j8 install && make -j8"
```

The stock devkitpro container's libnx is too old for the pinned Atmosphere-libs, hence the libnx install from the
submodule. Output: `mc_mitm/out/nintendo_nx_arm64_armv8a/release/mc_mitm.nsp`. Back up
`atmosphere/contents/010000000000bd00/`, copy the `.nsp` over `exefs.nsp` there, and reboot.

App: `cd ds4color-setup && podman run --rm -v "$PWD":/src:Z -w /src docker.io/devkitpro/devkita64 make`, then copy
`ds4color.nro` to `/switch/` on the SD card.

## Staying in sync with upstream

```bash
git remote add upstream https://github.com/ndeadly/MissionControl.git   # if not already set
git fetch upstream
git rebase upstream/master
```
