---
status: done
priority: medium
area: perf
depends: [0205]
touches: [src/expression_evaluator.cpp, src/filters.cpp, src/global_functions.cpp, src/template_parser.h, src/testers.cpp]
---
# Fixed-cost drift from feature merges, 2026-10-12 to 2026-10-19

**Problem.** The weekly scout of 2026-10-19 compared master 9cf8586 with 8f13350d (0158),
41 merges earlier. No perf work landed in that span except 0205. Each merge stayed under
the +3% preflight gate and under the 3% drift alert (0123), but together they added:

- **Load**: +2.6% to +5.3% on every case except chat_mistral and mitsuhiko_table_wide.
  Examples: expressions +5.3%, filters +4.1%, many_tags +3.9%, plain_text +3.9%.
- **Render**:
  - strings +6.8%
  - plain_text +5.5% (+56 Ir)
  - chat_mistral +5.2%, with allocations 360 to 448
  - substitute +3.7%, for_range +3.5%, dict_ops +2.2%, macros +2.0%, expressions +1.9%

Attribution from the `bench-data` trend:

| Merge | Load (median step) | Render |
|---|---:|---|
| f4e312c, train 8 (#501-#507; 0043 ordered ValuesMap) | +0.82% | chat_mistral +3.6% and +90 allocations, strings +2.0% |
| 941c91c, train 26 (public BigInt, C++-only filters) | +1.21% | chat_* +0.7..1.45% |
| 9cf8586, train 27 (FindUndeclaredVariables, 0236) | +1.00% | none |
| #543, #545, #559, #588 (parity fixes) | none | plain_text +1.4/+0.2/+1.0/+1.4%, inheritance up to +1.7% |
| #558 | -0.93% | strings +1.6% |
| 0168/0193 after 0205 | about +0.9% | none (accepted in 0205) |

Report: `/mnt/project-files/perf-track/2026-10-19-scout.md`.

**Approach.** Work as 0205 did: per-merge `bench/count.py` and callgrind diffs, the
largest steps first.

1. **Train 27, Load +1.0%.** 0236 says Load pays nothing, but plain_text has no `set` or
   `trans`. Find the frames in `template.cpp`/`template_impl.h` (an inlining or layout
   shift) or in the `ParseSet` frame change.
2. **Train 26, Load +1.2% and chat Render +1.4%.** Check whether the BigInt alternative
   or visitor arms changed the evaluator's hot dispatch (`value_visitors.h`), or the
   conversion of user data.
3. **Train 8, chat_mistral +90 allocations.** If the ordered ValuesMap (0043) costs one
   allocation per converted message dict, that is a 2.0 design cost. Measure it and say
   so. If another PR in the train is the cause, fix it.
4. **strings +6.8% and plain_text/substitute Render.** Trace the parity merges' per-render
   fixed cost.

Each fix must be behaviour-neutral. A cost that is the price of a feature is documented
here as accepted, with its Ir.

**Done when.** Each listed step is either recovered or documented as accepted with its
cost, and master's Load and Render counts are back within +1% of 8f13350d or the
remainder is listed.

**Process note.** Fixed costs below 3% per PR pass every gate today. The steward's train
run could ask the perf track for a verdict when the train moves any case past +0.5%. That
is Ruslan's call; it is not part of this task.

## Result

Measured with `bench/count.py` on Release GCC 13 builds of 8f13350d, master dd26b6c and
this change, all running today's cases. Per-merge attribution came from callgrind diffs of
each merge against its first parent (#545, #558, #606, trains 26 and 27), as in 0205.

Recovered (behaviour-neutral):

- **`x in [...]` (Render/expressions -6.3% vs master).** `IsInEqual` kept its int fast path,
  but the number visitors it falls back to grew with BigInt (train 26), and every call paid
  their frame: 5 to 8 Ir per comparison more. The fallback is now a separate out-of-line function.
- **`map(...)` (Render/strings).** It copied the filter's argument error string for every
  item (#558). Now it reads it once per call and still raises at the first item.
- **`~` (Render/strings, macros).** The markup join of 0051 made `ConcatAsStrings` recursive,
  so it stopped inlining into `BinaryExpression::Apply`: 17 Ir per concatenation for its
  frame. The markup join is now a separate function and the plain concatenation is inlined again.
- **`range()` (Render/for_range -0.75%).** It looked each argument up twice by name. Now it
  looks each up once.
- **Keyword lookup at Load (Load/plain_text -0.35%).** `FindKeyword` was no longer inlined
  into the lexer callback after train 26. It is forced inline again.

| Case | 8f13350d | master | this change | change vs master |
|---|---:|---:|---:|---:|
| Render/expressions | 445,855 | +1.89% | -4.55% | -6.33% |
| Render/strings | 584,377 | +8.42% | +5.58% | -2.62% |
| Render/many_tags | 723,649 | +1.26% | -0.06% | -1.31% |
| Render/for_range | 38,030 | +3.17% | +2.39% | -0.75% |
| Render/macros | 637,225 | +1.80% | +1.29% | -0.49% |
| Render/html_autoescape | 1,254,242 | -1.68% | -1.99% | -0.31% |
| Render/mitsuhiko_table | 5,104,201 | +1.83% | +1.83% | 0 |
| Render/plain_text | 996 | +4.06% | +4.06% | 0 |
| Render/substitute | 2,208 | +3.01% | +3.01% | 0 |
| Load/expressions | 44,681 | +2.90% | +2.87% | -0.02% |
| Load/plain_text | 3,672 | +2.29% | +1.93% | -0.35% |
| Load/filters | 71,656 | +1.89% | +1.88% | -0.02% |
| Load/strings | 72,112 | +1.65% | +1.64% | -0.01% |
| Load/macros | 55,334 | +1.35% | +1.33% | -0.01% |

No case got worse than master. Cases that are within 1% of 8f13350d on both sides are left
out of the table.

**Step 3 (chat_mistral allocations).** This is not a regression. The case changed in the
window. With today's case, 8f13350d makes 450 allocations per render and master makes 428.
The +3.6% Render step at train 8 is the ordered ValuesMap (0043), a 2.0 decision. Like for like,
chat_mistral Render is now -0.26% against master and +0.05% against 8f13350d.

Accepted remainder, each the price of a feature:

- **Per-render fixed cost, +40 Ir** (plain_text, substitute):
  - `self` name passed to every template render: +14 (0143, 0198)
  - `RenderContext` construction and destruction: +11 (EvalContext id 0179, entry-binding state 0201)
  - the per-render `CycleCollector`: +6 (0110)
  - autoescape selector and frame name in `RenderBody`: +5 (0181)

  substitute pays another +26 Ir in the parameter lookup; this was not traced further.
- **For-loop items, about +8 Ir per item** (mitsuhiko_table and _wide, +1.7..1.8%):
  - `loop.cycle` position cell written per item: +2 Ir (415f1d41, a lifetime fix)
  - `LoopState::Recycle` resets the cycle cell and the recursive site: +21 Ir per loop
  - `RenderLoopInSlots` code after the loop `else` scope (0202) and the output-autoescape
    split (0179): about +3 Ir per item
  - the item ring: +1 Ir
- **strings, +5.6% (about 32,600 Ir per render):**
  - `namespace()` is a `TemplateDict` with non-string keys (0036): about 80 Ir more per
    `set ns.s`, about 12,000 per render
  - `format()`: about 9,000, 6,000 of it from #606
  - Python `%d` integer formatting (0019): about 3,000
  - value destruction no longer inlined: about 6,000
- **macros, +1.29%.** Reading `f.name` from host dicts goes through the ordered ValuesMap
  (0043): about 8,000 Ir per render.
- **for_range, +2.39% (about 900 Ir).** `range()` checks each argument's type (BigInt, train 26).
- **Load, +1.3..2.9%** (expressions, filters, macros, strings, plain_text):
  - the 0168/0193 remainder accepted in 0205: about +0.9%
  - every built-in test parses its arguments at Load (0200): about 260 Ir per `is` test
  - the lexer's token `emplace_back` and the `NodeArena` destructor are no longer inlined
    after trains 26 and 27: about 10 Ir per token

Tried and dropped:
- Moving the value of `set ns.attr` into the namespace (an rvalue `SetValue`) saved 0.17%
  on Render/strings, too little for the interface change.
- Padding `UndefinedValue` to a whole word (`std::uint16_t spare`) saved 0.2..1.2% Render
  but cost up to +1.1% Load (expressions).
