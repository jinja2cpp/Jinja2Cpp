---
status: done
priority: medium
area: release
depends: []
touches: [include/jinja2cpp/, CMakeLists.txt]
shares: [src/, test/, README.md]
---
# Public API review and migration path for 2.0.0

**Problem.** The public API grew over years and is inconsistent: `Value` uses
`camelBack` methods (`isString`, `asString`, `asWString`, `getPtr`, `isEmpty`) while the
rest of the API is `CamelCase` (`Template::Load`, `TemplateEnv::AddGlobal`); `Settings`
mixes styles in public members (`m_defaultMetadataType` carries a private-member prefix,
`Do`, `I18n`, `LoopControls` are CamelCase fields); `Jinja2CompatMode::Vesrsion_2_10` is
misspelt. Found by `readability-identifier-naming` in the clang-tidy survey (0054) on
two TUs; a full inventory is part of this task. The 2.0.0 release already breaks ABI for
the ordered `ValuesMap` (0043, SOVERSION 2), so it is the one window to fix the API too.
But every rename breaks every user, so a cleanup without a migration path costs more
adoption than it buys consistency.

**Proposal.**
1. Inventory (explorer + architect roles): every public name, signature and type in
   `include/jinja2cpp/`, grouped by kind of inconsistency (naming, narrow/wide pairs,
   error reporting via `expected` vs exceptions, ownership of returned values, missing
   `const`/`noexcept`), each with the proposed 2.0 form.
2. Migration, in three layers, cheapest first:
   - **1.4 deprecation bridge:** the last 1.x release adds every new name next to the old
     one and marks the old one `[[deprecated("use AsString()")]]` (available in C++14).
     Users move warning by warning, with the compiler pointing at each site.
   - **2.0 compat header:** `jinja2cpp/compat/v1.h`, opt-in, keeps the old names as inline
     forwarding wrappers for one major version, so upgrading can be separated from
     migrating. Removed in 3.0.
   - **Rewrite tool** for the mechanical part: renames unambiguous enough to apply
     automatically (`asString` on `jinja2::Value`), as a clang-tidy plugin check with
     fix-its or a `clang-query`-driven script, run with the same
     `--export-fixes`/`clang-apply-replacements` pipeline as 0054.
3. ABI hygiene for 2.0: `SOVERSION 2`, and an inline namespace (`jinja2::v2`) so 1.x and
   2.x objects cannot be linked together silently.

**Done when** the inventory and the chosen 2.0 forms are agreed, the 1.4 bridge is
released, and 2.0 builds the existing test suite through the compat header unchanged.

**Next.** Once the public names follow one convention, `readability-identifier-naming`
(left open in 0054) can be switched on for `include/` as well as `src/`. Until the API
design is agreed, 0054 batches leave `include/` names alone and keep their public-header
edits (`google-explicit-constructor`, `[[nodiscard]]`) to ones the new API keeps.

**Outcome (2026-10-02, PR #328).** Design agreed with Ruslan in `docs/api-2.0.md`. Two of the
layers above changed: there is no 1.4 bridge (master already breaks 1.3.2 source and
ABI, so it ships as 2.0.0 directly), and the old names are deprecated in place in 2.0
instead of an opt-in `compat/v1.h` (which would be an ODR hazard). The rewrite tool is
driven by the compilers' deprecation warnings, so no Clang plugin is needed.
Implementation is filed as 0070-0077; 0067-0069 are defects found by the inventory.
