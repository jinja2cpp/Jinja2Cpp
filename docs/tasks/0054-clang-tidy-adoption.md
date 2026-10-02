---
status: open
priority: medium
area: style
depends: []
touches: [.clang-tidy, test/.clang-tidy, .gitignore, .clang-format, include/jinja2cpp/config.h, scripts/null_compare.query, .github/workflows/clang-tidy.yml, scripts/clang_tidy_fix.py, CLAUDE.md]
---
# clang-tidy: adopt the latest checks and modernize the code in batches

**Problem.** The code base has a formatter gate (`.clang-format`, task 0009) but no static
analysis beyond compiler warnings and CodeQL. clang-tidy 22 has 605 checks; switched on
all at once they report 20.6k hits on `src/` and `include/`, most of them from rule sets
written for other code bases (LLVM libc, Fuchsia, Altera FPGA). A useful configuration
must keep the checks that find bugs or remove real noise from the code, drop the ones
that only restate another project's style, and still be cheap enough that new code is
held to it while the old code is brought up to it batch by batch, without freezing the
parity work that edits the same files.

**Proposal.** Commit the measured `.clang-tidy` (below), gate pull requests on the lines
they change, and clean the existing code in batches of one check family each, applied
with `--export-fixes` + `clang-apply-replacements` and proven by the full CI matrix.

## Measurements

clang-tidy 22.1.8 (latest release, `pip install clang-tidy==22.1.8`), master b0991e3,
C++14 GCC compile database, 22 library TUs plus the headers they include, each
diagnostic counted once per source location (headers are seen from many TUs, and the
`src/binding/../x.h` spelling is folded into `src/x.h`).

| Run | Hits | Wall time (4 cores) |
|---|---:|---:|
| `Checks: '*'` on `src/` + `include/` | 20 616 (178 checks fire) | ~9 min |
| same minus foreign rule sets (llvmlibc, fuchsia, altera, google, hicpp, llvm, abseil, cert, boost) | 7 889 | |
| proposed `.clang-tidy` on `src/` + `include/` | 812 | ~9 min (3 jobs) |
| proposed `test/.clang-tidy` on `test/` (26 TUs), hits in test files | ~110 | ~6 min (3 jobs) |
| `Checks: '*'` at C++20 (what C++14 hides) | +2 340 designated-init, +465 nodiscard, +40 use-ranges, +32 use-constraints, +15 unchecked-optional-access, +13 integer-sign-comparison, +10 concat-nested-namespaces | |

By group, all checks on: llvmlibc 6 523, readability 3 324, modernize 2 136, hicpp 2 096,
google 1 678, fuchsia 1 610, misc 1 226, cppcoreguidelines 1 007, altera 531, llvm 225,
performance 125, bugprone 59, boost 40, cert 18, abseil 6, concurrency 4, portability 4,
clang-analyzer 4.

## Groups

| Group | Decision | Why |
|---|---|---|
| `bugprone-*` | on (minus `easily-swappable-parameters`) | Bug finders, 59 hits, low noise. Swappable parameters: 24 hits, all deliberate (`lhs, rhs`), not fixable without API churn. |
| `clang-analyzer-*` | on | Path-sensitive analysis; found the latent dangling reference in task 0055. |
| `concurrency-*` | on (minus `mt-unsafe`) | `mt-unsafe` flags `wcsrtombs`/`mbsrtowcs` by name; the code passes its own `mbstate_t`, so all 4 hits are false. |
| `cppcoreguidelines-*` | on, with 20 exclusions | Keep member init, special members, missing `std::forward`, rvalue params, slicing, virtual dtor. Drop the bounds/cast profile (461 hits on `operator[]`, needs GSL), magic numbers, `do while`, const/ref members, and every alias of a check enabled elsewhere. |
| `google-explicit-constructor` | on | The only google check worth having. 24 of its hits are in `include/`, where `Value`'s converting constructors are implicit by design: those get `// NOLINT(google-explicit-constructor)`, not `explicit`. |
| `misc-*` | on, minus `include-cleaner`, `no-recursion`, `multiple-inheritance`, `non-private-member-variables`, `use-internal-linkage` | `no-recursion` is the parser and visitor design. `include-cleaner` (697 hits) is worth doing, but as its own batch with a mapping for the nonstd and Boost umbrella headers. `use-internal-linkage` conflicts with templates instantiated across TUs; `use-anonymous-namespace` stays on. |
| `modernize-*` | on, minus `use-trailing-return-type`, `return-braced-init-list`, `avoid-c-arrays`, `use-designated-initializers` | Trailing return types (1 809 hits) and braced returns (172) are a style flip with no gain. C arrays are static tables. Designated initializers need C++20. |
| `performance-*` | on, minus `enum-size` | `unnecessary-value-param` (65) is real: `FilterParams` is copied on every filter call. `enum-size` changes the ABI of public enums. |
| `portability-*` | on | 4 hits, virtual members of class templates. |
| `readability-*` | on, with 9 exclusions | Excluded: `identifier-length` (564), `magic-numbers` (270), `named-parameter` (188), nested `?:` (13), `redundant-casting` (int64_t is `long` here and `long long` on MSVC, so a cast that is redundant on Linux is not on Windows), `braces-around-statements` (enforced by clang-format instead, below) and `identifier-naming` pending a decision. `implicit-bool-conversion` allows pointer and integer conditions. `function-cognitive-complexity` is on at 25 (below). |
| `abseil`, `altera`, `android`, `boost`, `darwin`, `fuchsia`, `google` (rest), `hicpp`, `linuxkernel`, `llvm`, `llvmlibc`, `mpi`, `objc`, `openmp`, `zircon` | off | Other projects' rules or aliases of enabled checks. `boost-use-ranges` (40) proposes Boost.Range rewrites; `llvm-header-guard` wants LLVM's guard names. `cert-*` are aliases. |

Decided (Ruslan, 2026-10-02):

1. **Braces are required** around every single-statement body. The tool is clang-format's
   `InsertBraces: true` rather than the tidy check: the format gate already checks
   `src/` and `include/` whole, so enforcement is immediate and needs no tidy run.
   With the current `BraceWrapping` (braces on their own lines) it adds 3 300 lines to
   `src/` + `include/` (34k). The braces batch sets the option and reformats in one
   mechanical commit, like 0009; build and tests prove it (clang-format documents
   `InsertBraces` as able to miscompile around macros).
2. **Implicit pointer-to-bool in conditions, enforced.** `implicit-bool-conversion` with
   `AllowPointerConditions`/`AllowIntegerConditions` permits `if (ptr)` but does not flag
   `if (ptr != nullptr)`, and no clang-tidy check does. The enforcement is a clang-query
   matcher (`scripts/null_compare.query`): an `==`/`!=` against `nullptr`, raw or smart
   pointer, whose parent is an `if`/`while`/`for` condition, `!`, `&&`, `||` or `?:`.
   It finds 111 sites today (119 comparisons in total; the other 8 are `return p !=
   nullptr;` and the like, where the explicit form stays, since `return p;` is the
   implicit conversion the tidy check rejects). The CI job runs it on changed files; the
   one-off rewrite (`p != nullptr` to `p`, `p == nullptr` to `!p`, parenthesised where
   needed) is part of batch 2. If clang-query output proves too coarse, the same matcher
   becomes a small clang-tidy plugin check with fix-its (`-load`), at the cost of building
   it against the exact clang-tidy version in CI.
3. **Cognitive complexity on at 25** (upstream default). Of 743 scored functions, 94
   exceed 10, 35 exceed 25, 13 exceed 50 and 3 exceed 100 (`WordWrap` 158,
   `FormatValue` 155, the `urlize` filter 132). Adoption: new functions are held to 25 by
   the PR job (the diagnostic sits on the declaration line, so a new function is always a
   changed line); the 35 existing ones get
   `// NOLINT(readability-function-cognitive-complexity): score N, see 0054` in batch 2,
   a greppable debt list; each refactor that brings one under 25 deletes its NOLINT.
   Splitting the 13 above 50 is follow-up work, one function per PR, the parity corpus
   and unit tests being the safety net. The threshold can then step down (25, 20, 15)
   once the list is short.
4. **`[[nodiscard]]` now, through a macro.** `modernize-use-nodiscard` takes a
   `ReplacementString`, and with one it runs at C++14 too: `JINJA2CPP_NODISCARD` in
   `include/jinja2cpp/config.h`, `[[nodiscard]]` when `__cplusplus` (or `_MSVC_LANG`)
   is at least 201703L, otherwise empty. 354 sites, 83 of them in public headers. Users
   building at C++17+ will then see warnings where they drop a result, which belongs in
   the release notes.
5. `readability-identifier-naming` (open): the options in `.clang-tidy` encode the
   conventions the code already follows (`CamelCase` types and functions, `m_`/`s_`
   members and statics, `camelBack` locals). On two large TUs it reports ~190 names,
   nearly all deliberate: STL-shaped containers (`ordered_map::insert`), `*_t` aliases
   and the public `Value::asString`. Proposed: switch on for `src/` after a batch that
   adds the remaining ignore patterns; never rename public API.
6. Version: pin clang-tidy 22.1.8 from PyPI in CI (Ubuntu 24.04 ships 18, which lacks
   about 40 of the checks counted here). `.clang-format` stays on the version it is.

## Automation

What clang-tidy proposes is applied by `clang-apply-replacements`; `run-clang-tidy -fix`
wraps both (each TU writes `--export-fixes` YAML, then the tool merges identical edits
that several TUs made to a shared header and applies them, `-format` reformatting the
touched lines with `.clang-format`). A trial on this repo with five checks
(`modernize-use-override`, `readability-qualified-auto`, `modernize-use-equals-default`,
`performance-move-const-arg`, `modernize-use-emplace`) found two traps and then worked:

- **Unfiltered fixes rewrite dependencies.** Without a header filter, `-fix` edited
  Boost, fmt, variant-lite and expected-lite in the FetchContent cache and broke the build
  (`polymorphic_allocator(...) : = default;`). The proposed config sets
  `HeaderFilterRegex`/`ExcludeHeaderFilterRegex`; a later step can mark dependency
  include directories `SYSTEM`.
- **One header under two spellings gets the fix twice.** `src/binding/*.cpp` include
  `"../internal_value.h"`, so the same header appears as `src/binding/../internal_value.h`
  and `src/internal_value.h`; deduplication is by path, and the result was
  `override override` and `const const auto*`. Normalising paths in the exported YAML
  (or including through the `src/` include directory) fixes it.

With both handled, the trial changed 38 files (+321 −328), built warning-free, passed
`ctest`, and left `src/`/`include/` clang-format clean. The steps go into
`scripts/clang_tidy_fix.py`:

```
scripts/clang_tidy_fix.py --checks 'modernize-use-override' [--paths src/filters.cpp]
  1. clang-tidy -p build --checks=-*,<checks> --export-fixes=<tmp>/<tu>.yaml  (per TU, -j N)
  2. rewrite every FilePath in the YAML to os.path.normpath and drop files outside the repo
  3. clang-apply-replacements -format -style=file <tmp>
  4. git clang-format --diff origin/master   (should be empty)
```

Each batch PR then runs the verifier role: Debug build and tests, the sanitizer build,
and the full CI matrix (MSVC sees `#ifdef _MSC_VER` code the Linux run never parsed).

CI gating, in `.github/workflows/clang-tidy.yml`:

- **Pull requests, changed lines only.** `git diff -U0 $BASE | clang-tidy-diff.py -p1
  -path build -j 4 -quiet` reports only diagnostics on lines the PR touches, so an old
  hit does not block a parity PR that happens to edit the line next to it. Diagnostics
  appear as annotations; the job fails only for checks in `WarningsAsErrors`, which
  lists each check once a batch has cleaned it from the whole tree (a ratchet).
- **Whole tree, weekly and on `.clang-tidy` changes.** Keeps the ratchet honest and shows
  the remaining count per check.
- **C++17 advisory job.** `bugprone-unchecked-optional-access` only understands
  `std::optional`, which nonstd maps to at C++17; it found 15 unchecked accesses that the
  C++14 run cannot see. Advisory until it is clean.
- Editors pick up `.clang-tidy` through clangd with no extra setup.

## Batches

One PR per batch, each a single concern so a reviewer reads a uniform diff. Batches touch
many files, so each one runs when no parity wave is open and merges before the next starts.

0. Config, `test/.clang-tidy`, `scripts/clang_tidy_fix.py`, the CI workflow in report-only
   mode, and a `CLAUDE.md` paragraph. No code changes.
1. Bug-class findings fixed by hand: task 0055.
2. Mechanical, behaviour-neutral fixes: `modernize-use-override`, `use-equals-default`,
   `use-emplace`, `use-using`, `use-bool-literals`, `type-traits`, `readability-qualified-auto`,
   `redundant-access-specifiers`, `container-contains`, `container-data-pointer`,
   `redundant-string-cstr`, `else-after-return`, `simplify-boolean-expr`,
   `isolate-declaration`, `math-missing-parentheses`, `uppercase-literal-suffix` (~380 hits);
   the nullptr-comparison rewrite (111); `JINJA2CPP_NODISCARD` and `modernize-use-nodiscard`
   (354); NOLINT markers on the 35 over-complex functions.
2b. Braces: `InsertBraces: true` in `.clang-format` and the whole-tree reformat (+3 300 lines).
3. Fixes that change signatures or copies, reviewed by hand: `performance-unnecessary-value-param`,
   `modernize-pass-by-value`, `performance-move-const-arg`, `unnecessary-copy-initialization`,
   `noexcept-move-constructor`, `cppcoreguidelines-missing-std-forward`,
   `rvalue-reference-param-not-moved`, `prefer-member-initializer`,
   `readability-convert-member-functions-to-static`, `misc-use-anonymous-namespace`,
   `google-explicit-constructor` (NOLINT on the public converting constructors),
   `cppcoreguidelines-special-member-functions`, `readability-implicit-bool-conversion`.
4. `test/` under `test/.clang-tidy` (only lines CI already checks, per 0009).
5. Optional: identifier naming, include-cleaner.
6. With the next standard bump (task 0007):
   `concat-nested-namespaces`, `use-integer-sign-comparison`, `use-starts-ends-with`,
   `use-ranges`, `use-constraints`, and `unchecked-optional-access` gating.

After each batch its checks move into `WarningsAsErrors`. **Done when** the whole-tree
job reports zero hits with every check enabled in `.clang-tidy` listed in
`WarningsAsErrors`, and the PR job fails on a new violation.

**Next.** Once the tree is clean, a check whose hits are all false positives shows up as
a stream of NOLINT comments; those are the signal to disable it. Raising the minimum
standard (0007) unlocks the C++17/20 checks above, about 900 more fixes.
