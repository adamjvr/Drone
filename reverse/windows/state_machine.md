# Win32 game-state protocol — Phase 1 map

`game_dispatch_update` at `0x0040BA50` begins:

```asm
mov eax, [0x42B188]
cmp eax, 5
ja  return
jmp dword ptr [eax*4 + 0x411D8C]
```

Recovered jump table:

| raw state | target |
|---:|---:|
| 0 | `0x0040BA6A` |
| 1 | `0x0040BA98` |
| 2 | `0x0040BAB9` |
| 3 | `0x00411D86` (return/no-op) |
| 4 | `0x00411D5A` |
| 5 | `0x00411D4A` |

The same global is assigned at least `1,2,3,4,5,6,7,8,13,99` elsewhere. This means values >5 are not valid direct entries to this dispatcher but are transition/status values consumed by subordinate routines before the next dispatch. Do **not** model this as a closed six-value enum yet.

### Known branch behavior

- state 0 performs a screen clear/presentation-like path and returns;
- state 1 calls `0x417F50(1)` and `0x418AC0`, then checks for raw state 13;
- state 2 enters the large active gameplay/update/render path;
- state 3 returns immediately;
- state 4 calls `0x418AC0`; if it observes 13 it sets globals and transitions to 2;
- state 5 directly writes state 1 and returns.

Naming these title/menu/gameplay/debrief values will wait for caller/asset evidence.
