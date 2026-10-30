---
status: open
priority: medium
area: api
depends: [0353]
touches: [python/, docs/python.md]
---
# Python module P2: the supported subset, sandbox and fallback

**Problem.** P1b renders only the basic cases. "Drop-in" needs the Tier A subset of the 0248
plan (D1), Jinja2's sandbox, and an explicit answer for everything outside the subset.

**Proposal.**
- **Loaders:** every remaining loader. A real `jinja2` loader instance is accepted too.
- **Filters, tests and globals:** live dicts, with Jinja2's binding times.
- **Context access:** the `pass_context`, `pass_eval_context` and `pass_environment`
  decorators, with a read-only Context proxy.
- **Other API:** i18n, `select_template`, `make_module`/`module` (0284), `meta` through
  0236, and the exception classes subclassing `jinja2.exceptions` when Jinja2 is installed.
- **Sandbox (D8):** `SandboxedEnvironment` and `ImmutableSandboxedEnvironment` as an
  attribute and call policy inside the adapters. Every attribute path passes one `GetAttr`
  checkpoint, and `MAX_RANGE` maps to `Limits::maxRange`.
- **Fallback (D7, `auto` by default):** the decision is made when the environment is
  constructed, when a template is loaded, and on a render retry only when no callable has
  run. Modes are `auto`, `warn` and `never`, and `jinja2cpp.stats()` counts native versus
  fallback renders. A divergence marked deliberate never triggers fallback.
- **Docs:** `docs/python.md` with the subset table and the known differences.

**Done when.** Every row of the Tier A table has a test. Fallback is tested both with and
without Jinja2 installed. The sandbox passes Jinja2's `test_security.py` cases, and the PR
had a Codex security review.
