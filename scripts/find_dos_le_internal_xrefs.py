#!/usr/bin/env python3
"""Find simple internal 32-bit-offset LE fixups in Drone's DOS executable.

Drone's Watcom DOS/4GW build stores data/string references as LE internal
fixups. Raw object disassembly therefore shows object-relative immediates such
as 0x4860 rather than final linear addresses. This evidence helper locates the
canonical source-type-0x07 internal fixup form used by Drone for those 32-bit
offset references and reports the source patch location in linear address
space.

This is intentionally a narrow evidence scanner, not a complete OS/2 LE loader.
It only reports records matching the directly observed forms:
  source type 0x07 (32-bit offset fixup)
  target flags 0x00 (16-bit target offset) or 0x10 (32-bit target offset)
  8-bit internal target-object number
Because the caller supplies an exact target object/offset and each candidate is
validated against the LE page/object tables, this is sufficient for stable
cross-reference work on the canonical Drone executable without pretending to
support unrelated LE fixup variants.
"""
from __future__ import annotations

import argparse
import struct
from dataclasses import dataclass
from pathlib import Path


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


@dataclass(frozen=True)
class LoadObject:
    index: int
    virtual_size: int
    relocation_base: int
    first_page: int
    page_count: int


def parse_int(text: str) -> int:
    return int(text, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("exe", type=Path)
    ap.add_argument("target_object", type=int)
    ap.add_argument("target_offset", type=parse_int)
    args = ap.parse_args()

    data = args.exe.read_bytes()
    if data[:2] != b"MZ":
        raise SystemExit("not an MZ executable")
    le = u32(data, 0x3C)
    if data[le:le + 2] != b"LE":
        raise SystemExit("secondary header is not LE")

    page_size = u32(data, le + 0x28)
    object_table_rel = u32(data, le + 0x40)
    object_count = u32(data, le + 0x44)
    fixup_page_table_rel = u32(data, le + 0x68)
    fixup_record_table_rel = u32(data, le + 0x6C)
    module_pages = u32(data, le + 0x14)

    if not (1 <= args.target_object <= object_count):
        raise SystemExit(f"target object must be 1..{object_count}")

    objects: list[LoadObject] = []
    for i in range(object_count):
        off = le + object_table_rel + i * 24
        virtual_size, relocation_base, _flags, first_page, page_count, _reserved = struct.unpack_from("<6I", data, off)
        objects.append(LoadObject(i + 1, virtual_size, relocation_base, first_page, page_count))

    target = objects[args.target_object - 1]
    if not (0 <= args.target_offset < target.virtual_size):
        raise SystemExit(
            f"target offset 0x{args.target_offset:X} is outside object {target.index} "
            f"(size 0x{target.virtual_size:X})"
        )

    page_table = le + fixup_page_table_rel
    record_table = le + fixup_record_table_rel
    page_offsets = [u32(data, page_table + i * 4) for i in range(module_pages + 1)]

    def logical_page_owner(logical_page: int) -> tuple[LoadObject, int] | None:
        for obj in objects:
            first = obj.first_page
            last = first + obj.page_count - 1
            if first <= logical_page <= last:
                return obj, logical_page - first
        return None

    matches: list[tuple[int, int, int, int]] = []
    for logical_page in range(1, module_pages + 1):
        start = record_table + page_offsets[logical_page - 1]
        end = record_table + page_offsets[logical_page]
        blob = data[start:end]
        owner = logical_page_owner(logical_page)
        if owner is None:
            continue
        source_obj, page_in_obj = owner

        # Search for the two internal target-width variants. We scan rather than
        # parse every LE record variant so the tool remains explicit about its
        # narrow supported evidence form.
        for target_flags, width in ((0x00, 2), (0x10, 4)):
            # A target offset may be larger than the encoding width used by a
            # particular LE fixup form. Such a form simply cannot represent
            # this target; skip it rather than raising OverflowError.
            if args.target_offset >= (1 << (width * 8)):
                continue
            encoded_target = args.target_offset.to_bytes(width, "little", signed=False)
            for i in range(0, max(0, len(blob) - (5 + width) + 1)):
                if blob[i] != 0x07 or blob[i + 1] != target_flags:
                    continue
                source_offset = struct.unpack_from("<H", blob, i + 2)[0]
                if source_offset >= page_size:
                    continue
                if blob[i + 4] != args.target_object:
                    continue
                if blob[i + 5:i + 5 + width] != encoded_target:
                    continue
                source_patch = source_obj.relocation_base + page_in_obj * page_size + source_offset
                matches.append((source_patch, logical_page, source_obj.index, target_flags))

    for source_patch, logical_page, source_object, flags in matches:
        print(
            f"source_patch=0x{source_patch:08X} "
            f"source_object={source_object} logical_page={logical_page} "
            f"target=object{args.target_object}+0x{args.target_offset:X} flags=0x{flags:02X}"
        )

    return 0 if matches else 1


if __name__ == "__main__":
    raise SystemExit(main())
