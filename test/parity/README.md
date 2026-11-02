# Parity corpus

Templates rendered by both Python Jinja2 (the oracle) and Jinja2C++, compared case by
case. The feature map built from it is [docs/parity.md](../../docs/parity.md).

| Path | What |
|---|---|
| `cases/<area>.py` | The cases: `CASES = [(name, template[, options])]` and an optional default `CONTEXT` |
| `expected/<area>.json` | Generated: each case with Python's output or error. Do not edit by hand |
| `expected/index.json` | Generated: the list of areas, read by the C++ suite |
| `divergences/<area>.txt` | Allow-list: every case where Jinja2C++ differs, with kind, owning task and reason; one file per area |
| `parity_test.cpp` | The gtest suite (`ctest -R parity`) |
| `generate.py` | Regenerates `expected/`, checks it (`--check`), prints the parity table (`--report`) |
| `update_divergences.py` | Applies what a suite run observed to `divergences/` (see below) |
| `requirements.txt` | Pinned jinja2 and MarkupSafe |

## Running

```bash
ctest --test-dir build -R parity --output-on-failure
build/jinja2cpp_tests --gtest_filter='Parity/ParityTest.MatchesPython/filters_*'
build/jinja2cpp_tests --gtest_filter='ParityWide/*'           # the wide-string run only
python3 test/parity/generate.py --report     # match counts by area, from divergences/
```

Python is needed only to change cases: `pip install -r test/parity/requirements.txt`.

## What the suite checks

For every case it renders the template with Jinja2C++ and classifies the result:

- **match**: same output, or both engines report an error (messages are not compared);
- **output**: both render, outputs differ;
- **rejects**: Jinja2C++ fails, Python renders;
- **accepts**: Jinja2C++ renders, Python fails;
- **unsupported**: the case sets an Environment option Jinja2C++ cannot express.

A case not in `divergences/<area>.txt` must match. A listed case must still diverge in the
listed kind: when a fix makes it match, the suite fails until its line is deleted, so the
list only shrinks on purpose. Kind `platform` accepts either result, for output that
differs by standard library or locale. Kind `crash` skips a case that would bring the binary down or trip the sanitizers
(undefined behaviour).

### The wide-string run

`ParityWide/*` renders every case a second time through `TemplateW`: the template and the
loader files are converted from UTF-8 to `wchar_t` by the suite itself (not by
`jinja2::ConvertString`, the conversion under test), and the output back to UTF-8. Context
strings stay narrow UTF-8, so every case also renders narrow values in a wide template.
It is held to the narrow result, so a narrow divergence is listed
once: a wide case diverges only when its result differs from the narrow one and does not
match Python either. Such a case is listed as `wide.<id>`, with its kind relative to
Python, and follows the same rules as narrow lines (including `crash`). Cases with an
unsupported option or a narrow `crash` line are not run wide.

### The reflect run

`ParityReflect/*` (docs/tasks/0322) renders every case with its context as items of one
`jinja2::Reflect` view of a shared copy of the document, instead of `jinja2::ToValue`
copies, so the corpus also checks the generic map and list adapters a view answers through.
It runs only with `JINJA2CPP_PARITY_CONTEXT=reflect` (nightly in `bindings.yml`, under
ASan, where it checks the owner chain of every item a template reaches):

```bash
JINJA2CPP_PARITY_CONTEXT=reflect build/jinja2cpp_tests --gtest_filter='ParityReflect/*'
```

Like the wide run it is held to the copy's result, and a case that differs is listed as
`reflect.<id>` in `divergences/reflect.txt`, one file for the whole pass.

## The allow-list

One file per area, `divergences/<area>.txt` (wide lines `wide.<id>` go in the file of
`<id>`'s area), so parity PRs that fix different areas never edit the same file. One
line per case, sorted by id:

```
<case id> <kind> <task> <reason>
```

`kind` is one of `output`, `rejects`, `accepts`, `unsupported`, `platform`, `crash`
(above); `task` is the `docs/tasks/` number that owns the fix. A case blocked by several
gaps is listed under the first one that has to be fixed. `ParityRegistry.*` fails on an
unknown case, kind or condition, a missing task or reason, an id listed twice, or a line
in the wrong area's file.

### Build conditions

Some cases depend on an optional build feature (task 0273). A kind may carry a condition:
`rejects@nobigint` lists a divergence of builds without the feature, and builds with it
must match; `output@bigint` is the reverse. The known features are `bigint` (an integer
arithmetic backend in the suite's default env, task 0178); `platform` takes no condition.
Where the condition does not hold, the case is held to a match like an unlisted one.

```
operators.int_overflow_mul rejects@nobigint 0178 int64 overflow needs a big-integer backend
```

Results files start with the build's features (`# features: bigint`, or `# features:`
for the default build). `update_divergences.py` edits only lines whose condition holds in
that build, and keeps the condition when it rewrites a kind; `generate.py --report`
describes the default build, so it counts `@no<feature>` lines and not `@<feature>`
ones, and lists all conditional lines separately.

## Updating the allow-list

The suite can record every outcome; the script then deletes the lines of cases that now
match and rewrites changed kinds, keeping task and reason. New divergences are only
printed, because they need an owning task, and make it exit non-zero.

```bash
rm -f /tmp/parity.txt
JINJA2CPP_PARITY_RESULTS=/tmp/parity.txt build/jinja2cpp_tests --gtest_filter='Parity*'
python3 test/parity/update_divergences.py /tmp/parity.txt
```

Run it after each fix and after every merge of master into a parity branch.
`update_divergences.py --self-test` checks the script itself on built-in fixtures (CI runs
it in `parity-expectations`). A changed
kind usually means the case is now blocked by another gap: move its line to that task.
When two branches add cases to the same area, take either side of the conflict in
`expected/<area>.json` and run `generate.py` again; never merge that file by hand.

## Adding a case

1. Add a tuple to the right `cases/<area>.py` (or a new area file). Options:
   `ctx` (replaces the area context; must survive JSON), `env` (Environment options:
   `trim_blocks`, `lstrip_blocks`, `keep_trailing_newline`, `autoescape` (a bool, or a list of
   extensions for `select_autoescape`), `undefined`,
   `extensions`, delimiters, line prefixes, `newline_sequence`, and `filters`, `tests` and
   `finalize`, which name fixed functions defined in `generate.py` and, with the same
   names, in `parity_test.cpp`) and `templates`
   (name → source, for include/import/extends).
2. `python3 test/parity/generate.py`. It refuses non-deterministic output.
3. Build and run the suite. If the case diverges, add a line to `divergences/<area>.txt` naming
   the task that owns the fix (open one in `docs/tasks/` if none fits).

Keep cases small and aimed at one feature. Print booleans as `'T' if ... else 'F'` and
iterate lists instead of printing them when the case is about something else, so that the
repr gaps (task 0012) do not mask the behaviour under test.

## Other oracles

The checked-in `expected/` comes from the pinned pair (`.python-version`, `requirements.txt`;
docs/parity.md, "Oracle pin"). To see how Jinja2C++ compares with another Python Jinja2, such
as an older 3.x, Jinja2's main branch or another Python, configure the build with
`JINJA2CPP_TEST_ORACLE` (docs/tasks/0186):

| Value | What the oracle is |
|---|---|
| `bundled` (default) | `expected/`; nothing else runs and no Python is needed |
| `python` | the jinja2 and markupsafe installed for `JINJA2CPP_TEST_ORACLE_PYTHON` (default: the Python 3 CMake finds) |
| `venv` | a venv in `<build>/parity-oracle/venv`, made from `JINJA2CPP_TEST_ORACLE_PYTHON` and filled by pip from `JINJA2CPP_TEST_ORACLE_REQUIREMENTS` (`;`-separated, e.g. `jinja2==3.1.2;markupsafe==2.1.5` or `jinja2 @ git+https://github.com/pallets/jinja@main`; a `;` always separates requirements, so environment markers do not fit) |

```bash
cmake -S . -B build -DJINJA2CPP_TEST_ORACLE=venv -DJINJA2CPP_TEST_ORACLE_REQUIREMENTS='jinja2==3.1.2'
ctest --test-dir build -R parity_oracle --output-on-failure
cat build/parity-oracle/report.md
```

`jinja2cpp_parity_oracle_generate` (a CTest fixture, so configure needs no Python packages
and no network) runs `generate.py --out <build>/parity-oracle --any-version`, which writes the
other oracle's expectations there with `oracle.json` naming its Python, Jinja2 and MarkupSafe.
`venv` needs network at test time; the venv is rebuilt when the requirements change, and
deleting it picks up a new commit of a branch requirement. Supported oracles: Python >= 3.10,
Jinja2 >= 3.0. Configure warns when the oracle's Python is not the pinned one, because the
Unicode tables and Python's own behaviour (`textwrap`, `\N{}` names) follow the pinned one.

`jinja2cpp_parity_oracle` runs the narrow suite with `JINJA2CPP_TEST_ORACLE_DIR` naming that
directory (set it by hand to compare against any `generate.py --out` directory; the test binary
needs no rebuild). Each case is judged by whether the two oracles agree (same output, or both
an error):

- **they agree**: judged as without an oracle, allow-list included; a regression fails;
- **they differ** (a Jinja2 or Python version delta): reported, not failed. The verdict is
  `tracks-bundled` (Jinja2C++ matches the pinned oracle), `tracks-oracle` (matches the other
  one), `neither`, or `not-run` (a listed crash, an unsupported option, or a build without the
  Unicode name table). With `JINJA2CPP_TEST_ORACLE_STRICT=1` every verdict but
  `tracks-oracle` fails.

The run writes `report.md` next to the expectations: the two oracles' versions, the counts,
and one row per differing case with the bundled result, the other oracle's and Jinja2C++'s.
Against Jinja2 3.1.2 with MarkupSafe 2.1.5 on Python 3.12 that is 9 rows, all
`tracks-bundled`; against 3.0.3 on Python 3.13, 22. `ParityRegistry.OracleCorpusMatches`
fails when the directory does not hold the same cases as the corpus (a stale generation).
The wide suite is held to the narrow result rather than to an oracle, so the oracle test does
not run it.
