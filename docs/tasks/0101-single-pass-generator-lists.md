---
status: done
priority: low
area: parity
depends: [0088]
touches: [src/internal_value.cpp, src/filters.cpp]
---
# Lazy filter results are reusable, Python generators are single-pass

**Problem.** `select`, `reject`, `map` and friends return a Python generator, which is
exhausted after one pass:
`{% set a = [1, 2, 3]|select('odd') %}{{ a|list }}{{ a|list }}` renders `[1, 3][]` in
Python and `[1, 3][1, 3]` in Jinja2C++ (`filters.select_single_pass`, divergence under
this task). The generator `ListAdapter` accessor clones on every copy (0088 S6,
`IListAccessor::ClonesOnCopy`), so each use starts afresh.

**Decision needed.** Matching Python means a copy shares the enumerator instead of
cloning it (`ClonesOnCopy() == false` plus a consumed flag shared by copies), which is
a one-line policy change after #362 but makes templates that reuse a filtered list
silently print nothing the second time. Keeping today's behaviour means recording it as
a deliberate divergence in docs/parity.md, with the corpus line kept.

**Resolution.** Ruslan chose on 2026-10-03 to keep today's behaviour: a reused
filter result is iterated again from the start. Matching Python would make a template
that uses a filtered list twice silently print nothing the second time, which breaks
existing C++ templates without an error. It is listed as deliberate in docs/parity.md
(Filters), and its corpus line stays in
`test/parity/divergences/filters.txt`.

**Done when.** Ruslan has chosen; the corpus line is either gone (matching Python, with
`length`/`first`/`last` on a consumed generator checked against Python) or kept
and listed as deliberate, with the reason, in docs/parity.md.
