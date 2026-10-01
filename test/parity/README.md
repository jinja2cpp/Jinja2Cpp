# Parity corpus

Templates rendered by both Python Jinja2 (the oracle) and Jinja2C++, compared case by
case. The feature map built from it is [docs/parity.md](../../docs/parity.md).

| Path | What |
|---|---|
| `cases/<area>.py` | The cases: `CASES = [(name, template[, options])]` and an optional default `CONTEXT` |
| `expected/<area>.json` | Generated: each case with Python's output or error. Do not edit by hand |
| `expected/index.json` | Generated: the list of areas, read by the C++ suite |
| `divergences.txt` | Allow-list: every case where Jinja2C++ differs, with kind, owning task and reason |
| `parity_test.cpp` | The gtest suite (`ctest -R parity`) |
| `generate.py` | Regenerates `expected/`, checks it (`--check`), prints the parity table (`--report`) |
| `update_divergences.py` | Applies what a suite run observed to `divergences.txt` (see below) |
| `requirements.txt` | Pinned jinja2 and MarkupSafe |

## Running

```bash
ctest --test-dir build -R parity --output-on-failure
build/jinja2cpp_tests --gtest_filter='Parity/ParityTest.MatchesPython/filters_*'
build/jinja2cpp_tests --gtest_filter='ParityWide/*'           # the wide-string run only
python3 test/parity/generate.py --report     # match counts by area, from divergences.txt
```

Python is needed only to change cases: `pip install -r test/parity/requirements.txt`.

## What the suite checks

For every case it renders the template with Jinja2C++ and classifies the result:

- **match**: same output, or both engines report an error (messages are not compared);
- **output**: both render, outputs differ;
- **rejects**: Jinja2C++ fails, Python renders;
- **accepts**: Jinja2C++ renders, Python fails;
- **unsupported**: the case sets an Environment option Jinja2C++ cannot express.

A case not in `divergences.txt` must match. A listed case must still diverge in the
listed kind: when a fix makes it match, the suite fails until its line is deleted, so the
list only shrinks on purpose. Kind `unordered` accepts either result, for output that
depends on `std::unordered_map` order and so differs by standard library. Kind `crash` skips a case that would bring the binary down or trip the sanitizers
(undefined behaviour).

### The wide-string run

`ParityWide/*` renders every case a second time through `TemplateW`: the template, the
loader files and the context strings are converted from UTF-8 to `wchar_t` by the suite
itself (not by `jinja2::ConvertString`, which depends on the C locale, task 0035), and the
output back to UTF-8. It is held to the narrow result, so a narrow divergence is listed
once: a wide case diverges only when its result differs from the narrow one and does not
match Python either. Such a case is listed as `wide.<id>`, with its kind relative to
Python, and follows the same rules as narrow lines (including `crash`). Cases with an
unsupported option or a narrow `crash` line are not run wide.

## Updating divergences.txt

The suite can record every outcome; the script then deletes the lines of cases that now
match and rewrites changed kinds, keeping task and reason. New divergences are only
printed, because they need an owning task, and make it exit non-zero.

```bash
rm -f /tmp/parity.txt
JINJA2CPP_PARITY_RESULTS=/tmp/parity.txt build/jinja2cpp_tests --gtest_filter='Parity*'
python3 test/parity/update_divergences.py /tmp/parity.txt
```

Run it after each fix and after every merge of master into a parity branch. A changed
kind usually means the case is now blocked by another gap: move its line to that task.
When two branches add cases to the same area, take either side of the conflict in
`expected/<area>.json` and run `generate.py` again; never merge that file by hand.

## Adding a case

1. Add a tuple to the right `cases/<area>.py` (or a new area file). Options:
   `ctx` (replaces the area context; must survive JSON), `env` (Environment options:
   `trim_blocks`, `lstrip_blocks`, `keep_trailing_newline`, `autoescape`, `undefined`,
   `extensions`, delimiters, line prefixes, `newline_sequence`) and `templates`
   (name → source, for include/import/extends).
2. `python3 test/parity/generate.py`. It refuses non-deterministic output.
3. Build and run the suite. If the case diverges, add a line to `divergences.txt` naming
   the task that owns the fix (open one in `docs/tasks/` if none fits).

Keep cases small and aimed at one feature. Print booleans as `'T' if ... else 'F'` and
iterate lists instead of printing them when the case is about something else, so that the
repr gaps (task 0012) do not mask the behaviour under test.
