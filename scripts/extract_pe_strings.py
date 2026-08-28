#!/usr/bin/env python3
"""Small dependency-free printable-string extractor with file offsets."""
from __future__ import annotations
import argparse, pathlib

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('file',type=pathlib.Path); ap.add_argument('-n','--min',type=int,default=4)
    a=ap.parse_args(); b=a.file.read_bytes(); start=None
    for i,x in enumerate(b+b'\0'):
        ok=32 <= x < 127
        if ok and start is None: start=i
        elif not ok and start is not None:
            if i-start>=a.min: print(f'{start:08x}  {b[start:i].decode("ascii")}')
            start=None
if __name__=='__main__': main()
