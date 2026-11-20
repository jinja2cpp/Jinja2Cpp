---
status: in-progress
pr: https://github.com/jinja2cpp/jinja2cpp/pull/960
priority: high
area: parity
depends: [0538]
touches: [src/template_dict.h, src/template_dict.cpp, src/internal_value.cpp#DictViewEnumerator]
shares: [test/parity/cases/statements.py, test/internal_value_test.cpp, test/limits_test.cpp]
---
# A dict walk misses a key erased and added again when the entry's address is reused

**Problem.** 0538 made a walk of a template's dict read it live. The walk keeps the token of the entry it stopped at
and that entry's address, and at the next step fails with `dictionary keys changed during iteration` when the token
names another entry: an entry erased and added again under the same key. The address cannot tell that: once the
entry is erased its node is freed, and the allocator may hand the same address to the entry added next. Then the
walk takes the new entry for the old one and goes on. The corpus case `statements.for_dict_live_readd_current`
(`e.pop('a')` then `e.update({'a': 5})` while the walk is at `a`) printed `a` on the MSVC C++20 Debug shared bigint
job of PR #954, where Python raises; it passed elsewhere, as the temporary dict `update` builds usually takes the
freed node first. `setdefault` adds the entry with no allocation in between, so it reproduces on glibc too.

**Fix.** Each dict counts an erase epoch: every `Erase` and `Clear` moves it on. An entry added while the
epoch is past 0 is noted with the epoch it was added at, in the dict's side table (the table that already holds the
first spellings of keys that are not strings), keyed by the entry's address and dropped when the entry is erased. A
walk keeps the epoch with the token; at the next step the entry it finds is the one it stopped at only if its
address matches and it was not added after that epoch. Without a side table the epoch lives in the bits of the
table's pointer, so a dict that is never erased from allocates nothing more, `Clear` stays noexcept and `TemplateDict`
keeps its size (the `TemplateDictLayout` assert). `TemplateDict` loses its assignment operators, which nothing
used: the entries a dict took over would carry no epoch, so a walk could take one of them, at a freed address, for
its own (found by the verifier's model test).

**Acceptance.**
- `TemplateDictTest.EraseEpochMarksEntriesAddedAgain` covers erase, both ways of adding, keys that are not strings,
  `Clear` and copies; `LimitsTest` row `MemoryDictReAdd` (20000 re-adds stay within a 256 KiB budget).
- Corpus cases `statements.for_dict_live_readd_current_setdefault`, `..._readd_last_setdefault` (fail as Python) and
  `..._readd_other_setdefault` (another key re-added: the walk goes on, as Python's).
- No measurable change on the dict iteration benches (`bench/count.py`).

**Limits.** The notes are one small node per entry added after the dict's first erase, bounded by the dict's size;
the render's memory budget counts them as it counts the entries (charged while all the dict's entries are, given back
with the entry or the dict). The inline epoch saturates at 2^62 (2^30 on 32-bit) erases of one dict, after which the
check is the address alone again. As before 0553, a walk fails at the
step after its own entry was erased, where Python's goes on over the entries after it and fails only when it would
give more than the dict had at the start; the render fails in both.
