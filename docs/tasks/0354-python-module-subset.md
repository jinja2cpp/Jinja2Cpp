---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/823
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
  constructed and when a template is loaded (the render retry is deferred, below). Modes are `auto`, `warn` and `never`, and `jinja2cpp.stats()` counts native versus
  fallback renders. A divergence marked deliberate never triggers fallback.
- **Deferred (D7 item 3, the render retry):** a render that failed on an include, import or extends of a
  template the engine cannot parse is not retried with Jinja2. "No Python ran in the render" cannot be
  proven (host objects call into Python in many adapters), so the engine's `TemplateSyntaxError` is raised
  instead. Task 0430 proposes routing the static names at load time.
- **Docs:** `docs/python.md` with the subset table and the known differences.

**Done when.** Every row of the Tier A table has a test. Fallback is tested both with and
without Jinja2 installed. The sandbox passes Jinja2's `test_security.py` cases, and the PR
had a security review: Codex's, or our own since Ruslan's decision on 2026-11-10. That review is done (below).

**Follow-up (2026-11-10).** Codex's review of merged #801 found two sandbox gaps, fixed in the follow-up PR:
the sandboxed `str.format` counted only the fields against `MaxSequenceSize`, so the literal text could double
the joined string past the cap before the check (now counted first); and a number's data attributes
(`real`, `imag`, `numerator`, `denominator`, 0374) skipped the sandbox policy on `x.real`, `|attr`, `[]` and
`attribute=` paths (now asked like any attribute). `x['real']` of a number also falls back to the attribute,
as Jinja2's getitem does (corpus case `methods.number_data_attributes_by_item`).

**Security review (2026-11-10, in place of a Codex security review, which this workspace's plan
does not offer).** One verifier pass over #794, #801, #803 and the follow-up PR, run against
REVIEW.md's untrusted-input rules. It covered about 700 sandbox probes against Jinja2 3.1.6, autoescape,
loaders, fallback and GIL/lifetime, plus an ASan+UBSan module run. It found no P0 and no sandbox, autoescape
or loader escape. Security review: 8 findings, 3 fixed, 5 declined or filed:
- Fixed in the follow-up PR: a refused attribute of a long string copied the whole string into Python on each
  read (now an empty stand-in of its type above 4096 characters); oldstyle i18n `%` formatting built a 1 GB
  width before the cap (now checked first); `PyHost::ItemAtTo` converted a borrowed list item (now held).
- Already in 0263 (engine growth checks; docs/security.md Known gaps): `lipsum(100000000)` ends in
  `bad_alloc` after 36 s; a lazy `[0] * 100000000` renders 300 MB; `range(10**7)|join` takes 15 s before the
  cap; a cap-sized `'{:>67108864}'.format(1)` takes 38 s; `('x' * N)|length` builds a list of characters.
- Filed as 0451 (P3 parity): `{% trans %}100%{% endtrans %}` prints `100%%` through the module; `range()`
  prints nothing instead of raising TypeError.
- Declined (P3): `'%1000000000d'|format(1)` is clamped to 2^20 characters where Python builds 1 GB. This is a
  safe divergence.
