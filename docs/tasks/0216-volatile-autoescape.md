---
status: done
priority: low
area: parity
touches: [src/render_context.h, src/statements.cpp#AutoescapeStatement, src/renderer.h, src/expression_parser.cpp]
shares: [src/statements.cpp, src/expression_evaluator.cpp, src/markup.h]
---
# `{% autoescape %}` with a runtime value (volatile EvalContext)

Found by 0179 (#556). When the value of `{% autoescape x %}` is not a constant, Jinja2 marks the
enclosed code's EvalContext volatile at compile time: `{{ }}` output (and `{% filter %}`
bodies, macro and loop results) then read the runtime setting, `context.eval_ctx.autoescape`,
instead of the setting where the code is written. Jinja2C++ evaluates the value and treats
it as a constant: it sets both the output and the runtime setting (RenderContext,
docs/tasks/0179) and a macro defined inside keeps the output setting it was defined under.
So a macro defined under `{% autoescape on %}` (`on` a variable) and called under a nested
`{% autoescape false %}` escapes its output in Jinja2C++, not in Jinja2
(`autoescape.volatile_macro_output`: Jinja2 `<`, Jinja2C++ `&lt;`).

Jinja2 3.1.6's volatile `~` reads `context.eval_ctx.volatile` (a bug: always false at run
time), so it joins as str; Jinja2C++ happens to give the same output for
`autoescape.volatile_concat`. Keep that case matching.

**Proposal.** Mark the parse of a non-constant `{% autoescape %}` body as volatile (a flag
on the statements and macros parsed inside it), and make the output setting of a volatile
body follow the runtime one: `OutputAutoescapeGuard` in a volatile macro body sets nothing,
and the runtime `{% autoescape false %}` nested in it sets both, as now.

**Done when.** `autoescape.volatile_macro_output` matches and its line is gone from
`test/parity/divergences/autoescape.txt`.

**Outcome (PR #614).** Jinja2 3.1.6 gives `<` for `autoescape.volatile_macro_output` because it folds the
constant `{{ '<' }}` and escapes it by the setting the code is compiled under, the
environment's here, not because the macro reads the runtime setting. Both rules are in now.
RenderContext keeps Jinja2's compile-time state at run time: the static setting
(`IsStaticAutoescape`, `frame.eval_ctx.autoescape`) and whether the code is volatile
(`IsVolatileAutoescape`, `frame.eval_ctx.volatile`); `IsOutputAutoescape` is the setting
non-constant output follows, the runtime one in volatile code. `{% autoescape %}` with a value
that is not foldable keeps the static setting and makes its body volatile; a macro, `caller`
or recursive loop captures both where it is defined; a block or template body is not
volatile. In volatile code a `{{ }}` Jinja2 folds is a `StaticOutputRenderer`, filters and
tests do not fold, `~` joins as str, and filter blocks and `loop(...)` follow the runtime
setting. Cases `autoescape.volatile_*` pin each rule.
