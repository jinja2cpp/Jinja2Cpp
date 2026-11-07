---
status: done
priority: medium
area: parity
depends: [0354]
touches: [python/tests/, bench/run.py, python/jinja2cpp/integrations/]
shares: [.github/workflows/python.yml]
---
# Python module P3: upstream test suite, parity corpus, chat templates, bench row

**Problem.** Gate G13 needs the module's pass rate and a Python row in the comparison report
(0246), and 0248's done-when needs a transformers chat-template workload rendered
identically. Pallets' own test suite is also the first parity measure we did not write
ourselves: it reaches the environment API, loaders, errors, Undefined, the sandbox and i18n,
which the corpus does not (Ruslan 2026-10-30).

**Proposal.**
- **Upstream suite (closes 0237's open decision).** Run all of the pallets/jinja 3.1.6
  `tests/` with `jinja2` mapped to `jinja2cpp` and `fallback="never"`.
  `python/tests/upstream/expected-failures.txt` holds one line per test id, with one of
  these classes:
  - `internals`: nodes, lexer, compiler. These are excluded from the denominator.
  - `async`, `extension`: outside the supported subset.
  - `deliberate <divergence>`: behaviour we do not want to keep (Ruslan 2026-10-30). These
    are reported apart from failures and do not count against the rate.
  - `divergence <task>`: a real parity gap.
  Every `divergence` line is filed as a parity task, with a corpus case.
- **Parity corpus through the module.** Run the corpus through the module; its pass count must
  equal the C++ ParityTest count, and any difference is a binding bug.
- **Chat templates.** About 15 chat templates of widely used models, each with its source and
  license, rendered identically by `jinja2cpp` and by Jinja2 configured as transformers
  configures it. Add `jinja2cpp.integrations.transformers.patch()` (D9), which falls back for
  `{% generation %}` and for the assistant-token mask.
- **Bench row.** `bench/run.py --python-module` adds the `jinja2cpp (Python)` row for 0246.
- **Streaming.** `generate`/`stream` stream through `OutputSink`.

**Done when.** CI prints the upstream pass rate as passed / (collected − internals), with the
deliberate and divergence counts. The corpus counts match. The chat templates are identical.
0246 can read the bench row.

**Result (2026-11-07).**
- Upstream suite: `python/tests/upstream/run.py` (in `CI-python`, under ASan): 596 / 856 =
  69.6% passed, 91.0% within the supported subset (596 / 655); listed: async 126, extension
  10, unsupported 65 (a class added for API outside the subset: NativeEnvironment, compiled
  modules, bytecode caches, ...), deliberate 0, divergence 59, filed as 0431-0436 and in the
  open 0412 and 0415.
- Parity corpus through the module (`python/tests/test_parity_corpus.py`): every case ends as
  in the C++ ParityTest (3207 of 3225 match), after three crossing fixes found by it (folded
  finalize under autoescape, loop.length of a Python list, an undefined slice bound).
- Chat templates: the 72 templates of `test/chat_templates` (722 renders) match transformers
  through `chat_template_environment()`; `jinja2cpp.integrations.transformers.patch()` is
  checked against the pinned transformers in CI. The plan's ~15 templates are the existing
  corpus of 0360.
- Bench row: `bench/run.py --python-module`.
- Streaming: `generate`/`stream` yield the `OutputSink` chunks; lazy rendering is 0436. The
  GIL-released render of the plan's phase 3 is 0437.

