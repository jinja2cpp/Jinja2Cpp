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
    `path#region` names one part of a file (`src/value_visitors.h#ValueRenderer`,
    `src/template_parser.cpp#splitter`); it conflicts with the same region or the whole
    file, not with other regions of that file.
  - `shares`: files the task edits only in its own places (its own functions, an
    appended table row, a new `Settings` field). Overlaps through `shares` do not keep
    tasks apart: git merges them, and the PR that merges second brings master in first.
    Put a file under `touches` when the task restructures it or edits code another task
    also edits.
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
"Batching work" in `CLAUDE.md`): `python3 scripts/task_batches.py` (`--area parity` for
one area). It groups tasks into waves whose `touches` do not overlap and whose `depends`
are met, highest priority first, and lists the overlapping pairs with the shared paths. `touches` is a plan, not a contract: when a
PR turns out to edit more, update the task file in that PR.

Parity tasks (0012 onwards) come from the differential corpus; [docs/parity.md](../parity.md)
maps them by feature area and suggests an order.
Every parity PR deletes its own lines from `test/parity/divergences/<area>.txt` and edits
its own feature rows in `docs/parity.md`; those shared files are left out of `touches`.
The summary table and the corpus size in `docs/parity.md` are not edited by parity PRs.
Two PRs that are each green can still be red together (one fixes the parser, the other
the printer, and a case listed under one of them now matches), so a wave of parity PRs
lands through one integration branch: each PR stays on its own base, and once all are
final they are merged together, the corpus is re-run with
`test/parity/update_divergences.py`, the summary is regenerated, and one CI run
validates the lot (docs/tasks/0040).
Unit tests that encode old behaviour are updated by the task that changes it; such
files go under `touches` (0012 and 0024 each rewrite about a hundred rows).

## Index

| # | Task | Area | Priority | Status |
|---|---|---|---|---|
| [0001](0001-python-parity-corpus.md) | Differential parity corpus against Python Jinja2 | parity | high | done |
| [0002](0002-reflect-nlohmann-array-segfault.md) | Segfault iterating arrays reflected from `nlohmann::json` | robustness | high | open |
| [0003](0003-fuzzing.md) | Continuous fuzzing of lexer/parser/evaluator | robustness | high | done |
| [0004](0004-agent-roles.md) | Agent roles with per-role model and effort | agents | high | done |
| [0005](0005-ci-matrix-by-standard.md) | CI matrix organised by C++ standard, pairwise-sparse | ci | high | in-progress |
| [0006](0006-conan-ci-and-releases.md) | Conan package in CI and resumed releases | release | high | in-progress |
| [0007](0007-cxx23-support.md) | C++23 support | standards | medium | done |
| [0008](0008-cxx-standard-floor.md) | Decide the minimum supported C++ standard | standards | medium | done |
| [0009](0009-clang-format-convergence.md) | Converge the tree on one clang-format style | style | medium | in-progress |
| [0010](0010-coverage-gate.md) | Coverage as a gate, not a number | ci | medium | open |
| [0011](0011-performance-baseline.md) | Re-enable performance tests and track a baseline | perf | high | in-progress |
| [0012](0012-python-value-stringification.md) | Print values the way Python `str()` does | parity | high | done |
| [0013](0013-literal-syntax.md) | Literal syntax: `none`, numeric forms, dict and tuple literals | parity | high | done |
| [0014](0014-operator-and-postfix-grammar.md) | Operator and postfix grammar | parity | high | done |
| [0015](0015-arithmetic-and-logic-semantics.md) | Python arithmetic, comparison and `and`/`or` semantics | parity | high | done |
| [0016](0016-strings-as-sequences.md) | Strings behave as sequences | parity | high | done |
| [0017](0017-builtin-tests.md) | Complete the builtin tests | parity | medium | done |
| [0018](0018-missing-builtin-filters.md) | Missing builtin filters | parity | high | done |
| [0019](0019-filter-behaviour.md) | Filter behaviour divergences | parity | medium | done |
| [0020](0020-python-methods-on-values.md) | Python methods on str, list and dict values | parity | high | done |
| [0021](0021-loop-and-assignment-statements.md) | Loop controls, loop object, namespace, tuple assignment | parity | high | done |
| [0022](0022-macro-call-semantics.md) | Macro call semantics | parity | medium | done |
| [0023](0023-inheritance-and-import.md) | Template inheritance and import semantics | parity | medium | done |
| [0024](0024-whitespace-and-newlines.md) | Trailing newline, `-` modifiers, newline normalisation | parity | high | done |
| [0025](0025-autoescape.md) | Autoescape and Markup | parity | medium | done |
| [0026](0026-undefined-semantics.md) | Undefined semantics and undefined policies | parity | medium | done |
| [0027](0027-reject-invalid-templates.md) | Reject what Jinja2 rejects | parity | medium | done |
| [0028](0028-delimiters-and-line-statements.md) | Custom delimiters, line statements | parity | low | done |
| [0029](0029-i18n-extension.md) | i18n extension | parity | low | done |
| [0030](0030-global-functions.md) | Global functions: `cycler`, `joiner`, `lipsum`, `range` | parity | medium | done |
| [0031](0031-insertion-ordered-mappings.md) | Mappings keep insertion order | parity | medium | in-progress |
| [0032](0032-custom-filters-and-tests.md) | Register custom filters and tests | parity | medium | done |
| [0033](0033-wide-string-parity.md) | Run the corpus through the wide-string API | parity | low | done |
| [0034](0034-none-versus-undefined.md) | Tell `None` apart from undefined | parity | high | done |
| [0035](0035-locale-independent-string-conversion.md) | Convert narrow/wide strings without the C locale | robustness | medium | open |
| [0036](0036-non-string-mapping-keys.md) | Mapping keys that are not strings | parity | low | open |
| [0037](0037-sequence-protocol-follow-ups.md) | Sequence protocol follow-ups from 0016 | parity | medium | open |
| [0038](0038-lexical-scoping-for-macros.md) | Lexical scoping for macros | parity | medium | open |
| [0039](0039-float-values-print-widened.md) | C++ `float` values print with their widened digits | parity | low | open |
| [0040](0040-parity-shared-files-serialise-merges.md) | Parity PRs collide in shared generated files | process | high | done |
| [0041](0041-string-literal-escapes.md) | String literal escape sequences | parity | low | open |
| [0042](0042-loop-cycle-magic-number.md) | Global function follow-ups: `loop.cycle` is the integer 2, globals are maps | parity | low | open |
| [0043](0043-ordered-valuesmap-2-0.md) | Insertion-ordered `ValuesMap` (2.0.0) | release | medium | open |
| [0044](0044-lstrip-blocks-leftovers.md) | `lstrip_blocks` and modifier leftovers | parity | low | open |
| [0045](0045-ordering-none-and-undefined.md) | `sort`, `min` and `max` over `None`, undefined values or dicts | parity | low | open |
| [0047](0047-none-leftovers.md) | None and undefined: JSON null, `Undefined` repr, string filters on None | parity | medium | open |
| [0048](0048-filter-behaviour-leftovers.md) | Filter leftovers: Unicode case, HTML entities, big ints, unused JSON serializers | parity | low | open |
| [0049](0049-aliasing-borrowed-containers.md) | Mutation follow-ups: aliases of context data, cycles, loops over changing lists | parity | low | open |
| [0050](0050-error-location-quadratic.md) | Error reporting is quadratic for many errors on one long line | perf | low | open |
| [0051](0051-markup-leftovers.md) | Markup leftovers: `~` under autoescape, Markup methods and repr, Markup from C++ | parity | low | open |
| [0052](0052-undefined-in-filters.md) | Undefined in filters, tests and `range()`: what 0026 left behind | parity | low | open |
| [0054](0054-clang-tidy-adoption.md) | clang-tidy: adopt the latest checks and modernize the code in batches | style | medium | in-progress |
| [0055](0055-clang-tidy-bug-findings.md) | Bug-class findings from the clang-tidy survey (0054, batch 1) | robustness | medium | done |
| [0056](0056-public-api-2-0-review.md) | Public API review and migration path for 2.0.0 | release | medium | open |
| [0057](0057-tidy-mechanical-fixes.md) | clang-tidy: behaviour-neutral mechanical fixes | style | medium | open |
| [0058](0058-implicit-null-conditions.md) | Implicit pointer-to-bool in conditions: rewrite and enforce | style | medium | open |
| [0059](0059-nodiscard-macro.md) | [[nodiscard]] on results that must not be dropped | style | medium | open |
| [0060](0060-insert-braces.md) | Braces around every single-statement body | style | medium | open |
| [0061](0061-cognitive-complexity.md) | Bring functions under cognitive complexity 25 | style | low | open |
| [0062](0062-tidy-signature-fixes.md) | clang-tidy: fixes that change signatures, copies or linkage | perf | medium | open |
| [0063](0063-tidy-tests.md) | clang-tidy on test/ | style | low | done |
| [0064](0064-include-cleaner.md) | Include what you use (misc-include-cleaner) | build | low | done |
| [0065](0065-identifier-naming-src.md) | readability-identifier-naming for src/ | style | low | done |
| [0066](0066-tidy-cxx17-checks.md) | clang-tidy bug-class findings that only C++17 shows | robustness | medium | done |
| [0067](0067-value-integral-construction.md) | `Value` from unsigned and wide integers stores `bool` | robustness | high | open |
| [0068](0068-package-abi-facts.md) | The installed package does not carry the library's ABI choices | build | high | open |
| [0069](0069-public-header-defects.md) | Defects in the public headers | robustness | medium | open |
| [0070](0070-cxx23-floor.md) | Drop C++14: C++17 floor, C++23 supported | standards | high | done |
| [0071](0071-drop-nonstd.md) | Replace optional/variant/string-view-lite with `std::`; pin expected-lite | standards | high | done |
| [0072](0072-value-api-2-0.md) | 2.0 API: `Value` accessors and `ToString` | release | high | open |
| [0073](0073-template-api-2-0.md) | 2.0 API: `BasicTemplate<CharT>` | release | medium | open |
| [0074](0074-template-env-api-2-0.md) | 2.0 API: `TemplateEnv` pimpl and `Settings` | release | medium | open |
| [0075](0075-containers-reflection-api-2-0.md) | 2.0 API: containers, reflection, errors | release | medium | open |
| [0076](0076-abi-namespace-and-headers.md) | 2.0: inline ABI namespace, header layout, version | release | medium | open |
| [0077](0077-migration-script.md) | 2.0 migration script and notes | release | medium | open |
| [0078](0078-nix-toolchains.md) | Pinned toolchains from Nix for tool and bleeding-edge CI rows | ci | low | open |
| [0079](0079-msvc-runtime-shared-override.md) | Warn when a shared build overrides `JINJA2CPP_MSVC_RUNTIME_TYPE` | build | low | open |
| [0080](0080-nodiscard-result-void.md) | `[[nodiscard]]` on `Load` and `Render`, whose only output is an error | api | low | open |
| [0081](0081-tidy-leftovers.md) | clang-tidy hits left after the batches | style | low | done |
| [0082](0082-msvc-warning-flags.md) | MSVC never gets the strict warning flags | build | low | open |
| [0083](0083-cxx17-idioms.md) | C++17 idioms clang-tidy does not automate | style | low | open |
| [0084](0084-dead-value-sources.md) | Delete the dead `src/value.cpp` and `src/value_helpers.h` bodies | style | low | open |
| [0085](0085-header-include-cleaner.md) | include-cleaner on headers analysed on their own | build | low | open |
| [0086](0086-keyword-regex-in-parser.md) | Statement keywords are matched with a regex compiled on every `Load` | perf | high | open |
| [0087](0087-for-loop-state-per-iteration.md) | `for` loops rebuild the `loop` map and a scope map on every iteration | perf | high | open |
| [0088](0088-expression-evaluation-overhead.md) | Expression evaluation is several times slower than Python Jinja2 | perf | medium | open |
| [0089](0089-gcc-release-maybe-uninitialized.md) | GCC Release builds warn `-Wmaybe-uninitialized` in `polymorphic_cxx14.h` | build | low | open |
| [0090](0090-differential-fuzzing-findings.md) | Divergences found by differential fuzzing | parity | medium | open |
