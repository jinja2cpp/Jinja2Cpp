---
status: open
priority: high
area: release
touches: [docs/project-support.md]
---
# Choose and verify the documentation and examples supported for 2.0

**Problem.** The engine and the public entry points describe different products.
At the 2026-10-17 review the website's build guide still advertised C++14-era
compilers, CMake 3.5, removed nonstd dependencies and submodule installation. The
main README still used find_package(jinja2cpp 1 REQUIRED). The examples-build and
Beast repositories date from 2020, conan-recipes from 2019, the selected recipe-index
fork from 2021, and tools-cli has no runnable implementation. Listing these without
a support policy suggests workflows that have not been validated for 2.0.

**Proposal.** Inventory the user-facing routes and assign each an owner and one of
maintained-for-2.0, historical/version-pinned, redirected, or candidate-for-archive.
Archival is a separate maintainer action, not something this task silently performs.

Keep the main library, a versioned 2.0 documentation set and a small executable
example set as the proposed supported surface. Coordinate README, MIGRATION.md and
website content changes through 0077, not a second competing rewrite. Track other
repositories' implementation PRs explicitly: task touches paths are relative to this
repository, not instructions to modify sibling checkouts.

Validate every advertised install command and example against the RC, including
error handling and a host-callable/reflection example. Clearly label the 1.x docs
and preserve useful historical links. Decide which package managers are supported;
do not advertise vcpkg availability without confirming a maintained port. The Beast
example's unconditional Ws2_32/Mswsock linkage needs a separate Linux support fix
before that workflow can be advertised as portable.

If LLM chat-template support is advertised, distinguish the current style-based
benchmarks from verbatim model templates. Select pinned, appropriately licensed
real templates and reference outputs, including the intended host globals and
tojson behavior, before making an interoperability claim.

**Done when.** docs/project-support.md lists supported routes, owners and validation
results; linked cross-repository PRs have their actual status; the public 2.0 docs
and examples agree on versions, toolchains and dependencies. Advertised snippets
compile/render against the RC, and historical projects are labeled or redirected
according to the recorded decision.
