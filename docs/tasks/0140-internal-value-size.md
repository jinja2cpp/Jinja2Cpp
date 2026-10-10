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
- P1 (#445): `RecursiveWrapper` holds a shared immutable value instead of
  `boost::recursive_wrapper`, which allocated on every copy and every move of a pair or
  callable and made `InternalValue`'s move throwing (so growing vectors copied their items).
  CI: dict_ops -12.1%, config_file -5.9% Render instructions.
- P0a: the access seam outside wave 2's files (internal_value.h). `Kind()` says what a
  value is; `AsList`/`AsMap` give non-owning `ListRef`/`MapRef` views, which visitors now
  receive instead of the adapters; `AsStringView<CharT>` and `TakeString` read and take
  strings; `IMapAccessor::SetValue` is const. Left for P0b: `GetIf<ListAdapter>`/
  `GetIf<MapAdapter>`, `GetStringView` (markup.h) and `NarrowStringView` callers in
  expression_evaluator.*/statements.*, then `GetData()` itself. Counts (#453): Render
  within ±0.5% except large_static +0.55% (about 2 instructions per string write in
  `OutStream::WriteValueTo`), accepted as an interim cost of the seam; mitsuhiko_table
  -1.6%. P2 owes it back, together with #445's Load/plain_text +0.75%.
- Follow-ups filed: 0149 (no refcount for template constants), 0150 (24 B only on P2's
  numbers), 0151 (strings built straight into their heap object).

**Next.** 0151 if P2's string creation costs more allocations than its copies save; 0141
(switch dispatch) after P2.
