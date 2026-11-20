---
status: open
priority: low
area: parity
depends: [0546]
touches: [src/expression_parser.cpp#FoldCheckPasses, src/template_impl.h#FailedFold]
shares: [test/parity/cases/undefined.py, test/parity/divergences/undefined.txt]
---
# Strict undefined fold errors outside tuple-of-slices templates

**Problem.** Jinja2's optimizer folds every constant node when it compiles, bottom-up, and the UndefinedError
of bool() or str() of a StrictUndefined in an `and`, `or`, `~` or conditional leaves its as_const: the compile
fails, also in code that never renders and in an operand a short-circuit skips. The engine's fold check
(FoldCheckPasses) runs only for an expression with a tuple-of-slices subscript, so elsewhere the same template
loads and fails only if it renders that code:

- `{% if false %}{{ 1.5 ~ ([]|max) }}{% endif %}b` and `{% if false %}{{ 1 if []|max else 2 }}{% endif %}b`
  render `b` (corpus `undefined.strict_fold_concat_unrendered`, `..._conditional_test_unrendered`, accepts);
- `{{ 1 or ([]|max ~ 1) }}` fails the render instead of the compile (both fail: the corpus matches).
- inside a tuple-of-slices check, an operand the check does not evaluate (the right one of a short-circuiting
  `or`) is not looked into: `undefined.strict_fold_unfolded_operand_conditional` (accepts; found by the 0546
  verifier, whose narrowing of `and`/`or`/`~` failures removed an accidental match there).

The sources of a constant strict undefined are few: filters such as `max`, `min` and `first` of an empty
literal, and a missing item or attribute of a literal.

**Proposal.** Under StrictUndefined only, record a fold candidate for an `and`, `or`, `~` or conditional whose
operands may fold, as for tuple-of-slices subscripts, and check them at Load with `LetsUndefinedOut` (0546),
or find at parse time the constant sources that give a strict undefined and check only expressions with one.
Measure the Load cost on bench/ templates (StrictUndefined is uncommon but chat templates use it).

**Done when.** The three corpus lines are deleted, and a strict template without such constants loads with no
measurable cost (bench/count.py Load workloads within noise).
