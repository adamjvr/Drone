#!/usr/bin/env python3
"""Extract load objects from Drone's DOS/4GW Linear Executable (LE).

This is an evidence tool, not a DOS loader/emulator. It reconstructs object bytes
using the LE object table and page map so disassemblers can operate on stable
linear-address-oriented binaries. Original game bytes remain local.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("exe", type=Path)
    ap.add_argument("out_dir", type=Path)
    args = ap.parse_args()

    data = args.exe.read_bytes()
    if data[:2] != b"MZ":
        raise SystemExit("not an MZ executable")
    le = u32(data, 0x3C)
    if data[le:le + 2] != b"LE":
        raise SystemExit("secondary header is not LE")

    object_table_rel = u32(data, le + 0x40)
    object_count = u32(data, le + 0x44)
    page_map_rel = u32(data, le + 0x48)
    page_size = u32(data, le + 0x28)
    last_page_bytes = u32(data, le + 0x2C)
    module_pages = u32(data, le + 0x14)
    data_pages_file_offset = u32(data, le + 0x80)
    entry_object = u32(data, le + 0x18)
    entry_offset = u32(data, le + 0x1C)
    stack_object = u32(data, le + 0x20)
    stack_offset = u32(data, le + 0x24)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    objects: list[dict[str, int | str]] = []

    for index in range(object_count):
        off = le + object_table_rel + index * 24
        virtual_size, relocation_base, flags, first_page, page_count, reserved = struct.unpack_from("<6I", data, off)
        out = bytearray()
        page_flags: list[int] = []
        page_numbers: list[int] = []
        for relative_page in range(page_count):
            map_index = first_page - 1 + relative_page
            map_off = le + page_map_rel + map_index * 4
            entry = data[map_off:map_off + 4]
            if len(entry) != 4:
                raise SystemExit(f"truncated page-map entry {map_index + 1}")
            # LE stores the physical page number as a 24-bit big-endian field
            # followed by an 8-bit page flag field.
            physical_page = int.from_bytes(entry[:3], "big")
            flags_byte = entry[3]
            if physical_page == 0:
                page = bytes(page_size)  # zero/invalid pages are materialized as zeroes for analysis
            else:
                file_off = data_pages_file_offset + (physical_page - 1) * page_size
                physical_size = last_page_bytes if physical_page == module_pages else page_size
                page = data[file_off:file_off + physical_size]
                if len(page) != physical_size:
                    raise SystemExit(f"truncated physical page {physical_page}")
                if physical_size < page_size:
                    page += bytes(page_size - physical_size)
            out += page
            page_numbers.append(physical_page)
            page_flags.append(flags_byte)

        out = out[:virtual_size]
        filename = f"object{index + 1}_base_{relocation_base:08x}.bin"
        (args.out_dir / filename).write_bytes(out)
        objects.append({
            "object": index + 1,
            "virtual_size": virtual_size,
            "relocation_base": relocation_base,
            "flags": flags,
            "first_page_map_index": first_page,
            "page_count": page_count,
            "reserved": reserved,
            "first_physical_page": page_numbers[0] if page_numbers else 0,
            "last_physical_page": page_numbers[-1] if page_numbers else 0,
            "nonzero_page_flags": sum(flag != 0 for flag in page_flags),
            "output": filename,
        })

    meta = {
        "format": "LE",
        "le_header_file_offset": le,
        "page_size": page_size,
        "last_page_bytes": last_page_bytes,
        "module_pages": module_pages,
        "data_pages_file_offset": data_pages_file_offset,
        "entry": {
            "object": entry_object,
            "offset": entry_offset,
            "linear_address": objects[entry_object - 1]["relocation_base"] + entry_offset,
        },
        "stack": {
            "object": stack_object,
            "offset": stack_offset,
            "linear_address": objects[stack_object - 1]["relocation_base"] + stack_offset,
        },
        "objects": objects,
    }
    (args.out_dir / "le_metadata.json").write_text(json.dumps(meta, indent=2) + "\n")
    print(json.dumps(meta, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
