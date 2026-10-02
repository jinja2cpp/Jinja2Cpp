---
status: done
priority: medium
area: parity
depends: [0001, 0018, 0034]
touches: [src/internal_value.h#InternalValue, src/render_context.h, src/markup.h, src/statements.cpp#macro, src/template_parser.cpp, src/expression_evaluator.cpp#BinaryExpression]
shares: [src/internal_value.h, src/internal_value.cpp, src/value_visitors.h, src/filters.cpp, src/statements.cpp, src/template_parser.cpp, include/jinja2cpp/template_env.h]
---
# Autoescape and Markup

**Problem.** Jinja2C++ has no autoescaping: no `autoescape` Environment option, no
`{% autoescape %}` block, no notion of a safe string, so `safe`, `forceescape` and the
`escaped` test cannot exist. Any HTML use relies on the template author writing `|e`
everywhere, which is the class of bug autoescape exists to prevent (26 cases).

**Proposal.** Add a "markup" flag to string values and Markup semantics (escaping the
other operand on `~`/`+`, `join`, `replace`, `format`; macros and block `set` return
markup; `safe`/`forceescape`/`e` respect the flag), a `Settings::autoescape` (bool, and
later a callback by template name like `select_autoescape`), and the block statement.
This touches the value model: start with an architect plan.

**Done when.** No line of `test/parity/divergences/` names task 0025, and `ctest -R parity` passes.

## Plan (architect review, 2026-10-02)

**Decision.** The markup flag is a `bool` on `InternalValue`, outside the variant: no
visitor changes, it travels with copies through scopes, lists, dicts and macro arguments,
and a value built fresh by a visitor is a plain str, which is Python's default. A new
variant alternative was rejected: every visitor would need an overload, direct
`GetIf<string>` checks would stop seeing Markup as a string, and C++14 builds have two
`nonstd::variant` slots left. Cost: `InternalValue` grows from 128 to 136 bytes.

**Where autoescape lives.** `RenderContext::IsAutoescape()`, set with `AutoescapeGuard`:
- each template render (top level, include, import, extends parent) and each block body
  takes `Settings::autoescape`, so they ignore an enclosing `{% autoescape %}` as Python's
  compile-time setting does;
- a macro captures the setting where it is defined for its body; its result is Markup
  when autoescape is on where it is called (`caller()` too);
- `{% autoescape expr %}` evaluates `expr` (truthiness), opens a scope and sets it.

`{{ }}` output, `CallExpression::Render` and the filter-call paths escape a value unless
it is Markup (`OutputValue` in `src/markup.h`). Filter-block output is written as is.

**Done (PR #320).** All 28 corpus cases of 0025 match; 50 more cases pin Markup rules
(macro definition vs call site, includes and blocks inside `{% autoescape %}`, scoping,
set-blocks with filters, `join`/`replace` with Markup items, `%` and `format`). Gaps left
for [0051](0051-markup-leftovers.md).

