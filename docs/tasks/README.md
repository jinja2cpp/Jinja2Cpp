# Task registry

Work items for Jinja2C++ that are bigger than a single PR or need a decision first:
audit findings, strategic directions and follow-ups. One file per task, so a task can
be read, linked and edited without merge conflicts on a shared list.

## Conventions

- File name: `NNNN-short-slug.md`, numbered in order of filing. Numbers are never reused.
- Front matter keys:
  - `status`: `open` | `in-progress` | `blocked` | `done` | `dropped`
  - `priority`: `high` | `medium` | `low`
  - `area`: `parity`, `robustness`, `ci`, `build`, `style`, `agents`, `standards`, `release`, `perf`
  - `depends`: list of task numbers, optional
  - `touches`: paths the work will edit, as a list of globs relative to the repo root
    (`src/filters.cpp`, `test/parity/`, `.github/workflows/*.yml`, `include/**`); a
    trailing `/` means the whole directory. Optional, but a task without it is assumed
    to touch everything and is never scheduled alongside another one.
  - `pr`/`issues`: links, optional
- Body: **Problem** (the contradiction: what pulls against what), **Proposal** (how to
  resolve it), **Done when** (a check someone else can run), **Next** (what the
  resolution is expected to surface; optional).
- Close a task by setting `status: done` and linking the PR; keep the file.
- A task that turns into a concrete bug can also get a GitHub issue; link it under `issues`.

Example front matter:

```yaml
---
status: open
priority: high
area: robustness
depends: [0002]
touches: [test/fuzz/, CMakeLists.txt, .github/workflows/fuzz.yml]
---
```

To list open tasks: `grep -l 'status: open' docs/tasks/0*.md`.

To see which active tasks can run at the same time (one project thread each, see
"Batching work" in `CLAUDE.md`): `python3 scripts/task_batches.py`. It groups tasks
into waves whose `touches` do not overlap and whose `depends` are met, and lists the
overlapping pairs with the shared paths. `touches` is a plan, not a contract: when a
PR turns out to edit more, update the task file in that PR.

Parity tasks (0012 onwards) come from the differential corpus; [docs/parity.md](../parity.md)
maps them by feature area and suggests an order.
Every parity PR deletes its own lines from `test/parity/divergences.txt` and edits its
own rows in `docs/parity.md`; those shared files are left out of `touches`, because
line deletions in different places merge cleanly.

## Index

| # | Task | Area | Priority | Status |
|---|---|---|---|---|
| [0001](0001-python-parity-corpus.md) | Differential parity corpus against Python Jinja2 | parity | high | done |
| [0002](0002-reflect-nlohmann-array-segfault.md) | Segfault iterating arrays reflected from `nlohmann::json` | robustness | high | open |
| [0003](0003-fuzzing.md) | Continuous fuzzing of lexer/parser/evaluator | robustness | high | open |
| [0004](0004-agent-roles.md) | Agent roles with per-role model and effort | agents | high | in-progress |
| [0005](0005-ci-matrix-by-standard.md) | CI matrix organised by C++ standard, pairwise-sparse | ci | high | in-progress |
| [0006](0006-conan-ci-and-releases.md) | Conan package in CI and resumed releases | release | high | in-progress |
| [0007](0007-cxx23-support.md) | C++23 support | standards | medium | open |
| [0008](0008-cxx-standard-floor.md) | Decide the minimum supported C++ standard | standards | medium | open |
| [0009](0009-clang-format-convergence.md) | Converge the tree on one clang-format style | style | medium | in-progress |
| [0010](0010-coverage-gate.md) | Coverage as a gate, not a number | ci | medium | open |
| [0011](0011-performance-baseline.md) | Re-enable performance tests and track a baseline | perf | low | open |
| [0012](0012-python-value-stringification.md) | Print values the way Python `str()` does | parity | high | open |
| [0013](0013-literal-syntax.md) | Literal syntax: `none`, numeric forms, dict and tuple literals | parity | high | open |
| [0014](0014-operator-and-postfix-grammar.md) | Operator and postfix grammar | parity | high | open |
| [0015](0015-arithmetic-and-logic-semantics.md) | Python arithmetic, comparison and `and`/`or` semantics | parity | high | open |
| [0016](0016-strings-as-sequences.md) | Strings behave as sequences | parity | high | open |
| [0017](0017-builtin-tests.md) | Complete the builtin tests | parity | medium | open |
| [0018](0018-missing-builtin-filters.md) | Missing builtin filters | parity | high | open |
| [0019](0019-filter-behaviour.md) | Filter behaviour divergences | parity | medium | open |
| [0020](0020-python-methods-on-values.md) | Python methods on str, list and dict values | parity | high | open |
| [0021](0021-loop-and-assignment-statements.md) | Loop controls, loop object, namespace, tuple assignment | parity | high | open |
| [0022](0022-macro-call-semantics.md) | Macro call semantics | parity | medium | open |
| [0023](0023-inheritance-and-import.md) | Template inheritance and import semantics | parity | medium | open |
| [0024](0024-whitespace-and-newlines.md) | Trailing newline, `-` modifiers, newline normalisation | parity | high | open |
| [0025](0025-autoescape.md) | Autoescape and Markup | parity | medium | open |
| [0026](0026-undefined-semantics.md) | Undefined semantics and undefined policies | parity | medium | open |
| [0027](0027-reject-invalid-templates.md) | Reject what Jinja2 rejects | parity | medium | open |
| [0028](0028-delimiters-and-line-statements.md) | Custom delimiters, line statements | parity | low | open |
| [0029](0029-i18n-extension.md) | i18n extension | parity | low | open |
| [0030](0030-global-functions.md) | Global functions: `cycler`, `joiner`, `lipsum`, `range` | parity | medium | open |
| [0031](0031-insertion-ordered-mappings.md) | Mappings keep insertion order | parity | medium | open |
| [0032](0032-custom-filters-and-tests.md) | Register custom filters and tests | parity | medium | open |
| [0033](0033-wide-string-parity.md) | Run the corpus through the wide-string API | parity | low | open |
