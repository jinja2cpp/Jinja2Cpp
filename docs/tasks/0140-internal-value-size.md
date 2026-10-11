---
status: done
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
- P0b: the seam in wave 2's files. expression_evaluator.*, statements.cpp, python_format
  and serialize_filters read maps through `AsMap` (0154's `x.name` fast path included),
  strings through `AsStringView`, and tell scalars apart by `Kind()`; `GetStringView` and
  `NarrowStringView` are gone. `GetData()` is gone too: `detail::InternalValueAccess` is the
  one way into the representation, used only in internal_value.* and value_visitors.h, so
  P2's call sites are exactly those. `TargetString`/`TargetStringView` are no longer
  alternatives: a value holds `std::string`, `std::wstring`, `std::string_view` or
  `std::wstring_view` itself, so `Apply` checks one wrapper (a host value) instead of three.
  CI: Render 0..-3.5% (many_tags -3.5%, for_loop_vars -2.9%), Load flat. The size stays 72 B, not
  the 64 B the plan expected: `ListAdapter` is 40 B too (accessor, two flags and
  `m_fieldNames` from #401), and P2 moves it behind the object pointer anyway, so no
  interim packing was done.
- P2: the 32-byte layout. A value is a trivially copyable `std::variant` (24 B: undefined,
  None, bool, int, double, strings of up to 15 narrow or 3 wide (7 on Windows) characters kept
  in place, owned strings, string views, host value references and typed pointers to lists,
  maps, pairs, callables and renderers) plus one `boost::intrusive_ptr<const ValueObject>` that
  owns what the pointer names. Copying is a memcpy and one refcount increment; nothing
  dispatches on the kind. Where the plan differs:
  - The variant holds a typed pointer to the object (the list accessor, the string) rather than
    a kind tag, and the owner may be a bigger object than the one pointed to. That replaces the
    aliasing `shared_ptr`s: a `loop` object points to the accessor inside its `LoopFrame`, which
    is the refcounted object, and a lent nested item of a host list points into its 0154
    `ItemAdapters` block, now a ValueObject (the obligation recorded in 0154 is done).
  - `m_parentData` is an `Anchor` object that owns the value's own object and its parent; the
    value keeps pointing where it did, so readers never see it.
  - Visitors receive a short string as a `std::basic_string` built in its SSO buffer, not as a
    view into the value: views stay what they were, borrowed text a visitor may keep (the first
    run of the suite caught `Value` conversions keeping views into dead short strings).
  - Markup is a flag of the string kinds only; `SetMarkup` on any other value does nothing (no
    caller marks one). A Slot's unbound bit is a flag of the undefined alternative.
  - namedtuple field names moved from the adapter into a wrapping accessor
    (`IListAccessor::GetFieldNames`).
  Local counts (GCC 13, Release, vs 6471f17): Render -8..-27% on every case (mitsuhiko_table
  -21.0%, dict_ops -11.7%, strings -10.2%, macros -20.1%, for_loop_vars -26.6%), Load -1..-6%
  except mitsuhiko_table_wide +0.9% and plain_text +0.3%. P2's gates: Render/large_static
  21,424 (<= 21,472), chat_qwen -8.5%, inheritance -8.3%, chat_llama -7.9%, Load/expressions
  44,804 (<= 46,953), Load/plain_text 3,667 (<= 4,014). Allocations rise where long strings are
  made (contradiction 2): a string over 15 characters takes its object besides its buffer, so
  strings +32%, chat_qwen +24%, chat_llama +20%, html_autoescape +17%, config_file +14% per
  render, while bytes fall on most cases. That misses the plan's +5% allocation bar and is put
  to the perf track as a trade; 0151 (building strings straight into their object) is the fix.
- Follow-ups filed: 0149 (no refcount for template constants), 0150 (24 B only on P2's
  numbers), 0151 (strings built straight into their heap object).

- Closed 2026-10-11: P2 merged in #462 (ce547f1). The perf track accepted the one criterion it
  missed, allocations within +5%, as a trade (CI job 114511456425: Render -5.9..-26.6%); 0151
  owns bringing render allocations back. P3 lives on as its own tasks, 0149 and 0150.

**Next.** 0151, as P2's long strings cost one more allocation each; 0141 (switch dispatch).
