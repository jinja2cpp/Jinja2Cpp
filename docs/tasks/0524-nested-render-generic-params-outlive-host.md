---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/927
priority: high
area: robustness
depends: [0481]
touches: [src/template_impl.h, src/template_module.cpp]
shares: [test/borrowed_cells_test.cpp]
---
# A nested render's GenericList param stored in a shared global is read after the host freed it

**Problem.** Found by the verifier of batch 8 C (0491). A render nested in another one that shares its
globals converts its params as owned copies (`OwnedArgument`, 0481), but `OwnedArgument` deep-copies only
`ValuesList`, `ValuesMap` and string views. A `GenericList` (or `GenericMap`) param whose host container the
host callable destroys when it returns stays a view: `{% set _ = g.append(p) %}` in the nested render, then
`{{ g[-1] }}` in the outer one, is a heap-use-after-free under ASan (reproduced on master ca7086c with
`MakeGenericList(vec.begin(), vec.end())`, `vec` local to the callable). Since 0491 a closure, `self` or
`super()` of the nested render that reads `p` reaches it too (the render's params are in its closures' copy).

**Proposal.** Under a keeper, convert generic params by value as well (materialise the list or map once, as
`NestedGenericRenderAsksTheHostOnce` already avoids asking the accessor twice), or keep the host's accessor
alive with the value when it is shared (`GenericList` holds a shared accessor).

**Done when.** A `BorrowedCellsSingle` row with a destroyed `GenericList` and `GenericMap` param, stored
directly and through a closure, prints Python's output under ASan.

**Resolution.** Fixed in #927 (Codex review, P0): under a keeper `OwnedArgument(value, true)` takes a generic
list's or map's items once into a `ValuesList` or `ValuesMap`, nested lists and maps too, at most
`MaxSequenceSize` items a list (`ResourceLimitError` past it). `NestedRenderGenericParamsOutliveTheHost` stores
both directly and through a macro.
