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
already configured and built `build/`. The cloud proxy blocks GitHub archive downloads,
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
  a new row in an existing parameterised table.
- Parser/lexer/evaluator changes: also run the sanitizer configuration locally; crashes on
  malformed templates are bugs (see issues tagged from fuzzing).
- Formatting: `.clang-format` is derived from the committed code, but the tree is not
  clean under it (docs/tasks/0009), so never reformat whole files. CI checks only the
  lines a PR touches; run `git clang-format origin/master` before committing.
- Keep PRs to one concern; open them as drafts and let CI (Linux GCC/Clang matrix,
  macOS, Windows MSVC, sanitizers, Conan, CodeQL, format) go green before review.
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

| Role | Use for |
|---|---|
| `explorer` | locating code, tracing call paths, finding covering tests (cheap, read-only) |
| `architect` | cross-module or public-API design; returns a plan (strongest model, high effort) |
| `implementer` | one scoped change plus its test, built and run |
| `verifier` | adversarial pre-push check: build, tests, sanitizers, Python oracle |
| `parity-checker` | rendering the same templates with Jinja2C++ and Python Jinja2 |

Typical flows: bug report → explorer → implementer → verifier; new feature or API change →
explorer → architect → implementer → verifier.
