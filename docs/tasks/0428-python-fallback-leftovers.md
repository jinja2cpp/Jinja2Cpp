---
status: open
priority: low
area: api
depends: [0354]
touches: [python/jinja2cpp/fallback.py, python/jinja2cpp/environment.py, python/jinja2cpp/loaders.py, docs/python.md]
---
# Python fallback leftovers (from the review of 0354 PR3)

**Problem.** The review of the automatic fallback to Jinja2 (0354, plan D7) found eight small
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

5. **`context_class` assigned on an instance is ignored.** `env.context_class = Ctx` after construction does
   not make the environment Tier B, as it does in Jinja2 (only the class attribute is looked at, at construction).
6. **`import jinja2cpp` imports `jinja2`** eagerly (`exceptions.py` builds its classes on Jinja2's when it is installed), which costs start-up time and pulls
   Jinja2 into processes that never fall back. Import it at first use.

7. **A leak through the class cache.** `fallback._jinja2_classes` is a `WeakKeyDictionary` keyed by the
   jinja2cpp Environment class, but the generated Jinja2 class of an environment with a custom `context_class` or
   `code_generator_class` that points back at the environment class keeps its key alive: such dynamic classes
   are never freed.
8. **`unsafe_undefined` with `self.undefined`.** An overridden `unsafe_undefined` (or `getattr`/`getitem`) that
   makes its undefined with `self.undefined` gives a jinja2cpp Undefined, which Jinja2's runtime treats as a
   defined value, so the refusal does not happen on the fallback routes. Map it to the matching Jinja2 class.

**Proposal.** Fix each in `python/jinja2cpp/fallback.py` (and `environment.py` for the policies setter),
with a regression test in `python/tests/test_fallback.py`: copy the newstyle wrappers or raise
`UnsupportedError` naming newstyle gettext; validate that `policies` has every default key when it
is replaced; bound `_warned`; pass the owning jinja2cpp environment to the inner loader; read the instance's `context_class`
where it matters, or document that it is class-level; decide whether `jinja2` can be imported lazily (the exception classes subclass Jinja2's, so it may need a lazy-class design).

**Done when.** Each case has a test that fail before and pass after, and `docs/python.md`
("Fallback", what the fallback does not carry over) lists only what remains.

For 7, hold the generated class on the user's class (as the fallback template class is) instead of in a
global dict. For 8, convert a returned jinja2cpp Undefined into the Jinja2 class of the same kind.
