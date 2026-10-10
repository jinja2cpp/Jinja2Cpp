---
status: open
priority: low
area: perf
depends: [0140]
touches: [src/internal_value.h, src/internal_value.cpp]
---
# Template constants pay an atomic refcount on every copy

**Problem.** After 0140 P2 a heap value (string, list, callable) is copied by one atomic
increment. A template's own constants are copied the most and shared by every thread that
renders the template, so their count is the most contended one. Yet the template outlives
every render that reads them.

**Proposal.** A "static" heap kind for `ConstantExpression` values (0118 P5c) that skips the
refcount. Before it can ship, a static value must turn into a refcounted copy wherever it can
outlive the template: `CreateGenericList`/`CreateGenericMap` capture `adapter = *this`
(internal_value.cpp:1048-1051) and reach user callables, so the host can keep a value after
the Template is destroyed. TemplateExpired does not cover that path.

**Done when.** Measured after 0140 P2 (count.py and MT wall clock): kept with numbers and an
ASan test of a constant escaping to a user callable, or dropped.
