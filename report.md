# Mission Control Multi-Device Bluetooth LE Audit Report

**Repository / Branch Audited:** `arkseal-ble-xbox` (`ble-xbox-latency`)  
**Baseline Comparisons:** `ndeadly:master` (`71bbe2d`), `ferrolho:ble-xbox-probe` (`1d63ce4`)  
**Hardware Log Analyzed:** `./ble.log` (Nintendo Switch HOS, Broadcom BCM4356 / BSA 1.6)

---

## Executive Summary

The `arkseal-ble-xbox` branch refactors the single-controller Bluetooth Low Energy (HID over GATT) proof-of-concept from `ferrolho:ble-xbox-probe` into a 4-slot multi-controller coordinator with low-latency connection interval negotiation, real-time thread priority (`-11`), shared-memory report locking (`g_fake_report_lock`), per-slot GATT `client_if` assignment (resolving `0x29e71`), and integer-only formatting (resolving `0xFFE` newlib `_dtoa_r` panics).

A line-by-line audit of the implementation cross-referenced against the hardware execution trace (`./ble.log`) identified **10 architectural and implementation flaws** across four categories:

1. **Critical Multi-Controller & Input Blockers (P0):**
   * **Issue 1.1:** Controllers in Slots 1–3 (`conn_id = 260, 517, 774`) exceed `MaxConnIdRange = 32`, causing 100% of their input notifications to be silently dropped.
   * **Issue 1.2:** A struct layout change in `XboxOneInputReport0x01` (`dpad` moved out of `XboxOneButtonData` in upstream `d450104`) shifted `buttons[]` by 1 byte, breaking both the **View (`-`)** and **Share (`Capture`)** buttons on all BLE Xbox controllers.
   * **Issue 1.3:** Leaving the *Change Grip/Order* screen while a 5-second BLE scan is in flight causes `mc.mitm` to mutate BLE scan filters after `btm` stops inquiry, triggering a stack-wide BLE GATT reset (`result=55, status=2, conn_id=0xFFFFFFFF`) that `OnBleEvent` ignores, leaving connected controllers in a dead "zombie" state.

2. **Real-Time Latency & Fast-Path Violations (P1):**
   * **Issue 2.1:** Synchronous SD card file I/O (`fs::OpenFile` + `fs::WriteFile(Flush)` + `fs::CloseFile`) still executes directly on the real-time `-11` priority input thread (`LeHidThreadFunc`) inside `ble::profile::RecordReportEnd()` and on every rumble GATT write completion (`notify.type = 1`), as well as on `mc::EventThread` (`-11`) inside `OnBleEvent` / `OnCoreEvent`.
   * **Issue 2.2:** BLE rumble deduplication vs. `pulse_sustain_10ms = 1` causes steady vibration to cut off after 10 ms, while dynamic 60 Hz HD Rumble floods the BLE link with up to 240 GATT writes/sec synchronously on `mc::BtdrvMitmThread`.

3. **Joy-Con Coexistence & Radio Scheduling Contention (P1/P2):**
   * **Issue 3.1:** Continuous back-to-back BLE scanning while *Change Grip/Order* is open (and 1 Hz retry spam during `0x2a871` `BTA_DM_BUSY` when `btm`'s 10.24s classic BR/EDR inquiry is active) starves the shared BCM4356 2.4 GHz radio.
   * **Issue 3.2:** `PreferredConnectionParameters` clamping math forces `max_conn_interval = 6` (7.50 ms) during SMP pairing, executes synchronous IPC under `mc::EventThread` locks, and ignores connected Classic Bluetooth Joy-Cons when selecting BLE connection intervals.

4. **Concurrency, State Machine & BTM Service Flaws (P1/P2):**
   * **Issue 4.1:** Global `g_gatt_results` is incremented by rumble GATT write completions (`notify.type = 1`) from active controllers, prematurely waking `WaitForGattResult()` while a second controller is subscribing to GATT characteristics.
   * **Issue 4.2:** The `Adopt()` connection path skips `SetSlotConnecting()`, leaving `g_slots[slot].active = false` and `address = 00:00:00:00:00:00`, which guarantees a 10-second pairing timeout.
   * **Issue 4.3:** Concurrent mutation of `EmulatedSwitchController` state between `mc::BtdrvMitmThread` (`HandleOutputDataReport`) and `mc::BleHidEventThread` (`HandleInputReport`), and fragile 20 ms sleep-based teardown in `DismissSlot()`.
   * **Issue 4.4:** `BtmMitmService::GetDeviceCondition` and `GetShortenedDeviceCondition` copy `device_condition[0]` into virtual BLE controller entries, inheriting Joy-Con (`0x2006`) or Bluetooth Audio device properties when a Joy-Con or headset is in slot 0.

---

## Detailed Findings

### Issue 1.1 (P0 Critical): Controllers 2–4 Drop 100% of Input Reports (`conn_id >= 32`)

* **Files Affected:**
  * `mc_mitm/source/ble/ble_types.hpp:23`
  * `mc_mitm/source/ble/ble_events.cpp:25, 224, 248–264, 307`
  * `mc_mitm/source/ble/ble_hid_host.cpp:173–179, 469, 479`
* **Hardware Log Evidence (`ble.log` lines 74, 123–125):**
  ```text
  [   213.118] evt ClientConnection result=0 status=0 client_if=3 conn_id=3 reason=0x0000
  [   213.124] slot 0: connected conn_id=3
  ...
  [   225.594] evt ClientConnection result=0 status=0 client_if=4 conn_id=260 reason=0x0000
  [   225.599]   address 40:8e:2c:7a:3a:9a
  [   225.602] slot 1: connected conn_id=260
  ```
* **Root Cause:**
  In Broadcom BSA (`bluetooth.nso`), `conn_id` is a 16-bit composite value combining the GATT transport control block index (`tcb_idx`) in the upper byte and `client_if` in the lower byte:
  $$\text{conn\_id} = (\text{tcb\_idx} \ll 8) \mid \text{client\_if}$$
  * Slot 0 (`tcb_idx = 0`, `client_if = 3`): `conn_id = 3` (`0x0003`)
  * Slot 1 (`tcb_idx = 1`, `client_if = 4`): `conn_id = 260` (`0x0104`)
  * Slot 2 (`tcb_idx = 2`, `client_if = 5`): `conn_id = 517` (`0x0205`)
  * Slot 3 (`tcb_idx = 3`, `client_if = 6`): `conn_id = 774` (`0x0306`)

  Because `MaxConnIdRange` is hardcoded to `32`:
  ```cpp
  void RegisterActiveController(u32 conn_id, controller::XboxOneBleController *ctrl) {
      if (conn_id < MaxConnIdRange) {
          g_active_controllers[conn_id].store(ctrl, std::memory_order_release);
      }
  }
  ```
  `RegisterActiveController(260, ...)` does nothing, and `LeHidThreadFunc` (`if (notify.conn_id < MaxConnIdRange)`) drops every input notification from Slot 1, 2, and 3. Controller 2 only appears on the Switch UI because `PresentSlot()` manually invokes `controller->HandleInputReport(neutral, sizeof(neutral))` once.
* **Resolution:**
  Replace the `conn_id < 32` direct-indexed array with a 4-slot lock-free lookup table (`g_fast_slots[MaxBleControllers]`) storing `{ std::atomic<u32> conn_id; std::atomic<XboxOneBleController *> controller; }` per slot (`0..3`). Matching `conn_id` against 4 slots in registers takes ~2 ns and supports arbitrary 32-bit `conn_id` values.

---

### Issue 1.2 (P0 Critical): View (`-`) and Share (`Capture`) Buttons Broken by Struct Offset Shift

* **Files Affected:**
  * `mc_mitm/source/controllers/xbox_one_ble_controller.cpp:45–58`
  * `mc_mitm/source/controllers/xbox_one_controller.hpp:38–80`
* **Root Cause:**
  In `ferrolho:ble-xbox-probe`, `u8 dpad;` was the first field inside `XboxOneButtonData` (`sizeof(XboxOneButtonData) == 4`), so `buttons[2]` was the 3rd button byte (containing `view` at bit 2) and `buttons[3]` was the 4th byte (containing `share` at bit 0).
  In `ndeadly:master` (commit `d450104`), `DirectionalPadType<1, 0> dpad;` was moved out of `XboxOneButtonData` into `XboxOneInputReport0x01` (`sizeof(XboxOneButtonData) == 3`, `sizeof(XboxOneInputReport0x01) == 16`).
  Consequently, `&xbox_report->input0x01.buttons` now starts 1 byte later:
  * `buttons[0]`: `A, B, X, Y, LB, RB`
  * `buttons[1]`: `view (0x04), menu (0x08), guide (0x10), lstick (0x20), rstick (0x40)`
  * `buttons[2]`: `share (0x01)`
  * `buttons[3]`: Out-of-bounds (1 byte past the 16-byte `XboxOneInputReport0x01` struct).

  In `XboxOneBleController::ParseInputReport`:
  ```cpp
  if ((xbox_report->id == InputReportId) && (size >= sizeof(XboxOneInputReport0x01) + 1)) {
      const u8 *buttons = reinterpret_cast<const u8 *>(&xbox_report->input0x01.buttons);
      m_buttons.Assign(SwitchButton::Minus, (buttons[2] & 0x04) != 0);
      if (size >= sizeof(XboxOneInputReport0x01) + 2) {
          m_buttons.Assign(SwitchButton::Capture, (buttons[3] & 0x01) != 0);
      }
  }
  ```
  `buttons[2] & 0x04` reads bit 2 of the Share byte (always `0`), and `size >= 18` (`sizeof + 2`) is never true for the 17-byte framed report (`1` report ID byte + `16` payload bytes).
* **Resolution:**
  Read `Minus` from `(buttons[1] & 0x04) != 0` and `Capture` from `(buttons[2] & 0x01) != 0` when `size >= sizeof(XboxOneInputReport0x01) + 1`.

---

### Issue 1.3 (P0 Critical): Unhandled Stack-Wide BLE Reset (`result=55`) & Post-Inquiry Scan Race

* **Files Affected:**
  * `mc_mitm/source/ble/ble_events.cpp:277–328`
  * `mc_mitm/source/ble/ble_hid_host.cpp:287–338`
* **Hardware Log Evidence (`ble.log` lines 191–216):**
  ```text
  [   270.565] evt ScanFilter result=0 action=2
  [   270.569] evt ScanFilter result=0 action=4
  [   270.571] evt ScanResult status=0x00
  [   273.261] btm stopped its inquiry
  [   273.305] evt ScanResult status=0x01
  [   275.566] evt ScanFilter result=0 action=3
  [   283.306] evt ClientConnection result=55 status=2 client_if=255 conn_id=4294967295 reason=0x0000
  [   283.314]   address 00:00:00:00:00:00
  [   283.317] evt ClientRegistration result=55 client_if=255 status=0
  ...
  [   285.500] evt ClientRegistration result=0 client_if=3 status=1
  ...
  [   319.984] ble switched off by the system, disconnecting all
  [   320.091] slot 0: controller removed from system
  [   320.117] slot 1: controller removed from system
  ```
* **Root Cause:**
  1. `Scan()` waits up to 5 seconds inside `events::WaitFor` without checking `events::IsPairingWindowOpen()`. When the user exits the *Change Grip/Order* screen at `273.261` (`btm stopped its inquiry`), `Scan()` keeps running until `275.566` and calls `btdrvStopBleScan()` and `btdrvEnableBleScanFilter(true)` after `btm` has already resumed normal operation.
  2. At `283.306`, the stack resets BLE GATT (`result=55`, `status=2`, `client_if=255`, `conn_id=0xFFFFFFFF`, `address=00:00:00:00:00:00`) and re-registers `client_if=3..6`.
  3. Because `OnBleEvent` only matches disconnections by specific slot address or `conn_id < 32`, it ignores the wildcard disconnect event (`conn_id = 0xFFFFFFFF`), leaving Slot 0 and Slot 1 as dead zombie controllers on the system until console sleep.
* **Resolution:**
  1. In `Scan()`, abort `WaitFor` immediately when `!events::IsPairingWindowOpen()`.
  2. In `OnBleEvent`, when `BtdrvBleEventType_ClientConnection` reports `!connected` with `conn_id == InvalidConnId` (`0xFFFFFFFF`) or when `BtdrvBleEventType_ClientRegistration` reports `status == 0` (client deregistered), mark all active slots as disconnected and update `g_available_client_ifs` cleanly.

---

### Issue 2.1 (P1 High): Synchronous SD Card I/O on Real-Time (`-11`) Threads

* **Files Affected:**
  * `mc_mitm/source/ble/ble_profile.cpp:173–182`
  * `mc_mitm/source/ble/ble_hid_host.cpp:180–183`
  * `mc_mitm/source/ble/ble_events.cpp:331, 360, 412, 419`
* **Root Cause:**
  1. In `ble_profile.cpp:173`, `RecordReportEnd()` (called directly on `LeHidThreadFunc` at priority `-11`) calls `WriteSummaryToFile()` every 500 reports and resets `g_window_reports = 0`. This executes `fs::OpenFile` + `fs::WriteFile(..., Flush)` + `fs::CloseFile` on the real-time input thread (stalling input processing for 10–80 ms) and renders `FlushIfDue()` on `HostThreadFunc` dead code. Furthermore, holding `ble::profile::g_lock` during `WriteSummaryToFile()` blocks `RecordReportStart()` on `LeHidThreadFunc`.
  2. In `ble_hid_host.cpp:182`, `LeHidThreadFunc` calls `events::Log(g_le_hid_record)` in its `else` branch. Every rumble GATT write completion arrives on `LeHidThreadFunc` as `BtdrvBleEventType_ClientNotify` with `notify.type = 1` (`!= GattNotifyType_Notification`), triggering synchronous SD card logging on the real-time input thread (`ble.log` lines 113–116).
  3. In `ble_events.cpp:331, 360`, `OnBleEvent` calls `log::Write()` directly on `mc::EventThread` (priority `-11`, which handles all Classic Bluetooth Joy-Con events) while holding `g_event_data_lock` and `g_lock`—and duplicates the `PreferredConnectionParameters` log line that `Log()` already prints when popped from `g_ring` (`ble.log` lines 86 and 88).
* **Resolution:**
  1. In `ble_profile.cpp`, remove `WriteSummaryToFile()` from `RecordReportEnd()`. Snapshot completed window stats into a staging buffer under `g_lock`, release `g_lock`, and perform all file I/O exclusively on `HostThreadFunc` (`FlushIfDue()` / `FlushNow()`) outside the mutex.
  2. In `LeHidThreadFunc`, do not call `events::Log()` synchronously on the `-11` input thread; push non-input events (excluding noisy `notify.type == 1` rumble write completions) to `events::g_ring` so `HostThreadFunc` logs them asynchronously.
  3. Remove synchronous `log::Write()` calls from `OnBleEvent` and `OnCoreEvent` on `mc::EventThread`.

---

### Issue 2.2 (P1 High): BLE Rumble 10ms Sustain Cutoff vs. 60Hz GATT Write Flood

* **Files Affected:**
  * `mc_mitm/source/controllers/xbox_one_ble_controller.hpp`
  * `mc_mitm/source/controllers/xbox_one_ble_controller.cpp:60–78`
  * `mc_mitm/source/controllers/xbox_one_controller.cpp:28–38`
* **Root Cause:**
  1. `XboxOneController::SetVibration` hardcodes `pulse_sustain_10ms = 1` (10 ms). Because `XboxOneBleController::SetVibration` drops consecutive identical `(strong, weak)` values, a steady vibration command only sends one packet, which stops vibrating after 10 ms.
  2. Conversely, Switch HD Rumble streams slightly varying amplitudes at 60 Hz (`strong = 40, 42, 41...`). Because consecutive values differ, `XboxOneBleController::SetVibration` issues 60 GATT writes/sec per controller (up to 240 writes/sec for 4 controllers) synchronously on `mc::BtdrvMitmThread`, starving BLE input notifications and stalling `btdrv` IPC for Joy-Cons.
  3. `WriteDataReport` does not check `m_has_output_report` before calling `btdrvWriteGattCharacteristic`.
* **Resolution:**
  * Track `m_has_output_report` in `XboxOneBleController`.
  * Use `pulse_sustain_10ms = 25` (250 ms) for non-zero BLE rumble and `0` (`enable = 0`) when stopping rumble.
  * Rate-limit non-zero BLE rumble GATT writes to at most once every 32 ms (~30 Hz) per controller while allowing stop-rumble (`strong == 0 && weak == 0`) through immediately, and refresh steady non-zero rumble if held for >180 ms.

---

### Issue 3.1 (P1 High): Continuous BLE Scanning & `0x2a871` Busy Spam Disrupt Joy-Cons

* **Files Affected:**
  * `mc_mitm/source/ble/ble_hid_host.cpp:287–338, 620–624`
* **Hardware Log Evidence (`ble.log` lines 31–64, 164–190):**
  * From `200.908` to `210.998` (while `btm`'s 10.24s classic BR/EDR inquiry is active on the BCM4356 radio), `btdrvStartBleScan()` fails 11 times with `rc=0x2a871` (`BTA_DM_BUSY`), spamming 33 `ScanFilter` IPC calls.
  * From `234.530` to `273.305` (after 2 BLE controllers are already connected), `HostThreadFunc` runs 5-second BLE scans continuously back-to-back without any cooldown, contending with Joy-Con 15 ms sniff polls and active BLE connection events.
* **Resolution:**
  * When `btdrvStartBleScan()` fails, restore the scan filter state and wait 2 seconds before retrying.
  * When at least one controller (Classic BT or BLE) is connected, insert a 3-second cooldown between BLE scan windows while the pairing screen is open so active controllers have uncontested radio airtime.

---

### Issue 3.2 (P2 Medium): Connection Parameter Clamping Bug, IPC Under Event Lock & Joy-Con Coexistence

* **Files Affected:**
  * `mc_mitm/source/ble/ble_events.cpp:334–364, 801–824`
  * `mc_mitm/source/ble/ble_hid_host.cpp:442–451`
* **Root Cause:**
  1. `OnBleEvent` calls `btdrvSetBleConnectionParameter` synchronously on `mc::EventThread` while holding `g_event_data_lock` and `g_lock`, and does so *during* `btdrvCreateBond` (`ble.log` line 86).
  2. In `ble_events.cpp:355`:
     ```cpp
     .min_conn_interval = std::max<u16>(6, pref.min_conn_interval),
     .max_conn_interval = std::max<u16>(pref.min_conn_interval, std::min<u16>(max_interval, pref.max_conn_interval)),
     ```
     Because the Xbox controller sends `pref.min_conn_interval = 6, pref.max_conn_interval = 6`, `std::min(max_interval, 6)` evaluates to `6`, forcing `max_conn_interval = 6` (7.50 ms) even when `max_interval = 12`! And if `pref.max_conn_interval < 6`, `min_conn_interval (6) > max_conn_interval`, producing invalid parameters.
  3. `RequestLowLatencyConnectionParameters` only counts BLE controllers (`GetConnectedCount()`), ignoring connected Classic Bluetooth Joy-Cons (`controller::GetConnectedControllerCount()`). When wireless Joy-Cons (15.0 ms sniff interval = 12 BLE units) are connected alongside a BLE controller, allowing `max_conn_interval = 12` (15.0 ms harmonic interval) avoids TDMA collisions.
* **Resolution:**
  * Remove the synchronous `btdrvSetBleConnectionParameter` call from `OnBleEvent` (deferring connection parameter updates to `HostThreadFunc` after pairing and subscription succeed).
  * Account for total connected controllers (Classic BT + BLE) when choosing `max_conn_interval = (total_active > 1) ? 12 : 9`, and only re-request connection parameters on existing BLE links when their target `max_interval` actually changes.

---

### Issue 4.1 (P1 High): Global `g_gatt_results` Race Between Active Rumble and New Controller Subscription

* **Files Affected:**
  * `mc_mitm/source/ble/ble_hid_host.cpp:181`
  * `mc_mitm/source/ble/ble_events.cpp:469–498`
  * `mc_mitm/source/ble/ble_gatt.cpp:194–209, 241–249`
* **Root Cause:**
  `events::NoteGattResult(notify.result)` increments a single global `g_gatt_results` and sets a single global `g_gatt_failure`, and `WaitForGattResult(slot_index, count_before, timeout)` checks the global counter without filtering by `slot_index` or `conn_id`.
  When Controller 0 is connected and receives rumble writes (`notify.type = 1` on `GattCharReport`), those write completions increment `g_gatt_results` and wake `WaitForGattResult` prematurely while Controller 1 is in `gatt::Subscribe()`.
* **Resolution:**
  Track `gatt_results` and `gatt_failure` per slot inside `g_slots[slot]`, route `NoteGattResult(notify.conn_id, notify.type, notify.char_uuid, notify.result)` by `conn_id`, and ignore output report write completions (`notify.type == 1` on `GattCharReport`).

---

### Issue 4.2 (P1 High): `Adopt()` Path Leaves Slot Inactive in `ble_events.cpp`

* **Files Affected:**
  * `mc_mitm/source/ble/ble_hid_host.cpp:560–590`
  * `mc_mitm/source/ble/ble_events.cpp:173–218`
* **Root Cause:**
  When `Adopt(&address, &conn_id)` succeeds in `ScanAndPairNextController()`, `Connect()` is bypassed, so `events::SetSlotConnecting(slot, address)` is never called. Subsequent calls to `SetSlotConnection(slot, conn_id)` and `SetSlotClaimed(slot, true)` do not set `g_slots[slot].active = true` or `g_slots[slot].address = address`. As a result, `OnCoreEvent` fails to match the slot during `Pair(slot, address)`, causing a guaranteed 10-second pairing timeout.
* **Resolution:**
  Ensure `SetSlotConnection(slot, address, conn_id)` always sets `g_slots[slot].active = true` and `g_slots[slot].address = address`, and remove all legacy single-slot globals (`g_target`, `g_claimed`, `g_conn_id`, `g_conn_id_fast`, `g_disconnected`, `g_bond_complete`).

---

### Issue 4.3 (P2 Medium): Input/Output Report Data Race & `DismissSlot()` Teardown Synchronization

* **Files Affected:**
  * `mc_mitm/source/controllers/xbox_one_ble_controller.hpp` / `.cpp`
  * `mc_mitm/source/ble/ble_hid_host.cpp:456–492`
* **Root Cause:**
  1. `XboxOneBleController::HandleInputReport` runs on `mc::BleHidEventThread` while `HandleOutputDataReport` runs concurrently on `mc::BtdrvMitmThread`, mutating shared `EmulatedSwitchController` state (`m_input_report_mode`, `m_hid_command_processor`, `m_latency_timer++`) without synchronization.
  2. `DismissSlot()` uses `os::SleepThread(20ms)` to wait for in-flight `HandleInputReport` calls, sets `slot.state = Vacant` before `gatt::ResetSlot()` / `events::ClearSlot()`, and calls `btdrvDisconnectGattServer(conn_id)` even when the link already disconnected on its own.
* **Resolution:**
  Add `os::SdkMutex m_report_mutex` in `XboxOneBleController` protecting `HandleInputReport` and `HandleOutputDataReport`, and acquire `m_report_mutex` (or synchronize via `UnregisterSlotController`) during `DismissSlot()` before detaching, checking `!was_disconnected` before calling `btdrvDisconnectGattServer(conn_id)`.

---

### Issue 4.4 (P2 Medium): `BtmMitmService` Virtual Device Condition Template Bug

* **Files Affected:**
  * `mc_mitm/source/btm_mitm/btm_mitm_service.cpp:69–88, 117–135`
* **Root Cause:**
  In `GetDeviceCondition` and `GetShortenedDeviceCondition`, when `total_out.GetValue() > 0`, the code copies `*device = device_condition[0];` as a template for virtual BLE controllers. If `device_condition[0]` is a Joy-Con (`vid = 0x057e, pid = 0x2006`) or a Bluetooth Audio device (`profile = BtmProfile_Audio`), the virtual BLE controller inherits the wrong `vid`/`pid` or audio union data.
* **Resolution:**
  Always zero-initialize the virtual `BtmDeviceCondition` / `BtmShortenedDeviceCondition` entry, explicitly set `profile_condition.hid.vid = 0x057e`, `pid = 0x2009`, `sniff_mode = BtmSniffMode_15ms`, `slot_mode = BtmSlotMode_2`, and avoid adding duplicate entries if the address is already present in `device_condition`.

---

## 5-Player Super Smash Bros. Ultimate Hardware Test Validation (7 Concurrent Controllers)

During hardware stress testing in *Super Smash Bros. Ultimate* (SSBU) with 7 concurrent wireless Bluetooth controllers (4 Joy-Cons + 1 Pro Controller + 2 BLE Xbox Series S controllers), two hardware test issues were diagnosed, root-caused, and resolved:

### Issue 11 (SSBU Match Xbox `B` / Switch `A` Fast-Attack Lockout)

* **Files Affected:**
  * `mc_mitm/source/controllers/xbox_one_controller.cpp`
* **Root Cause:**
  Xbox Series X|S BLE input reports (`0x01`) transmit uncalibrated 16-bit raw ADC stick values (`0..65535`, center `0x8000`) designed for Microsoft's XInput software radial deadzones (`XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE = 7849` / `23.95%`, `XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE = 8689` / `26.52%`), whereas `SwitchVirtualControllerMemory` (`DefaultAnalogStickModelValues`) only configures a `12.5%` (`0x100/0x800`, `0x1000` in 16-bit) inner deadzone in SPI flash. When one Xbox controller's Right Stick rests between `12.5%` and `26.5%` off-center, it works normally in SSBU menus (which ignore the Right Stick), but inside an SSBU match the Right Stick acts as the C-Stick (`Direction + A` attack macro) which locks out the `A` button (fast attack) until the stick returns inside the neutral deadzone.
* **Resolution:**
  Added `ApplyRadialDeadzone` in `XboxOneController::MapInputReport0x01` (`xbox_one_controller.cpp`) that snaps deflections within the XInput deadzone (`7849` left, `8689` right) to exact center (`0x8000`) and smoothly rescales `(deadzone, 0x7fff]` onto `(HosInnerDeadzone (0x1000), 0x7fff]` so it meets Horizon OS `hid`'s 12.5% inner deadzone edge with zero step jump and zero deadzone stacking.

---

### Issue 12 (4th Joy-Con Input Delay & Latency with 7 Concurrent Bluetooth Controllers)

* **Files Affected:**
  * `mc_mitm/source/ble/ble_events.hpp`
  * `mc_mitm/source/ble/ble_events.cpp`
  * `mc_mitm/source/ble/ble_hid_host.cpp`
  * `mc_mitm/source/controllers/controller_management.cpp`
  * `mc_mitm/source/controllers/xbox_one_ble_controller.cpp`
* **Root Cause:**
  1. `RequestLowLatencyConnectionParameters` hardcoded `min_conn_interval = 6` (`7.5 ms`), causing the Broadcom BCM4356 link layer to negotiate `7.5 ms` (`133.3 Hz`) for both BLE Xbox controllers (`4` BLE anchor events per `15 ms` BR/EDR sniff frame) and preempt the 5th Classic BT device (the 4th Joy-Con) at the tail of each 15 ms piconet frame.
  2. `ble_hid_host` only rebalanced BLE connection intervals when BLE controllers connected/disconnected, ignoring Classic BT Joy-Con attach/detach events.
  3. `mc::BleHidEventThread` ran at priority `-11`, contending directly with `mc::HidReportThread` (`-11`).
  4. 60 Hz rumble across 7 controllers saturated BLE GATT writes.
* **Resolution:**
  * Scaled BLE connection intervals in `ble_events.cpp` to `min=6, max=9` (`7.5–11.25 ms`) for 1 controller, `min=12, max=12` (`15.0 ms`) for 2–3 controllers, and `min=16, max=20` (`20.0–25.0 ms`) for 4+ controllers (preventing 15.0 ms phase-lock with the 4th Joy-Con).
  * Triggered `ble::events::NotifyControllerCountChanged()` from `controller::AttachHandler` and `RemoveHandler` to dynamically rebalance BLE slots when Classic BT controllers connect/disconnect.
  * Adjusted `LowLatencyThreadPriority` to `-10` (1 level below `mc::HidReportThread`).
  * Added 3-tier dynamic rumble throttling (`32 ms` / `64 ms` / `96 ms`) in `XboxOneBleController::SetVibration`.

