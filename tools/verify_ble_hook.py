#!/usr/bin/env python3
"""Verify/fix the HOS 23.0.1 Bluetooth IPS hook without bundling firmware."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


BASELINE_IPS_SHA256 = "5f008737e43b75836dca64a2e15f9eab64abedc06c463c95ac3284bb3d85ac3d"
PATCHED_IPS_SHA256 = "6cdeb57f421dfab2d887b773ef5ffe03615cc970c79983a4b8d267f2b337afa0"
TEXT_SHA256 = "432a3f080809d4dab6c6174fe85c05ac51fd287b6c6d5f9a7bca5e541fd18d8e"
BUILD_ID = "064521c83399e09105d9a190688bf8b8c2c7f17c"

HOOK_FILE_OFFSET = 0x11AB0
OLD_HOOK_FILE_OFFSET = 0x11AA0
CAVE_FILE_OFFSET = 0x117E44
REMOVE_BOND_GUARD_FILE_OFFSET = 0x534C
REMOVE_BOND_GUARD_RUNTIME_VA = 0x524C
CAVE_RUNTIME_VA = 0x117D44
HOOK_RUNTIME_VA = 0x119B0
CONTINUE_RUNTIME_VA = 0x119B4

ORIGINAL_TEXT = {
    0x119A0: bytes.fromhex("ffc300d1"),  # sub sp, sp, #0x30
    0x119A4: bytes.fromhex("fd7b01a9"),  # stp x29, x30, [sp, #0x10]
    0x119A8: bytes.fromhex("f44f02a9"),  # stp x20, x19, [sp, #0x20]
    0x119AC: bytes.fromhex("fd430091"),  # add x29, sp, #0x10
    0x119B0: bytes.fromhex("28fc60d3"),  # lsr x8, x1, #32
    0x119B4: bytes.fromhex("e10b00b9"),  # str w1, [sp, #0x8]
    0x119F0: bytes.fromhex("f44f42a9"),  # ldp x20, x19, [sp, #0x20]
    0x119F4: bytes.fromhex("fd7b41a9"),  # ldp x29, x30, [sp, #0x10]
    0x119F8: bytes.fromhex("ffc30091"),  # add sp, sp, #0x30
    0x119FC: bytes.fromhex("c0035fd6"),  # ret
    0x524C: bytes.fromhex("40010036"),  # tbz w0, #0, 0x5274 (peer-info guard)
}


@dataclass
class Record:
    offset: int
    data: bytes
    rle: bool = False


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_ips32(data: bytes) -> list[Record]:
    if not data.startswith(b"IPS32"):
        raise ValueError("input is not IPS32")
    pos = 5
    records: list[Record] = []
    while True:
        if pos + 4 > len(data):
            raise ValueError("truncated IPS32 record header")
        if data[pos : pos + 4] == b"EEOF":
            if pos + 4 != len(data):
                raise ValueError("unexpected bytes after IPS32 terminator")
            return records
        if pos + 6 > len(data):
            raise ValueError("truncated IPS32 record")
        offset = int.from_bytes(data[pos : pos + 4], "big")
        size = int.from_bytes(data[pos + 4 : pos + 6], "big")
        pos += 6
        if size:
            if pos + size > len(data):
                raise ValueError("truncated IPS32 payload")
            payload = data[pos : pos + size]
            pos += size
            records.append(Record(offset, payload))
        else:
            if pos + 3 > len(data):
                raise ValueError("truncated IPS32 RLE payload")
            count = int.from_bytes(data[pos : pos + 2], "big")
            value = data[pos + 2 : pos + 3]
            pos += 3
            if count == 0:
                raise ValueError("zero-length IPS32 RLE record")
            records.append(Record(offset, value * count, True))


def encode_ips32(records: list[Record]) -> bytes:
    out = bytearray(b"IPS32")
    for record in records:
        out += record.offset.to_bytes(4, "big")
        if record.rle:
            if not record.data or any(b != record.data[0] for b in record.data):
                raise ValueError("cannot serialize invalid RLE record")
            out += b"\0\0" + len(record.data).to_bytes(2, "big") + record.data[:1]
        else:
            if not record.data or len(record.data) > 0xFFFF:
                raise ValueError("invalid IPS32 payload size")
            out += len(record.data).to_bytes(2, "big") + record.data
    out += b"EEOF"
    return bytes(out)


def single_record(records: list[Record], offset: int) -> Record:
    found = [record for record in records if record.offset == offset]
    if len(found) != 1:
        raise ValueError(f"expected one record at IPS offset 0x{offset:x}, found {len(found)}")
    return found[0]


def verify_native(text: bytes, supplied_build_id: str) -> None:
    if supplied_build_id.lower() != BUILD_ID:
        raise ValueError(f"wrong BuildID {supplied_build_id}; expected {BUILD_ID}")
    actual_sha = sha256(text)
    if actual_sha != TEXT_SHA256:
        raise ValueError(f"native text SHA-256 mismatch: {actual_sha}")
    for va, expected in ORIGINAL_TEXT.items():
        if text[va : va + len(expected)] != expected:
            raise ValueError(f"native instruction mismatch at VA 0x{va:x}")


def overlay_text(text: bytes, records: list[Record]) -> bytes:
    max_end = max((record.offset - 0x100 + len(record.data) for record in records), default=len(text))
    if max_end < len(text):
        max_end = len(text)
    # The cave occupies the loader-zeroed alignment gap after .text and before .rodata.
    result = bytearray(text) + bytearray(max_end - len(text))
    for record in records:
        if record.offset < 0x100:
            raise ValueError(f"record offset below NSO header at 0x{record.offset:x}")
        runtime_offset = record.offset - 0x100
        end = runtime_offset + len(record.data)
        if end > len(result):
            raise ValueError(f"record at 0x{record.offset:x} exceeds padded module image")
        result[runtime_offset:end] = record.data
    return bytes(result)


def verify_patched_ips(ips: bytes, text: bytes, supplied_build_id: str) -> None:
    verify_native(text, supplied_build_id)
    actual_sha = sha256(ips)
    if actual_sha != PATCHED_IPS_SHA256:
        raise ValueError(f"corrected IPS SHA-256 mismatch: {actual_sha}")
    records = parse_ips32(ips)
    if len(records) != 29:
        raise ValueError(f"unexpected IPS record count: {len(records)}")
    if any(record.offset == OLD_HOOK_FILE_OFFSET for record in records):
        raise ValueError("old prologue-clobbering hook record remains at 0x11aa0")
    hook = single_record(records, HOOK_FILE_OFFSET)
    if hook.data != bytes.fromhex("e5180414"):
        raise ValueError("hook branch at IPS offset 0x11ab0 is incorrect")
    cave = single_record(records, CAVE_FILE_OFFSET)
    if len(cave.data) != 0x2BC:
        raise ValueError("unexpected cave size")
    if cave.data[:12] != bytes.fromhex("3f0000f16000005428fc60d3"):
        raise ValueError("unexpected cave entry bytes")
    if cave.data[12:16] != bytes.fromhex("19e7fb17"):
        raise ValueError("cave continuation does not resume at 0x119b4")
    guard = single_record(records, REMOVE_BOND_GUARD_FILE_OFFSET)
    if guard.data != bytes.fromhex("1f2003d5"):
        raise ValueError("RemoveBond peer-info guard patch is not a NOP")

    patched = overlay_text(text, records)
    expected_hook = struct.pack("<I", 0x140418E5)
    expected_continue = struct.pack("<I", 0x17FBE719)
    if patched[HOOK_RUNTIME_VA : HOOK_RUNTIME_VA + 4] != expected_hook:
        raise ValueError("patched runtime hook branch does not target cave 0x117d44")
    if patched[CAVE_RUNTIME_VA + 12 : CAVE_RUNTIME_VA + 16] != expected_continue:
        raise ValueError("patched cave branch does not target continuation 0x119b4")
    if patched[REMOVE_BOND_GUARD_RUNTIME_VA : REMOVE_BOND_GUARD_RUNTIME_VA + 4] != bytes.fromhex("1f2003d5"):
        raise ValueError("patched RemoveBond guard instruction is incorrect")
    if patched[0x119A0 : 0x119B0] != text[0x119A0 : 0x119B0]:
        raise ValueError("IPS modifies the original function prologue")
    if patched[0x119B4 : 0x119F0] != text[0x119B4 : 0x119F0]:
        raise ValueError("IPS unexpectedly changes original post-hook instructions")


def apply_fix(source_ips: bytes, text: bytes, supplied_build_id: str) -> bytes:
    verify_native(text, supplied_build_id)
    if sha256(source_ips) != BASELINE_IPS_SHA256:
        raise ValueError("baseline IPS SHA-256 mismatch; refusing to rewrite unknown patch")
    records = parse_ips32(source_ips)
    if len(records) != 28:
        raise ValueError(f"unexpected baseline IPS record count: {len(records)}")
    old_hook = single_record(records, OLD_HOOK_FILE_OFFSET)
    if old_hook.data != bytes.fromhex("e9180414"):
        raise ValueError("baseline hook bytes differ from reviewed source")
    cave = single_record(records, CAVE_FILE_OFFSET)
    if len(cave.data) != 0x2BC or cave.data[12:16] != bytes.fromhex("15e7fb17"):
        raise ValueError("baseline cave differs from reviewed source")
    if any(record.offset == HOOK_FILE_OFFSET for record in records):
        raise ValueError("unexpected pre-existing record at corrected hook offset")
    if any(record.offset == REMOVE_BOND_GUARD_FILE_OFFSET for record in records):
        raise ValueError("unexpected pre-existing record at RemoveBond guard offset")

    old_hook.offset = HOOK_FILE_OFFSET
    old_hook.data = bytes.fromhex("e5180414")
    cave.data = cave.data[:12] + bytes.fromhex("19e7fb17") + cave.data[16:]
    records.insert(0, Record(REMOVE_BOND_GUARD_FILE_OFFSET, bytes.fromhex("1f2003d5")))
    corrected = encode_ips32(records)
    verify_patched_ips(corrected, text, supplied_build_id)
    return corrected


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("apply", "verify"))
    parser.add_argument("--ips", required=True, type=Path, help="input IPS32 (baseline for apply; corrected patch for verify)")
    parser.add_argument("--text", required=True, type=Path, help="extracted text.bin for the targeted native module")
    parser.add_argument("--build-id", required=True, help="40-character native NSO BuildID")
    parser.add_argument("--out", type=Path, help="output path required for apply")
    args = parser.parse_args()

    try:
        ips = args.ips.read_bytes()
        text = args.text.read_bytes()
        if args.mode == "apply":
            if args.out is None:
                raise ValueError("--out is required in apply mode")
            corrected = apply_fix(ips, text, args.build_id)
            args.out.parent.mkdir(parents=True, exist_ok=True)
            args.out.write_bytes(corrected)
            print(f"wrote {args.out} sha256={sha256(corrected)}")
        else:
            verify_patched_ips(ips, text, args.build_id)
            print(f"verified corrected IPS and native text sha256={sha256(ips)}")
        return 0
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
