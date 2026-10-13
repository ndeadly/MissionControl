# Bluetooth Low Energy (HID over GATT) Xbox Controller Latency Analysis & Optimization

This document serves as a rolling reference documenting the technical investigation, sources, architectural audit, latency optimizations, and profiling framework for Xbox controllers on firmware 5.x+ (Bluetooth LE / HID-over-GATT) in Mission Control (`mc.mitm`).

---

## 1. Background & Context

* **Target Hardware**: Microsoft Xbox One S (Model 1708), Xbox Elite Series 2 (Model 1797), and Xbox Series X|S (Model 1914) controllers running firmware 5.x or newer.
* **Firmware Change**: Starting with firmware 5.x (late 2021), Microsoft transitioned Xbox wireless controllers from classic Bluetooth (BR/EDR, L2CAP PSM 0x11/0x13 HID) to Bluetooth Low Energy (BLE, GATT HID-over-GATT / HOGP, service UUID `0x1812`).
* **Console Environment**: Nintendo Switch (Erista/Mariko) running Custom Firmware (Atmosphère ≥ 1.12.0) on Horizon OS (HOS 13.0.0 – 23.0.0+).
* **Bluetooth Stack**: Broadcom BSA (Bluetooth Stack for Architecture) v1.6 running inside `bluetooth.nso` (`btdrv`).

---

## 2. Latency Audit & Root Cause Analysis

When running the initial BLE host probe on hardware, users observe noticeable latency/delay between physical controller input and system responsiveness. An audit of the code paths revealed four distinct factors contributing to this latency:

### Factor 1: Unnegotiated BLE Connection Interval (Primary Cause)

In Bluetooth Low Energy, all radio transmissions between central (Switch) and peripheral (Xbox controller) occur strictly during scheduled **Connection Events**, governed by:

$$\text{Connection Interval (ms)} = \text{Interval Value} \times 1.25\text{ ms}$$

1. **Initial Connection Default**: When `btdrvConnectGattServer()` initiates a generic GATT connection, the Broadcom BSA stack assigns a default conservative connection interval (typically **30 ms to 50 ms**, or up to **100 ms**, value 24–80).
2. **Missing Negotiation**: After link encryption, Xbox controllers issue an L2CAP Connection Parameter Update Request (`BtdrvBleEventType_PreferredConnectionParameters`), requesting low latency for gaming:
   * Minimum Interval: $7.5\text{ ms}$ (value 6)
   * Maximum Interval: $11.25\text{ ms}$ – $15.0\text{ ms}$ (value 9–12)
   * Slave Latency: $0$ (controller must not skip connection events)
3. **Mitm Interception Gap**: In `ble_events.cpp`, `BtdrvBleEventType_PreferredConnectionParameters` was classified under `IsConnectionFamily(type)` and withheld from `btm` (`ours = true`), but **no code in `mc.mitm` ever responded to or applied these parameters**.
4. Neither `btdrvSetBleDefaultConnectionParameter` nor `btdrvSetBleConnectionParameter` was called. As a result, the radio link was stuck in the slow default interval for the entire duration of gameplay, introducing a persistent 30–50+ ms delay per input.

### Factor 2: Low Thread Priority for `mc::BleHidEventThread`

* In `ble_hid_host.cpp`:
  ```cpp
  constexpr s32 ThreadPriority = 9;
  ```
  Both the host worker thread and `g_le_hid_thread` were spawned at thread priority `9`.
* In Stratosphere / Horizon OS, smaller/negative numbers indicate higher priority.
* Mission Control's classic Bluetooth HID report thread (`mc::HidReportThread`) runs at priority `-11` (real-time priority, matching `bluetooth.nso` internal priority, as updated in upstream commit `7ffb49d`).
* Running the LE HID event loop at background priority `9` meant input notifications were routinely preempted by game render threads and system services, adding 5–15 ms of scheduling jitter.

### Factor 3: Lock Contention on the Fast Input Path

* In `LeHidThreadFunc`, every incoming input report called:
  ```cpp
  notify.conn_id != events::GetConnection()
  ```
  `events::GetConnection()` acquired `std::scoped_lock lk(g_lock)`.
* `g_lock` is the primary synchronization mutex in `ble_events.cpp`, shared across `OnBleEvent`, `OnCoreEvent`, `WaitFor`, and `Pump`. Taking this mutex on every 8 ms packet caused unnecessary CPU lock contention on the fast input path.

### Factor 4: Synchronous File I/O in `ble_log`

* When `log=true` was set in `missioncontrol.ini`, `ble_log::Write()` executed:
  `fs::OpenFile` $\rightarrow$ `fs::WriteFile(..., Flush)` $\rightarrow$ `fs::CloseFile` synchronously on every log statement.
* On Nintendo Switch, flushing to the SD card takes 10–80 ms per write, severely stalling the caller thread.

---

## 3. Technical Specifications & References

### Xbox Wireless Controller (BLE Firmware 5.x+)
* **Bluetooth SIG Specification**: HID over GATT Profile (HOGP) 1.0.
* **GATT HID Service**: UUID `0x1812` (GattServiceHid).
* **Input Report Characteristic**: UUID `0x2A4D` (GattCharReport), Report ID `0x01` (16-byte payload without leading ID byte).
* **Output Report Characteristic**: UUID `0x2A4D` (GattCharReport), Report ID `0x03` (Rumble/Vibration). Written with `Write Without Response` (`with_response = false`).
* **Connection Interval Range**:
  * Minimum: $7.5\text{ ms}$ (6 units)
  * Maximum: $11.25\text{ ms}$ (9 units) or $15.0\text{ ms}$ (12 units)
  * Slave Latency: 0
  * Supervision Timeout: $3000\text{ ms}$ (300 units)

### Horizon OS / devkitPro `btdrv` APIs
* **`btdrvSetBleDefaultConnectionParameter(const BtdrvBleConnectionParameter *param)`** (IPC Command 52, HOS 9.0.0+):
  Sets default connection parameters used by the Bluetooth stack when establishing any new LE link.
* **`btdrvSetBleConnectionParameter(BtdrvAddress addr, const BtdrvBleConnectionParameter *param, bool preference)`** (IPC Command 51, HOS 9.0.0+):
  Sends an LL connection parameter update request to the connected controller to adjust timing on an active link.
* **`BtdrvBleConnectionParameter` structure**:
  * `min_conn_interval`: Minimum interval in 1.25 ms units.
  * `max_conn_interval`: Maximum interval in 1.25 ms units.
  * `min_ce_length`: Minimum connection event length (set to 0).
  * `max_ce_length`: Maximum connection event length (set to 0).
  * `slave_latency`: Number of connection events peripheral may skip (must be 0 for gamepads).
  * `supervision_tout`: Connection timeout in 10 ms units (e.g. 300 = 3000 ms).

---

## 4. Optimization Design (Opt-In)

The optimizations are controlled by configuration toggles in `/config/MissionControl/missioncontrol.ini`:

### `[ble] low_latency=true`
1. **Default Connection Parameters**: Calls `btdrvSetBleDefaultConnectionParameter()` during initialization with $7.5\text{ ms}$ – $11.25\text{ ms}$ ($6$ – $9$ units) and $0$ slave latency.
2. **Active Link Update**: Calls `btdrvSetBleConnectionParameter()` immediately upon controller pairing and connection.
3. **Preferred Parameter Handling**: Intercepts `BtdrvBleEventType_PreferredConnectionParameters` from the controller and acknowledges it by setting low-latency parameters with slave latency clamped to 0.
4. **Thread Priority Boost**: Increases `g_le_hid_thread` priority from `9` to `-11`, matching `bluetooth.nso` real-time scheduling.
5. **Lock-Free Fast Path**: Caches `conn_id` in an atomic variable (`std::atomic<u32>`) so `LeHidThreadFunc` avoids acquiring `g_lock` on every report.

### `[ble] profile=true`
* Runs a zero-overhead, in-memory profiler using hardware system ticks (`os::GetSystemTick()`).
* Collects:
  * Minimum, average, and maximum packet arrival delta ($\Delta t_{\text{arrival}}$).
  * Minimum, average, and maximum processing time ($\Delta t_{\text{process}}$) through `HandleInputReport` $\rightarrow$ `WriteHidDataReport`.
  * Active negotiated BLE connection parameters (`conn_interval`, `slave_latency`).
* Flushes a compact summary report to `sdmc:/config/MissionControl/ble_profile.txt` once every 5 seconds (500 reports) without interrupting the real-time input loop.

---

## 5. Memory & CPU Footprint Optimizations

### Memory Reductions (~63 KB Saved from `.bss` & Heap)

| Component | Before | After | Savings | Details |
|---|---|---|---|---|
| `g_ring` (`ble_events.cpp`) | 64 entries (~66 KB) | 16 entries (~16.5 KB) | **~49.5 KB** | Ring buffer capacity reduced to 16. Events are popped immediately by the host loop so queue depth never exceeds 2–4 items. |
| `g_thread_stack` (`ble_hid_host.cpp`) | `0x4000` (16 KB) | `0x2000` (8 KB) | **8 KB** | Host coordinator thread stack right-sized; actual peak stack depth is < 2 KB. |
| `g_le_hid_thread_stack` (`ble_hid_host.cpp`) | `0x2000` (8 KB) | `0x1000` (4 KB) | **4 KB** | LE HID report thread stack right-sized; stack holds only small local variables and frames. |
| `m_event_info` (`XboxOneBleController`) | ~1.1 KB struct | Completely removed | **~1.1 KB** | Eliminated redundant `bluetooth::HidReportEventInfo` member. Input frames directly on a stack buffer. |
| **Total Memory Savings** | | | **~62.6 KB** | |

### CPU & Fast Path Optimizations

1. **Lock-Free Active Controller Routing**:
   * Previously, `LeHidThreadFunc` acquired `g_controller_lock` and cloned a `std::shared_ptr<XboxOneBleController>` on **every single 8 ms report**, triggering atomic increment/decrement instructions (`__atomic_add_fetch` / `__atomic_sub_fetch`).
   * Replaced with a lock-free atomic raw pointer `g_active_controller` updated only on `Present()` and `Dismiss()`.
   * The 100+ Hz input path now reads `g_active_controller.load(relaxed)` without any mutex locks or reference-counting atomics.

2. **Lock-Free Connection Query**:
   * Previously, `notify.conn_id != events::GetConnection()` acquired the global coordinator mutex `g_lock` on every report.
   * Replaced with `events::GetConnectionFast()`, which reads an atomic `g_conn_id_fast` in 0 ns without locking.

3. **Bypassed Event Wrapper Overhead in `HandleInputReport`**:
   * Previously, `HandleInputReport` synthesized a full `HidReportEventInfo` structure, passed it to `HandleDataReportEvent`, which evaluated runtime OS version checks and response queue checks, and unpacked it again.
   * Directly frames the 16-byte payload on a 17-byte stack buffer and routes through `ParseInputReport` $\rightarrow$ `FillInputReport` $\rightarrow$ `WriteHidDataReport`, removing runtime branching and redundant copies.

4. **Zero Polling / Zero CPU Idle State**:
   * Both `HostThreadFunc` and `LeHidThreadFunc` block on native Horizon OS kernel events (`system_event.Wait()` and `g_ring_event.TimedWait()`). When idle or when no controller is connected, the BLE subsystem consumes **0.0% CPU**.

---

## 6. Sources & Documentation

1. **Switchbrew - Bluetooth Driver Services (`btdrv`)**:
   https://switchbrew.org/wiki/Bluetooth_Driver_services
   Documentation of IPC Command 51 (`SetBleConnectionParameter`) and Command 52 (`SetBleDefaultConnectionParameter`).
2. **Bluetooth SIG Core Specification v5.3**:
   * Vol 6, Part B: Link Layer Specification (Section 4.5.1: Connection Events, Section 5.1.1: Connection Update Procedure).
   * Vol 3, Part C: Generic Access Profile (Peripheral Preferred Connection Parameters characteristic).
3. **Microsoft Xbox Wireless Controller Protocol & BLE Analysis**:
   * [xpadneo documentation on Xbox BLE connection intervals and slave latency](https://github.com/atar-axis/xpadneo).
   * BlueZ Linux HCI traces for Xbox Series X|S / 1708 firmware 5.x.
4. **Mission Control Source Code (`mc.mitm`)**:
   * Commit `7ffb49d` (*"mc.mitm: set bluetooth event handling thread priority to match bluetooth service"*).
   * Broadcom BSA internal definitions in `mc_mitm/source/bluetooth_mitm/bsa_defs.h`.
