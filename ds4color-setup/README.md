# DS4 Controller Setup

> AI notice: written with AI assistance (Claude, by Anthropic). See `../AI_NOTICE.md`.
> Licensed under GPL-2.0-only, same as the rest of this repository (`../LICENSE`).

A Switch homebrew app (.nro) for DualShock 4 controllers running through the patched
[Mission Control](https://github.com/ndeadly/MissionControl) sysmodule:

* **Buttons page** - rebind every DS4 input (face buttons, shoulders, triggers, stick clicks, D-pad, Share,
  Options, PS **and the three touchpad zones**) to any Switch button, several at once (combos), or nothing.
* **Lightbar page** - colour (all players or per player), brightness, presets.

It edits `sdmc:/config/MissionControl/missioncontrol.ini`. Only the keys it manages are touched
(`dualshock4_lightbar_*`, `dualshock4_map_*`); the rest of your file is preserved, and the original is
backed up once to `missioncontrol.ini.bak`. Only bindings that differ from the default are written.

**Requires the patched Mission Control sysmodule** (`mc_mitm`, see the patch that ships with this app).
The patched sysmodule re-reads the ini about four times a second, so **no reboot is needed**: with
**Live apply** on (default) every change is written within ~0.25 s and the lightbar/bindings follow in real time.

## Build
1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the `switch-dev` group.
2. `make` in this folder -> `ds4color.nro`
3. Copy to `sdmc:/switch/ds4color/ds4color.nro` and launch from the Homebrew Menu.

## Buttons page
Tap a control on the drawn controller (or a cell in the grid underneath), then tap the Switch button it
should press. Amber dots / amber text mark inputs you have rebound; green dots in the picker mark the default.

| Input | Action |
|---|---|
| D-pad | Choose input (or move in the picker) |
| A | Open the picker for the chosen input / assign the highlighted button |
| B | Close the picker / quit |
| X | Reset this input to default (ZL+X resets all) |
| Y | Combo mode: picker toggles buttons so one input presses several |
| + | Save now |
| - | Switch page |
| Touch | Tap controls, grid cells and picker buttons; header toggles Live apply |

The touchpad acts as three buttons (left / centre / right 15 %) when the pad is **clicked**.
Quitting with unsaved changes (Live apply off) asks you to press B a second time.

## Lightbar page
| Input | Action |
|---|---|
| L / R | Previous / next target (All, P1..P8) |
| Up / Down | Select slider |
| Left / Right | Adjust (hold = repeat, hold ZL/ZR = big steps) |
| Y | Cycle presets |
| X | Clear this target (back to default) |
| Touch | Tap tabs / presets / buttons, drag sliders |

"All" sets one colour for every player; a player tab overrides it for that player number.
The 4th slider edits `dualshock4_lightbar_brightness` (0-9).

## ini keys
```
dualshock4_map_<input>=<button>[+<button>...]     e.g. dualshock4_map_cross=a   dualshock4_map_l2=zl+l
<input>:  cross circle square triangle l1 r1 l2 r2 l3 r3 share options ps
          dpad_up dpad_down dpad_left dpad_right touchpad_left touchpad_centre touchpad_right
<button>: a b x y l r zl zr plus minus home capture stickl stickr up down left right none
```

## Tests
`test/run.sh` builds the ini + UI logic on a PC with ASan/UBSan and runs it (no Switch needed); it renders PPM
screenshots to /tmp and exports `/tmp/ds4_roundtrip.ini` for the sysmodule cross-check.

`tools/genfont.py` regenerates the embedded bitmap font.
