---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1010
priority: low
area: parity
depends: []
touches: [src/statements.cpp]
shares: [test/template_module_test.cpp]
---
# A block stored from inside an extending include renders nothing two calls later

**Problem.** Found by the verifier of 0542 (identical on master 784eab1). With `TemplateLookup::EveryUse`, no cache
and a source changing before every call, a `Template::MakeModule` macro
`{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include "lib" %}{% endmacro %}`
whose `lib` first is `{% extends "base" %}{% block b %}X{{ super() }}{% set ns.f = self.b %}{% endblock %}`
(`base` = `{% block b %}BASE{% endblock %}{% block c %}C0{% endblock %}`), then `{% block b %}B2{% endblock %}`,
then `{% block b %}B3{% endblock %}`, renders `[]XBASEC0`, `[XBASE]B2`, `[]B3`. Jinja2 3.1.6 renders
`[]XBASEC0`, `[XBASE]B2`, `[XBASE]B3`: the stored block (`ns.f`) still runs the first tree. The second call lets
something go that the stored block needs (probably the slots or the parent's tree pruned once no `self` holds a
handle, 0503/0529), and the third call reads it as empty. Memory-safe.

**Done when** a `TemplateModuleChangingSourceTest` row with the three calls above matches Jinja2, also with
`ns.f = self.c` and with `super` stored instead.

**As built.** Only `ns.f = self.b` failed: the stored block's body sets `ns.f = self.b` again when the second call
runs it, and `SelfBlockCall` made that `self` with `MakeTemplateSelf(owner->ended, ...)`, a new `SelfBlocks` holding
the handles of the calling render's table, so the first tree's slots went once the replaced callable did. The ended
block's `self` is now made of the blocks' own owner (`MakeSelfOf`), which holds the table they resolve in.
`TemplateModuleChangingSourceTest.StoredBlockOfAnExtendingInclude` covers `self.b`, `self.c` and `super`.
