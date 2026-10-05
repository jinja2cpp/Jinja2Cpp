---
status: done
priority: low
area: perf
touches: [src/markup.h#MarkupEscape, src/expression_evaluator.cpp#ExpressionEvaluatorBase::Render, src/render_context.h#IRendererCallback, src/template_impl.h#RendererCallback]
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

**Done** in [#403](https://github.com/jinja2cpp/Jinja2Cpp/pull/403). `IRendererCallback::IsWideTarget()` tells `MarkupEscape` the template's
width: a string of that width is escaped from a view of its own text. `{{ ... }}` output goes
through `WriteOutput`/`WriteEscaped`, which escape a narrow string of up to 4096 bytes straight
from the per-thread buffer into the stream, with no string or `InternalValue` in between.
`WriteEscaped` is out of line: inlined, it made the paths without autoescape 3-4.5% dearer.
Measured with `bench/count.py --baseline` against master 698f881: `Render/html_autoescape`
-16.0% instructions, 1960 -> 1340 allocations per render; every other case within +0.6%.
