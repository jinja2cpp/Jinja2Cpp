---
status: open
priority: low
area: parity
depends: [0309]
touches: [src/testers.cpp#IsEscapedMode, src/filters.cpp#Join, src/serialize_filters.cpp#XmlAttrFilter]
---
# `m is escaped` of an imported module

**Problem.** Found doing 0309. `{% import 'lib' as m %}` binds Jinja2's `TemplateModule`, whose
`__html__` gives its output as Markup. Since 0309 the module object prints that way under
autoescape (markup.h, `IMapAccessor::GetStrValue`), but the `escaped` test (testers.cpp,
`IsEscapedMode`) asks only Markup strings and host objects, so `{{ m is escaped }}` is False where
Jinja2 says True. Pinned by `loader.module_object_is_escaped`. testers.cpp belonged to parity
batch 2 group A when this was found, so it was not fixed in 0309's PR.

Two more sites take str() of the module where Jinja2 takes `__html__` under autoescape (found by
the 0309 verifier, no corpus case yet): `{{ [m]|join }}` escapes the output twice
(`&amp;lt;i&amp;gt;`, Jinja2 `&lt;i&gt;`), and `{{ dict(a=m)|xmlattr }}` escapes it once
(`a="A&lt;b&gt;B"`, Jinja2 `a="A<b>B"`).

**Proposal.** In `IsEscapedMode`, and in join's and xmlattr's Markup checks, treat a map whose
`GetStrValue()` is set as having `__html__`, as `host::IsMarkupLike` does for host objects; add a
corpus case for each.

**Done when** the line is gone from `test/parity/divergences/loader.txt`.
