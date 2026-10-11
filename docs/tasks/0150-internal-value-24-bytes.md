---
status: dropped
priority: low
area: perf
depends: [0140]
touches: [src/internal_value.h, src/internal_value.cpp]
---
# Is a 24-byte `InternalValue` worth a hand-written union?

**Problem.** 0140 P2 builds a 32 B value from library types: a trivially copyable
`std::variant` (24 B) and a `boost::intrusive_ptr` (8 B). A 24 B value needs the immediate
part in 16 B. `std::variant` cannot do that, because a view `{pointer, 32-bit length}`
already fills 16 B with padding before the index. So it would need a hand-written union.
The prototype measured only 2-3 instructions per operation cheaper than the library layout
(docs/design/0140-internal-value-plan.md §2).

**Proposal.** Decide on P2's numbers. Do it only if cache-sim shows density still matters
(D1 misses on mitsuhiko_table and list-heavy cases), and only with the union encapsulated in
one class with `static_assert`s on layout.

**Done when.** Decided with P2's cache-sim numbers; default is dropped.

## Result: dropped (measured on master 320c725)

The question is how much of the miss count follows the size of a value. A 24 B value was not
built for that; the cheaper inverse was: master's 32 B value against the same sources with
8 unused bytes appended (40 B, `static_assert` moved, the bytes neither initialised nor
copied), both built as the PR gate builds them (`-ffile-prefix-map`, Release) and counted
with `bench/count.py --cache-sim`. Going 32 -> 40 B is the same 8 B step as 32 -> 24 B, so it
bounds what the union could recover.

| Case | D1 read misses | D1 write misses | Instructions |
|---|--:|--:|--:|
| Render/mitsuhiko_table | 9,337 (+0.6%) | 6,493 (+0.2%) | +0.91% |
| Render/mitsuhiko_table_wide | 9,347 (-0.3%) | 22,972 (+0.0%) | +0.84% |
| Render/config_file | 6,182 (+8.6%) | 1,610 (+10.8%) | +0.86% |
| Render/dict_ops | 619 (+4.6%) | 424 (+6.0%) | +0.20% |
| Render/many_tags | 6,131 (+1.9%) | 690 (-0.4%) | -0.01% |
| Render/macros | 780 (+1.4%) | 220 (+2.3%) | +1.39% |
| Render/html_autoescape | 4,106 (-2.4%) | 1,359 (+14.9%) | +0.38% |
| Render/chat_* | +0.8..+3.3% | +1.9..+5.1% | +0.03..+0.34% |

- Most misses are not values. On `mitsuhiko_table` they are user data read in place and the
  output buffer. On `config_file`, the most value-sensitive case, the top D1 read missers
  are the host `ValuesMap` hash table (20%) and the host list enumerator (9%), then memcmp
  and output (callgrind_annotate --sort=D1mr). The 8 B step there is about 650 misses per
  render: at L2 latency, 1-2% of the cycles of its 1.33 M instructions.
- The instruction moves (up to +1.4% on `macros`) are codegen and layout around a larger
  object, not copies: the extra bytes are never copied. They are about the size of the 2-3
  instructions per operation the prototype saved (docs/design/0140-internal-value-plan.md §2).
- So 24 B would buy at most 1-2% on a few Render cases. Its price: a hand-written union
  (P2's trivially copyable `std::variant` cannot fit a `{pointer, 32-bit length}` view and
  its index in 16 B), in-place strings of 14 narrow characters instead of 15,
  and the layout and inlining fragility P2 and 0151 paid for in red pushes.
  Not worth it.

**Reopen when** a workload keeps large materialised `InternalValueList`s hot (sort, groupby,
map over thousands of items, where a list outgrows D1 at 32 B per item) and `--cache-sim`
shows D1 misses there moving by tens of percent with value size; rerun the 40 B padding
experiment above first, it takes one line in `src/internal_value.h`.
