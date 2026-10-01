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
  - `pr`/`issues`: links, optional
- Body: **Problem** (the contradiction: what pulls against what), **Proposal** (how to
  resolve it), **Done when** (a check someone else can run), **Next** (what the
  resolution is expected to surface; optional).
- Close a task by setting `status: done` and linking the PR; keep the file.
- A task that turns into a concrete bug can also get a GitHub issue; link it under `issues`.

To list open tasks: `grep -l 'status: open' docs/tasks/0*.md`.

## Index

| # | Task | Area | Priority | Status |
|---|---|---|---|---|
| [0001](0001-python-parity-corpus.md) | Differential parity corpus against Python Jinja2 | parity | high | open |
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
