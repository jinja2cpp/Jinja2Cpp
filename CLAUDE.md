# Jinja2C++ — notes for coding agents

Jinja2C++ is a C++ implementation of the Python [Jinja2](https://jinja.palletsprojects.com/)
template engine. The project goal is maximum achievable behavioural parity with Python
Jinja2, with good performance and an API that is easy to embed.

## Build and test

CMake (≥ 3.23) + Ninja. Dependencies are pulled by FetchContent in the default
`JINJA2CPP_DEPS_MODE=internal` (Boost, fmt, nonstd *-lite, nlohmann_json, googletest).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure        # one gtest binary: build/jinja2cpp_tests
build/jinja2cpp_tests --gtest_filter='FilterGenericTest*'   # run a subset
```

In Claude Code on the web the SessionStart hook (`.claude/hooks/session-start.sh`) has
already configured and built `build/`. Project threads open the session in the parent
directory of the clone, where the hook does not fire: if `build/` is missing, run
`CLAUDE_CODE_REMOTE=true .claude/hooks/session-start.sh` from the clone and export the
`JINJA2CPP_CMAKE_INIT` it prints. The cloud proxy blocks GitHub archive downloads,
so for any *new* build directory pass the generated initial cache:
`cmake -S . -B build-x -G Ninja -C "$JINJA2CPP_CMAKE_INIT" ...`.
`-DJINJA2CPP_WITH_JSON_BINDINGS=rapid` still needs an archive download (patched
RapidJSON) and does not configure in the cloud.

Useful configurations (all exercised in CI, see `.github/workflows/`):

| Purpose | Extra CMake flags |
|---|---|
| Language standard | `-DJINJA2CPP_CXX_STANDARD=14` (default), `17`, `20` |
| Sanitizers | `-DJINJA2CPP_WITH_SANITIZERS=address+undefined -DCMAKE_BUILD_TYPE=RelWithDebInfo` |
| JSON bindings | `-DJINJA2CPP_WITH_JSON_BINDINGS=boost` (default), `nlohmann`, `rapid` |
| Shared library | `-DJINJA2CPP_BUILD_SHARED=ON` |
| Coverage | `-DJINJA2CPP_WITH_COVERAGE=ON` (GCC/Clang, Debug) |
| Conan dependencies | `conan install . -of .conan --build=missing` then `-DCMAKE_TOOLCHAIN_FILE=.conan/conan_toolchain.cmake -DJINJA2CPP_DEPS_MODE=conan-build` (ConanCenter is blocked in the cloud) |

`JINJA2CPP_STRICT_WARNINGS` is ON by default (`-Wall -Werror` on GCC/Clang); keep the
library warning-free rather than turning it off.

## Layout

- `include/jinja2cpp/` — public API (`Template`, `TemplateEnv`, `Value`, reflection,
  user callables, JSON bindings). Must stay C++14-compatible; uses nonstd
  `expected`/`variant`/`optional`/`string_view`.
- `src/` — lexer (`lexer.*`, vendored `lexertk.h`), parsers (`template_parser.*`,
  `expression_parser.*`), evaluator (`expression_evaluator.*`, `internal_value.*`),
  statements, filters, testers, serializers. `robin_hood.h` and `lexertk.h` are vendored.
- `test/` — googletest suites; most are table-driven `InputOutputPair` tests
  (`SUBSTITUTION_TEST_P`, `MULTISTR_TEST` in `test/test_tools.h`) that render a
  template and compare the output. `MULTISTR_TEST` checks both narrow and wide templates.
- `thirdparty/`, `cmake/` — dependency modes, sanitizer/coverage helpers, install config.

## Working conventions

- **Python Jinja2 is the oracle.** Before changing rendering/parsing behaviour, check
  what Jinja2 does (`python3 -c "import jinja2; print(jinja2.Template('{{ x }}').render(x=1))"`)
  and make the C++ output match. Note deliberate divergences in the PR description.
- **Every behaviour fix ships with a test** in the matching `test/*_test.cpp`, ideally
  a new row in an existing parameterised table. For parity tasks (0012 onwards) the
  corpus is that test: the PR deletes its lines from `test/parity/divergences/<area>.txt` and
  adds cases under `test/parity/cases/` where the corpus lacks one. Edit existing unit
  tests only where they encode the old behaviour, and only in files the task lists, so
  that parallel parity PRs do not collide in the shared test tables.
- **Parity PRs land in waves through one integration branch** (docs/tasks/README.md):
  do not merge master into a parity PR yourself; when it is final, say so to the merge
  steward, who merges the wave together, re-runs the corpus with
  `test/parity/update_divergences.py` and regenerates the `docs/parity.md` summary. Two
  PRs that are green apart can be red together when one unblocks a case the other listed.
- Parser/lexer/evaluator changes: also run the sanitizer configuration locally; crashes on
  malformed templates are bugs (see issues tagged from fuzzing).
- Formatting: `src/` and `include/` are clean under `.clang-format` and CI checks them
  whole; `test/` is not (hand-aligned tables, docs/tasks/0009), so never reformat whole
  test files, and CI checks only the lines a PR touches there. Run
  `git clang-format origin/master` before committing.
- **clang-tidy** (docs/tasks/0054): `.clang-tidy` holds the checks, measured with
  clang-tidy 22.1.8 (`pip install clang-tidy==22.1.8`; apt's 18 lacks many). CI reports
  hits on changed lines only and fails on checks in `WarningsAsErrors`. Apply fix-its
  with `scripts/clang_tidy_fix.py --checks <check>`, never `run-clang-tidy -fix`: headers
  reached as `src/binding/../x.h` get every fix twice, and the script normalises paths.
  House style it cannot express: `if (p)`/`if (!p)`, never `p != nullptr` in a condition
  (`scripts/null_compare.py`).
- Keep PRs to one concern; open them as drafts and let CI (Linux GCC/Clang matrix,
  macOS, Windows MSVC, sanitizers, Conan, CodeQL, format) go green before review.
  Changes limited to `docs/`, `*.md`, `.claude/`, `scripts/`, `.gitignore` or `LICENSE`
  skip the build workflows and CodeQL (`paths-ignore`); only the format check runs.
- CI matrices are organised by C++ standard and covered pairwise (comment at the top of
  `.github/workflows/linux-build.yml`); keep that property when editing them.
- Do not bump dependency pins casually: they are hashed in `thirdparty/internal_deps.cmake`
  and must be updated together with `URL_HASH`.

## Task registry

`docs/tasks/` holds work bigger than one PR: audit findings, strategic directions and
follow-ups, one file per task with `status`/`priority`/`area` front matter (conventions in
`docs/tasks/README.md`). Check it before starting larger work; when a PR advances a task,
update its status and link the PR. File new findings there rather than in PR descriptions.

## Agent roles

`.claude/agents/` defines role subagents, each with a model and effort sized to the job
(docs/tasks/0004). Delegate rather than doing everything in the main session:

| Role | Use for | Where it runs |
|---|---|---|
| `explorer` | locating code, tracing call paths, finding covering tests (cheap) | read-only, shared checkout |
| `architect` | cross-module or public-API design; returns a plan (strongest model, high effort) | read-only, shared checkout |
| `implementer` | one scoped change plus its test, built, run and committed | own worktree and `build/` |
| `verifier` | adversarial pre-push check: build, tests, sanitizers, Python oracle | own worktree and build dirs |
| `parity-checker` | rendering the same templates with Jinja2C++ and Python Jinja2 | read-only, scratch dir |

`implementer` and `verifier` have `isolation: worktree`: each call gets a fresh checkout
under `.claude/worktrees/` made from the caller's **last commit** (commit before
delegating; uncommitted edits are invisible to them) and builds there, so a sanitizer
build or a second implementer never clobbers `build/`. In the cloud the SessionStart
hook sets up ccache so a worktree build reuses `build/`'s objects. An implementer hands
back a commit on its worktree branch; bring it in with `git cherry-pick <sha>`.

Recipes:
- Bug report: explorer (where, which tests) → implementer → verifier.
- Feature or API change: explorer → architect → implementer(s) → verifier.
- Parity batch: parity-checker over the templates → group mismatches by root cause →
  one implementer per cause → verifier on the combined branch.
- Review: the verifier's checklist is the review checklist; run it before marking a PR
  ready, and paste its verdict into the PR conversation.

In the PR description, note which roles ran, how many verifier rounds it took and how
many pushes went red in CI. That is the data 0004 needs to tune models and boundaries.

## Batching work

Parallelism comes at three levels; pick the outermost one that fits.

1. **Project threads** (one task per thread). Each thread has its own container,
   clone, `build/`, branch and PR, and costs a cold start (SessionStart hook, about two
   minutes). Use a separate thread for each task in `docs/tasks/` and for anything that
   should be its own PR with its own review and CI. Run
   `python3 scripts/task_batches.py` to see which tasks can run side by side: tasks in
   one wave have disjoint `touches`. Tasks that overlap run one after another, or one
   thread owns the shared files and the others send it their changes.
2. **Subagents in one thread.** Read-only roles (explorer, architect, parity-checker)
   fan out freely: launch them in one message. Implementers run in parallel only when
   their changes touch different files; each gets its own worktree, so the risk is
   merge conflicts on cherry-pick, not corrupted builds. A container has 4 cores: at
   most two concurrent builds, each with `--parallel 2`. The verifier runs last, on the
   committed result.
3. **Tool calls.** Independent reads, searches and commands go in one message.

Keep work in one thread when the pieces must land in one PR, or when each step needs
the previous one's output (explore → design → implement is a pipeline, not a batch).
Split into threads when the pieces could merge in any order.
