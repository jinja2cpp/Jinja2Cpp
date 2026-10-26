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

**Progress (2026-10-25).** Contract: `docs/api-2.0.md` §3.10 (decisions D1-D7 of
/mnt/project-files/release-2.0/0223-plan.md, approved by Ruslan 2026-10-25), with notes in
`template_env.h`, `value.h` and `reflected_value.h`. Suite: `test/embedding_lifecycle_test.cpp`,
30 rows (A1-A4, B1-B6, C1-C6, D1-D4, E1, E2, E4-E6, F1-F5). The rows that fail on master land
with their fixes: A5 and D5 with 0321 (copy on load), E3 with 0318 (globals keep their snapshot),
A6 and C7 with 0264. Found while writing the suite: 0319 (`MakeCallable` with a `Value`
parameter), 0326 (nested macro closure, F5), 0323 (stored `self` and the includer's names, F2),
0324 (nested render and a global changed in place, F6), 0325 (reload date read after the text,
C6).
