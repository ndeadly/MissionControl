# Dependency audit — 2026-11-06

Git refs were queried directly from upstream (not inferred from cached release pages). Only the direct build dependencies and installed toolchain components used by this project were audited; this is not an audit of every package on the workstation.

| Dependency | Before | Verified upstream / installed | Action |
| --- | --- | --- | --- |
| libnx source | `feebd026` | upstream master `b156b4774fb2df7b93402169509266da7547fc4c` | Advanced two commits; rebuilt and staged headers and archives together. |
| Atmosphere-libs | custom `1900a198`, based on `aae2da01` | upstream master `cf48d23375cf36742876f4b014971b2ebabc6dc8` | Advanced nine upstream commits; preserved the custom linker settings in MissionControl's own `system_module.mk`. |
| devkitA64 | r30-1, GCC 16.1.0 | latest published `devkitA64_r30` tag | Already current. Installed components: binutils 2.46.0, newlib 4.6.0.20260123-4. |
| switch-tools | 1.13.1-1 | latest published `v1.13.1` tag | Already current. |
| System libnx package | 4.12.0-1 | latest tagged libnx release `v4.12.0` | Already current, but this package is not the libnx linked by the default project build. |
| Console Atmosphère | user-confirmed 1.12.1 | latest release 1.12.1 | No console update required for this audit. |

The old libnx source pin already contained the v4.12.0 mutex fix (`7644c9b2`) and subsequent changes; `git describe` showed an older tag because the local tag refs were incomplete. Its two missing upstream commits add GATT operation enums/rename `client_notify.type` to `op_type`, and correct the scan-filter action field to `u8` plus three padding bytes. MissionControl now uses the updated field name and enums. This layout correction could prevent padding bytes from breaking action comparisons, but the supplied log does not demonstrate that failure.

The Atmosphere-libs update includes 23.0.1 version definitions, filesystem interface additions and SharedMemory/TransferMemory initialization fixes. The previous local commit only changed linker flags to honor the custom LIBNX path and allow text relocations. Those settings now live in `mc_mitm/system_module.mk`, so both submodule pins refer to public upstream commits; the user's original local commit remains reachable through the previous checkpoint.

The default Makefile previously used `build_libnx/libnx` without rebuilding it when the source submodule changed. `prepare_libnx` now records the staged Git revision and refreshes headers and libraries together on a pin change or tracked source edit. An explicitly supplied external `LIBNX` path remains under the caller's control. The revision 5 build cleans and rebuilds libnx, libstratosphere and MissionControl. There are no nested submodules in either dependency.

References: [libnx update](https://github.com/switchbrew/libnx/commit/b156b4774fb2df7b93402169509266da7547fc4c), [Atmosphere-libs update](https://github.com/Atmosphere-NX/Atmosphere-libs/commit/cf48d23375cf36742876f4b014971b2ebabc6dc8), [devkitA64 r30](https://github.com/devkitPro/buildscripts/tree/devkitA64_r30), [switch-tools 1.13.1](https://github.com/switchbrew/switch-tools/tree/v1.13.1), [Atmosphère 1.12.1](https://github.com/Atmosphere-NX/Atmosphere/releases/tag/1.12.1).
