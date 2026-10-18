---
status: open
priority: low
area: docs
depends: [0239]
touches: ["site:playground/", .github/workflows/wasm.yml]
---
# A browser playground built with WebAssembly

Paths prefixed `site:` are in jinja2cpp/jinja2cpp.github.io.

**Problem.** Trying Jinja2C++ needs a C++ toolchain and a build. A parity-focused engine
gains a lot from letting users paste a template and a JSON context and see the output
next to what Python Jinja2 gives.

**Proposal.** Build the library with Emscripten in CI and serve a playground page
(template, JSON context, rendered output or error), pinned to a release tag. Optionally
show Python Jinja2's output through Pyodide for comparison. Not a 2.0 condition.

**Done when.** The playground page is served from a CI-built wasm artifact of a tagged
release and renders the getting-started examples.
