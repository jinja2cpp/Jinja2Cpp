---
status: done
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/628
area: parity
touches: [src/renderer.h#FinalizedExpressionRenderer]
---
# Evaluate at run time when folding a finalized `{{ }}` raises

Found by the 0288 verifier. Jinja2's `CodeGenerator.visit_Output` folds a `{{ }}` as
`finalize(escape(const))` at compile time and, when that raises any exception, compiles the
expression for run time instead: `escape(finalize(value))`. Jinja2C++ decides foldability at
parse time and calls finalize the folded way at render time (`FinalizedExpressionRenderer`,
docs/tasks/0288), so an error of the folded call is reported.

Repro: a finalize that raises unless its argument is an int and returns `v + 1`; template
`{{ 1 }}` with autoescape on. Python renders `2` (folding fails on `Markup('1')`, the run-time
call gets the int); Jinja2C++ reports "Exception thrown by a user callable: not int".

**Proposal.** In the folded path, when the finalize call throws, retry as the run-time path
(finalize of the unescaped value, then `WriteOutput`). The parity harness needs a finalize that
raises on a string (a third entry in `FINALIZE` / `CustomFinalize`) to pin it.

**Done when.** A parity case with such a finalize matches, narrow and wide.

**Done** in #628 (Codex found it as a regression of 0288): the folded path retries as the
run-time path when finalize raises a host error or a `TemplateRuntimeError`; the harness has
a `plus_one` finalize, case `autoescape.env_on_finalize_fold_raises`.
