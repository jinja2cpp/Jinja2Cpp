---
status: open
priority: medium
area: api
depends: [0352]
touches: [python/, pyproject.toml, .github/workflows/python.yml, thirdparty/internal_deps.cmake#nanobind, sbom/components.json]
shares: [CMakeLists.txt]
---
# Python module P1b: build, Environment/Template/render, value crossing

**Problem.** The 0248 module needs a skeleton that builds, renders and crosses values, before
any subset work can start.

**Proposal.** Build with nanobind and scikit-build-core from a root `pyproject.toml` and
`-DJINJA2CPP_BUILD_PYTHON=ON`. nanobind is pinned with its hash and recorded in the SBOM. The
floor is CPython 3.12, with abi3. This task covers:
- `Environment`, `Template`, `from_string`, `get_template` and `render`;
- the D4 adapters for dict, list, tuple, object and generator, with identity kept and strings
  copied;
- Python callables bound through the #645 call protocol;
- exceptions, with a Python exception re-raised as itself;
- `Markup` as the same type as `markupsafe.Markup`;
- the four Undefined policies;
- `DictLoader` and `FileSystemLoader` (through ConfinedFileSystem);
- `tp_traverse` and `tp_clear` on every wrapper that holds Python references.
The GIL is held for the whole render. `PyErr_CheckSignals` is polled from the render step
counter, so Ctrl-C works during long renders.

**Performance gate (Ruslan 2026-10-30).** The module must beat Python Jinja2 on Load and
Render for every `bench/cases` case. The bench row from P3 starts here as a local script. If a
case loses, the D5 snapshot path moves into this task: `JsonTraits<PyObject*>` plus `ToValue`
with `JsonCopyLimits`, rendering with the GIL released, made the default for JSON-like
contexts.

**Done when.** `pip install .` works. The module's own tests are green under ASan, with
libasan preloaded. A refcount-leak loop is clean. A Linux CI job runs on changes to
`python/`, `src/` and `include/`. The perf gate holds, or each losing case is recorded with
its cause.
