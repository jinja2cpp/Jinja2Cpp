---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/842
priority: low
area: parity
depends: []
touches: [src/value_methods.cpp]
shares: [test/parity/divergences/methods.txt]
---
# str.format_map is missing from the engine's string methods

**Problem.** Python Jinja2 calls a string's `format_map` as Python does, and its sandbox runs it
through `SandboxedFormatter` (`wrap_str_format`). The engine has `str.format` but no
`format_map`:

    {{ '{a}!'.format_map({'a': 5}) }}

renders `5!` in Jinja2 and fails with `'str object' has no attribute 'format_map'` in
Jinja2C++ (corpus case `methods.str_format_map`). The public `ISandboxPolicy::Format` already
takes a `formatMap` flag (0357 B7), but the engine never calls it with `true`: only the Python
module's strings reach it, through the module's own `format_map`. Found in 0357 S4, when a test
of the policy's `format_map` path failed.

**Proposal.** Add `format_map(mapping)` next to `format` in `src/value_methods.cpp`, sharing
its field formatter with the mapping as the only source of named fields (no positional
fields: `'{}'.format_map({})` is Python's `ValueError: Format string contains positional
fields`). With a sandbox policy, ask `Format(self, true, params)` first, as `format` asks it
with `false`.

**Done when.** `methods.str_format_map` matches (its line is deleted from
`test/parity/divergences/methods.txt`), with a narrow and a wide row, a row for the positional
field error, and a `PublicSandboxPolicyTest` row for a policy that formats `format_map`.

**Resolution.** `format_map` (str and Markup) shares `str.format`'s field parser; the mapping
is the only source of named fields (looked up as a `[key]` part is, so a missing key is a
KeyError and a non-subscriptable mapping a TypeError) and any positional field raises
`Format string contains positional fields`. A sandbox policy is asked
`Format(self, true, params)` first. Corpus cases `methods.str_format_map*` and
`methods.markup_format_map`; `PublicSandboxPolicyTest` rows in
`TheEnginesFormatAsksAboutFieldAttributes` and `APolicyMayFormatItself`. The Python module's
own `format_map` callable is now redundant: 0465.
