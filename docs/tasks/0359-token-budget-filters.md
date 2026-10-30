---
status: open
priority: medium
area: api
depends: [0358]
touches: [src/token_budget.cpp, src/token_budget.h]
shares: [include/jinja2cpp/template_env.h, src/filter_factories.cpp, src/filters.h, test/filters_test.cpp, docs/cpp-only-filters.md, docs/api-2.0.md, CMakeLists.txt]
---
# Token budget filters: `count_tokens`, `truncate_tokens` and `pack`

**Problem.** A prompt has a hard context limit, but the data that goes into it
(conversation history, retrieved chunks) does not. The template is where the prompt is
laid out, yet it cannot measure tokens, so fitting content to the budget happens in host
code before rendering: the host has to know the template's layout, and the template and
its budget logic drift apart. Tokenizers are model-specific and large, so the library
cannot bundle them.

**Proposal.** The host registers a tokenizer callback on the environment
(text → token count, and optionally text → the longest prefix within N tokens); without
one, a documented byte-based estimate is used. Filters on top of it:
- `count_tokens`: the token count of a string.
- `truncate_tokens(n, end="...")`: cut to at most `n` tokens on a token boundary.
- `pack(budget, key=none, order=none, reserve=0)`: from a list, keep the items whose
  rendered text fits in `budget` tokens, taking them greedily in `order` (an attribute,
  for example a retrieval score) and returning them in their original order. Items are
  iterated once, so generators work.
The callback signature is public API: architect plan and a recorded decision in
`docs/api-2.0.md` first; it lands in the API wave before the G1 freeze. 0358 comes first
because both add environment settings and should share one design pass.

**Done when.** Tests with a fake tokenizer callback (narrow and wide) cover counting,
truncation on token boundaries, `pack` ordering, an item larger than the budget, an
empty list, a generator input and the fallback estimate. The filters are specified in
`docs/cpp-only-filters.md`. A bench case shows the callback is not called more than once
per item in `pack`.

**Next.** Whether `pack` should also see the tokens already used by the rest of the
template (a running budget) needs render-time state; leave it out unless the plan finds
a cheap way.
