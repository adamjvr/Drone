# Structure Recovery

## Policy

The project records original memory layouts as evidence but does not require the clean engine to copy them. Original structures are reconstructed from access patterns and only receive semantic field names when multiple uses establish them.

Machine-readable records:

- `reverse/structures/structure_ledger.csv`
- `reverse/structures/fields.csv`

## Current candidates

### `WIN_UNKNOWN_STRIDE_14`

**Status:** candidate / low confidence semantics.  
**Observed fact:** code in a cleanup/destruction context iterates objects with a stride of `0x14` bytes.  
**Not established:** whether this is an enemy, projectile, audio slot, generic object, or another record family.

Required next evidence:

- allocation/base pointer;
- element count or sentinel;
- initializer;
- non-cleanup consumers;
- per-offset access widths;
- any parallel DOS loop.

No production `Entity` structure should be modeled after this stride until those relationships are established.

## Field naming

Use names in this order of confidence:

```text
unknown_00
field0
flags_raw
position_x   // only after position semantics are proven
```

Width and signedness are independent claims. A field can be documented as `uint8/unknown semantics` before its semantic name is known.

## Arrays and pools

When an array-like region is found, record separately:

- base global/address;
- element stride;
- maximum/active count;
- initialization pattern;
- lifetime;
- free/in-use flag or sentinel behavior;
- update iteration order.

Iteration order can be behaviorally significant for collision and projectile logic, so the clean engine should not reorder objects casually before parity is established.
