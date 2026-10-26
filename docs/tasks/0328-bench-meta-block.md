---
status: open
priority: low
area: perf
touches: [bench/cases/meta_block/, bench/jinja2cpp_bench.cpp, bench/run.py, bench/README.md]
---
# A benchmark for `{% meta %}` and the internal JSON reader

**Problem.** #652 (0230 D6) replaced the JSON library behind `{% meta %}` with an internal
reader, and no benchmark case contains a `{% meta %}` block, so neither the PR gate nor
the trend measures it. The perf verdict on #652 could only say that the other cases did
not move. `{% meta %}` is a Jinja2C++ extension: Python Jinja2 cannot parse it, and
`run.py` requires every case to render the same text with both engines. So a case cannot
be added as an ordinary directory.

The cost sits in two places:
- `Load` lexes the block and keeps its text;
- the first `GetMetadata()` parses the JSON and caches the result (`m_metadata` under a
  mutex). Later calls return the cached map.

Neither `Load/<case>` nor `Render/<case>` calls `GetMetadata()`.

**Proposal.**
1. Add `bench/cases/meta_block/`: a template that starts with a realistic metadata block
   of 2-4 KB (nested objects and arrays, escaped strings, numbers; for example a chat
   template's `{"tools": [...], "stop": [...]}`), followed by a short body.
2. Mark it C++-only. For example, `settings.json` gets `"cpp_only": true`. `run.py` then
   skips the Python comparison and the speedup column for it, and `python_bench.py` skips
   the case. `Load/meta_block` and `Render/meta_block` then run in `count.py` like the
   other cases, so the gate and the trend track them.
3. Add a `Metadata/meta_block` benchmark in `jinja2cpp_bench`. Each iteration loads the
   template and calls `GetMetadata()` once, so the measured cost is the JSON parse plus
   `Load`. Subtracting `Load/meta_block` gives the reader alone. Run it only for cases
   whose source has a `{% meta %}` block.
4. Document the case and the `cpp_only` flag in `bench/README.md`.

**Done when.** `count.py` lists `Load/`, `Render/` and `Metadata/meta_block`. `run.py`
passes with the case present and prints no Python column for it. The trend record of the
merge that adds it contains the new rows.
