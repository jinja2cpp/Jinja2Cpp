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
- Formatting: the existing tree is not clang-format clean, so never reformat whole files.
  CI checks only the lines a PR touches; run `git clang-format origin/master` before committing.
- Keep PRs to one concern; open them as drafts and let CI (Linux GCC/Clang matrix,
  macOS, Windows MSVC, sanitizers, JSON bindings, CodeQL, format) go green before review.
- Do not bump dependency pins casually: they are hashed in `thirdparty/internal_deps.cmake`
  and must be updated together with `URL_HASH`.
