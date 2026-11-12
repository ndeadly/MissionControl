# Bluetooth 23.0.1 peer-removal path notes

Static analysis of the extracted Bluetooth module only; this does not prove what the Switch controller firmware or persistent key store does on hardware.

## Binary identity and reproduction

- Extracted module: Bluetooth NSO0 from the local 23.0.1 firmware dump (`Program #0/0/main`)
- NSO SHA-256: `9d7775dae8893cdb0b1e26e3c1709364fae6a56984292cbe4629451dfd814532`
- NSO build ID: `064521c83399e09105d9a190688bf8b8c2c7f17c`
- Decompressed `.text` SHA-256: `432a3f080809d4dab6c6174fe85c05ac51fd287b6c6d5f9a7bca5e541fd18d8e`
- The extractor emits `image.elf` as a section-based AArch64 disassembly wrapper, not a loadable module.

Recreate the extraction and disassembly with the local helper/toolchain:

```sh
python3 /tmp/mc-native-analysis/nso_extract.py \
  '/home/aarush/switch/MissionControl/nxdt_rw_poc/NCA FS/System/Extracted/bluetooth [010000000000000B][v1543505072][SYSPRG]/Program #0/0/main' \
  --out-dir /tmp/mc-native-analysis/bluetooth-23.0.1
llvm-objdump --disassemble --section=.text --triple=aarch64 \
  /tmp/mc-native-analysis/bluetooth-23.0.1/image.elf
python3 tools/verify_ble_hook.py verify \
  --ips /tmp/mc-native-analysis/patches/064521C83399E09105D9A190688BF8B8C2C7F17C.ips.corrected \
  --text /tmp/mc-native-analysis/bluetooth-23.0.1/text.bin \
  --build-id 064521C83399E09105D9A190688BF8B8C2C7F17C
```

## RemoveBond path and cache gate

Horizon `btdrv` RemoveBond command 11 wrapper at `0x11340` obtains common and LE interfaces through getters at `0xd190` and `0xd120`, then invokes the common bookkeeping method `0x3d70`, the LE bookkeeping method `0x8c10`, and the final core method `0x5230`. The first two methods clear their respective address-scoped bookkeeping. The final core method calls `0xcc10`, which searches a 20-entry address-keyed paired-device info table. The same table and address comparison are used by GetPairedDeviceInfo (`0xc580`, exposed by command 25). If `cc10` finds no row, it returns success without issuing API 308; the earlier common and LE methods have still run. If found, it queues API 308 with that six-byte address and clears the local table entry after successful submission.

Therefore a successful RemoveBond IPC result alone does not establish that the native API 308 peer-removal handler ran: an absent cached paired-info row makes the final core callback a no-op, although the earlier common and LE bookkeeping callbacks have run. This behavior is directly visible in this build.

## API 308 native handler

API 308 is dispatched at `0x4f770`; it forwards the six-byte address and two `1` flags to `0x6e2b0`. That function allocates internal message `0x010c` and posts it through `0x83e10`. The task dispatcher at `0x83ee0` selects event-group 1; its handler table at `0x159d00` starts at `0x74540`, which dispatches low byte `0x0c` to `0x70420`.

For the `(delete=1, transport=1)` flags used by API 308, `0x70420` checks for an active transport-1 ACL matching the address via `0xa1880`/`0x9f7e0`:

- If an ACL is present, the handler takes the connected-peer path through `0xa2bd0`; that path operates on the matching connection record and can request disconnect for its handle. It does not run the disconnected cleanup below.
- If no transport-1 ACL is present, `0x70420` calls `0xabd10` for the address, then queues internal messages `0x1f02` (`0x75e70`) and `0x1f0c` (`0x76d60`) with that same address.

`0xabd10` checks matching ACLs for transport selectors 2 and 1 and returns without resetting the record if either is live. Otherwise it looks up that address's record (`0xab750`), clears the halfword at record offset `+0x46`, resets associated per-peer fields through `0xbbed0`, and calls `0xae9c0(address, NULL)`. These operations are address-scoped in the inspected code; no all-peer table clear or global radio reset is on this path.

This establishes a real per-peer native cleanup path beyond the wrapper cache. The inspected instructions do **not** identify the record field at `+0x46` as an SMP key, prove deletion of an LTK/IRK from persistent storage, or show that the Xbox controller retains or clears any particular security material. The follow-up callback from `ae9c0` is not decoded here far enough to make a stronger key-store claim. Treat API 308 as a targeted native peer-state cleanup request, not as a proven hardware bond/key erase.

## Diagnostic limit

The observed repeat-connect failure and recovery after all controllers are powered off are consistent with a stale per-peer or connection-security state, but this binary trace does not identify the precise cause of status `0x0016`. The exact SMP/security record, whether removal resolves the hardware case, and any durable-key effect remain unverified until tested on the console. This note makes no claim about the meaning of hidden key flags or native security internals beyond the instruction evidence above.
