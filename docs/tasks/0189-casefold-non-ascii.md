---
status: done
priority: low
area: parity
depends: [0037, 0048]
touches: [src/value_visitors.h#CaseInsensitiveLess]
shares: [test/parity/divergences/]
---
# Case-insensitive comparison folds only ASCII

**Problem.** `sort`, `min`, `max`, `unique` and `dictsort` compare strings case-insensitively
by default, through `CaseInsensitiveLess` (`src/value_visitors.h`). It lower-cases ASCII
letters and orders the remaining UTF-8 bytes as unsigned, so non-ASCII letters are not folded:
`['é','É']|min` differs from Jinja2, whose `ignore_case` uses `str.lower()` on every letter
(`sequences.casefold_non_ascii`, carried over from 0037 when that task closed). Since 0048 the
library has full Unicode case tables (`src/unicode_case.h`, `ToLower`), used by the `lower`,
`upper` and `title` filters but not by comparisons.

**Proposal.** Compare by decoded code points lower-cased with `unicode_case.h`'s `ToLower`
(multi-code-point results included), with an ASCII fast path so the common case stays as
cheap as today; keep `boost::iequals` callers in step. Measure `Render/filters` and
`dict_ops` with `bench/count.py`.

**Done when.** `sequences.casefold_non_ascii` matches, its line in
`test/parity/divergences/sequences.txt` is deleted, and the bench cases move by under 0.5%.

**Done** in #516. `detail::CompareIgnoreCase` compares an ASCII prefix inline and passes
the whole comparison to the 0048 tables at the first non-ASCII unit; dictsort keys use
`detail::LowerUtf8`. Pinned by `sequences.casefold_*`; Render/dict_ops -0.2%, Render/filters
-2.7%. Wide dict-literal keys stay with 0035 (`wide.sequences.casefold_dictsort`).
