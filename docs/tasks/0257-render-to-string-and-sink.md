---
status: in-progress
priority: high
area: api
depends: [0252, 0076, 0253]
touches: [include/jinja2cpp/template.h, include/jinja2cpp/fwd.h, src/template.cpp, src/template_impl.h, src/out_stream.h, src/out_stream.cpp, test/output_test.cpp, test/template_api_test.cpp, test/fwd_test.cpp, bench/jinja2cpp_bench.cpp, bench/README.md]
---
# `RenderTo` a caller's string or an output sink

**Release impact (0221).** API wave: adds exported signatures, so it lands before the G1 freeze,
after 0076 PR-1 (fwd.h) and 0253 (host exception classification).

**Decided (0252, docs/api-2.0.md 3.9).** Exact API in
[design/0252-memory-contract-plan.md](../design/0252-memory-contract-plan.md) section 4:
- `RenderTo(StringType& out, const ValuesMap&)` and the `GenericMap` template form: append; on
  any exit with an error (or `bad_alloc`) `out` is truncated back to its size at entry;
- `RenderTo(BasicOutputSink<CharT>&, …)`: streams; written chunks stay. `BasicOutputSink` has
  one pure virtual `Write(std::basic_string_view<CharT>)`, protected non-virtual destructor,
  inline only (no export), frozen for 2.x. `OutputSink`/`OutputSinkW` aliases, also in fwd.h;
- header-only `ContainerSink<Container>` over any container with `append(first, last)`;
- sink exceptions are host exceptions (0250: code 15 with the pointer; a `jinja2::Exception`
  keeps its code, so `ResourceLimitExceeded` from a sink returns as that code), classified at
  the render boundary too, since the final flush runs after the root body.

`OutStream` gets a sink target (`FlushBuffer`/`WriteLong` call `Write`); the inline fast paths
stay unchanged.

**Tests.** `ExpectRender` in test/output_test.cpp also renders via `RenderTo` into a pre-filled
string and into a recording sink, so every existing row checks the three paths agree, narrow
and wide. New rows: output past the 512-character buffer, long fragments, an error mid-render
(string truncated, sink kept chunks), a throwing sink, a sink throwing
`ResourceLimitExceeded`, a callable rendering another template to a sink, `ContainerSink` over
`std::string`, `std::vector<char>` and `fmt::memory_buffer` (tests only). `static_assert`s on
signatures; `RenderTo(std::string{}, …)` must not compile.

**Perf.** New bench families `RenderTo/*`, `RenderSink/*` (plain_text, substitute,
large_static, mitsuhiko_table, mitsuhiko_table_wide). Existing `Render/*` within ±0.3%.
Expected: one allocation fewer per render, plain_text -8..-15% instructions, large cases ±0.5%,
`ContainerSink<fmt::memory_buffer>` within ±0.5% of `RenderTo`; sink peak memory far below
`Render`'s on mitsuhiko_table.
