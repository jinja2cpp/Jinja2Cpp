# Fuzzing

Templates are often user input, so Jinja2C++ must reject a malformed or hostile template
with an error, never crash on it (docs/tasks/0003). This directory holds the libFuzzer
targets, the seed corpus tooling, the crash regressions and a differential check against
Python Jinja2.

| File | Purpose |
|---|---|
| `fuzz_targets.cpp`, `fuzz_common.h` | The fuzzed work: environment, context and support templates |
| `fuzz_parse.cpp` | `jinja2cpp_fuzz_parse`: `Template::Load` only, as a reload over a good template that must still render when the input is rejected |
| `fuzz_render.cpp` | `jinja2cpp_fuzz_render`: load and render, with includes, imports and extends of support templates and of the input itself |
| `fuzz_render_wide.cpp` | `jinja2cpp_fuzz_render_wide`: the same through `TemplateW` (input decoded as UTF-8) |
| `replay_corpus.py` | Runs a libFuzzer target once over a corpus, carrying on past timeouts and crashes (the pull-request mode) |
| `replay_main.cpp` | `jinja2cpp_fuzz_replay`: runs all targets over files without libFuzzer (any compiler); `--json` prints render outcomes |
| `regressions/` | One input per crash ever found; ctest `jinja2cpp_fuzz_regressions` replays them in every CI build |
| `make_corpus.py` | Seed corpus: parity cases, unit-test template literals, `test/test_data`, `regressions/` |
| `jinja2.dict` | Dictionary of Jinja2 syntax, statements, filters and tests |
| `differential.py` | Renders a corpus with Jinja2C++ and Python Jinja2 and reports where they disagree |

## Running locally

libFuzzer needs clang (Ubuntu: `clang-18 libclang-rt-18-dev`).

```bash
cmake -S . -B build-fuzz -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DJINJA2CPP_WITH_SANITIZERS=address+undefined \
  -DJINJA2CPP_BUILD_FUZZERS=ON -DJINJA2CPP_BUILD_TESTS=OFF
cmake --build build-fuzz --parallel
python3 fuzz/make_corpus.py corpus/render
build-fuzz/fuzz/jinja2cpp_fuzz_render corpus/render -dict=fuzz/jinja2.dict -max_len=4096 \
  -timeout=25 -fork=2 -max_total_time=600 -artifact_prefix=artifacts/
```

A crash leaves `artifacts/crash-<sha1>`; reproduce it with
`build-fuzz/fuzz/jinja2cpp_fuzz_render artifacts/crash-<sha1>`. In the cloud sessions
pass `-C "$JINJA2CPP_CMAKE_INIT"` to the configure step (CLAUDE.md).

## When a fuzzer finds a crash

1. Minimize it: `jinja2cpp_fuzz_render -minimize_crash=1 -runs=10000 artifacts/crash-<sha1>`.
2. Fix it, or file a task in `docs/tasks/` with the reproducer if the fix belongs elsewhere.
3. Add the minimized input to `regressions/` under a name that says what it does
   (`recursive-include.j2`) and, for a behaviour the fix defines, a unit test.

Timeouts and out-of-memory reports (`timeout-*`, `oom-*`) are findings too, but a template
can legitimately ask for a lot of work (`range(10**9)`, `'x' * 10**9`); Jinja2C++ has no
sandbox limits yet, so CI reports them without failing.

## Differential check

```bash
python3 fuzz/make_corpus.py --parity-only seeds-parity
python3 fuzz/differential.py build/jinja2cpp_fuzz_replay corpus/render \
  --known seeds-parity --out differential.md
```

It needs Python Jinja2 (`pip install -r test/parity/requirements.txt`, Python 3.12 like the
parity corpus). Inputs the parity corpus already covers (`--known`) and invalid UTF-8 are
skipped. A divergence worth keeping becomes a parity case (`test/parity/cases/`), listed in
`test/parity/divergences/<area>.txt` until it is fixed.

## Continuous fuzzing

`.github/workflows/fuzz.yml` runs in two modes (docs/tasks/0185):

- **Replay**, on pull requests that touch the engine: each target runs once over every
  input of the corpus the nightly run kept, plus the seeds (`replay_corpus.py`), and
  generates nothing new. The same corpus gives the same verdict, so the job fails only
  when the change makes a known input crash or leak; that input is uploaded as an artifact.
- **Generate**, every night on master and on demand (`workflow_dispatch`, any branch): the
  fuzzer mutates the corpus for an hour per target, keeps the minimized corpus in the
  Actions cache for the next runs, and runs the differential check (report only). A crash
  it finds fails that run and uploads its reproducer; fix it or file it as a task, and add
  the input to `regressions/`, which every build replays.

Replay the corpus locally the way a pull request does:

```bash
python3 fuzz/replay_corpus.py build-fuzz/fuzz/jinja2cpp_fuzz_render corpus/render
```
