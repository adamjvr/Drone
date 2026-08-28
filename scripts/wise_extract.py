#!/usr/bin/env python3
"""Extract the known 1999 Drone shareware Wise installer without executing it.

The installer payload was recovered as chained raw-DEFLATE streams. The checked-in
manifest contains stream offsets/lengths/CRCs and installed paths. This tool is
hash-agnostic at the decompression layer but validates every stream before writing.
"""
from __future__ import annotations
import argparse, binascii, csv, pathlib, zlib


def extract(installer: pathlib.Path, manifest: pathlib.Path, out: pathlib.Path) -> int:
    blob = installer.read_bytes()
    count = 0
    with manifest.open(newline='', encoding='utf-8') as f:
        for row in csv.DictReader(f):
            off = int(row['compressed_offset']); clen = int(row['compressed_bytes'])
            ulen = int(row['uncompressed_bytes']); expected = int(row['crc32'], 16)
            raw = blob[off:off+clen]
            if len(raw) != clen:
                raise RuntimeError(f"stream {row['index']} lies outside installer")
            data = zlib.decompress(raw, -15)
            if len(data) != ulen:
                raise RuntimeError(f"stream {row['index']} length mismatch")
            crc = binascii.crc32(data) & 0xffffffff
            if crc != expected:
                raise RuntimeError(f"stream {row['index']} CRC mismatch: {crc:08x} != {expected:08x}")
            path = row.get('installed_path', '').strip()
            if not path:
                continue
            dest = out / pathlib.PurePosixPath(path)
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
            count += 1
    return count


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('installer', type=pathlib.Path)
    ap.add_argument('output', type=pathlib.Path)
    ap.add_argument('--manifest', type=pathlib.Path,
                    default=pathlib.Path(__file__).resolve().parents[1] / 'manifests/windows_shareware_wise_streams.csv')
    args = ap.parse_args()
    n = extract(args.installer, args.manifest, args.output)
    print(f"extracted {n} installed files to {args.output}")
    return 0
if __name__ == '__main__': raise SystemExit(main())
