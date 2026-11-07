---
status: open
priority: low
area: api
depends: [0354]
touches: [python/jinja2cpp/fallback.py, python/jinja2cpp/environment.py, python/jinja2cpp/loaders.py, docs/python.md]
---
# Python fallback leftovers (from the review of 0354 PR3)

**Problem.** The review of the automatic fallback to Jinja2 (0354, plan D7) found four small
differences that were not fixed in the PR that introduced it:

1. **Newstyle gettext in the fallback.** A native environment with `newstyle_gettext=True` whose template
   falls back to Jinja2 gets an opaque error from Jinja2's runtime (the newstyle wrappers run in the
   engine and are not copied), where Jinja2 itself renders the template.
2. **`env.policies = {...}`.** Replacing the whole dict with one that lacks a default key renders in the
   fallback, where Jinja2 raises `KeyError` for the missing policy (`policies_reason` only compares the
   dict with the defaults). The two should agree.
3. **`_warned` grows without bound** in `warn` mode: it holds one string per distinct reason, and a
   reason includes the first line of an engine error (a source excerpt), so a long-lived process that
   loads many different unparsable templates keeps all of them. Bound it (an LRU, or key by the error kind).
4. **Loaders receive the Jinja2 environment.** Through `WrappedLoader`, a jinja2cpp loader's `get_source`
   is called with the delegate `jinja2.Environment` instead of the jinja2cpp one. A loader that reads
   `environment.undefined`, `environment.globals` or a subclass attribute sees Jinja2's.

**Proposal.** Fix each in `python/jinja2cpp/fallback.py` (and `environment.py` for the policies setter),
with a regression test in `python/tests/test_fallback.py`: copy the newstyle wrappers or raise
`UnsupportedError` naming newstyle gettext; validate that `policies` has every default key when it
is replaced; bound `_warned`; pass the owning jinja2cpp environment to the inner loader.

**Done when.** The four cases have tests that fail before and pass after, and `docs/python.md`
("Fallback", what the fallback does not carry over) lists only what remains.
