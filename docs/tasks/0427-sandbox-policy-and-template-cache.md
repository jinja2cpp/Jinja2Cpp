---
status: open
priority: low
area: api
depends: [0354, 0357]
touches: [src/template_env.cpp, include/jinja2cpp/template_env.h]
shares: [docs/design/]
---
# Sandbox policy and the template cache (for 0357)

**Problem.** `TemplateEnvImpl::SetSandboxPolicy` and `SetDialect` replace the environment's `LoadSettings`
but do not flush its template cache: templates loaded earlier keep the policy and dialect they were loaded
with, and `get_template` returns them. The Python module makes the policy once per environment and rebuilds
the engine environment when it changes, so it never sees this; a C++ embedder that changes the policy of a
live environment would. 0357 makes the policy public, so the behaviour becomes a contract.

**Proposal.** Decide one of: the policy is fixed when the environment is made (setter only before the first
load, else an error); changing it flushes the caches; or it is documented that a template keeps the policy
it was loaded with (as it keeps the delimiters). Whichever is chosen, `TemplateEnvImpl::IsEqual` and the
Python module's per-load refresh of the public-names flag follow it.

**Done when.** The decision is in `docs/design/0357` and a C++ test pins it.
