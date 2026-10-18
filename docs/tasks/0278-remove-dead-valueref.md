---
status: open
priority: low
area: internals
depends: []
touches: [src/internal_value.h, src/internal_value.cpp, src/out_stream.cpp, src/value_visitors.h]
shares: []
---
# Remove the dead ValueRef alternative of InternalValue

**Problem.** `ValueRef` (a host `jinja2::Value` held by reference) is still an alternative of
`detail::ValueStorage`, with an implicit `InternalValue(ValueRef)` constructor, a case in
`OutputValueConvertor`, a branch in `OutStream` (out_stream.cpp) and an overload in
`visitors::BaseVisitor`, but nothing makes one: in 0180 the constructor was made to abort
and the whole test suite, parity corpus included, still passed. Host data reaches templates
through `LendItem` and the adapters instead (0115, 0125). 0180 dropped the in-place visit of
the Value's own variant from `Apply`/`Apply2`, which cost every visitor instantiations for
all Value alternatives (and grew units past GCC's inlining budget); a ValueRef now reaches a
visitor as itself.

**Proposal.** Remove the alternative, its constructor and the cases that handle it; update
`kinds` in `InternalValue::Kind()` and anything that indexes `ValueStorage`. Measure with the
perf gate (the variant index of later alternatives shifts).

**Done when.** `ValueRef` is gone from src/, tests and the perf gate pass.
