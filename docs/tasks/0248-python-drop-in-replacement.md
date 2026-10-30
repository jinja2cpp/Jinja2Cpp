---
status: open
priority: medium
area: api
depends: []
touches: [python/]
---
# Jinja2C++ as a drop-in replacement for Python Jinja2 in common cases

**Problem.** Python projects that render many templates (LLM chat formatting in serving
stacks, static site and config generation) pay Python Jinja2's cost: Load is 230-590x and
Render 2-25x slower than Jinja2C++ on our cases (2026-10-12 engines table). They could
switch if `import jinja2cpp as jinja2` worked for their usage. Ruslan (2026-10-18) asked
for an effort towards a drop-in replacement "in some cases". 0237 asked for bindings as a
test tool; this task is the product.

**Proposal.** Needs an architect plan first:
- **Which cases:** define the supported subset by real users: `Environment(...)`,
  `from_string`, `get_template` with `FileSystemLoader`/`DictLoader`, `render(**ctx)`,
  `filters`/`globals`/`tests` with Python callables, `ImmutableSandboxedEnvironment` as
  used by Hugging Face `transformers` chat templates, `TemplateError` subclasses. Outside
  the subset, raise a clear error, or fall back to real Jinja2 when it is installed.
- **Binding:** nanobind or pybind11 with scikit-build-core. Python values cross by
  reflection without copying where possible (dict/list adapters). Callables run with the
  GIL held. Markup maps to `markupsafe.Markup`, which needs 0180.
- **Proof:** run pallets/jinja's own test suite (the 0237 idea) and the parity corpus
  through the module; publish the pass rate in the comparison report (0246).
- Wheels for Linux, macOS, Windows via cibuildwheel; publishing to PyPI follows the tag,
  like the other registries (docs/release-2.0.md section 5).

**Done when.** The supported subset is documented, a chat-template workload from
`transformers` renders identically through the module, the upstream test subset runs in
CI with its pass rate reported, and wheels build in CI.

**Plan approved (Ruslan 2026-10-30).** The plan is in
`/mnt/project-files/release-2.0/0248-plan.md`. Its decisions:
- **Binding:** nanobind with scikit-build-core, CPython 3.12 as the floor, abi3 wheels.
- **D3:** the module links the library statically and uses a private engine seam.
- **D4:** Python containers cross as lazy adapters that keep their identity.
- **D5:** the GIL is held during a render; a measured snapshot path renders with the GIL
  released.
- **D6:** `Markup` is the same type as `markupsafe.Markup`.
- **D7:** fallback to real Jinja2 is `auto` by default, `never` in CI, and is decided only
  at environment construction, at template load, or on a render retry before any callable
  has run.
- **D8:** the sandbox is an attribute and call policy inside the adapters.
- **D9:** transformers switch over through an explicit `patch()`, with no import hijack.
- **Performance gate:** the module must beat Python Jinja2 on Load and Render for every
  bench case.
- **Deliberate divergences:** behaviour we do not want to keep is reported apart in the
  upstream pass rate.

It is split into these tasks:
- 0352: P1a, the private host-object seam
- 0353: P1b, the core and the perf gate
- 0354: P2, the subset, sandbox and fallback
- 0355: P3, the upstream suite, the corpus, chat templates and the bench row
- 0356: P4, wheels
- 0357: before the G1 freeze, move the module to public API only, so the project's contract
  is the public interface alone (Ruslan 2026-10-30)

This task is done when 0352-0356 are.
