---
status: open
priority: medium
area: style
depends: [0054, 0008]
touches: [src/, include/jinja2cpp/]
---
# clang-tidy: behaviour-neutral mechanical fixes

**Problem.** About 380 hits of the 0054 config are fixes that change no behaviour and
need no judgement, yet they hide the hits that do matter: `readability-qualified-auto`
(272), `modernize-use-override` (16), `use-equals-default` (17), `use-emplace` (11),
`use-using`, `use-bool-literals`, `type-traits`, `readability-redundant-access-specifiers`
(15), `container-contains`, `container-data-pointer`, `redundant-string-cstr`,
`else-after-return` (8), `simplify-boolean-expr`, `isolate-declaration`,
`math-missing-parentheses` (19), `uppercase-literal-suffix`. At the C++23 floor (0008)
the same kind adds `use-ranges` (37), `redundant-typename` (26), `type-traits` (20 more),
`use-integer-sign-comparison` (13), `concat-nested-namespaces` (7), `container-contains`
(4) and `use-starts-ends-with` (1). Runs after 0008, at C++23.

**Proposal.** One PR, one commit per check, made with `scripts/clang_tidy_fix.py` (0054):
export fixes per TU, normalise paths, `clang-apply-replacements -format`. A trial of five
of these checks changed 38 files and stayed green. Public-header edits limited to what
0056 keeps. The PR moves these checks into `WarningsAsErrors`.

**Done when** the whole-tree job reports zero hits for these checks and CI is green on
the full matrix (MSVC sees `#ifdef _MSC_VER` code the Linux run did not).
