---
status: open
priority: low
area: parity
issues: ["https://github.com/jinja2cpp/Jinja2Cpp/issues/256"]
touches: [python/]
---
# Python bindings, so Jinja2's own test suite can run against Jinja2C++

**Problem.** Issue #256 asks for Python bindings (pybind11/nanobind, scikit-build-core).
Beyond users who want a faster drop-in, the payoff for this project is a second oracle:
pallets/jinja's own test suite, adapted to call Jinja2C++, would test behaviour the
corpus does not reach (environment APIs, loaders, error types).

**Proposal.** A decision first: a prototype binding that renders `Environment`/`Template`
through Jinja2C++ and runs a subset of the upstream tests, with a report of what fails,
or won't do with a reason (build and packaging cost, wheel matrix). If done, the binding
stays a test tool until someone asks for a published wheel.

**Done when.** The decision is recorded here; if prototyped, a CI job runs the chosen
upstream test subset and its failures are filed as parity tasks.
