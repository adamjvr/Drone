#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, hashlib, pathlib

def sha256(path: pathlib.Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024), b''): h.update(b)
    return h.hexdigest()

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('root',type=pathlib.Path); ap.add_argument('output',type=pathlib.Path)
    a=ap.parse_args(); rows=[]
    for p in sorted(x for x in a.root.rglob('*') if x.is_file()):
        rows.append((p.relative_to(a.root).as_posix(),p.stat().st_size,sha256(p),p.suffix.lower()))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('w',newline='',encoding='utf-8') as f:
        w=csv.writer(f); w.writerow(['path','bytes','sha256','extension']); w.writerows(rows)
    print(f'wrote {len(rows)} entries to {a.output}')
if __name__=='__main__': main()
