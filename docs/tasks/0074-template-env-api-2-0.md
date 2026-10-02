---
status: in-progress
priority: medium
area: release
depends: [0071]
touches: [include/jinja2cpp/template_env.h, src/template_env.cpp, src/template_impl.h]
shares: [src/template_parser.h, test/]
---
# 2.0 API: `TemplateEnv` pimpl and `Settings`

**Problem.** `TemplateEnv` is laid out inline, so every new member breaks the ABI;
`Settings` has a private-prefixed field, `CamelCase` fields, a dead option and a misspelt
enum (`docs/api-2.0.md`, 3.3).

**Proposal.**
- Pimpl (`std::shared_ptr<detail::TemplateEnvImpl>`), methods exported out of line; a
  `Template` keeps the environment's state alive.
- `AddTest`/`RemoveTest`/`FindTest` (unreleased names, decided 2026-10-02);
  `FromString(source, name)`; `ApplyGlobals` passes `const ValuesMap&` (if 0069 has not).
- `Settings`: `defaultMetadataType`, `Extensions::{doStatement, loopControls, i18n}`
  (decided 2026-10-02); remove `useLineStatements`, `jinja2CompatMode` and
  `Jinja2CompatMode`; `operator==` out of line with a guard against forgotten fields.
- Document Jinja2's rule: configure the environment before loading templates.

**Done when.** Tests pass, `sizeof(TemplateEnv)` is one pointer pair, and adding a member to
the impl needs no public header change.

**Notes (implementation).** The state lives in `detail::TemplateEnvImpl` (`src/template_env_impl.h`). Each
template holds its own `TemplateEnv` handle to it (`detail::TemplateEnvAccess::MakeHandle`), so the parser and
renderer keep using `TemplateEnv*`. Cached templates hold such handles too, which would make the state keep itself
alive; the `TemplateEnv` the user created is recorded as the owner, and its destructor drops the caches and stops
caching, so templates that outlive it still render and load their includes, uncached. `TemplateEnv` stays
non-copyable and non-movable (it was before), which keeps the owner pointer stable. `operator==(Settings)`
names every field through structured bindings, so a new field fails to compile until it is compared.
