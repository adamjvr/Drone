# FLY trajectory/script format

**Confidence: storage structure confirmed; semantics unresolved.**

The file is CRLF ASCII integers. The first integer is a record count `N`, followed by exactly `N` triples.

The original game stores each triple as:

```cpp
struct FlyRecordRecoveredStorage {
    int16_t field0;
    int16_t field1;
    int8_t  field2;
};
```

Example `CURRENT.FLY` declares 443 records. Semantic names such as x/y/heading are intentionally not assigned until consumers in the executable are reconstructed.
