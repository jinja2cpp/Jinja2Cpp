---
status: done
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/631
area: api
depends: [0180]
touches: [include/jinja2cpp/markup.h, include/jinja2cpp/user_callable.h, src/markup.cpp, CMakeLists.txt, test/basic_tests.cpp]
shares: [docs/api-2.0.md]
---
# A public escape helper for Markup

**Problem.** 0180 lets a host pass safe HTML as `jinja2::Markup`, but making one is only
the trusted constructor: `Markup(text)` marks text as already safe. Python offers the
other half in `markupsafe.escape(text)`, which HTML-escapes `text` and returns a Markup,
and `Markup.escape`. A host that wants to build a Markup from user text, for example
`Markup("<b>") + escape(name)`, has to escape by hand or call a template. The engine
has the function (`MarkupEscape` in `src/markup.h`, behind the `e` filter), but it works
on `InternalValue`, so nothing public exposes it.

**Proposal.** Add `jinja2::Escape(std::string_view)` and the wide form, returning
`Markup`/`WMarkup`, with `markupsafe.escape`'s rules (`& < > ' "`, an already safe Markup
passes through unchanged), implemented on `detail::EscapeHtmlToBuffer`. Decide whether
`Markup` gets the arithmetic Python has (`Markup + str` escapes the `str`) or stays a plain
`std::string` subclass whose operators return `std::string`; the helper makes the first
option possible without making it required.

**Done when.** `Escape("<a&'\">")` equals `Markup("&lt;a&amp;&#39;&#34;&gt;")`, a Markup
argument is returned as is, and a table in `test/` pins both against Python's
`markupsafe.escape`. Documented in docs/api-2.0.md section 3.1.

**Also (found by the 0180 verifier).** A typed `MakeCallable` parameter of type `Markup`
(`ArgInfoT<Markup>`) can never be filled: `ParamUnwrapper` hands a Markup argument over as
its base `std::string`, and there is no implicit string-to-Markup conversion. Either let a
`Markup` parameter take a Markup argument (and reject or escape a plain string, as
`markupsafe.escape` would), or document that callables needing the flag take
`UserCallableParams`.

**As built.** `jinja2::Escape(std::string_view)` and `Escape(std::wstring_view)` (exported,
in `src/markup.cpp` on `EscapeHtml`, so the short narrow case uses the thread buffer) return
`Markup`/`WMarkup`; inline `Escape(const Markup&)`/`Escape(const WMarkup&)` return the
argument unchanged. `Markup` gets no `operator+`: a `+` that escapes one side silently reads as
plain concatenation, so the host writes `Markup(a.str() + Escape(b).str())`. A `Markup` parameter of a
`MakeCallable` callable (by value or `const&`, on both the lambda path through `ArgPromoter`
and the function-pointer path through `ArgInfoT`) now takes a Markup argument as is and a
plain string of its character type escaped, as `markupsafe.escape` would make one; None gives
an empty Markup, as None gives an empty string to a `std::string` parameter, and any other
type leaves the call unmade (it returns None), as for other typed parameters. A `std::string`
parameter still receives a Markup's text. `test/basic_tests.cpp` `EscapeMatchesMarkupsafe` pins the rows against markupsafe
3.0.3 and `TypedMarkupParameter` covers both character types.
