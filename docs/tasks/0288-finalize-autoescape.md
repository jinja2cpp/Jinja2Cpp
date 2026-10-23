---
status: done
priority: high
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/628
area: parity
touches: [src/renderer.h#FinalizedExpressionRenderer]
shares: [src/expression_parser.cpp]
---
# `finalize` output is not escaped under autoescape

Found by the 0216 verifier. `FinalizedExpressionRenderer` writes `finalize(value)` with
`WriteValue`, so with `Settings::finalize` set no `{{ }}` output is escaped, whatever the
autoescape setting (`autoescape.env_on_finalize`: Jinja2 `[&lt;][&lt;][&lt;][&lt;]`, Jinja2C++
`[<][<][<][<]`).

Jinja2 3.1.6 writes `escape(finalize(value))` for a value it evaluates at run time and
`finalize(escape(const))` for a value it folds (`CodeGenerator._output_child_to_const`), with
the static setting for a folded value in volatile code (docs/tasks/0216). So the escaping
and its order both differ between the two forms.

**Proposal.** Write the finalized value through `WriteOutput`; for a folded `{{ }}` escape
before finalize, by the static setting in volatile code (as `StaticOutputRenderer`), which
needs a static variant of the finalized renderer or a flag on it.

**Done when.** `autoescape.env_on_finalize` matches and its line is gone from
`test/parity/divergences/autoescape.txt`; add a volatile case with finalize.
