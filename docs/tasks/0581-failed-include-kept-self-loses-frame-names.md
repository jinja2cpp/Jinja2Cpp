---
status: in-progress
priority: low
area: parity
depends: []
touches: [src/statements.cpp]
shares: [test/template_module_test.cpp]
---
# A `self` an include stored before it failed reads the include's names as undefined

**Problem.** Found by the verifier of 0542 (identical on master 784eab1). A module macro
`{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include "lib" %}{% endmacro %}`
whose `lib` is `{% set t="T1" %}{% block b %}B1{{ t }}{% endblock %}{% set l = [self] %}{% set ns.s = l[0] %}{{ 1//0 }}`
fails (as Jinja2 does), and a later call with `lib` = `{% block b %}B2{% endblock %}` renders `[B1]B2` where Jinja2
3.1.6 renders `[B1T1]B2`: the stored `self`'s block reads `t` as undefined. A frame that ends by an error does not
keep the names its escaped closures read (the failing-frame path of `CaptureClosures`), the same family as 0560 (a
block in a macro stored by a failed nested render). Also with `{{ nofn() }}` (UndefinedError) as the failure.

**Done when** both rows render `[B1T1]B2` after the failed call, in narrow and wide templates.

**As built.** `RenderIncludedTemplate` closes the include's closures over its names whenever it fails
(`EndFailedFrame(innerContext, innerContext, ...)`), not only where the including context ends before the render: a
module whose macro ran the include keeps what a later call reads. Failure path only.
`TemplateModuleChangingSourceTest.SelfStoredByAFailedInclude` covers both stores and both failures, and the wide
template.
