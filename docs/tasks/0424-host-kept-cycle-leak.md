---
status: open
priority: low
area: robustness
depends: [0357]
touches: [src/cycle_collector.cpp]
shares: [test/public_host_object_test.cpp]
---
# A template container that holds itself leaks when a host keeps its export

**Problem.** Found by the 0357 verifier (LSan). A C++ callable that stores the `Value` it gets for
`{% set xs = [] %}{% do xs.append(xs) %}{{ keep(xs) }}` (or a dict that holds itself) keeps the
storage alive past the render, and when the host drops it the storage is never freed: the cycle
collector skips a container while an export holds a reference to it, and after the render nothing
collects it. Probably as old as the export views (0331/0348); not checked on master.

**Proposal.** Check master first. Then either let the host's last reference break the template's
own cycle (the export view clears the container it keeps alone, as `Clear` does for the
collector), or document that a host that keeps a self-holding container owns the cycle. Row in
`test/public_host_object_test.cpp` under the ASan job.
