---
status: open
priority: low
area: parity
depends: []
touches: [src/filters.cpp, test/parity/cases/, test/parity/divergences/]
---
# `sort` and `unique` with `attribute=` do not fail on a StrictUndefined key

**Problem.** `{{ items|sort(attribute='zzz')|list|length }}` and `{{ items|unique(attribute='zzz') }}`
over three dicts without `zzz` raise `UndefinedError` in Jinja2 with `StrictUndefined` (the sort
key and the unique key are compared, and a strict undefined refuses the comparison). Jinja2C++
returns the items. With the default `Undefined` both agree. Found while sweeping the sandbox's
attribute paths (0354): under the sandbox the same difference hides a `SecurityError` Jinja2
raises for `sort(attribute='_private')`.

**Proposal.** `CheckStrictUndefined` on the keys the sort and unique filters compare, as the
comparison operators already do (`UndefinedUse::Operator`); add the two rows to the parity corpus.

**Done when** both templates raise `UndefinedError` under the strict policy in the narrow and wide
paths, the parity cases pass, and the sandbox sweep in python/tests/test_sandbox.py drops its
`STRICT_ONLY` entries for `sort` and `unique`.
