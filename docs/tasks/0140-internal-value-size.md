---
status: in-progress
priority: medium
area: perf
touches: [src/internal_value.h, src/internal_value.cpp, src/value_visitors.h, src/slot_frame.h]
---
# `InternalValue` is 72 bytes

**Problem.** `sizeof(InternalValue)` is 72 (GCC, x86-64): more than one cache line once
neighbours count, a list of 10 values spans 12 lines, and every by-value `Evaluate` copies
it. Both Rust engines (MiniJinja, Tera 2) use small values. Measured on master 50dec81, the
bytes are the smaller half of it: copying, moving and destroying values is 9-33% of Render
instructions, because every one of them goes through `std::variant`'s jump tables, even for
an `int64_t` (50-110 instructions per operation).

**Proposal.** Plan reviewed and approved by Ruslan 2026-10-10:
[docs/design/0140-internal-value-plan.md](../design/0140-internal-value-plan.md). A
trivially copyable `std::variant` of the immediate kinds (24 B) plus one
`boost::intrusive_ptr<const ValueObject>` (8 B), 32 B in all, 2.5-3.3x cheaper per
operation. Phases, one PR each: P1 (copies and moves of pairs and callables stop
allocating, move becomes `noexcept`), P0a/P0b (one access seam, outside then inside wave 2's
files), P2 (the 32 B layout), P3 (0149, 0150). Public `Value` (40 B, API) is out of scope
(0043).

**Done when.** P2 is merged with `sizeof(InternalValue) == 32`, Render instructions no worse
anywhere and at least 5% lower on mitsuhiko_table, dict_ops and strings, and allocations no
worse than +5% per case.

**Progress.**
- P1: `RecursiveWrapper` holds a shared immutable value instead of `boost::recursive_wrapper`,
  which allocated on every copy and every move of a pair or callable and made
  `InternalValue`'s move throwing (so growing vectors copied their items).

**Next.** 0151 if P2's string creation costs more allocations than its copies save; 0141
(switch dispatch) after P2.
