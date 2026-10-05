---
status: open
priority: low
area: perf
touches: [src/markup.h#MarkupEscape, src/render_context.h#IRendererCallback, src/template_impl.h#RendererCallback]
---
# Autoescape renders each value to a string before escaping it

**Problem.** `MarkupEscape` calls `IRendererCallback::GetAsTargetString`, which renders the
value into a fresh `std::basic_string<CharT>` (a copy for a string value, an allocation
past 15 bytes), then `EscapeHtml` writes the escaped copy. After 0124 the escape pass
itself is cheap, so in `Render/html_autoescape` (bench/cases from 0119) the remaining
`MarkupEscape` cost is mostly the first copy: `GetAsTargetString` is about 5% of the render
and the `TargetString` variant moves another 1-2%.

**Proposal.** When the value already holds a string of the template's character type
(`std::string`, `TargetString`, `TargetStringView`), escape straight from a view of it.
`MarkupEscape` does not know the template's character type, so the callback needs a way to
say it (or an `EscapeAsTargetString(val)` that does both steps). Numbers and other values
keep the render-then-escape path.

**Done when.** `Render/html_autoescape` -4% instructions (`bench/count.py --baseline`), no
other case slower, escaping corpus cases unchanged.
