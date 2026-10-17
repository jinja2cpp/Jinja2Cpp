---
status: done
priority: low
area: parity
depends: [0165]
touches: [src/render_context.h, src/statements.cpp#macro, src/expression_evaluator.cpp#InvokeCallable]
shares: [src/statements.cpp, src/filters.cpp, src/global_functions.cpp, src/serialize_filters.cpp, src/string_converter_filter.cpp]
---
# Autoescape has a lexical and a runtime side

**Problem.** Jinja2 decides two things by autoescape, at two different times. Output
(`{{ }}`) and `~` use the setting compiled into the code where they are written (lexical).
Everything that reads `context.eval_ctx.autoescape` at run time uses the setting in force
when it runs: whether a macro's or `caller()`'s result is Markup, `{% set %}` and
`{% filter %}` blocks, and the `pass_eval_context` filters (`join`, `replace`, `xmlattr`,
`urlize`, ...). `{% autoescape %}` changes both, and the runtime one is dynamic: a macro
body runs with its caller's runtime setting but escapes its output by its definition's.

Jinja2C++ has one flag (`RenderContext::IsAutoescape`), and a macro sets it to the value at
its definition for the whole body (`AutoescapeGuard` in `MacroStatement::MakeCallable`).
So inside a macro defined under autoescape and called in `{% autoescape false %}`,
`caller()` returns Markup where Jinja2 returns a plain str
(`autoescape.env_on_caller_markup_call_site`):

```
{% macro w() %}[{{ caller() }}]{% endmacro %}
{% autoescape false %}{% call w() %}<{{ html }}>{% endcall %}{% endautoescape %}
```

Jinja2 gives `[&lt;&lt;b&gt;...]`, Jinja2C++ `[<<b>...]`.

**Proposal.** Keep two flags in `RenderContext`: the lexical one for `WriteOutput` and `~`
(`BinaryExpression::Apply`), the runtime one for every Markup decision. `{% autoescape %}`
and a new template (include, import, extends) set both; a macro body sets only the lexical
one. Then audit each `IsAutoescape()` call for which of the two it means.

**Why not in 0051.** It changes `render_context.h` and the macro code in `statements.cpp`,
which 0165 (lexical scoping of macro bodies) owns.

**Done when.** `autoescape.env_on_caller_markup_call_site` matches and its line is gone
from `test/parity/divergences/autoescape.txt`; add corpus cases for a macro result and a
`pass_eval_context` filter inside a macro called under `{% autoescape false %}`.

**Done** (PR to be linked): `RenderContext` keeps the output setting (`IsOutputAutoescape`,
for `{{ }}`, `~`, `{% filter %}`, the filter input of `{% set x | f %}` and `loop(...)`
results) apart from the runtime one (`IsAutoescape`, for macro, `caller()`, `self.b()`,
`{% set %}` block and `pass_eval_context` filter results). A macro or block body sets only
the output one. The runtime setting is one per Jinja2 Context: contexts carry an
EvalContext id, new for a template render and an include, and a macro body whose caller
shares its home's id runs with the caller's runtime setting (`HomeCall`). `{{ m() }}`
writes a macro's output straight to the stream except where the output escapes and the
runtime setting is off. A non-constant `{% autoescape %}` (Jinja2's volatile EvalContext)
is left to 0210.
