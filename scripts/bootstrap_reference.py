#!/usr/bin/env python3
"""Build a local, gitignored evidence tree from user-supplied shareware archives."""
from __future__ import annotations
import argparse, hashlib, pathlib, shutil, subprocess, sys, zipfile

KNOWN = {
    'drone_sw.zip':'ceba0a398a6b3260b415227383bae35b7ffac442a723329581bb2007f023d192',
    'drone_sw(win).zip':'4f4c0d4c7d0333f1066b1d3f0e4ff622e4df5d911a7b7440eb2ebb9e4407837d',
}
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(); ap.add_argument('input_dir',type=pathlib.Path); ap.add_argument('work_dir',type=pathlib.Path)
    a=ap.parse_args(); a.work_dir.mkdir(parents=True,exist_ok=True)
    for name,expected in KNOWN.items():
        p=a.input_dir/name
        if not p.exists(): raise SystemExit(f'missing {p}')
        got=digest(p)
        if got != expected: raise SystemExit(f'hash mismatch for {name}: {got}')
    dos=a.work_dir/'dos'; win_outer=a.work_dir/'windows-installer'; win=a.work_dir/'windows'
    for p in (dos,win_outer,win): shutil.rmtree(p,ignore_errors=True); p.mkdir(parents=True)
    with zipfile.ZipFile(a.input_dir/'drone_sw.zip') as z: z.extractall(dos)
    with zipfile.ZipFile(a.input_dir/'drone_sw(win).zip') as z: z.extractall(win_outer)
    installer=next(win_outer.glob('*.exe'))
    script=pathlib.Path(__file__).with_name('wise_extract.py')
    subprocess.run([sys.executable,str(script),str(installer),str(win)],check=True)
    inv=pathlib.Path(__file__).with_name('inventory.py')
    subprocess.run([sys.executable,str(inv),str(dos),str(a.work_dir/'dos_inventory.csv')],check=True)
    subprocess.run([sys.executable,str(inv),str(win),str(a.work_dir/'windows_inventory.csv')],check=True)
    print('reference workspace ready:',a.work_dir)
if __name__=='__main__': main()
