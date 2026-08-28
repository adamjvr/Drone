#!/usr/bin/env python3
import subprocess
import sys

EXPECTED = (
    "updates=120 total=120 phase=0 scroll=569 player=147,171,0 "
    "missiles=7 cooldown=7 fired=15 shield=2765900,0 shield_sfx=12 "
    "special=1,3,230,-34 bomb_gate=-330 lives=3 score=0,0"
)

proc = subprocess.run([sys.argv[1], "120"], check=True, text=True, capture_output=True)
actual = proc.stdout.strip()
if actual != EXPECTED:
    raise SystemExit(f"session probe mismatch\nexpected: {EXPECTED}\nactual:   {actual}")
print("Drone session probe oracle OK")
