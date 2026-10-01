---
status: open
priority: medium
area: parity
touches: [include/jinja2cpp/template_env.h, include/jinja2cpp/user_callable.h, src/filters.cpp, src/testers.cpp, src/template_env.cpp]
---
# Register custom filters and tests

**Problem.** Jinja2 users extend the engine with `env.filters['name'] = fn` and
`env.tests['name'] = fn`, and post-process every printed value with `finalize`. Jinja2C++
can only add user callables as globals; using one as a filter needs the C++-only
`applymacro` filter. Porting a Jinja2 setup therefore means rewriting templates, which
conflicts with the goal of an API that is easy to embed.

**Proposal.** `TemplateEnv::AddFilter(name, UserCallable)` and `AddTester(name,
UserCallable)` looked up after the builtins (or before, to allow overrides: decide in
the plan), with the first argument bound to the piped value, plus an optional
`Settings::finalize` callable. Add corpus support for custom filters by registering a
few fixed Python/C++ pairs in the generator and the harness.

**Done when.** A template using a filter and a test registered from C++ renders the same
as Jinja2 with the equivalent Python functions, covered by a parity case.
