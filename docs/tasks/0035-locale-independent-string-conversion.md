---
status: open
priority: medium
area: robustness
touches: [include/jinja2cpp/string_helpers.h]
shares: [src/value_visitors.h, src/internal_value.cpp]
---
# Convert between narrow and wide strings without the C locale

**Problem.** `jinja2::ConvertString` (include/jinja2cpp/string_helpers.h) converts with
`mbsrtowcs`/`wcsrtombs`, which follow the process's C locale, and returns an empty
string when a character does not convert. A program that does not call `setlocale` runs
in the "C" locale, so every non-ASCII string that crosses between `char` and `wchar_t`
silently becomes empty. The library does this whenever a template and its data differ in
width: a `TemplateW` given a `std::string` context value (`ValueRenderer`, src/value_visitors.h),
a narrow template given a `std::wstring`, wide field names in subscripts and reflection
(src/internal_value.cpp), and the JSON serializers. Found by task 0033: with narrow context
values in the wide corpus run, `{{ u }}` with `u = "héllo"` renders `""`, and
`truncate(20, false, '…')` loses the `…`.

**Proposal.** Treat `std::string` as UTF-8 (what the narrow API already assumes in
practice) and `std::wstring` as UTF-32, or UTF-16 where `wchar_t` is 16 bits, and convert
with a small built-in codec independent of the locale, replacing invalid sequences with
U+FFFD rather than dropping the whole string. Document the encoding in the public header.

**Done when.** A unit test renders non-ASCII narrow values through `TemplateW`, and wide
values through `Template`, with no `setlocale` call, and gets the text back; the parity
suite can pass narrow context values to the wide run (test/parity/parity_test.cpp,
`RenderCpp`) with no new `wide.` divergences.
