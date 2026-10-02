---
status: open
priority: low
area: build
touches: [CMakeLists.txt]
---
# Warn when `JINJA2CPP_MSVC_RUNTIME_TYPE` is overridden by a shared build

**Problem.** `CMakeLists.txt` sets `JINJA2CPP_MSVC_RUNTIME_TYPE` to `/MD` whenever
`JINJA2CPP_BUILD_SHARED` is on, replacing whatever the user passed. Configuring with
`-DJINJA2CPP_BUILD_SHARED=ON -DJINJA2CPP_MSVC_RUNTIME_TYPE=/MT` builds against the DLL
runtime; the only trace is the "Selected MSVC runtime type" status line. The Windows CI
matrix carried two such rows for a long time, believing they tested `/MT` (0005).

**Proposal.** When the user set the cache variable to a static runtime (`/MT`, `/MTd`)
and asked for a shared build, emit `message(WARNING ...)` saying the value is ignored, or
fail with `FATAL_ERROR` if a static CRT in a DLL should be refused outright.

**Done when.** That configuration warns (or fails) at configure time.
