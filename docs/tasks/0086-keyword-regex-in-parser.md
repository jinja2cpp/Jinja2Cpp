---
status: done
priority: high
area: perf
depends: [0011]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/356
touches: [src/template_parser.h#GetKeyword, src/template_parser.h#GetKeywords]
shares: [src/template_parser.h]
---
# Statement keywords are matched with a regex compiled on every `Load`

**Problem.** Found with the benchmark suite (0011). `TemplateParser` builds its keyword
matcher in the constructor (`m_keywords(traits_t::GetKeywords())`): one `boost::regex`
with an `(^name$)` alternative per keyword, compiled again for every `Template::Load`.
`GetKeyword` then runs `regex_search` over that alternation for every identifier
at the start of a statement. Callgrind on the Release build:

- `Load/plain_text` (24 bytes, no tags) costs about 20 µs; regex construction is 89%
  of it. Python Jinja2 takes 120 µs to compile the same template, so we are still
  ahead, but the fixed cost dominates every small template.
- `Load/many_tags` (300 statements, 38 KB) costs 14 ms; `GetKeyword` is 67% of it
  and `regex_search` 48%.

**Proposal.** Replace the regex with a plain lookup: a static table sorted by name and
`std::lower_bound`, or a switch on length then `memcmp`, over `s_keywordsInfo` for
both character types. Nothing else in the parser uses the regex, so `<boost/regex.hpp>`
may drop out of `template_parser.h` too (check `JINJA2CPP_USE_REGEX` users).

**Done when.** `bench/run.py --filter 'Load/'` shows `Load/plain_text` under 5 µs and
`Load/many_tags` at least 2x faster than the 0011 baseline, with the unit and parity
tests unchanged.

**Result.** `ParserTraitsBase::FindKeyword` binary-searches a table sorted once per
character type; `template_parser.h` no longer includes a regex header. Release build,
7 repetitions: `Load/plain_text` 40 µs → 1.85 µs, `Load/many_tags` 20.9 ms → 6.9 ms,
every other `Load/` case 37-93% faster, `Render/` unchanged within noise. The regex
dependency itself is now unused by `src/`: see 0090.
