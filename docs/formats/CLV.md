# CLV audio format

**Confidence: confirmed structurally and by DOS→Windows asset comparison.**

`.CLV` is headerless PCM:

- sample rate: 22,050 Hz;
- channels: 2 interleaved;
- sample format: unsigned 8-bit PCM;
- byte order: `[L0,R0,L1,R1,...]`.

The Windows port generally replaced matching CLV sounds with mono WAV files. For the common sample region the conversion rule is:

```cpp
mono[n] = uint8_t((unsigned(left[n]) + unsigned(right[n])) / 2);
```

This is integer floor-average. Some Windows files omit zero or a very small number of final source frames, so equality checks should compare the common sample range before treating tail length differences as semantic.
