---
status: open
priority: medium
area: release
depends: [0071]
touches: [include/jinja2cpp/template.h, src/template.cpp]
shares: [src/template_impl.h, test/]
---
# 2.0 API: `BasicTemplate<CharT>`

**Problem.** `Template` and `TemplateW` are two hand-copied classes, with `Result`/`ResultW`
beside them; `Load` has two overloads for one job; rendering is non-`const`
(`docs/api-2.0.md`, 3.2).

**Proposal.** `template<class CharT> class BasicTemplate`, explicitly instantiated in the
library, with `Template`/`TemplateW` as aliases; `Result<T, CharT = char>`;
`Load(std::basic_string_view<CharT>, std::string name = {})`; `const` rendering once a TSan
run shows concurrent renders of one template are safe (otherwise document it);
`RenderAsString(const GenericMap&)` and `Render(os, const GenericMap&)`.

**Done when.** Existing tests build unchanged against the aliases, and a TSan test renders
one template from several threads (or the docs say why not).
