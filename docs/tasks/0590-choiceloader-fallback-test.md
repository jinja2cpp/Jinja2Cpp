---
status: open
priority: low
area: parity
touches: [test/filesystem_handler_test.cpp, docs/parity.md#api-level]
---
# Pin `ChoiceLoader` behaviour: several handlers on one prefix

**Problem.** `TemplateEnv::LoadTemplate` tries the registered filesystem handlers in
registration order and the first that opens the name wins (`src/template_env.cpp`), which is
what Jinja2's `ChoiceLoader` does. No test pins it, so the `docs/parity.md` API-level row
stays 🟡 "not verified" (found by 0235).

**Proposal.** Add a test with two `MemoryFileSystem` handlers on the same (empty) prefix: a
name only in the second loads; a name in both loads the first's text; a name in neither is
`FileNotFound`; with the template cache on, a later change in the first handler wins over the
cached second one. Compare each with `jinja2.ChoiceLoader` through the corpus `templates`
option where it can express two loaders, otherwise by hand in the test comment.

**Done when.** The test passes and the `ChoiceLoader` row of `docs/parity.md` is ✅.

**Outcome.** Two tests in `test/filesystem_handler_test.cpp` pin the order, the fallback, `FileNotFound` and includes. The
proposal's last row was wrong: Jinja2 3.1.6 and this engine both keep the cached second handler's template when the
first handler later gains the name (the entry is checked only against the handler that loaded it); with `cacheSize = 0`
the first wins at once. The test pins that.
