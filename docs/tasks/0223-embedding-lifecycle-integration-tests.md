---
status: open
priority: high
area: robustness
touches: [test/embedding_lifecycle_test.cpp, CMakeLists.txt#JINJA2CPP_TEST_SOURCES]
---
# Test embedding behavior across lifetime, cache and callback boundaries

**Problem.** Recent fixes around imported modules, closures, self and stored super
calls (0165, 0167, 0198, 0207 and 0211) cross ownership and scope boundaries. An
isolated feature test can pass while its combination with caching, reentrant renders
or environment destruction reads dead state. The parity corpus's JSON context also
cannot represent all host-callable and loader interactions.

**Proposal.** Add a bounded scenario suite built from the supported public API:

- Repeated and concurrent renders of one loaded template with independent contexts,
  plus a host callable that renders another template in the same environment.
- Cached include/import/inheritance chains, including reload or cache eviction
  between renders and retained templates after TemplateEnv destruction.
- Host callables returning reflected nested data, errors and exceptions; verify
  ownership and documented error behavior after the callable has returned.
- Stored macro/self/super values crossing frame boundaries, including supported
  behavior when the defining frame has ended and frame storage is reused.

First write down the expected contracts from the API and existing decisions.
Do not introduce races by mutating settings or handlers during rendering: those
operations are documented as unsynchronized. Use deterministic synchronization,
bounded iterations and functional assertions; a no-crash loop is insufficient.
Run relevant scenarios under ASan/UBSan/LSan and concurrency scenarios under TSan
using the existing sanitizer CI. Keep Python-only parity cases in the corpus.

**Done when.** The scenarios assert output, error and lifetime behavior, execute in
the existing test runner, and the supported sanitizer configurations pass. Newly
found engine defects receive separate tasks and minimal reproducers; do not hide
them behind skips or turn this task into a rewrite of ownership/scoping.
