# Jinja2C++ — shared instructions for coding agents

Jinja2C++ implements Python Jinja2 in C++. Aim for maximum achievable behavioral
parity, good performance and an API that is easy to embed. These are the shared
project instructions for Codex, Claude Code and other coding agents. Claude Code
also reads [CLAUDE.md](CLAUDE.md) for its environment hooks and agent roles; other
runners must not assume those hooks, model aliases or subagents are available.

## Code Review Rules

Before reviewing a pull request, read [REVIEW.md](REVIEW.md) and apply its review
criteria; it is shared by every reviewer of this repository (Codex, Claude Code
Review, the pre-push verifier). It covers both automatic and explicitly requested
reviews. Review the proposed changes without editing files, committing fixes or
posting additional messages unless that work was requested. Review automation is
enabled separately in Codex/GitHub settings; these files provide instructions, not
review triggers, and they do not choose the reviewer's model or reasoning effort:
that belongs to the invoking runtime or product settings. The rules that matter most,
in case REVIEW.md is not read:

- Template text and data may be untrusted. Memory corruption or an autoescape bypass
  reachable from template text is P0; a reachable crash, abort, stack overflow or
  allocation that bypasses the `MaxSequenceSize` cap is P1. Flag changes that make
  loader name handling more permissive (task 0224).
- Lifetimes are this project's worst defect class (tasks 0112, 0115, 0125, 0134):
  flag any view, lent item, generator item or reflected item used after the statement,
  frame, render or reload that owned it, and escaping callables without a keep-alive.
- Python Jinja2 at the pinned version is the oracle: a rendering or rejection
  difference not on the divergence allow-list is a bug, in narrow and wide paths.
- Performance claims need `bench/count.py` instruction counts; on the per-node
  render path, a new allocation, value copy or per-element virtual call is a finding.
- Public headers stay C++17 and ABI changes need a recorded task decision.

## Build and test

Use CMake >= 3.23, Ninja and a C++17-capable compiler. C++17 is the public API floor;
C++20 and C++23 are supported, and C++26 is a forward-compatibility CI check. The
default internal dependency mode fetches pinned Boost, fmt and expected-lite, plus
JSON libraries and GoogleTest when required. Do not assume a runner has already
built the project or that network access is available or blocked; inspect its setup.

From the repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
build/jinja2cpp_tests --gtest_filter='FilterGenericTest*'
```

Reuse a valid existing build directory when possible; use separate directories for
incompatible configurations. Adjust parallelism to the runner's CPU/memory limits.
In isolated cloud tasks, use the existing checkout; do not create a worktree merely
to start a task. Follow runner-specific isolation rules when delegation requires it,
and never let concurrent builds share a mutable build directory.

Useful configurations (see `.github/workflows/` for the actual CI matrix):

| Purpose | Additional CMake arguments |
|---|---|
| Language standard | `-DJINJA2CPP_CXX_STANDARD=17`, `20`, `23` or `26` |
| Address/undefined sanitizers | `-DJINJA2CPP_WITH_SANITIZERS=address+undefined -DCMAKE_BUILD_TYPE=RelWithDebInfo` |
| JSON bindings | `-DJINJA2CPP_WITH_JSON_BINDINGS=boost`, `nlohmann` or `rapid` |
| Shared library | `-DJINJA2CPP_BUILD_SHARED=ON` |
| Node reference checks | `-DJINJA2CPP_NODEREF_CHECKS=OFF`, `ON` (Release default), `FULL` (Debug/sanitizer/fuzz default) |
| Coverage | `-DJINJA2CPP_WITH_COVERAGE=ON` (GCC/Clang, Debug) |

For current Conan dependency-mode builds, use Conan 2:

```bash
conan profile detect   # only if `conan profile list` has no default profile
conan install . -of .conan --build=missing -s compiler.cppstd=17 -s build_type=Release
cmake -S . -B build-conan -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=.conan/conan_toolchain.cmake \
  -DJINJA2CPP_DEPS_MODE=conan-build
```

Historical example repositories may use Conan 1; do not apply their commands to
the current library workflow.

Keep `JINJA2CPP_STRICT_WARNINGS=ON`; do not disable diagnostics to hide a defect.
Match any targeted compiler workaround to the documented CI configuration.

## Repository layout

- `include/jinja2cpp/`: public templates, environment, values, reflection, callables
  and JSON bindings. Public types use C++17 vocabulary types; `Result` uses pinned
  expected-lite rather than selecting a different ABI with the consumer's standard.
- `src/`: lexer, parsers, evaluator, statements, filters, tests and serializers.
  `robin_hood.h` and `lexertk.h` are vendored; generated Unicode/HTML tables have
  generators in `scripts/`.
- `test/`: GoogleTest suites and the differential corpus in `test/parity/`.
  `MULTISTR_TEST` exercises narrow and wide templates.
- `fuzz/`: fuzz targets, differential tooling and regression inputs.
- `bench/`: Google Benchmark workloads, Python reference driver and performance
  gates. See `bench/README.md` before measuring a performance change.
- `thirdparty/` and `cmake/`: dependency modes, tooling, package and install support.
- `docs/tasks/`: the backlog; `docs/api-2.0.md` records agreed API decisions, not a
  guarantee that every proposal is already implemented.

## Behavior changes and validation

- Python Jinja2 is the behavioral oracle. Use the Python version in
  `.github/workflows/parity-expectations.yml` and packages pinned by
  `test/parity/requirements.txt`; record deliberate divergences. Do not infer
  expected output from C++ behavior or casually upgrade the oracle. A quick probe:
  `python3 -c "import jinja2; print(jinja2.Template('{{ x }}').render(x=1))"`.
- Every behavior fix needs a focused regression case. Prefer an existing table or
  parity case; edit older unit expectations only where they encode changed behavior,
  and only in files the task lists, so parallel parity PRs do not collide in the
  shared test tables.
  Check relevant malformed arguments, empty/undefined values and both character types.
- The parity suite uses checked-in Python expectations and a divergence allowlist.
  A parity PR deletes its lines from `test/parity/divergences/<area>.txt` (one
  allow-list per area) and adds cases under `test/parity/cases/` where the corpus
  lacks one; do not weaken assertions
  or add an allowance just to make a check pass. When cases change, use the pinned
  environment to regenerate expectations and run `test/parity/generate.py --check`.
- Parser/lexer/evaluator changes also require relevant address/undefined sanitizer
  checks. Malformed templates must report an error rather than crash. Check lifetime
  and concurrency changes under the appropriate sanitizer when supported.
- Run `scripts/preflight.sh` before pushing implementation changes; add `--perf` for
  `src/` changes. It checks changed-line style/tidy, headers, task metadata and, when
  requested, the instruction-count gate. It does not replace builds or tests.
- For documentation-only changes, run preflight and check affected links/commands;
  do not rebuild unrelated configurations merely to produce a test count. Distinguish
  passed, skipped, failed and unrun checks, and report platform/tool limitations.

## Style, dependencies and generated files

- Follow `.clang-format`. `src/` and `include/` are formatted as whole files; tests
  include hand-aligned tables, so format only changed test lines. Use
  `git clang-format origin/master` before committing C++ changes.
- `.clang-tidy` is based on clang-tidy 22.1.8 (`pip install clang-tidy==22.1.8`;
  older distribution packages lack many checks). CI reports hits on changed lines and
  fails on checks in `WarningsAsErrors`. Use
  `scripts/clang_tidy_fix.py --checks <check>`, not `run-clang-tidy -fix`, which can
  apply header edits twice. For pointer conditions use `if (p)`/`if (!p)` rather
  than explicit null comparisons; see `scripts/null_compare.py`.
- Keep dependency pins and hashes aligned with the relevant dependency modes: pins in
  `thirdparty/internal_deps.cmake` change together with their `URL_HASH`, and a new
  dependency is added to `sbom/components.json`. Do not
  disable TLS/checksum verification or edit an expected hash to bypass a mismatch.
- Regenerate Unicode/HTML data through its scripts and pinned source environment.
  Preserve vendored/generated provenance and notices; do not hand-edit generated data.
- CI matrices are organised by C++ standard and covered pairwise (comment at the top
  of `.github/workflows/linux-build.yml`); preserve that when changing them. Verify compiler-specific
  code on supported runners; a Linux build does not establish MSVC or Apple Clang support.

## Task and PR workflow

- Read related tasks and agreed designs before starting larger work. Keep each PR
  focused; obtain a design for public-API, ownership or cross-module changes before
  implementing a different contract. Do not reopen agreed choices without evidence.
- Follow `docs/tasks/README.md` for metadata and acceptance criteria. Check the latest
  target branch before allocating a task number and recheck before pushing; numbers
  are shared across project threads. Update a task's status and PR link when advancing it.
- File independent follow-up work as a task. In a review, still report actionable
  regressions in the PR; do not turn each finding into an unsolicited task-file edit.
- Leave the generated task index in `docs/tasks/README.md` to the merge steward.
  `scripts/task_index.py --check` may warn about expected drift in task-only PRs.
- Parity PRs land through integration waves. Do not merge master into an individual
  parity PR independently of the merge steward. The steward integrates the wave,
  reruns the corpus with `test/parity/update_divergences.py` and regenerates the
  `docs/parity.md` summary; parity PRs never edit that summary. Two PRs that are green
  apart can be red together when one unblocks a case the other listed.
- Open implementation PRs as drafts and wait for applicable CI before marking them
  ready. Changes limited to `docs/`, `*.md`, `.claude/`, `scripts/`, `.gitignore` or
  `LICENSE` skip the build workflows and CodeQL (`paths-ignore`; only the format check
  runs), so do not claim an unrun matrix passed.
- PR descriptions explain the problem, resulting behavior, validation and remaining
  limits. Record roles used, verifier rounds and observed red CI pushes/causes;
  distinguish unavailable CI results from zero failures. Never claim tests were run
  or independent review occurred without evidence.
