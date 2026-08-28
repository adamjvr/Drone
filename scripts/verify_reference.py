#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, pathlib
EXPECTED={
'dos/DRONE_SW.EXE':'e7de54f9cd158289df58a7eee4ecbe6f1b9b04972e7eae88b26bfe32515a0fc0',
'windows/Drone_sw.exe':'4fffc0406157b0def539b619fa53d1bb6c59537a44a6551df3aa3c060eaf3784'}
def main():
 ap=argparse.ArgumentParser(); ap.add_argument('root',type=pathlib.Path); a=ap.parse_args(); good=True
 for rel,exp in EXPECTED.items():
  p=a.root/rel
  if not p.exists(): print('MISSING',rel); good=False; continue
  got=hashlib.sha256(p.read_bytes()).hexdigest(); print('PASS' if got==exp else 'FAIL',rel,got); good &= got==exp
 raise SystemExit(0 if good else 1)
if __name__=='__main__': main()
