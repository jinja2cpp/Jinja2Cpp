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
| `fuzz_metadata.cpp` | `jinja2cpp_fuzz_metadata`: the input as the text of a `{% meta %}` block, read by the JSON reader with default and untrusted limits |
| `fuzz_prompt_escape.cpp`, `prompt_escape_case.h` | `jinja2cpp_fuzz_prompt_escape`: prompt autoescape (docs/tasks/0358). The input chooses a template from a grammar of escaping constructs and data strings with private-use special tokens the template never contains; a special in the output aborts. Then the input itself renders in prompt mode, for crashes only. `PromptEscapeProperty` in `test/filters_test.cpp` runs a fixed-seed sample |
| `untrusted_targets.cpp`, `fuzz_untrusted.cpp` | `jinja2cpp_fuzz_untrusted`: load and render under `FuzzLimits()` (`UntrustedLimits()` tightened so that no template may legitimately run past the timeout or the RSS limit), so a timeout or out-of-memory is a finding (docs/tasks/0267). The first byte picks settings: autoescape, trim and lstrip blocks, `#` line statements, one of four delimiter sets, strict undefined, wide |
| `fuzz_host.cpp` | `jinja2cpp_fuzz_host`: the same with host data: reflected structs (nested by `shared_ptr`, lists of them), reflected vectors, a generated list, callables with typed, `*args`, `**kwargs` and `*context` parameters, one returning a fresh host object, one throwing, host filters and tests |
| `fuzz_loader.cpp` | `jinja2cpp_fuzz_loader`: templates on disk. The second byte picks `RealFileSystem` over a plain root or `ConfinedFileSystem` over one with links out of it; canary files outside the roots get new text for each input, and that text in the output aborts |
| `alloc_failure_target.cpp`, `fuzz_alloc_failure.cpp` | `jinja2cpp_fuzz_alloc_failure`: the first two bytes (little-endian) pick an allocation to fail during Load and Render, the third the settings. Each call must return or throw `std::bad_alloc`; when the failure fired and no `bad_alloc` came out the outcome must not change, and the template and environment must render as before afterwards. It replaces the global `operator new`, so its replay is `jinja2cpp_fuzz_alloc_replay` |
| `replay_corpus.py` | Runs a libFuzzer target once over a corpus, carrying on past timeouts and crashes (the pull-request mode) |
| `replay_main.cpp` | `jinja2cpp_fuzz_replay`: runs all targets over files without libFuzzer (any compiler); `--json` prints render outcomes |
| `regressions/` | One input per crash ever found; ctest `jinja2cpp_fuzz_regressions` replays them in every CI build |
| `make_corpus.py` | Seed corpus: parity cases, unit-test template literals, `test/test_data`, `regressions/`; `--prefix TARGET` adds the bytes the limits-on targets read first (each seed with the default bytes and with bytes from its hash, its delimiters rewritten to match) |
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

Timeouts and out-of-memory reports (`timeout-*`, `oom-*`) are findings too. With default
settings a template can legitimately ask for a lot of work (`range(10**9)`, `'x' * 10**9`), so
CI reports them without failing for those targets. The limits-on targets (`untrusted`, `host`,
`loader`, `alloc_failure`) run under `FuzzLimits()`, which bounds both, so there a timeout or
out-of-memory fails the job (`replay_corpus.py --strict`, and no `-ignore_timeouts` when
generating). Every run of the untrusted job also checks that this holds: with
`JINJA2CPP_FUZZ_INJECT_SLOW=1` the target hangs on an input that starts with
`\xffjinja2cpp-fuzz-slow-seed`, and that replay must fail.

The limits-on targets read bytes before the template; seed them with
`python3 fuzz/make_corpus.py --prefix untrusted corpus/untrusted` (or `host`, `loader`,
`alloc_failure`). A regression input for them keeps its prefix bytes.

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
