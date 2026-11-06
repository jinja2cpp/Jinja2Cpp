---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/788
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

**Decision (Ruslan 2026-11-05, docs/api-2.0.md 3.3 and section 7 item 9).** `Tokenizer` in
`jinja2cpp/prompt.h`: `count(std::string_view utf8)` required, `prefix(text, maxTokens)` optional,
UTF-8 only (wide templates convert), results clamped, exceptions become `UserCallableException`;
without one, `ceil(utf8_bytes / 4)`. `pack(budget, attribute=none, by=none, reverse=false,
reserve=0, stop=false)` replaces the `key`/`order` naming above; no per-item overhead argument.
Specification: docs/cpp-only-filters.md. Lands after 0358 PR 2 (same files), with a fake
tokenizer test table (narrow and wide) and a `prompt_budget` bench case asserting at most one
count call per item. Running budget: 0395.

**Done when.** Tests with a fake tokenizer callback (narrow and wide) cover counting,
truncation on token boundaries, `pack` ordering, an item larger than the budget, an
empty list, a generator input and the fallback estimate. The filters are specified in
`docs/cpp-only-filters.md`. A bench case shows the callback is not called more than once
per item in `pack`.

**Next.** Whether `pack` should also see the tokens already used by the rest of the
template (a running budget) needs render-time state; leave it out unless the plan finds
a cheap way.
