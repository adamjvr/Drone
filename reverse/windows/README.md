# Canonical Win32 Analysis

Addresses in this directory refer to the Phase 1 canonical Windows shareware executable unless a file states otherwise.

SHA-256:

```text
4fffc0406157b0def539b619fa53d1bb6c59537a44a6551df3aa3c060eaf3784
```

Normal image base: `0x00400000`.

Use `reverse/ghidra/DroneWin32Labels.py` only after verifying the binary hash. `function_map.csv` and `global_map.csv` are the durable address ledgers; prose files explain major control paths.
