---
status: open
priority: medium
area: perf
touches: [src/internal_value.h, src/internal_value.cpp]
---
# `InternalValue` is 72 bytes

**Problem.** `sizeof(InternalValue)` is 72 (GCC, x86-64): more than one cache line once
neighbours count, a list of 10 values spans 12 lines, and every by-value `Evaluate` copies
it. Both Rust engines (MiniJinja, Tera 2) use small values.

**Proposal.** Architect plan first, after 0117 P2 and 0118 P4 (slots store
`InternalValue`, so they inherit the shrink). Start by counting copies and moves per render;
target 24-32 bytes by moving large alternatives behind shared or arena handles.
Public `Value` (40 B, API) is out of scope (0043).

**Done when.** A plan is reviewed. Expected -3..-8% on list- and dict-heavy renders
(low confidence); unlocks 0141.
