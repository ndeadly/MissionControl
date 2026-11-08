# BLE fixes and validation

Changes are in `arkseal-ble-xbox`, based on `7801b6d`. The sibling upstream and proof-of-concept checkouts were left unchanged. The workspace-level `../missioncontrol.ini` is the gameplay config included in the test package.

## Revision 7: preserve security on individual reconnects

**Candidate fix; individual reconnects are not yet verified on the Switch.** The supplied R6 trace proves that the previous 500 ms bond-removal delay did not solve this problem. R6 remains available as commit `22ebfe7` and `../MissionControl-BLE-reconnect-fix-r6.zip`.

The 3,783-line log identifies `reconnect-fix-r6` on HOS 23.0.1 (SHA-256 `50a4c446673a4091a4086610f665eec518db7207684b75b80c95ebd36e8bf920`). Three pads initially subscribe successfully. After `40:8e:2c:7a:3a:9a` disconnects, its returning link opens and discovers all 26 attributes, but four attempts end with `0x0016` about 140–170 ms after `CreateBond`; another ends with `0x0008`. Once the other two pads disconnect, recovery resets the stack at 5253 seconds, and all three subscribe again. Later, the same pattern affects `10:c7:35:b5:63:19`: two refusals with the other pads active, then another successful three-pad connection after the zero-link reset at 7172 seconds.

This establishes a native pairing-path failure, rather than a particular bad controller or a two-controller limit. The reason reconnecting **all** pads works is the zero-link radio reset. R6 deliberately defers that shared reset while other BLE players are connected. The logs do not reveal which internal security flag/key causes the native refusal: `CreateBond` IPC success is only acceptance of the request, and `0x0016` is the later local-host termination. The [probe follow-up](https://github.com/ferrolho/MissionControl/pull/1) reports the same reset-dependent refusal. The on-disk upstream/probe/refactor branches contain no separate persistent-bonding implementation.

### Changes

- Remember the addresses of successfully subscribed pads with their native client-registration generation, in four fixed entries. Discovery/cache presence alone cannot qualify a pad for reconnect.
- For a returning pad in that generation, do **not** call `RemoveBond` or force another `CreateBond`. Preserve whatever native LE security state remains and let `NoMitm` authenticated GATT reads request encryption/security. This is a different security path to test, not proof that a usable stored key exists. A new pad or a pad returning after stack recovery uses the original initial-pairing sequence. The unsuccessful 500 ms cleanup workaround is removed.
- Require matching connection, operation and characteristic for both HID metadata reads and the CCCD write. An unrelated notification or completion cannot mark security/subscription successful. Missing mandatory metadata, callback errors, timeouts, sleep and disconnect abort setup. No plaintext fallback is added.
- Retain the zero-active-BLE recovery rule. Failed reconnects back off 30 seconds. Native registration changes invalidate setup/history. No new threads, input processing, rumble work or per-report allocations are added.

The supplied profile records 515,740 reports and 4/6/11 microseconds min/average/max forwarding time in its last window. Its 33.9-second maximum arrival gap can include idle/disconnected time across the combined stream, so it cannot measure a pad's radio latency or CPU usage. It does not explain the reproducible security-stage refusals.

### Validation and console test

All six host suites, ASan/UBSan (leak detection disabled in the ptrace sandbox), the ARM release build and `git diff --check` pass. Production pairing tests check that a reconnect makes neither destructive bond removal nor explicit bond creation, while a fresh session still does. GATT tests use timed completions, unrelated-device/operation/characteristic traffic, missing replies, callback failures including raw result 26, missing authentication metadata, bounded local authentication retries, sleep/disconnect, and failed or absent CCCD completion. Deliberately restoring unconditional bond removal or accepting the wrong GATT operation makes the relevant regression fail. These fixtures do not emulate the Switch's native security stack.

The ARM ELF is 483,116 bytes text, 41,816 data and 477,888 BSS: text grows 368 bytes from R6 and the aligned data/BSS section sizes are unchanged. The new completion-wait frame is 1,120 bytes on the existing 16 KiB coordinator stack. These are static build figures, not measured gameplay latency or runtime memory use. Existing Atmosphere-libs header warnings remain; dependency pins are unchanged from the November 6 audit.

Install `../MissionControl-BLE-reconnect-fix-r7.zip` by copying `atmosphere/` and `config/` to the SD root and rebooting. This diagnostic package enables logging/profiling. Check for startup marker `reconnect-fix-r7`.

1. Pair all three pads, then turn **only one** off while leaving the other two on. Open Change Grip/Order and hold that pad's sync button. Repeat for each pad.
2. The returning pad should log `reconnect preserving native security state`, then `reconnect requesting encrypted GATT access without CreateBond`, successful authenticated reads/CCCD, and `paired and subscribed`. No global radio reset or disconnect of the other pads should occur.
3. Also test all-off/re-pairing and console sleep/wake; after a stack reset, the initial `CreateBond` path should still work. Save both logs before rebooting.
4. If the returning authenticated read is refused or the link still drops, this candidate has not solved the native limitation. Keep the other pads connected long enough to capture the failure, then use the existing all-off recovery. Do not interpret the passing host tests as proof of hardware success.
5. Disable `log` and `profile` and reboot before comparing gameplay performance.

## Revision 6: retain recovery across sleep and separate returning-pad cleanup

This is a **candidate reconnect fix**, not a claim of hardware verification. Revision 5 remains available as `../MissionControl-BLE-stability-fix-r5.zip` and commit `ca14594`.

### What the new trace establishes

The 2,153-line `../ble.log` (SHA-256 `847c4506ede6c058d2560ff4736777a775f18b6768952edc85ccf5f33560120d`) identifies itself as `stability-fix-r5` on HOS 23.0.1. All three pads initially reach `paired and subscribed`. Sleep at 201 seconds physically closes all three links before the PSC acknowledgement. After wake, the first bond is refused at 235.970 seconds, recovery runs with zero BLE links at 237 seconds, and **all three connect again** at 267.432, 282.664 and 294.583 seconds.

The link for **`14:cb:65:e8:2b:e2`** then ends at 316.288 seconds with reason `0x0008`. The trace does not establish whether this was intentional controller power-off or an unexpected radio loss. At 2819.839, 2855.328, 2888.565 and 2920.348 seconds, that address reconnects at the GATT transport level but disconnects with reason `0x0016` roughly 100–110 ms after `CreateBond`. `RemoveBond` and `CancelBond` return success, yet subsequent retries still fail. The two other BLE links remain active until sleep at 4878 seconds, so revision 5 correctly defers a global reset; it would otherwise reproduce revision 4's dropped working players.

All four supplied `controllers/*/spi_flash.bin` files are identical 65,536-byte images (SHA-256 `64433a0d807546c0f8e6754b316444b94a8a73f74f4c9e200d53e1413a3c116f`). They provide the emulated Switch controller's SPI memory/calibration, not Xbox Bluetooth keys. The failing attempt never reaches virtual-controller initialization. These files were not changed.

### Changes and limits

- Preserve pending recovery across sleep, including repeated empty PSC transitions. If sleep closed working BLE controllers, queue recovery before pairing the first pad on wake. Previously, sleep discarded this request, and the next failed bond had to request recovery again. Recovery still requires Change Grip/Order, an awake radio and **zero active BLE links**. A wake within the one-minute reset cooldown now waits rather than opening a new link before recovery can run. This can add a wait on rapid sleep/wake cycles; the existing 12-second Classic reconnect grace period also remains.
- For a returning address with a cached GATT identity, allow 500 ms between `RemoveBond` and opening the new transport, draining old lifecycle events and then rearming the slot. Initial first-time pairing has no added wait. Check the removal IPC result instead of ignoring it. This addresses a possible teardown/reopen overlap: the [related Broadcom-derived implementation](https://android.googlesource.com/platform/external/bluetooth/bluedroid/+/master/bta/dm/bta_dm_act.c) removes security state asynchronously and can close a live link during removal. **That source is not the Switch's implementation**; the trace does not prove the overlap caused this refusal, and btdrv exposes no removal-completion acknowledgement. The settling interval is a mitigation to test, not proof of completed unpairing.
- Clear pre-existing core bond indications before `CreateBond`, reject pairing while disconnected/asleep, match connection completions to both address and client, and retain a late live connection instead of opening a second one during cleanup. Move this production connection/pairing code into `ble_pairing.cpp` so the host tests execute it directly.
- Log core connection types, SSP reply results, and deferred recovery with the active BLE count. Do not add logging or waits to input forwarding. No connection-interval, rumble or stick settings changed.

There is still no verified per-device replacement for the global stack recovery. The [probe follow-up](https://github.com/ferrolho/MissionControl/pull/1) also reports refused re-pairing and recovery through a Bluetooth stack reset, and explains the lack of a persistent Secure Connections bond. Holding sync is still required; the LED pattern alone cannot establish a controller fault. If the settling change does not clear this refusal while other pads remain connected, a native Bluetooth stack change or a reset after all BLE pads are off is still needed. This revision deliberately does not force working BLE players offline.

The profiler processed 156,658 reports with a final average of 9 microseconds for parsing/forwarding. Its combined arrival window includes a 5.94-second gap; neither number measures per-controller or end-to-end latency. Dependencies remain at the audited revision 5 pins in [DEPENDENCY_AUDIT.md](DEPENDENCY_AUDIT.md); this failure is not evidence of an outdated dependency.

### Validation and console test

`make -C tests test` passes all six suites. The new suite compiles the unmodified production `ble_pairing.cpp` with a timed IPC fixture: delayed old teardown, failed bond removal, first-time/returning connects, wrong-client callbacks, late live connections, invalid connection IDs, connection timeout/cancellation, stale bond indications, immediate refusal, and sleep during setup. Recovery tests replay sleep with active links, repeated empty sleep transitions, an interrupted reset, cooldown and protection of other BLE players. These tests validate host ordering, not Broadcom security behavior. The suites also pass AddressSanitizer/UndefinedBehaviorSanitizer with leak detection disabled for the ptrace sandbox. Removing the settling guard or stale-indication reset in temporary test copies makes the new tests fail. The ARM release build passes; warnings originate in unchanged Atmosphere-libs headers. Static inspection shows roughly 1.1 KiB wait frames on the existing 16 KiB coordinator stack; no new thread or per-report allocation was added.

Install `../MissionControl-BLE-reconnect-fix-r6.zip` at the SD root and reboot. Its active config enables logs and profiling; the log marker must read `reconnect-fix-r6`.

1. Pair all three, sleep from the Home menu, wake, open Change Grip/Order and hold sync until each Xbox flashes rapidly. Confirm all three control the menus.
2. Separately turn off just the troublesome pad while the other two remain connected. Attempt sync pairing again. This is the decisive check for the per-device cleanup change.
3. If it is refused again, turn **all Xbox BLE pads off**, leave Change Grip/Order open, allow recovery and its grace period (up to a minute if rate-limited), then hold sync again. Classic controllers can briefly reconnect during that global reset. Keep the logs from the failed and recovered attempts together.
4. Disable `log` and `profile` and reboot before comparing gameplay latency.

## Revision 5: preserve working players, unblock retries, close links before sleep

The revision 4 trace has 4,205 lines and confirms three distinct software problems:

- At 191.598 seconds, recovery deliberately dismisses two working BLE controllers after the third pad's failed bond attempt. The apparent random disconnect is our active-link reset policy, not an unexplained radio failure. Revision 5 permits stack recovery only with zero active BLE links.
- At 7054 seconds, recovery becomes associated with `40:8e:2c:7a:3a:9a`. That pad receives a bond indication but its first authenticated GATT read loses the link. It never reaches `PresentSlot`, so the address-specific retry latch never rearms. Even after the other pads disconnect at 8964/12115 seconds, repeated attempts at 12846–13062 seconds cannot recover. The latch is replaced with a one-minute minimum between recovery attempts. Successful connection of a particular address is no longer required. Intentional sleep clears pending recovery.
- At 6691 and 6995 seconds, the host removes all virtual pads without issuing `DisconnectGattServer`, because `SleepReady` already disabled transport cleanup. The console becomes awake again roughly 1–2 seconds later. Revision 5 closes connections from the power thread at `MinimumAwake`, with Bluetooth declared as a PSC dependency so it remains awake until cleanup finishes. The coordinator preserves slot ownership during this step, then retires the virtual controllers without reusing old transport handles. Pending direct connects are cancelled; late connection IDs are also closed. All slots share a one-second completion deadline. Missing acknowledgements are logged rather than blocking console sleep indefinitely. Rumble writes and security replies are gated while asleep.

Initial connection also suffers real transport failures: one connection ends with reason `0x003e`; three first authenticated HID Information reads end with reason `0x0008` and raw callback result 26. A suspended/bond indication is not proof that encrypted GATT access succeeded. The log now says `paired and subscribed` only after successful subscription, and reports raw GATT failures without labelling every failure as authentication failure. Failed security setup explicitly calls the per-address `CancelBond` API before teardown. This is a targeted cleanup; the trace does **not** establish the underlying radio/security reason for those timeouts, and it does not prove `CancelBond` will eliminate them on hardware.

Transient connection/discovery/subscription failures now back off five seconds instead of thirty; refused bond attempts retain the thirty-second backoff. Discovery can continue after its first **fresh** cache event, retaining the two-second fallback when no fresh event arrives. The MTU exchange, final cache settling and post-bond attribute checks remain. The post-bond guard is 500 ms instead of one second. These changes remove avoidable waits without repeating revision 2's early-query/empty-cache regression.

The profiler successfully wrote 519,809 reports. Its last sample reports 8 microseconds average processing time, but includes a 178,997 ms arrival gap across inactivity; it cannot measure end-to-end latency or prove the radio is healthy. Starting the first controller of a new session now starts a fresh cadence window while retaining the boot-wide report count.

Dependencies were audited on 2026-11-06 and updated; see [DEPENDENCY_AUDIT.md](DEPENDENCY_AUDIT.md). In particular, upstream libnx corrects the BLE scan-filter action field from four bytes to one byte plus padding. The supplied log does not show corrupt action values, so that correction is not claimed as the proven cause of its failures.

Validation includes native recovery/routing/deadzone/advertising/GATT/profiler tests, production sleep teardown compiled against a host driver fixture, ASan/UBSan with leak detection disabled, and a clean Switch build including both libraries. Sleep tests cover four simultaneous links, a pending connection completing after cancellation, a queued adopted link, missing/error callbacks with a shared deadline, and no radio ownership. GATT tests cover earlier fresh-cache discovery with the old fallback preserved, and disconnect/sleep during authenticated reads. Host fixtures cannot verify Broadcom security, PSC timing, physical controller lights or gameplay latency.

Install **`../MissionControl-BLE-stability-fix-r5.zip`** by copying its `atmosphere/` and `config/` folders to the SD root, rebooting and waiting about 30 seconds. Startup must identify `stability-fix-r5`. Logging/profiling are enabled for this test. Earlier packages and commits remain available.

1. Pair two working pads, then the third. A failure of the third must no longer trigger removal/reset of the first two.
2. Turn all BLE pads off, open Change Grip/Order and re-pair. If stack recovery is needed, allow its one-minute rate limit plus bring-up/Classic settling, then hold sync again. If one pad refuses while others are active, recovery intentionally waits until those BLE links are down; there is no verified per-device substitute for the shared-radio reset.
3. Test both manual sleep and automatic idle sleep with all three pads active, plus sleep during pairing. Save the log showing `sleep: disconnect` and `sleep: all BLE links down before PSC acknowledgement` (or a timeout). After wake, use sync to pair again; Xbox-button-only reconnect remains unsupported.
4. Save both logs before rebooting. Disable logging/profiling and reboot for gameplay latency testing.

## Revision 4: recovery while other controllers remain connected

**Working checkpoint: `2c0e44f`.** The latest console test on HOS 23.0.1 / Atmosphère 1.12.1 confirmed three Xbox controllers paired and controlled the Switch as Pro Controllers. Of two powered-off controllers, `10:c7:35:b5:63:19` re-paired successfully, while `14:cb:65:e8:2b:e2` repeatedly disconnected with reason `0x0016` about 100 ms after `CreateBond`. `40:8e:2c:7a:3a:9a` stayed connected. The old recovery gate required zero BLE links, so it never issued a radio reset. The remaining 1914 is outside this revision's focus at the user's request.

With `radio_cycle=true`, a failed pairing now permits recovery while other BLE pads are connected. A normal disconnect alone still waits for the remaining players. Recovery runs only while awake and in Change Grip/Order. It removes all virtual BLE controllers, cancels their client requests and subscriptions, then requests the existing Bluetooth stack reset. The entire shared radio resets: **all connected Bluetooth controllers drop**, including Nintendo controllers. Xbox pads need their sync buttons again. This is a re-pairing workaround; Xbox-button-only reconnection is still unsupported.

Only the address that triggered failed-pairing recovery successfully reconnecting rearms further automatic resets. A different controller connecting cannot repeatedly reset the radio for a pad that keeps refusing pairing. If the target never reconnects, automatic resets stay blocked until the module restarts. Leaving Change Grip/Order or sleeping during teardown defers a reset that has not yet been requested. After an accepted reset, the coordinator cannot scan until fresh GATT clients arrive, even if the initial wait times out. It then leaves a 12-second settling interval for Classic controller reconnection, informed by the scan/Classic connection conflict reported in [the probe follow-up](https://github.com/ferrolho/MissionControl/pull/1). This fixed interval is a precaution, not proof that all Classic reconnects finish within it.

The empty `ble_profile.txt` was also a software bug: the profiler created a zero-length file and wrote with `OpenMode_Write`, which cannot extend files on Horizon. It now resizes before writing, truncates shorter summaries, and reports file errors through `ble.log`. The first report no longer counts as a zero-length arrival interval. Labels identify aggregate timing across all BLE pads and the most recently updated link parameters; these measurements are not end-to-end latency. File operations remain on the coordinator, never the input thread. No gameplay radio intervals, rumble scheduling, report translation or GATT setup timing changed in this revision.

Validation: native regression suites, the Switch release build and ASan/UBSan passed (leak detection disabled as documented below). Recovery tests replay the supplied multi-controller failure sequence and cover disabled/outside-screen/asleep gates, teardown deferral, stale/absent GATT clients and repeated failures after a different pad reconnects. The new profiler suite compiles production `ble_profile.cpp` against Horizon-like file semantics and checks first write, shorter overwrite, sample intervals, failure reporting and absence of input-thread SD writes. Restoring the old active-link gate or checkpoint profiler makes its new regression fail. These tests do not emulate the Broadcom security stack, and **revision 4 reconnects still require console verification**.

The revision 4 ARM ELF has 478,572 bytes of text, 41,688 bytes of data and 477,888 bytes of BSS: 1,504 bytes more text than revision 3, with unchanged data/BSS section sizes. No new thread or per-report allocation was added. Compiler stack reports show 2,224 bytes for profile formatting and 1,072 bytes for the recovery event wait on the existing 16 KiB coordinator stack. These are static section/frame figures, not measured runtime RAM, CPU or input latency.

Install **`../MissionControl-BLE-reconnect-fix-r4.zip`** by copying `atmosphere/` and `config/` to the SD root and rebooting. The package enables logging/profiling; startup must identify `reconnect-fix-r4`. The revision 3 ZIP remains available as a fallback.

1. Connect the three working Xbox pads, then power off two while leaving the third on. Open Change Grip/Order and try the powered-off pads using their sync buttons.
2. If one refuses pairing, allow the shared-radio reset and at least 12 seconds for recovery. Hold sync on the **failed pad first**, then the other Xbox pads. Nintendo controllers should reconnect through their normal path. If discovery does not resume, exit and reopen Change Grip/Order once.
3. Repeat power-off/re-pairing and test an idle timeout. Save both logs before the next reboot. The trace should show `BLE pairing recovery: radio cycle rc=0x0`, fresh GATT clients, then a successful `paired` for the previously failing address. If recovery is still unsuccessful, do not infer success from the host tests.
4. After capturing the trace, set `log=false` and `profile=false` and reboot for gameplay/latency comparisons.

## Revision 3: linked controllers dropped before pairing

The next trace confirms `scan-fix-r2` on HOS `0x17000100` (23.0.1); the user also confirmed Atmosphère 1.12.1. Xbox advertisements with an empty name, appearance `0x03c4`, flags `0x06`, and a zero transport field are now selected. Eight connections across two Xbox addresses opened successfully. Each was disconnected about 5.12–5.14 seconds after its connection event. There are no cache events and no `btdrvCreateBond` calls; the profile is still empty.

The coordinator's pre-bond `Discover` wait timed out and called its own teardown. This was another regression introduced in revision 1: it queried attributes immediately, removed the original MTU/setup sequence, and required an input report attribute before allowing pairing. A successful cache-query IPC is not evidence that the table is populated or that callbacks will follow immediately.

Revision 3 restores the original probe's two-second settling period, attribute query, 256-byte MTU request/completion wait, and 500 ms drain. A still-live link may proceed to `CreateBond` with an empty table. The one-second post-bond settling guard and authenticated report-map read are also restored. Subscription validates the HID input/CCCD **after** pairing, retrying the attribute query then if needed. Setup still cancels when the link closes or the radio sleeps. MTU refusal/timeout alone does not reject an otherwise live link. These changes add connection-setup time; they do not change input dispatch, connection intervals or rumble scheduling during gameplay.

Cache chunks still replace duplicate handles, but polling the rediscovery flag no longer causes a later chunk to erase previously collected attributes. Logs identify pre-bond setup, post-bond missing attributes, pairing failure state, and locally requested disconnects. Startup identifies this build as `gatt-fix-r3`.

The former GATT test preloaded a complete cache table and could not detect this regression. Its replacement uses a timed driver fixture: attributes can be delayed until a later query, an MTU request, or simulated pairing completion. It also covers multi-chunk tables, MTU responses for a different connection, missing/refused MTU replies, disconnect/sleep cancellation, post-bond timeout, cached reconnects and duplicate/late cache delivery. The actual production GATT source is compiled against the fixture. Reintroducing revision 2's early-query/pre-bond gate makes the new suite fail. The fixture does not emulate controller security or the Broadcom stack.

The Switch release build, native regression suites and ASan/UBSan checks passed. Subsequent console testing confirmed three paired controllers with working input; see revision 4 above for the remaining reconnect failure. The supplied trace established the failed stage, but cannot distinguish which removed setup step was needed on the radio; the whole original sequence has been restored rather than guessing which step to omit again.

Install **`../MissionControl-BLE-gatt-fix-r3.zip`** by copying its `atmosphere/` and `config/` directories to the SD root and rebooting. Both diagnostics are enabled in this package. Wait about 30 seconds after boot, enter Change Grip/Order, and hold sync on one 1914 until it flashes rapidly. Allow roughly 30 seconds for Classic inquiry and BLE setup. Then try the remaining controllers. Save `ble.log` and `ble_profile.txt` from this run; the startup marker must say `gatt-fix-r3`. Disable both diagnostics and reboot before measuring gameplay latency. Earlier packages have been preserved separately.

## Revision 2: no Xbox controllers discovered

The supplied `../ble.log` contains 50 rejected scan starts (`0x2a871`) during the initial 10.24-second Classic inquiry, then repeated scan start/stop events. There are no `found controller` or GATT connection attempts. `../ble_profile.txt` is empty. The old logger deliberately omitted device advertisements, so this trace cannot distinguish missing advertisements from advertisements rejected by our selection code.

A regression in the first revision required every advertisement to contain an Xbox name. Xbox can send its name separately in a scan response: [this captured Xbox advertisement](https://github.com/esp32beans/picow_bt#gap_le_advertisementsino) carries flags, power, gamepad appearance and the HID service UUID, without the name. The original probe accepted it; the first revision did not. Revision 2 restores the nameless-gamepad fallback, prefers complete names over shortened names, and recognizes explicit LE-only advertising flags even when the transport field is zero. Named non-Xbox devices and Classic-only Xbox inquiry results remain excluded. The nameless fallback is not vendor identification; keep other BLE gamepads out of pairing mode.

Filter commands and scan-stop now wait for their actual asynchronous completions, with bounded timeouts, before another scan/connection starts. Busy retries back off one second instead of issuing five scan/filter sequences per second. Diagnostic logs include scan return codes, completion errors, bounded per-address details and aggregate rejection/drop counts. No input-thread logging or new threads were added. The startup line identifies this build as `scan-fix-r2` and records the HOS version.

The checkout includes the firmware 23.0.0 patches from upstream commit `71bbe2d`; they are unchanged and included in the package. Atmosphère 1.12.1 is recommended for firmware 23: its [release notes](https://github.com/Atmosphere-NX/Atmosphere/releases/tag/1.12.1) describe DNS/filesystem fixes and an increase in the custom-sysmodule memory budget from 16 MB to 40 MB. They do not claim an Xbox BLE pairing fix. Update the matching fusee payload if used.

Revision 2 passed the Switch release build, the existing routing/deadzone/recovery/GATT suites, and a new regression suite using the production advertisement parser and actual libnx advertisement types. The new suite reproduces the previous nameless-advertisement rejection and checks name precedence, Classic exclusion, and truncated/oversized fields. All suites also passed ASan/UBSan with leak detection disabled as described below. Scan IPC ordering was reviewed in code and compiled for Switch; host tests do not emulate the radio's completion timing. The new console log is still needed to confirm the cause and fix on these four controllers.

Install **`../MissionControl-BLE-scan-fix-r2.zip`** by copying its `atmosphere/` and `config/` directories to the SD root and rebooting. This diagnostic package enables `log=true` and `profile=true`. The earlier package remains available separately.

1. Wait about 30 seconds after boot, open Change Grip/Order, and hold sync on one 1914 until it flashes rapidly. Allow at least 20 seconds for the initial Classic inquiry and BLE setup.
2. If it connects, add the remaining two 1914s and the 1708; then test power-off and re-pairing using the recovery instructions below.
3. Retain `ble.log` and `ble_profile.txt` from this run, including failures. Look for `scan-fix-r2` to confirm the installed binary. `scan summary` distinguishes zero advertisements from rejected ones; `decision` identifies why a received device was skipped.
4. After collecting a useful trace, disable both diagnostics and reboot before comparing gameplay latency.

## What changed

- **Re-pairing:** optional `radio_cycle` recovery uses `btdrvEmulateBluetoothCrash`, based on [ferrolho/MissionControl PR #1](https://github.com/ferrolho/MissionControl/pull/1). Revision 4 permits a failed pairing to recover with other BLE links active, waits for fresh GATT clients, and ties the retry guard to the failed address. See its disruption and testing details above. Persistent Secure Connections bonding remains unsupported.
- **Disconnect safety:** a report lookup now holds a shared reference through dispatch. Previously teardown could free the raw pointer before the input thread acquired its controller mutex. Disconnect also disables further reports/output, unregisters notifications, cancels the client request, and keeps event ownership through teardown. Delayed disconnects with a different connection ID are ignored. Lifecycle events arriving on the LE HID stream now update slot state.
- **GATT reuse:** matching vacant slots preserve each address's cache when reconnect order changes. Cache updates replace duplicate handles instead of filling the 64-attribute table with duplicates. Discovery, read, notification-registration and CCCD failures are checked. A disconnect no longer counts as a successful GATT completion. The HID data path is registered once per client-registration generation.
- **Pairing time:** scan waits end on actual completions and busy retries back off one second; unsuccessful active scans retain an airtime cooldown. Pairing backoff is per address, so one failing pad does not block the others. Revision 3 restores the probe's GATT/MTU/report-map and post-bond settling sequence after the attempted setup optimization prevented pairing.
- **Xbox 1708:** Classic-only scan results stay out of the BLE host; BLE packets without the Series Share byte are normalized. The controller's firmware still determines its transport. Revision 2 restores nameless gamepad candidates, as explained above.
- **Sticks:** configurable radial deadzones for each stick, globally or per address, default 15%. The previous constants were about 24% left / 27% right. Direction and full travel are preserved, with the existing HOS inner-deadzone compensation. Centered and outer-radius inputs avoid square roots. Actual drift still requires tuning on the physical controller.
- **Gameplay work:** bounded rumble writes now also cover repeated stop/start pulses, with stop commands immediate. Failed writes are retried on later commands. Disabled rumble skips decoding. Four BLE pads request 15 ms instead of the former 20–25 ms; crowded mixed Classic/BLE setups retain the prior coexistence intervals. Logging/profiling are disabled in the supplied gameplay config.

There are still four fixed BLE slots and no new threads or per-report heap allocations. The route lookup takes a short mutex and a shared-reference increment; it releases that mutex before processing input or performing IPC. Safety replaces the old unprotected-pointer assumption. There is no input smoothing or report-rate limiter.

## Validation performed

- Baseline and changed Switch release builds completed with the installed devkitA64 toolchain. Existing dependency / ignored-result warnings remain; the final incremental build added no warnings in changed source.
- `make -C tests test` passed. It covers composite connection IDs, a deterministic in-flight teardown, 100,000 concurrent slot replacements, stick boundary/monotonicity/symmetry/full-range checks, and recovery deferral/retry limits.
- The GATT tests compile the **unmodified production `ble_gatt.cpp`** against a host IPC shim. They exercise discovery, address-cache reconnects, stale cache events, duplicate cache delivery, subscription registration failure, completion timeout, and disconnect cancellation. The shim validates host logic; it does not emulate the Broadcom radio/security stack.
- The same suites passed AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection was disabled because LeakSanitizer cannot run under this environment's ptrace sandbox; leak checking is not claimed.
- `git diff --check` passed. ARM compiler stack-usage output was inspected: the scan frame is 3,200 bytes; event waits use roughly 1.1 KiB each on the 16 KiB coordinator stack. This is a static review, not a runtime stack high-water measurement.

ARM ELF section sizes from the first revision, bytes (not the revision 2 package):

| Build | text | data | bss | sum |
| --- | ---: | ---: | ---: | ---: |
| Baseline | 462284 | 41688 | 481984 | 985956 |
| Changed | 471900 | 41688 | 477888 | 991476 |

Zero-initialized storage decreased by 4 KiB after moving per-template event buffers onto the existing coordinator stack. Code grew by 9,616 bytes; the section sum grew by 5,520 bytes. This is not a measurement of total runtime RAM or CPU use.

## Install and validate on the Switch

The latest workspace package `../MissionControl-BLE-reconnect-fix-r7.zip` contains `atmosphere/` and `config/`, including an active `missioncontrol.ini`. Copy these to the SD root and reboot to load the new module/config. It uses this checkout's existing firmware patches; compatibility with firmware versions beyond those patches is not newly established.

1. Pair each controller, then all four together. For the 1708, record its firmware version and whether the log identifies a BLE scan/connection. No extra model selector is required.
2. Use the revision 7 console test above. Shared-radio recovery waits until all BLE pads are off; individual reconnects now try authenticated GATT without forced re-bonding. Xbox-button-only reconnect is not implemented.
3. In the Switch stick test, release the drifting stick and adjust just its override in `/config/MissionControl/controllers/<12 lowercase MAC digits>/config.ini`, under `[xbox]`. Start with `left_stick_deadzone=20` for a drifting left stick; use the smallest sufficient value. Reconnect after changing this file. Global changes require a reboot.
4. Compare Smash character select and gameplay with the same controllers, first with rumble enabled and then disabled. Keep `log=false` and `profile=false` for this comparison. CPU usage, radio packet loss and end-to-end latency require console measurements; none are claimed from the host tests.
5. If pairing still fails, temporarily set `log=true`, reboot, reproduce initial pairing → power-off → attempted re-pairing, and retain `/config/MissionControl/ble.log`. Include HOS, Atmosphere and Xbox firmware versions. Turn diagnostics off again for normal play.

Revision 3 pairing/input on three controllers is user-confirmed. Revision 4's active-player resets were removed following the reported disruption. The revision 5 trace confirms early sleep teardown. Revision 6's trace confirms repeated three-pad pairing after zero-link stack recovery, but still refuses individual re-pairing while other pads remain connected. Revision 7 requires console validation as described above.
