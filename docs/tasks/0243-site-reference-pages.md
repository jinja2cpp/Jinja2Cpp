---
status: open
priority: medium
area: docs
depends: [0242]
touches: ["site:docs/reference-pages/"]
---
# Website: reference pages for 2.0 features

Paths prefixed `site:` are in jinja2cpp/jinja2cpp.github.io.

**Problem.** The site has no page for most of what 1.3-2.0 added: builtin filters and tests
with signatures and divergences, global functions and methods on values (0020),
`TemplateEnv`/`Settings` options (`UndefinedPolicy`, `keepTrailingNewline`,
`newlineSequence`, `autoescape`, `finalize`, loop controls, lstrip/trim), autoescape and
Markup, the i18n extension (`InstallGettextCallables`), custom filters and tests
(`AddFilter`/`AddTest`, unknown names as Load errors), JSON bindings, thread safety and the
template cache, performance against Python Jinja2 (bench/README.md), security and trust
(0224's `docs/security.md`), and differences from Jinja2 (`docs/parity.md`).

**Proposal.** Generate what can be generated (the filter/test list from
`src/filter_factories.cpp`/`src/tester_factories.cpp`, the differences page from
`docs/parity.md` copied from the tag in site CI), write the rest with compiled examples
(0242's mechanism), and link the migration guide and security page rather than copying them.

**Done when.** Each topic above has a 2.x page with compiled examples, and generated pages
are rebuilt from the tag in CI.
