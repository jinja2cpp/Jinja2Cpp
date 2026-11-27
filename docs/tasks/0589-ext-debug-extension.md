---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1016
priority: low
area: parity
touches: [docs/parity.md#options, docs/release-2.0.md]
---
# `jinja2.ext.debug`: implement `{% debug %}` or record it as deliberate

**Problem.** `docs/parity.md` lists `jinja2.ext.debug` as ❌ with no owner (found by 0235).
Jinja2's `{% debug %}` prints a `pprint` of the context, the filter names and the test names.
That output lists Python's builtin filter and test tables and the order of the context, so it
cannot be pinned by a corpus case and a C++ port could not match it byte for byte, but the tag
is part of the extension set Jinja2 documents.

**Proposal.** Decide with the maintainer, then do one of: (a) record a deliberate divergence
(`➖` in the options table of `docs/parity.md` and a line in `docs/release-2.0.md`, with the
reason above); (b) add `Settings::Extensions::debug` that prints the same three sections for
this engine's own context and filter and test names, with a corpus case that checks the shape
(`{'context': {...}, 'filters': [...], 'tests': [...]}` keys) rather than the text.

**Done when.** The `jinja2.ext.debug` row of `docs/parity.md` is ✅ or ➖ and
`scripts/check_parity_rows.py` passes without a task citation for it.

**Outcome.** Option (a): recorded as a deliberate divergence (`➖` row in `docs/parity.md`, line in
`docs/release-2.0.md`). Taken because the task is low priority and the output cannot be pinned; option (b) can still be
filed later if a host asks for it.
