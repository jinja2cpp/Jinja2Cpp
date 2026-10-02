---
status: in-progress
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

**Outcome.** `include/jinja2cpp/template.h` declares `BasicTemplate<CharT>`; `src/template.cpp`
holds one generic implementation, instantiated for `char` and `wchar_t` (`extern template` in the
header for library users; MSVC rejects `extern` with `dllexport`, so the declaration is hidden
while the DLL itself is built).

- `Load(std::basic_string_view<CharT>, std::string name = {})` replaces the `const CharT*` and
  `const std::basic_string<CharT>&` overloads; a view need not be null-terminated.
- `Render`, `RenderAsString`, `GetMetadata`, `GetMetadataRaw` are `const`. The concurrent-render
  test (`TemplateApiTest.ConcurrentRenderOfOneTemplate`: extends, include, macros, a namespace,
  env globals, metadata) runs clean under `-DJINJA2CPP_WITH_SANITIZERS=thread`, a new sanitizer
  option with its own CI row. `GetMetadata` caches the parsed document behind a mutex.
- `Render(os, const GenericMap&)` and `RenderAsString(const GenericMap&)` take a reflected object
  or JSON object as the context. They are constrained templates that forward to out-of-line
  members, so `RenderAsString({})` still picks the `ValuesMap` overload.
- `TemplateW::GetMetadata`/`GetMetadataRaw` were stubs returning empty results; they now work
  (wide metadata is converted to narrow before the JSON parser), and
  `MetadataTest.Metadata_JsonData_Wide` is enabled.
- `operator==`/`operator!=` are inline templates (`operator!=` was declared but never defined).

Migration notes (for 0077's `MIGRATION.md`):
- A forward declaration `class Template;` / `class TemplateW;` no longer compiles: include
  `jinja2cpp/template.h`, or declare `template<typename CharT> class BasicTemplate;` and the aliases.
- Code that took the address of a `Load` overload, or passed something convertible only to
  `std::string` (not to `std::string_view`) to `Load`, needs a `std::string` first.
- ABI: every `Template`/`TemplateW` symbol changed (2.0 is ABI-breaking anyway).
