---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/903
priority: medium
area: perf
depends: [0478, 0506]
touches: [src/internal_value.cpp, src/template_impl.h, src/global_functions.cpp]
---
# Per-render fixed costs: the memory budget's per-object charge and the render's setup

**Problem.** Two items of the engine push (perf track, 2026-11-15 engines baseline):

1. The render memory budget (0459, 0478, 0483) was estimated to cost about 2% on the chat rows. Most of
   that was 0483's per-object TLS read, which #882 already moved behind the object's own bit. What is
   left is `ValueObject::operator new` and `operator delete`, out of line and exported (GCC's
   `-Wmismatched-new-delete`, and code outside a shared library makes objects). The allocation function
   called `::operator new` and then read the budget, so it kept a frame across the call: 10
   instructions an object with no budget, where a jump would do.
2. `Render/substitute` is behind Tera (169 ns against 122 ns) and `Render/plain_text` is almost all
   fixed cost. A callgrind profile of substitute at #898's head: about 1,680 of its 2,212 instructions a
   render are outside the template body (setup and teardown in `RenderCore`, `RenderContext`, the
   output string, the globals snapshot, `Flush`), spread over some 25 pieces of 15-90 instructions.

**Change.**
- `ValueObject::operator new` tests the budget first. Without one it jumps to `::operator new` (4
  instructions); with one an out-of-line helper allocates and charges as before. That is the static
  library's count; a shared library still reads the TLS through `__tls_get_addr` first (backlog item 3,
  the TLS model).
- `RenderCore` keeps the `std::list` of a `GenericMap`'s values only for a `GenericMap` render. For a
  `ValuesMap` it was empty, but destroying it was still a call: about 27 instructions a render.
- `GetBuiltinGlobals` reads one function-local static (a reference to both maps) instead of two, with
  the building out of line: 16 instructions instead of 23.

**Result** (bench/count.py, GCC Release, pinned name-hash key, against #898's head 6613149):
- plain_text Render -3.44%, Sink -4.81%, To -4.24%; substitute -1.63..-1.88%.
- chat, hf and config_file rows -0.14..-0.25%; large_static -0.17..-0.28%; no row up, allocations and
  bytes unchanged.

The floor of the budget's cost was measured too: with every budget check removed (no charge, no
release, `RenderMemoryCounted()` false), the chat and hf rows are 0.38-0.70% under #898's head. This
change takes about 40% of that back. The rest is the release check in `operator delete` and the
`RenderMemoryCounted()` tests of strings, lists and dicts; per-render chunk accounting cannot remove a
per-object test, so it would win at most that remaining 0.2-0.45% and is not worth a redesign.

**Not done here (engine push, next).** What remains of the per-render setup is not one block to fold.
The largest pieces on substitute are the conversion of the params into the lent params map (key copy,
`HashName`, insert, value conversion, then `EmptyMaps` at the end: about 350 instructions for two
params), the output string's `reserve` and growth, the globals snapshot's `shared_ptr` copy, and
`RenderContext`'s constructor and destructor (about 60 each). Reading the params from the host's map
without converting them first is a value-model and lifetime change (0049, 0481): it needs a design
before it is tried.

**Done when.** The count job on the PR shows no row above +0.3% and no new allocations, and the rows
above move as measured.
