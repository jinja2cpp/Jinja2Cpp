---
status: in-progress
priority: high
area: style
depends: []
touches: [.clang-tidy, src/error_handling.h, src/node_arena.h, src/expression_evaluator.h, src/string_converter_filter.cpp, test/json_test_suite_cases.inc]
shares: [src/, include/jinja2cpp/, test/]
---
# clang-tidy: 304 hits on master; the weekly whole-tree job has never been green

**Problem.** `.clang-tidy` makes every enabled check an error (`WarningsAsErrors: '*,-clang-diagnostic-*'`,
since 0081), and its comments promise a ratchet: the pull-request job fails any new hit, so master stays
clean. Master is not clean. All 8 scheduled runs of `.github/workflows/clang-tidy.yml` (whole tree,
Mondays, 2026-10-05 to 2026-11-23) concluded `failure` in the step "clang-tidy on the whole tree", and its
null-comparison and `use_count` steps were skipped each time. Nobody looked: no thread, routine or task
reads the scheduled result.

A local run on master 4a076e9 (clang-tidy 22.1.8, C++17 Debug, the job's own commands, GCC compile
database; hit list in project files `clang-tidy/2026-11-27-whole-tree-hits-4a076e9.txt`) finds 304 unique
locations: 207 in `src/`, 82 in `test/`, 14 in `include/`, 1 in `examples/`.

| hits | check | note |
|---:|---|---|
| 77 | modernize-raw-string-literal | all in `test/json_test_suite_cases.inc`, which the PR job excludes (`:!test/*.inc`) but the whole-tree job reaches through the test that includes it |
| 60 | misc-include-cleaner | 54 `src/`; reported on `#include` lines nobody changed |
| 39 | readability-function-cognitive-complexity | all `src/`; reported on the function's first line, not the line that pushed it past 25 (0061 had split all of them) |
| 35 | performance-move-const-arg | `std::move` of `Token`, which some change made trivially copyable: a hit on lines no PR touched |
| 23 | modernize-use-nodiscard | |
| 12 | readability-qualified-auto | |
| 8 | cppcoreguidelines-pro-type-member-init | `node_arena.h`, `template_parser.h`, `render_context.h`, `cycle_collector.h`, `template_impl.h` |
| 8 | bugprone-signed-char-misuse | `lexer.h`, `template_parser.h`, `include/jinja2cpp/big_int.h:72` |
| 7 | bugprone-exception-escape | `borrowed_cells.cpp` (2), `template_impl.h` (3), `small_vector.h`, `include/jinja2cpp/value.h:794` |
| 5 | cppcoreguidelines-virtual-class-destructor | `name_resolver.h`, `statements.h`, `render_context.h` |
| 2 each | bugprone-unhandled-self-assignment (`error_info.h:165`, `intrusive_ptr.h:68`), bugprone-branch-clone, cppcoreguidelines-special-member-functions, cppcoreguidelines-prefer-member-initializer | |
| 1 each | bugprone-use-after-move (`template_impl.h:459`, a false positive: two branches on one `const bool`), bugprone-random-generator-seed (`template_impl.h:1311`), and 12 others | |

The bug-class rows (signed-char, exception-escape, self-assignment, member-init, use-after-move) need a
look one by one; the rest are mechanical. CI uses a Clang compile database, so its counts may differ by a
few.

**Proposal.** Clean the tree in batches small enough to review, as 0057-0081 did: (1) fix-its for the
mechanical checks (`scripts/clang_tidy_fix.py --checks <check>`), with `test/json_test_suite_cases.inc`
either fixed or excluded from the whole-tree job the same way the PR job excludes it; (2) cognitive
complexity: split, or NOLINT with the score where a split hurts the hot path; (3) the bug-class rows,
each fixed or NOLINTed with the traced reason. Also drop the stale `.clang-tidy` comment that 35
functions carry a cognitive-complexity NOLINT (none do).

**Done when.** `workflow_dispatch` of `clang-tidy.yml` on master is green, including its null-comparison
and `use_count` steps.

**Next.** Without 0601 the count grows back: these hits got in through PRs that were green.

**Progress.** The first PR (this branch) clears most of the 304 hits, in every file no open PR owned at
the time, and leaves the rest in `big_int.cpp`, `borrowed_cells.cpp`, `cycle_collector.h`, `internal_value.*`,
`name_resolver.*`, `recursion_guard.*`, `render_context.h`, `serialize_filters.cpp`, `statements.cpp`, `template_impl.h`,
`template_parser.cpp`, `testers.cpp`, `undefined.cpp`, `value_methods.cpp` and `value_visitors.h`, which
batch 14 PRs rewrote at the same time. A second PR takes them once those merge. Decisions:
- `readability-function-cognitive-complexity` (21 here): NOLINT with the score, as 0061 left the rest;
  a split of a flat case dispatch would only scatter it.
- `performance-unnecessary-value-param` on `Reflector<shared_ptr<T>>::Create`: a false positive, the
  by-value parameter is moved into the view (0487).
- `bugprone-exception-escape` on `Value::GetIf`: `As` calls `DemoteMarkup` unchecked and relies on the
  `std::get` throw, so the throw cannot go; `GetIf` checks the alternative first. NOLINT with the reason.
- `bugprone-signed-char-misuse`: the lexer and parser read units through `char_class::CodeUnit`, which
  widens a narrow unit through `unsigned char`. In `BigInt(I)` a unary plus promotes first, keeping the
  sign extension the negation below needs.
