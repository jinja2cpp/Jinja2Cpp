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
- Close a task by setting `status: done` and linking the PR; keep the file. The PR that
  finishes a task sets `done` itself. When a PR leaves it `in-progress` (it lands through a
  train, or its owner waits for the merge), the merge steward sets `status: done` and the
  `pr` link in the train or index-sync commit for every task that PR's description says it
  closes, so that a merged task never stays `in-progress`.
- The index table at the end of this file is generated from the task files by
  `scripts/task_index.py` (title from the `# ` heading, the rest from front matter). PRs
  never edit its rows; the merge steward runs the script after each merge batch.
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
| [0002](0002-reflect-nlohmann-array-segfault.md) | Segfault iterating arrays reflected from `nlohmann::json` | robustness | high | done |
| [0003](0003-fuzzing.md) | Continuous fuzzing of lexer/parser/evaluator | robustness | high | done |
| [0004](0004-agent-roles.md) | Agent roles with per-role model and effort | agents | high | done |
| [0005](0005-ci-matrix-by-standard.md) | CI matrix organised by C++ standard, pairwise-sparse | ci | high | done |
| [0006](0006-conan-ci-and-releases.md) | Conan package in CI and resumed releases | release | high | in-progress |
| [0007](0007-cxx23-support.md) | C++23 support | standards | medium | done |
| [0008](0008-cxx-standard-floor.md) | Decide the minimum supported C++ standard | standards | medium | done |
| [0009](0009-clang-format-convergence.md) | Converge the tree on one clang-format style | style | medium | done |
| [0010](0010-coverage-gate.md) | Coverage as a gate, not a number | ci | medium | open |
| [0011](0011-performance-baseline.md) | Re-enable performance tests and track a baseline | perf | high | done |
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
| [0031](0031-insertion-ordered-mappings.md) | Mappings keep insertion order | parity | medium | done |
| [0032](0032-custom-filters-and-tests.md) | Register custom filters and tests | parity | medium | done |
| [0033](0033-wide-string-parity.md) | Run the corpus through the wide-string API | parity | low | done |
| [0034](0034-none-versus-undefined.md) | Tell `None` apart from undefined | parity | high | done |
| [0035](0035-locale-independent-string-conversion.md) | Convert between narrow and wide strings without the C locale | robustness | medium | done |
| [0036](0036-non-string-mapping-keys.md) | Mapping keys that are not strings | parity | low | done |
| [0037](0037-sequence-protocol-follow-ups.md) | Sequence protocol follow-ups from 0016 | parity | medium | done |
| [0038](0038-lexical-scoping-for-macros.md) | Lexical scoping for macros | parity | medium | done |
| [0039](0039-float-values-print-widened.md) | C++ `float` values print with their widened digits | parity | medium | done |
| [0040](0040-parity-shared-files-serialise-merges.md) | Parity PRs collide in shared generated files | process | high | done |
| [0041](0041-string-literal-escapes.md) | String literal escape sequences | parity | low | done |
| [0042](0042-loop-cycle-magic-number.md) | Global function follow-ups: globals are maps | parity | low | done |
| [0043](0043-ordered-valuesmap-2-0.md) | Insertion-ordered ValuesMap (2.0.0) | release | medium | done |
| [0044](0044-lstrip-blocks-leftovers.md) | lstrip_blocks and modifier leftovers | parity | low | done |
| [0045](0045-ordering-none-and-undefined.md) | `sort`, `min` and `max` over `None`, undefined values or dicts | parity | low | done |
| [0047](0047-none-leftovers.md) | None and undefined: what 0034 left behind | parity | medium | done |
| [0048](0048-filter-behaviour-leftovers.md) | Filter behaviour: what 0019 left behind | parity | low | done |
| [0049](0049-aliasing-borrowed-containers.md) | Mutation follow-ups: aliases of context data, cycles, loops over changing lists | parity | low | open |
| [0050](0050-error-location-quadratic.md) | Error reporting is quadratic for many errors on one long line | perf | low | done |
| [0051](0051-markup-leftovers.md) | Markup: what 0025 left behind | parity | low | done |
| [0052](0052-undefined-in-filters.md) | Undefined in filters, tests and global functions: what 0026 left behind | parity | low | done |
| [0054](0054-clang-tidy-adoption.md) | clang-tidy: adopt the latest checks and modernize the code in batches | style | medium | done |
| [0055](0055-clang-tidy-bug-findings.md) | Bug-class findings from the clang-tidy survey (0054, batch 1) | robustness | medium | done |
| [0056](0056-public-api-2-0-review.md) | Public API review and migration path for 2.0.0 | release | medium | done |
| [0057](0057-tidy-mechanical-fixes.md) | clang-tidy: behaviour-neutral mechanical fixes | style | medium | done |
| [0058](0058-implicit-null-conditions.md) | Implicit pointer-to-bool in conditions: rewrite and enforce | style | medium | done |
| [0059](0059-nodiscard-macro.md) | [[nodiscard]] on results that must not be dropped | style | medium | done |
| [0060](0060-insert-braces.md) | Braces around every single-statement body | style | medium | done |
| [0061](0061-cognitive-complexity.md) | Bring functions under cognitive complexity 25 | style | low | done |
| [0062](0062-tidy-signature-fixes.md) | clang-tidy: fixes that change signatures, copies or linkage | perf | medium | done |
| [0063](0063-tidy-tests.md) | clang-tidy on test/ | style | low | done |
| [0064](0064-include-cleaner.md) | Include what you use (misc-include-cleaner) | build | low | done |
| [0065](0065-identifier-naming-src.md) | readability-identifier-naming for src/ | style | low | done |
| [0066](0066-tidy-cxx17-checks.md) | clang-tidy bug-class findings that only C++17 shows | robustness | medium | done |
| [0067](0067-value-integral-construction.md) | `Value` from unsigned and wide integers stores `bool` | robustness | high | done |
| [0068](0068-package-abi-facts.md) | The installed package does not carry the library's ABI choices | build | high | done |
| [0069](0069-public-header-defects.md) | Defects in the public headers | robustness | medium | done |
| [0070](0070-cxx23-floor.md) | Drop C++14: C++17 floor, C++23 supported | standards | high | done |
| [0071](0071-drop-nonstd.md) | Replace optional-lite, variant-lite and string-view-lite with `std::`; pin expected-lite | standards | high | done |
| [0072](0072-value-api-2-0.md) | 2.0 API: `Value` accessors and `ToString` | release | high | done |
| [0073](0073-template-api-2-0.md) | 2.0 API: `BasicTemplate<CharT>` | release | medium | done |
| [0074](0074-template-env-api-2-0.md) | 2.0 API: `TemplateEnv` pimpl and `Settings` | release | medium | done |
| [0075](0075-containers-reflection-api-2-0.md) | 2.0 API: containers, reflection, errors | release | medium | done |
| [0076](0076-abi-namespace-and-headers.md) | 2.0: inline ABI namespace, header layout, version | release | high | done |
| [0077](0077-migration-script.md) | 2.0 migration script and notes | release | high | open |
| [0078](0078-nix-toolchains.md) | Pinned toolchains from Nix for the tool and bleeding-edge rows | ci | low | open |
| [0079](0079-msvc-runtime-shared-override.md) | Warn when `JINJA2CPP_MSVC_RUNTIME_TYPE` is overridden by a shared build | build | low | open |
| [0080](0080-nodiscard-result-void.md) | `[[nodiscard]]` on `Load` and `Render`, whose only output is an error | api | low | done |
| [0081](0081-tidy-leftovers.md) | clang-tidy hits left after the batches | style | low | done |
| [0082](0082-msvc-warning-flags.md) | MSVC never gets the strict warning flags | build | low | done |
| [0083](0083-cxx17-idioms.md) | C++17 idioms clang-tidy does not automate | style | low | done |
| [0084](0084-dead-value-sources.md) | Delete the dead `src/value.cpp` and `src/value_helpers.h` bodies | style | low | done |
| [0085](0085-header-include-cleaner.md) | include-cleaner on headers analysed on their own | build | low | done |
| [0086](0086-keyword-regex-in-parser.md) | Statement keywords are matched with a regex compiled on every `Load` | perf | high | done |
| [0087](0087-for-loop-state-per-iteration.md) | `for` loops rebuild the `loop` map and a scope map on every iteration | perf | high | done |
| [0088](0088-expression-evaluation-overhead.md) | Expression evaluation is several times slower than Python Jinja2 | perf | medium | done |
| [0089](0089-gcc-release-maybe-uninitialized.md) | GCC Release builds warn `-Wmaybe-uninitialized` in `polymorphic_cxx14.h` | build | low | done |
| [0090](0090-drop-boost-regex.md) | Boost.Regex and `JINJA2CPP_USE_REGEX` are dead weight after 0086 | build | medium | done |
| [0091](0091-for-loop-edge-parity.md) | `for` loop corners that still differ from Jinja2 | parity | low | done |
| [0092](0092-extended-warning-set.md) | Enforce a warning set beyond `-Wall` | build | medium | done |
| [0093](0093-deeply-nested-values.md) | Deeply nested values overflow the stack when they are destroyed or printed | robustness | medium | done |
| [0094](0094-differential-fuzzing-findings.md) | Divergences found by differential fuzzing | parity | medium | done |
| [0095](0095-structure-aware-fuzzing.md) | Structure-aware fuzzing: mutate templates, not bytes | robustness | high | open |
| [0096](0096-continuous-fuzzing-service.md) | Continuous fuzzing on ClusterFuzzLite or OSS-Fuzz | ci | low | open |
| [0097](0097-resource-limits.md) | Templates can ask for unbounded time and memory | robustness | medium | done |
| [0098](0098-configurable-recursion-limits.md) | Configurable recursion limits, closer to Python's | api | low | done |
| [0099](0099-differential-triage-routine.md) | Triage the nightly differential report into parity cases | process | low | open |
| [0100](0100-render-hot-path-round-2.md) | Render hot path, round 2 | perf | medium | done |
| [0101](0101-single-pass-generator-lists.md) | Lazy filter results are reusable, Python generators are single-pass | parity | low | done |
| [0102](0102-splitter-byte-scan.md) | The template splitter tries every delimiter at every byte of text | perf | medium | done |
| [0103](0103-load-allocations.md) | Loading a tag-heavy template allocates about 115 times per tag | perf | medium | done |
| [0104](0104-fixed-costs-per-render.md) | Fixed allocations per render and per macro call | perf | medium | done |
| [0105](0105-include-per-render.md) | `include` and `extends` go through the environment's locked cache on every render | perf | medium | done |
| [0106](0106-percent-format-divergences.md) | `%`-format divergences from Python | parity | low | done |
| [0107](0107-string-filter-divergences.md) | String filter divergences found by the 0061 differential | parity | low | done |
| [0108](0108-include-context-copy.md) | `include` copies every scope of the caller | perf | medium | done |
| [0109](0109-load-costs-round-2.md) | Load costs after 0103: filter construction, wrappers, the lexer | perf | low | done |
| [0110](0110-namespace-self-reference-leak.md) | A namespace that holds itself leaks | evaluator | high | done |
| [0111](0111-sum-attribute-generated-list-hang.md) | `sum(attribute=...)` never returns on a generated list | robustness | medium | done |
| [0112](0112-generated-item-string-view-lifetime.md) | Strings from generator items outlive the item under autoescape `join` | robustness | medium | done |
| [0113](0113-output-writer.md) | Output: one virtual call and one append per fragment | perf | medium | done |
| [0114](0114-reuse-fmt-for-formatting.md) | Lean on fmt for string building: `format`, `%`, numbers, concatenation | perf | medium | done |
| [0115](0115-user-data-without-conversion.md) | Reading user data builds keys and copies lists | perf | medium | done |
| [0116](0116-ascii-string-filters.md) | Case-mapping filters go through the Unicode path for ASCII text | perf | low | done |
| [0117](0117-name-slots.md) | Resolve variable names to slots at Load | perf | high | open |
| [0118](0118-parse-tree-arena.md) | Allocate a template's parse tree from one arena | perf | high | done |
| [0119](0119-realistic-bench-workloads.md) | Benchmark workloads people actually render | perf | medium | done |
| [0120](0120-compare-cpp-engines.md) | Compare with other C++ template engines | perf | low | done |
| [0121](0121-memory-footprint.md) | Track memory per loaded template and per render | perf | low | done |
| [0122](0122-lto-pgo.md) | Measure LTO and PGO builds | build | low | done |
| [0123](0123-perf-scout-routine.md) | A recurring perf scout and a drift alert on the trend | perf | medium | in-progress |
| [0124](0124-escapers-scan-spans.md) | `tojson`, HTML escaping and `trim` walk strings one character at a time | perf | medium | done |
| [0125](0125-reflected-nested-items-outlive-owner.md) | Nested items of a reflected value a callable returns outlive their owner | robustness | high | done |
| [0126](0126-escape-from-value-view.md) | Autoescape renders each value to a string before escaping it | perf | low | done |
| [0127](0127-filter-argument-types.md) | tojson accepts a non-int indent and calls accept a repeated keyword | parity | low | done |
| [0128](0128-compile-constant-percent-format.md) | Parse a constant `%` format once, at Load | perf | low | done |
| [0129](0129-nested-context-scope-storage.md) | Every nested render context allocates a scope deque | perf | low | done |
| [0130](0130-template-footprint.md) | A loaded template keeps 30 times its source | perf | low | done |
| [0131](0131-load-fixed-cost.md) | Load has a fixed cost that inja does not | perf | low | done |
| [0132](0132-wide-output-path.md) | Rendering to a wide string pays more per fragment than to a narrow one | perf | low | done |
| [0133](0133-loop-entry-allocations.md) | Each entry into a `for` loop allocates its state anew | perf | medium | done |
| [0134](0134-failed-reload-reads-freed-source.md) | Reloading a template, when the new Load fails, leaves it reading freed memory | robustness | high | done |
| [0135](0135-rust-engines-bench.md) | Compare with the Rust template engines | perf | low | done |
| [0136](0136-rust-engines-gaps.md) | Where MiniJinja and Tera are faster | perf | medium | done |
| [0137](0137-cachegrind-gate.md) | Cache misses are not measured | perf | high | done |
| [0138](0138-output-hint-false-sharing.md) | Every render writes a cache line that all threads read | perf | medium | done |
| [0139](0139-per-render-fixed-cost.md) | A tiny render is 98% setup and teardown | perf | high | done |
| [0140](0140-internal-value-size.md) | `InternalValue` is 72 bytes | perf | medium | done |
| [0141](0141-switch-dispatch.md) | Every node visit is a virtual call | perf | low | done |
| [0142](0142-single-pass-scanner.md) | Load scans the source twice | perf | low | done |
| [0143](0143-self-reference-leftovers.md) | `self` leftovers after 0139 | parity | low | done |
| [0144](0144-loop-cycle-sentinel.md) | `loop.cycle` is the integer 2 | parity | medium | done |
| [0145](0145-callable-identity-equality.md) | A macro or a callable is not equal to itself | parity | low | done |
| [0146](0146-load-settings-fast-path.md) | Load compares the whole Settings on every template | perf | medium | done |
| [0147](0147-macro-call-residual-cost.md) | What a macro call still costs after its arguments moved to slots | perf | low | done |
| [0148](0148-single-use-names-skip-cache.md) | Skip the lookup cache for names that are read once per render | perf | low | done |
| [0149](0149-static-template-constants.md) | Template constants pay an atomic refcount on every copy | perf | low | done |
| [0150](0150-internal-value-24-bytes.md) | Is a 24-byte `InternalValue` worth a hand-written union? | perf | low | dropped |
| [0151](0151-direct-string-building.md) | Strings built then moved into a value allocate twice | perf | low | done |
| [0152](0152-expression-precedence-descent.md) | Every operand descends all eleven precedence levels at Load | perf | medium | done |
| [0153](0153-string-slice-by-code-point.md) | Slicing a string builds a vector of every code point | perf | medium | done |
| [0154](0154-per-item-loop-costs.md) | What each item of a loop over user data costs that Tera does not pay | perf | medium | done |
| [0155](0155-from-import-same-name-twice.md) | `from ... import` of one name under two names | parity | low | done |
| [0156](0156-call-block-on-non-macro.md) | A call block on an undefined name or a non-callable value | parity | low | done |
| [0157](0157-agent-cost-and-models.md) | Agent spend goes to re-reading long threads, not to the model that runs them | agents | medium | done |
| [0158](0158-lookup-epoch-per-name.md) | A new name in a scope invalidates every cached lookup | perf | low | done |
| [0159](0159-dictsort-pair-subscript.md) | The pairs of dictsort read as 2-tuples | parity | low | done |
| [0160](0160-robin-hood-per-render-tables.md) | Per-render robin_hood tables the bench used to hide | perf | medium | done |
| [0161](0161-lexer-leniencies.md) | The tag lexer accepts two inputs Jinja2 rejects | parity | low | done |
| [0162](0162-ci-compile-cache-and-queue.md) | CI got slow: no compile cache, serial MSVC builds, macOS queue | ci | high | in-progress |
| [0163](0163-external-deps-package.md) | The external deps modes do not configure, and no CI job builds them | build | low | done |
| [0164](0164-tiny-template-fixed-costs.md) | Tiny templates: the last places another engine is ahead | perf | low | open |
| [0165](0165-macro-bodies-run-where-defined.md) | Macro and call bodies run where they were defined | parity | high | done |
| [0166](0166-macro-defaults-per-call.md) | Macro defaults evaluated per call | parity | medium | done |
| [0167](0167-imported-modules-own-their-context.md) | Imported modules own their context; imported macros run in their module | parity | medium | done |
| [0168](0168-idtracking-frame-entry-binding.md) | Bind a name the frame assigns at frame entry (Jinja2 idtracking) | parity | low | done |
| [0169](0169-caller-without-call-block.md) | `caller()` without a call block raises | parity | low | done |
| [0170](0170-dictsort-pair-tuple-semantics.md) | A dictsort pair still differs from a tuple in `in`, `==`, tests and truthiness | parity | low | done |
| [0171](0171-gcc-sanitizer-stringop-overflow.md) | The GCC address+undefined build fails in `OutStream::WriteInt` | build | medium | done |
| [0172](0172-value-unsigned-overflow.md) | Unsigned values above `INT64_MAX` wrap to negative | robustness | high | done |
| [0173](0173-pprint-python-repr.md) | `pprint` prints JSON-style scalars | parity | low | done |
| [0174](0174-msvc-c4702-suppression.md) | `/wd4702` hides unreachable code in project code on MSVC | build | low | open |
| [0175](0175-recursive-loop-filter-leftovers.md) | Recursive loop filter: the outer `loop` at depth > 0, and the legacy `recursive if` order | parity | low | done |
| [0176](0176-tuple-identity-and-sort-case.md) | A dictsort pair has no identity, and sort folds case inside tuples | parity | low | done |
| [0177](0177-filters-on-none.md) | Filters that accept None where Python raises | parity | low | done |
| [0178](0178-arbitrary-precision-integers.md) | Integers of arbitrary size, as Python's `int` | parity | medium | done |
| [0179](0179-caller-markup-eval-context.md) | Autoescape has a lexical and a runtime side | parity | low | done |
| [0180](0180-markup-from-cpp.md) | Markup from C++ | api | low | done |
| [0181](0181-select-autoescape.md) | Autoescape chosen by template name (`select_autoescape`) | api | low | done |
| [0182](0182-markup-percent-repr.md) | `%r` and containers in Markup's `%` | parity | low | done |
| [0183](0183-context-callable-positional-arg.md) | A user callable taking `*context` gets None for a positional argument | api | high | done |
| [0184](0184-gcc13-sanitizer-stringop-overflow.md) | The GCC 13 sanitizer build fails with -Wstringop-overflow | build | low | open |
| [0185](0185-fuzz-oom-below-sequence-cap.md) | The fuzz job fails on lists below the sequence cap | robustness | medium | done |
| [0186](0186-python-oracle-target.md) | Which Python and Jinja2 the parity oracle targets, and how users choose | parity | medium | open |
| [0187](0187-generated-data-acquisition.md) | How generated tables and third-party data are acquired (SBOM) | build | low | in-progress |
| [0188](0188-stored-loop-cycle-follows-loop.md) | A stored `loop.cycle` keeps the position it was read at | parity | low | done |
| [0189](0189-casefold-non-ascii.md) | Case-insensitive comparison folds only ASCII | parity | low | done |
| [0190](0190-sameas-arity-unique-unhashable.md) | `sameas` takes any number of arguments, `unique` takes unhashable items | parity | low | done |
| [0191](0191-objects-still-act-as-mappings.md) | namespace, cycler and joiner still act as mappings in a few places | parity | low | done |
| [0192](0192-namespace-self-reference-leak.md) | A namespace that holds itself leaks | robustness | low | dropped |
| [0193](0193-unknown-filter-at-load.md) | An unknown filter is a load error | parity | low | done |
| [0194](0194-self-block-runs-in-its-template.md) | A block called through `self` runs in its own template | parity | low | done |
| [0195](0195-imported-module-that-extends.md) | An imported module that extends a parent | parity | low | done |
| [0196](0196-loop-len-and-iter.md) | `loop` as a sized iterable | parity | low | done |
| [0197](0197-objects-left-as-mappings.md) | Objects and dicts still read as maps of names in a few places | parity | low | done |
| [0198](0198-self-block-call-leftovers.md) | Calls through `self` that still differ from Jinja2 | parity | low | done |
| [0199](0199-call-block-leftovers.md) | Call block leftovers | parity | low | done |
| [0200](0200-tester-arity.md) | Every built-in test checks its arguments | parity | low | done |
| [0201](0201-block-reads-root-binding.md) | A block reads the context, not the root's entry binding | parity | low | done |
| [0202](0202-loop-else-body-frame.md) | A loop's `else` body is a frame of its own | parity | low | done |
| [0203](0203-output-after-extends-not-evaluated.md) | Output after a top-level extends is not evaluated | parity | low | done |
| [0204](0204-deep-value-check-render-cost.md) | Render cost of the deep-value stack checks and trashcan release (0093) | perf | low | open |
| [0205](0205-load-drift-0168-0193.md) | Load drift after trains 11 and 12 | perf | medium | done |
| [0206](0206-call-argument-leftovers.md) | Argument checks that 0200 left over | parity | low | done |
| [0207](0207-self-frame-leftovers.md) | `self` cases left after 0143 and 0198 | parity | low | done |
| [0208](0208-macro-value-gaps.md) | Macros as values | parity | low | done |
| [0209](0209-include-sees-loop.md) | A template included in a loop sees `loop` | parity | low | done |
| [0210](0210-markup-percent-int-of-string.md) | `%d` and `%f` of a string in Markup's `%` | parity | low | done |
| [0211](0211-self-nested-store-and-globals.md) | `self` stored in a nested scope of a block; globals changed in place | parity | low | done |
| [0212](0212-objects-as-keys-and-through-callables.md) | Objects as dict keys and through user callables | parity | low | open |
| [0213](0213-if-condexpr-and-unclosed-raw.md) | An `if` test with a conditional expression, and an unclosed raw block at the end | parity | low | done |
| [0214](0214-macro-value-leftovers.md) | Macros and methods as values: what 0208 and 0145 left over | parity | low | done |
| [0215](0215-pprint-width.md) | `pprint` does not wrap at Python's width, and other repr gaps | parity | low | done |
| [0216](0216-volatile-autoescape.md) | `{% autoescape %}` with a runtime value (volatile EvalContext) | parity | low | done |
| [0219](0219-loop-iteration.md) | Iterating `loop` takes the rest of the loop | parity | low | done |
| [0220](0220-call-argument-unpacking.md) | `f(*args, **kwargs)` in calls | parity | medium | done |
| [0221](0221-release-acceptance-and-rc-signoff.md) | Define 2.0 release gates and release-candidate signoff | release | high | in-progress |
| [0222](0222-downstream-1x-migration-trial.md) | Prove a downstream upgrade from 1.3.2 to 2.0 | release | high | open |
| [0223](0223-embedding-lifecycle-integration-tests.md) | Test embedding behavior across lifetime, cache and callback boundaries | robustness | high | open |
| [0224](0224-template-trust-and-loader-boundary.md) | Define the template trust model and filesystem-loader boundary | robustness | high | done |
| [0225](0225-release-artifact-consumer-gate.md) | Validate the release source artifact from a clean downstream consumer | release | high | open |
| [0226](0226-license-metadata-and-distribution.md) | Reconcile package license metadata and ship required notices | release | high | open |
| [0227](0227-supported-docs-and-example-surface.md) | Choose and verify the documentation and examples supported for 2.0 | release | high | open |
| [0228](0228-task-number-uniqueness.md) | Two task files can share a number | agents | medium | done |
| [0229](0229-unused-boost-components.md) | Drop the Boost libraries and includes nothing uses | build | low | done |
| [0230](0230-boost-free-core.md) | A Boost-free core | build | medium | done |
| [0231](0231-robin-hood-successor.md) | robin_hood's upstream is archived | perf | high | open |
| [0232](0232-dependency-pin-drift.md) | Dependency pins disagree across modes and some are mutable | build | low | done |
| [0233](0233-cpp-only-filter-stubs.md) | C++-only filters `camelize`, `underscorize`, `escapecpp`, `toxml`, `toyaml` do nothing | api | medium | done |
| [0234](0234-github-tracker-cleanup.md) | Close GitHub issues that master already fixes, before 2.0 | release | medium | open |
| [0235](0235-parity-feature-rows-drift.md) | `docs/parity.md` feature rows contradict master | parity | medium | open |
| [0236](0236-meta-find-undeclared-variables.md) | Template introspection: `meta.find_undeclared_variables`, `Template.module` | api | low | done |
| [0237](0237-python-bindings.md) | Python bindings, so Jinja2's own test suite can run against Jinja2C++ | parity | low | open |
| [0238](0238-msvc-static-debug-info.md) | Installed MSVC static Debug library has no usable debug information | build | low | open |
| [0239](0239-site-build-and-deploy.md) | Website: pinned build, CI deploy and link check | docs | high | open |
| [0240](0240-site-versioned-docs.md) | Website: 1.x and 2.x documentation side by side | docs | high | open |
| [0241](0241-api-reference-in-ci.md) | API reference generated in CI from the library headers | docs | high | open |
| [0242](0242-site-content-refresh.md) | Website: 2.0 content refresh and compiled snippets | docs | high | open |
| [0243](0243-site-reference-pages.md) | Website: reference pages for 2.0 features | docs | medium | open |
| [0244](0244-wasm-playground.md) | A browser playground built with WebAssembly | docs | low | open |
| [0245](0245-bazel-build.md) | Bazel build and Bazel Central Registry module | release | medium | open |
| [0246](0246-public-comparison-report.md) | A user-facing benchmark and feature-parity comparison with rival engines | docs | high | open |
| [0247](0247-rival-benchmark-coverage.md) | Cover the rival engines' own benchmark cases, then close the gaps | perf | high | open |
| [0248](0248-python-drop-in-replacement.md) | Jinja2C++ as a drop-in replacement for Python Jinja2 in common cases | api | medium | open |
| [0249](0249-implicit-conversion-sanitizer.md) | Run the sanitizer CI with `-fsanitize=implicit-conversion` | robustness | medium | open |
| [0250](0250-public-api-error-contract.md) | One error contract for the 2.0 public API | api | high | done |
| [0251](0251-cycles-through-opaque-objects.md) | Reference cycles through objects the cycle collector cannot follow leak | evaluator | low | done |
| [0252](0252-public-api-allocator-contract.md) | Allocator and memory contract for the 2.0 public API | api | high | done |
| [0253](0253-error-contract-types.md) | Error contract P1: public error types and the Load/Render boundary | api | high | done |
| [0254](0254-render-error-locations.md) | Error contract P2: render errors carry their template location | robustness | high | done |
| [0255](0255-engine-runtime-error-codes.md) | Error contract P3: the engine's own runtime errors get their own codes | robustness | medium | done |
| [0256](0256-survive-allocation-failure.md) | Survive allocation failure in Load and Render: remaining coverage | robustness | high | open |
| [0257](0257-render-to-string-and-sink.md) | `RenderTo` a caller's string or an output sink | api | high | done |
| [0258](0258-positional-and-keyword-same-arg.md) | A call that gives one argument both positionally and by keyword is accepted | api | low | done |
| [0259](0259-vendored-internal-namespaces.md) | Vendored robin_hood and lexertk still live in global namespaces | release | medium | open |
| [0260](0260-public-headers-iosfwd.md) | Public headers include `<iostream>` where `<iosfwd>` would do | release | low | open |
| [0261](0261-windows-package-link-check.md) | Run the installed-package test and the ABI link probe on Windows | ci | low | open |
| [0262](0262-settings-limits.md) | `Settings::Limits`: one place for every limit a template can hit | api | high | done |
| [0263](0263-per-operation-size-checks.md) | Every growth operation checks its size before allocating | robustness | high | open |
| [0264](0264-template-cache-bound.md) | The template cache ignores its size, and `MemoryFileSystem` writes in a const path | robustness | medium | open |
| [0265](0265-hardening-by-default.md) | Build hardening on by default; invariant checks that fail the render, not the process | build | medium | open |
| [0266](0266-confined-filesystem.md) | `ConfinedFileSystem`: a loader that never leaves its root | api | medium | done |
| [0267](0267-untrusted-mode-fuzzing.md) | Fuzz targets for templates written by untrusted users | robustness | high | open |
| [0268](0268-untrusted-mode-followups.md) | Untrusted mode: additive follow-ups after the freeze | api | low | open |
| [0269](0269-public-big-int-value.md) | Public `BigInt` value (0178 phase 1, G1) | api | high | done |
| [0270](0270-big-int-core-text-form.md) | Big integers in the core, text form (0178 phase 2) | parity | medium | done |
| [0271](0271-big-int-backend-interface.md) | Big-integer backend interface and overflow promotion (0178 phase 3) | api | medium | done |
| [0272](0272-big-int-boost-backend.md) | Boost.Multiprecision backend and `JINJA2CPP_WITH_BIGINT` (0178 phase 4) | build | medium | done |
| [0273](0273-build-conditional-parity-divergences.md) | Build-conditional parity divergences (0178 phase 0) | parity | medium | done |
| [0274](0274-self-block-before-parent-pushed.md) | Blocks called through `self` at a child's top level lack the parent's blocks | parity | low | done |
| [0275](0275-release-thread-caches.md) | `ReleaseThreadCaches()` | api | medium | done |
| [0276](0276-memory-contract-docs.md) | User documentation for the memory contract | docs | medium | open |
| [0277](0277-markup-escape-helper.md) | A public escape helper for Markup | api | low | done |
| [0278](0278-remove-dead-valueref.md) | Remove the dead ValueRef alternative of InternalValue | internals | low | open |
| [0279](0279-nlohmann-destructor-allocates.md) | nlohmann::json destruction allocates, so OOM there terminates | robustness | low | open |
| [0280](0280-urlize-markup-input.md) | urlize escapes Markup input | filters | low | done |
| [0281](0281-attr-bound-method.md) | attr() of a method renders empty | filters | low | done |
| [0282](0282-settings-limits-followups.md) | `Settings::Limits` follow-ups (0262 leftovers) | api | low | open |
| [0283](0283-cycle-collector-oom-leaks.md) | Reference cycles leak when an allocation fails during a render | robustness | low | done |
| [0284](0284-template-module.md) | `Template.module` / `make_module`: a template's exports as a map | api | low | done |
| [0285](0285-abi-baselines-at-rc.md) | Commit the ABI baselines at the 2.0 RC and turn the abidiff gate on | release | high | open |
| [0286](0286-fixed-cost-drift-oct-12-19.md) | Fixed-cost drift from feature merges, 2026-10-12 to 2026-10-19 | perf | medium | done |
| [0287](0287-big-int-radix-conversion-cost.md) | Quadratic radix conversion of big integers near the bit cap | robustness | low | open |
| [0288](0288-finalize-autoescape.md) | `finalize` output is not escaped under autoescape | parity | high | done |
| [0289](0289-flat-concat-chains.md) | `~` chains as one node, as Jinja2's `Concat` | parity | low | done |
| [0290](0290-codeql-test-suite-cost.md) | Find what in the test suite makes CodeQL's data-flow stage explode | ci | low | open |
| [0291](0291-fold-chosen-by-foldable-test.md) | Fold a value chosen by a foldable test that is not a literal | parity | low | done |
| [0292](0292-finalize-fold-error-fallback.md) | Evaluate at run time when folding a finalized `{{ }}` raises | parity | low | done |
| [0293](0293-finalize-gets-undefined.md) | `finalize` gets None for an undefined value | parity | medium | done |
| [0294](0294-positional-after-keyword-syntax-error.md) | A positional argument after a keyword one is a syntax error | parity | low | done |
| [0295](0295-selectattr-extra-keyword.md) | `selectattr('a', attribute='a')` fails where Python ignores the keyword | parity | low | done |
| [0296](0296-typed-string-view-dangling.md) | A typed `std::string_view` parameter given the other character width dangles | api | high | done |
| [0297](0297-in-none-and-modulo-undefined.md) | `x in None` and `'text' % undefined` differ from Python | parity | low | open |
| [0298](0298-include-of-extending-template-loop.md) | An included template that extends a parent reading `loop` | parity | low | done |
| [0299](0299-module-exports-skip-imports.md) | A module exports the names its top level imports, and keeps a re-set import hidden | parity | low | open |
| [0300](0300-module-export-callable-gaps.md) | Callables that do not cross a `Template::MakeModule` export | parity | low | open |
| [0301](0301-module-call-loads-accumulate.md) | A `Template::MakeModule` module keeps what each of its calls loads | robustness | medium | done |
| [0302](0302-simdjson-binding.md) | An optional simdjson binding | api | low | open |
| [0303](0303-retire-rapidjson-backend.md) | Retire the built-in RapidJSON backend | build | medium | done |
| [0304](0304-loop-over-loop-peeks-early.md) | A for loop over `loop` advances the outer loop early | parity | low | open |
| [0305](0305-builtin-class-repr.md) | The builtin classes print as `<class 'range'>` | parity | low | open |
| [0306](0306-filter-test-argument-unpacking.md) | `*args` and `**kwargs` in filter and test calls | parity | low | open |
| [0307](0307-fold-by-choice-nested.md) | Fold a choice that sits inside another foldable expression | parity | low | open |
| [0308](0308-divisibleby-non-number.md) | `divisibleby` on a value that is not a number | parity | low | open |
| [0309](0309-repr-of-template-modules.md) | `pprint` of builtin globals and imported templates | parity | low | open |
| [0310](0310-undefined-filter-argument-is-omitted.md) | A filter argument given as an undefined value reads as omitted | parity | low | open |
| [0311](0311-json-bindings-as-components.md) | JSON bindings as components on a public adapter contract | build | high | done |
| [0312](0312-module-kept-while-stored-macro-reaches-it.md) | A macro a module call stores keeps its module, as in Jinja2 | parity | low | open |
| [0313](0313-module-keeps-every-use-reloads.md) | A module keeps every tree `TemplateLookup::EveryUse` reloads | robustness | low | open |
| [0314](0314-method-sort-format-unicode-case.md) | Methods 0020 left out: list.sort, nested format fields, Unicode case | parity | low | done |
| [0317](0317-str-numeric-classes-and-missing-methods.md) | str methods: Unicode digit and numeric classes, casefold, isascii, isidentifier, isprintable, format leftovers | parity | low | open |
| [0318](0318-kept-globals-keep-their-snapshot.md) | A list or map from the globals keeps the globals it was read from | robustness | high | done |
| [0319](0319-makecallable-value-parameter.md) | MakeCallable with a Value parameter does not compile | api | medium | done |
| [0320](0320-typed-callable-omitted-argument.md) | A typed callable with an omitted optional argument is not called | api | low | done |
| [0321](0321-load-copies-on-write.md) | Load replaces the template for its own object only | robustness | high | done |
| [0322](0322-parity-corpus-through-reflect.md) | Run the parity corpus with the context as a JSON view | parity | low | open |
| [0323](0323-stored-self-sees-includer-names.md) | A `self` an include stored does not see the names the includer set | parity | low | open |
| [0324](0324-nested-render-sees-global-changed-in-place.md) | A nested render does not see a global the outer render changed in place | robustness | medium | open |
| [0325](0325-autoreload-date-before-read.md) | A reload can cache the new date with the old text | robustness | low | open |
| [0326](0326-nested-macro-closure-after-outer-call.md) | A macro defined in a macro loses the outer call's names after that call returned | parity | medium | open |
| [0327](0327-change-globals-use-count-race.md) | Changing a global can race with a render that just dropped the globals map | robustness | medium | open |
| [0328](0328-bench-meta-block.md) | A benchmark for `{% meta %}` and the internal JSON reader | perf | low | open |
| [0329](0329-reflect-counts-in-ci.md) | Count `--data reflect` in the PR gate and the trend | perf | medium | open |
| [0330](0330-gate-reports-newly-outlined-frames.md) | The PR gate names hot functions that GCC stopped inlining | perf | low | open |
| [0331](0331-kept-container-contract.md) | What a host callable keeps of a container the render hands it | parity | medium | done |
| [0332](0332-module-cell-in-kept-container.md) | A module's live cell read through a container the host keeps | robustness | low | done |
| [0333](0333-locale-free-float-parsing.md) | Float parsing follows the C locale | robustness | high | done |
| [0334](0334-makecallable-per-argument-dispatch.md) | MakeCallable visits every combination of argument types | api | high | done |
| [0335](0335-late-export-in-a-later-module-call.md) | A module call that hands out a container before it writes it keeps the earlier call's state | parity | low | done |
| [0336](0336-module-cells-of-per-call-data.md) | A template module keeps the cells of generic containers made for one call | robustness | medium | done |
| [0337](0337-nested-module-export-written-by-outer-call.md) | A nested module call's kept value is written by the outer call | robustness | medium | done |
| [0338](0338-borrowed-cell-protocol-safety-proof.md) | From a bounded model of the borrowed-cell protocol to a safety argument | robustness | medium | done |
| [0339](0339-shared-ptr-uniqueness-needs-acquire.md) | A uniqueness test on a `shared_ptr` count has no acquire | robustness | low | done |
| [0340](0340-foreign-unlink-of-a-live-cell.md) | A host thread dropping a kept container unlinks an adapter from a live cell | robustness | medium | done |
| [0341](0341-call-id-wraparound.md) | A module call id wraps and a stale export mark equals it | robustness | low | done |
| [0342](0342-epoch-wraparound.md) | The 32-bit cell epoch wraps and an old adapter compares equal | robustness | low | done |
| [0343](0343-clang-lifetime-and-lock-annotations.md) | Check lifetimes and lock discipline at compile time with Clang annotations | robustness | medium | done |
| [0344](0344-debug-containers-test-job.md) | A test job with a checking standard library (iterator and reference invalidation) | ci | medium | done |
| [0345](0345-latest-llvm-analysis-job.md) | A report-only job with the latest LLVM for trunk lifetime and cross-TU analyses | ci | low | done |
| [0346](0346-global-item-link-dropped-on-another-thread.md) | A global's item adapter in a block the global let go of, dropped on another thread | robustness | high | done |
| [0347](0347-module-cells-in-lazy-and-namespace-exports.md) | Module cells reached through a lazy list or a namespace are not pinned | robustness | medium | open |
| [0348](0348-generic-list-shared-accessor.md) | GenericList and GenericMap hold their accessor by a shared pointer | api | medium | done |
| [0349](0349-ctu-analyzer-on-newest-llvm.md) | Run the cross-TU static analyzer on the newest LLVM and on libstdc++ | ci | low | open |
| [0350](0350-tla-runs-in-ci.md) | Run the long TLC checks of the cell model in GitHub Actions, and halve the two-thread runs | ci | medium | in-progress |
| [0351](0351-cell-model-past-the-depth-bound.md) | Check the cell model past its depth bound | robustness | medium | open |
| [0352](0352-python-host-object-seam.md) | Python module P1a: a private host-object seam in the engine | api | medium | done |
| [0353](0353-python-module-core.md) | Python module P1b: build, Environment/Template/render, value crossing | api | medium | open |
| [0354](0354-python-module-subset.md) | Python module P2: the supported subset, sandbox and fallback | api | medium | open |
| [0355](0355-python-module-proof.md) | Python module P3: upstream test suite, parity corpus, chat templates, bench row | parity | medium | open |
| [0356](0356-python-module-wheels.md) | Python module P4: wheels for Linux, macOS and Windows; free-threaded audit | build | medium | open |
| [0357](0357-python-module-public-api.md) | Python module: move from the private seam to the public API before the G1 freeze | api | medium | open |
| [0358](0358-prompt-autoescape.md) | Prompt autoescape: `fence`, `neutralize` and an escaping mode for LLM prompts | api | medium | open |
| [0359](0359-token-budget-filters.md) | Token budget filters: `count_tokens`, `truncate_tokens` and `pack` | api | medium | open |
| [0360](0360-hf-chat-template-compat.md) | Render Hugging Face chat templates as transformers does | parity | high | open |
| [0361](0361-failed-call-top-view-reads-empty.md) | A failed module call's view of a module cell reads no items | lifetime | low | open |
| [0362](0362-host-object-python-errors.md) | Host objects: Python's errors where generic filters guess | parity | low | open |
| [0363](0363-cell-protocol-inductive-proof.md) | Prove the cell protocol's invariants inductively | robustness | low | open |
| [0364](0364-frozen-version-deep-validation.md) | Validate a frozen version deeply, from time to time | robustness | medium | open |
| [0365](0365-fresh-generic-items-of-a-call-argument.md) | A fresh generic container nested in a call's argument keeps its module cell | robustness | low | open |
