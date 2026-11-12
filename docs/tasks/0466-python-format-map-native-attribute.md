---
status: done
priority: low
area: python
depends: [0443]
touches: [python/src/adapters.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/859
---
# The Python module answers format_map from NativeAttribute, which the engine now has

**Problem.** Before 0443 the engine's strings had no `format_map`, so the Python module's
`PySandboxPolicy::NativeAttribute` (python/src/adapters.cpp) returns its own callable for
`format_map` of a string: one that holds a weak reference to the environment and calls
`wrap_str_format` through `FormatWith(..., true, ...)`. The engine now has `str.format_map`
and asks `ISandboxPolicy::Format(self, true, params)` itself, as `str.format` asks it with
`false`, so the special case is a second path to the same call: one more `UserCallable` and
weak reference per `s.format_map` lookup, and a callable that can outlive the render (hence
its "used after the call it was passed to ended" error) where the engine's bound method needs
none of that.

**Proposal.** Drop the `format_map` branch from `PySandboxPolicy::NativeAttribute` and let
the engine's method reach `Format`. Check that `python/tests/test_sandbox.py`'s `format_map`
rows (lines with `format_map`, including the Markup and `{a:200000000}` width cases) still
pass, and that a refused field (`{x.__class__}`) still formats as the sandbox's undefined.
Update the `format_map` sentence of docs/python.md's sandbox section.

**Done when** the branch is gone, the sandbox suite passes and docs/python.md no longer names
`NativeAttribute` for `format_map`.

**PR.** [#859](https://github.com/jinja2cpp/Jinja2Cpp/pull/859)
