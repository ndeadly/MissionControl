# Technical Specification: Multi-Device Bluetooth LE HID Host Architecture for Mission Control

**Document Version:** 1.0.0  
**Target Architecture:** Nintendo Switch (Horizon OS / Atmosphère CFW ≥ 1.12.0)  
**Target Hardware:** Broadcom BCM4356 (Wi-Fi/Bluetooth Combo Chip), BSA Stack v1.6  
**Target Gamepads:** Microsoft Xbox Wireless Controllers (Model 1914 Series X|S, Model 1797 Elite 2, Model 1708 5.x+)  
**Branch:** `ble-xbox-latency` / `ble-xbox-multidevice`  

---

## 1. Executive Summary & Goals

### 1.1 Objective
Refactor the Bluetooth Low Energy (HID over GATT) subsystem in Mission Control (`mc.mitm`) from a single-device probe into a robust, high-performance **Multi-Device BLE Host** capable of supporting up to **4 concurrent Xbox Model 1914 controllers** simultaneously without introducing input latency.

### 1.2 Success Criteria
1. **Concurrent Connections:** Up to 4 BLE Xbox controllers can connect, pair, and send inputs simultaneously.
2. **Sub-10ms Input Latency:** Each connected BLE controller maintains an active negotiated Link Layer Connection Interval between **7.50 ms and 11.25 ms** (88 Hz – 133 Hz polling rate) with 0 slave latency.
3. **Lock-Free Fast Path:** Report demultiplexing in `LeHidThreadFunc` operates in $\mathcal{O}(1)$ time without mutex contention or reference-counting atomics.
4. **Coexistence:** Classic Bluetooth devices (official Joy-Cons, Nintendo Switch Pro Controllers, Bluetooth Classic Xbox Model 1708) can operate concurrently alongside active BLE links without interference.
5. **Memory Predictability:** Static footprint remains bounded and predictable within `.bss`, avoiding unbounded heap allocations.

---

## 2. Hardware & Protocol Constraints Analysis

### 2.1 Broadcom BCM4356 & BSA 1.6 Capabilities
* **Link Layer Capacity:** The Broadcom BCM4356 controller supports up to **7 concurrent BLE ACL connections** and up to 4 simultaneous active Bluetooth Classic (BR/EDR) links in hardware.
* **GATT Client Interfaces (`client_if`):**
  * The Broadcom BSA stack allocates client interfaces per application layer.
  * In Horizon OS, Nintendo's `btm.nso` claims interfaces 3, 4, and 5, leaving `client_if=6` available.
  * **Critical Insight:** A single `client_if` can maintain **multiple concurrent connection IDs (`conn_id`)**. All GATT operations (`btdrvReadGattCharacteristic`, `btdrvWriteGattDescriptor`, `btdrvSetBleConnectionParameter`) take explicit `conn_id` or `BtdrvAddress` arguments.

### 2.2 Time-Division Multiple Access (TDMA) & Airtime Physics
In Bluetooth Low Energy, all physical transmissions occur within scheduled Connection Events:
$$\text{Interval (ms)} = \text{Interval Units} \times 1.25\text{ ms}$$
* **Transmission Duration:** A standard 16-byte HOGP notification plus Link Layer acknowledgment occupies $\approx 0.6\text{ ms} - 0.8\text{ ms}$ on air.
* **Scheduling with 1 Controller:** 7.50 ms interval $\rightarrow$ $1 \times 0.8\text{ ms} = 0.8\text{ ms}$ airtime ($10.6\%$ duty cycle).
* **Scheduling with 4 Controllers:**
  * For 4 controllers, if all 4 negotiate $7.50\text{ ms}$ – $11.25\text{ ms}$, the Link Layer scheduler interleaves their connection events in time slices:
    * $t = 0.0\text{ ms}$: Device 0
    * $t = 2.5\text{ ms}$: Device 1
    * $t = 5.0\text{ ms}$: Device 2
    * $t = 7.5\text{ ms}$: Device 3 (or interleaved across 11.25 ms)
  * Airtime utilization is $\approx 4 \times 0.8\text{ ms} = 3.2\text{ ms}$ per 11.25 ms ($28.4\%$ duty cycle).
  * **Conclusion:** The 2.4 GHz radio retains ample clearance for Bluetooth Classic co-existence while delivering 88–133 Hz polling per pad without queue latency.

### 2.3 The Broadcom BSA 1.6 Security Subsystem
* Nintendo's `bluetooth.nso` only implements **LE Legacy Pairing (Just Works)**; it lacks LE Secure Connections (SC) cryptographic primitives (P-256 ECC).
* Xbox controllers on firmware 5.x accept Legacy Just Works for an active session, but do not store an LTK in non-volatile flash.
* Sequenced bonding (`btdrvCreateBond`) must be managed serially during pairing so BSA's SMP state machine processes one pairing handshake at a time without colliding.

---

## 3. Architecture & Data Structures Design

```
+-----------------------------------------------------------------------------------+
|                               mc.mitm BLE Subsystem                                |
+-----------------------------------------------------------------------------------+
                                          |
                +-------------------------+-------------------------+
                |                                                   |
      [ble_events Coordinator]                              [ble_hid_host]
      - Multi-Device Slot Array                             - Non-Blocking Coordinator Loop
      - Fast O(1) Atomic ConnId Router                      - Serialized Pairing Engine
      - Event Filter & Interceptor                          - Low-Latency Link Upgrader
                |                                                   |
                +-------------------------+-------------------------+
                                          |
                                [LeHidThreadFunc]
                     (Real-Time Priority -11 Fast Input Path)
                                          |
                           [g_conn_table: O(1) Lookup]
                                          |
                  +-----------------------+-----------------------+
                  |                       |                       |
            [Slot 0: BLE Pad]       [Slot 1: BLE Pad]       [Slot 2: BLE Pad]
            - ConnId = 6            - ConnId = 7            - ConnId = 8
            - Virtual Device #1     - Virtual Device #2     - Virtual Device #3
                  |                       |                       |
                  +-----------------------+-----------------------+
                                          |
                              [WriteHidDataReport]
                                          |
                             [Horizon OS Shared Memory]
```

### 3.1 Device Slot Definition (`BleDeviceSlot`)
A dedicated slot structure encapsulate all state for a single BLE gamepad:

```cpp
constexpr size_t MaxBleControllers = 4;
constexpr u32 MaxConnIdRange = 32;

struct BleDeviceSlot {
    enum class State : u8 {
        Vacant,
        Connecting,
        Discovering,
        Pairing,
        Connected,
        Disconnecting
    };

    State state = State::Vacant;
    u32 conn_id = InvalidConnId;
    bluetooth::Address address = {};
    gatt::HidService hid = {};
    std::shared_ptr<controller::XboxOneBleController> controller = nullptr;
    s64 last_active_tick = 0;
};
```

### 3.2 Lock-Free Report Dispatch Table
To maintain real-time performance on the 133 Hz input stream without acquiring mutexes or refcounting `std::shared_ptr`:

```cpp
// Direct atomic pointer table indexed by conn_id (0..31)
constinit std::atomic<controller::XboxOneBleController *> g_active_controllers[MaxConnIdRange] = {};
```
* **Read Path (`LeHidThreadFunc`):**
  ```cpp
  if (notify.conn_id < MaxConnIdRange) {
      if (auto *ctrl = g_active_controllers[notify.conn_id].load(std::memory_order_relaxed)) {
          ctrl->HandleInputReport(notify.data, notify.size);
      }
  }
  ```
  **Latency:** 0 ns. Single indexed pointer dereference. Zero locks.

---

## 4. Module Refactoring Specifications

### 4.1 Module 1: `ble_events` (Multi-Device Event Tracking)
* **Current Limitation:** Single scalar variables (`g_conn_id`, `g_target`, `g_have_target`, `g_claimed`).
* **Refactored Architecture:**
  1. Maintain `BleDeviceSlot g_slots[MaxBleControllers]`.
  2. Implement `FindSlotByConnId(u32 conn_id)` and `FindSlotByAddress(const bluetooth::Address &addr)`.
  3. `OnBleEvent`:
     * Match `BtdrvBleEventType_ClientConnection` to slot by address/conn_id.
     * Route `BtdrvBleEventType_ConnectionUpdate` and `PreferredConnectionParameters` to slot.
     * Consume connection-family events belonging to any active slot (`ours = true`).
  4. `OnCoreEvent`:
     * Verify whether incoming SSP/Connection event matches the address of *any* active slot before intercepting.

### 4.2 Module 2: `ble_gatt` (Multi-Table Attribute Caching)
* **Current Limitation:** Single static `AttributeTable g_table` holding 64 attributes.
* **Refactored Architecture:**
  1. Allocate per-slot tables: `AttributeTable g_tables[MaxBleControllers]`.
  2. Parameterize `DiscoverServices(u32 slot_index, const bluetooth::Address &address, u32 conn_id)`.
  3. Parameterize `Subscribe(u32 slot_index, u32 conn_id, HidService *out_hid)`.
  4. Ensure GATT cache saves (`ClientCacheSave`) are routed to the specific slot's table based on `conn_id`.

### 4.3 Module 3: `ble_hid_host` (Non-Blocking Coordinator Engine)
* **Current Limitation:** `HostThreadFunc` calls `Present()` and blocks in `RunConnected(conn_id)` indefinitely.
* **Refactored Architecture:**
  1. **Non-Blocking Coordinator Loop:**
     * Coordinator continuously runs:
       ```cpp
       for (;;) {
           WaitForRadio();
           ProcessPendingDisconnections();
           if (events::IsPairingWindowOpen() && (GetActiveCount() < MaxBleControllers)) {
               ScanAndPairNextController();
           }
           events::Pump(TimeSpan::FromMilliSeconds(100));
       }
       ```
  2. **Filtering Already-Connected Devices:**
     * During `Scan()`, compare advertisement address against all connected slots. If already connected, skip immediately.
  3. **Sequential Pairing Queue:**
     * Only one controller performs `btdrvCreateBond` and GATT discovery at any given second. Once presented, slot state transitions to `Connected`, freeing the pairing engine to pick up the next controller while *Change Grip/Order* remains open.

### 4.4 Module 4: Link-Layer Connection Parameter Optimization
* **Current Parameter Scheme:** Requests 6–9 units (7.5 ms – 11.25 ms) on active links.
* **Multi-Device Scheme:**
  * Slot 1 (Single Pad): Requests min 6 (7.5 ms), max 9 (11.25 ms), latency 0.
  * Slot 2–4 (Multiple Pads): Requests min 6 (7.5 ms), max 12 (15.0 ms), latency 0.
  * Setting `max_conn_interval = 12` gives the Broadcom BCM4356 Link Layer scheduler the necessary timing flexibility to interleave 4 connections without packet collision or retries, while ensuring worst-case latency never exceeds 15 ms (< 1 frame at 60 FPS).

---

## 5. Review & Verification Subagent Strategy

To ensure zero regressions, clean concurrent execution, and strict safety, implementation will be executed by a **Worker Agent** and independently reviewed by **Two Specialist Reviewer Subagents**:

### 5.1 Worker Subagent Tasks
1. Construct `BleDeviceSlot` state manager and per-connection routing tables.
2. Refactor `ble_gatt` for multi-slot attribute tables.
3. Update `ble_events` event filtering and atomic dispatch table.
4. Convert `ble_hid_host` coordinator loop to non-blocking multi-device state machine.

### 5.2 Reviewer Subagent 1: Protocol & BSA Stack Concurrency Reviewer
* **Focus Areas:**
  * Broadcom BSA 1.6 concurrency safety and serialization of `CreateBond`.
  * Proper handling of connection drops (`ClientConnection status != 0`).
  * Verify absence of race conditions in `conn_id` assignment and slot recycling.
  * Verify that GATT discovery caches do not leak or cross-contaminate between controllers.

### 5.3 Reviewer Subagent 2: System Architecture, Memory & Real-Time Performance Reviewer
* **Focus Areas:**
  * Real-time priority safety (`LeHidThreadFunc` running at priority `-11`).
  * Verify $\mathcal{O}(1)$ lock-free read path in `LeHidThreadFunc` (`g_active_controllers`).
  * Memory footprint audit: verify `.bss` expansion does not exceed system budget.
  * Verify thread stack usage and prevent stack exhaustion on Horizon OS.
  * Verify coexistence with classic Bluetooth Joy-Cons and standard Pro Controllers.

---

## 6. Implementation Milestones

| Milestone | Deliverables | Verification Criteria |
|---|---|---|
| **M1: Specification & Design** | `spec_multidevice_ble.md` completed and audited | Architecture and concurrency models verified. |
| **M2: Data Structures & Fast Router** | `BleDeviceSlot` array, atomic dispatch table, multi-device `ble_events` | Compiles cleanly; fast-path maintains 0 ns lock-free overhead. |
| **M3: Per-Slot GATT Caching** | Parameterized `ble_gatt` attribute tables | Multiple discovery streams store independent CCCD and Report handles. |
| **M4: Non-Blocking Coordinator Engine** | Continuous scan/pair loop in `ble_hid_host` | Sequential pairing of multiple controllers in *Change Grip/Order*. |
| **M5: Subagent Dual Review** | Protocol Reviewer + Architecture Reviewer passes | All concurrency, safety, and timing criteria satisfied. |
