---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/907
priority: medium
area: perf
depends: []
touches: [CMakeLists.txt, src/recursion_guard.h, src/render_context.h, README.md, docs/python.md]
---
# The thread-locals of a shared library: initial-exec TLS model

**Problem.** The perf-track backlog (handoff 2026-11-14, item 3) noted that `t_renderLimits` and the
other hot thread-locals took the initial-exec model only in the Python module's build, and that a
shared library paid `__tls_get_addr` for them. Measured on master 9cfd891 (GCC 13.3, Release, pinned
name-hash key), a shared library cost 5-20% more instructions per render than the static build, 27% on
`Render/plain_text` and 39% on `RenderSink/plain_text`; `__tls_get_addr` alone was 11.6% of
`Render/hf_qwen3`. The hot-set attribute (`JINJA2CPP_HOT_TLS`) covered only three thread-locals and
removed about half of it; the rest was the workspace, the render counter, the cycle collector and
others read once per render or per call.

Variants counted against the static build (Render rows):

| Variant | per-node rows | plain_text | substitute |
|---|---:|---:|---:|
| shared, default (global-dynamic) | +4.7..+20.6% | +27.4% | +16.0% |
| TLS descriptors (`-mtls-dialect=gnu2`) | +1.8..+8.5% | +11.6% | +6.7% |
| initial-exec on the old hot set | +0.2..+6.3% | +29.2% | +17.0% |
| hot set + descriptors | +0.2..+3.3% | +13.6% | +7.9% |
| initial-exec for the whole library | +0.0..+1.8% | +3.2% | +1.9% |

**Change.**
- `JINJA2CPP_TLS_INITIAL_EXEC` builds the whole library target with `-ftls-model=initial-exec`
  (GCC/Clang on ELF) instead of tagging three variables, and is ON by default for a shared library
  built against glibc (configure check for `__GLIBC__`). musl refuses initial-exec relocations in a
  library loaded with `dlopen` at all (Codex on #907), so there it stays opt-in. The per-variable `JINJA2CPP_HOT_TLS` attribute is gone; `JINJA2CPP_HOT_TLS_INITIAL_EXEC` still
  moves the 3 KB lookup cache to the heap, so the library's TLS block is 656 bytes. The configure
  error for a shared library with the option is gone. The Python module's static copy gets the whole
  library model too (it already needed the whole block in static TLS: a module with any initial-exec
  access is flagged `STATIC_TLS`, and the loader then places its entire TLS block there).
- A shared library on ELF is built with `-fno-semantic-interposition`: calls between its own exported
  functions go direct. `Load/plain_text` -4%, `Load/substitute` -2%, chat Render rows -0.6%.
- README documents the option and its failure mode; docs/python.md no longer says a shared library
  must go without it.

**Why not TLS descriptors.** They need no static TLS, but glibc before 2.40 clobbered vector registers
in `_dl_tlsdesc_dynamic` (glibc bug 31372), the path a library loaded with `dlopen` takes: a silent
corruption on unpatched systems, against a loud `dlopen` failure for initial-exec.

**Failure mode.** A library loaded with `dlopen` takes its static TLS from the loader's reserve. If
other libraries used it up, `dlopen` fails with "cannot allocate memory in static TLS block"; building
with `-DJINJA2CPP_TLS_INITIAL_EXEC=OFF` avoids it. 656 bytes is what the wheel already needed under
CPython's `dlopen`; `ctypes.CDLL` of the shared library loads on glibc 2.39, also with
`GLIBC_TUNABLES=glibc.rtld.optional_static_tls=0`.

**Result** (shared after vs static, the same count): per-node Render rows +0.0..+1.9%, Load rows
+0.3..+0.9%, `Render/plain_text` +4.5%, `Render/substitute` +2.7%. The static build is unchanged (the
option stays OFF and no source changed on that path).

**Remaining.** What is left on the small rows is the shared library's call overhead (PLT calls into
libstdc++ the static build also pays, `LookupCache::ForThisThread` no longer inlined across the heap
check). macOS (Mach-O TLV) and Windows are untouched; their TLS access has its own cost, not measured.

**Acceptance.** Shared ELF builds use initial-exec by default with an opt-out; the static build's
count is unchanged; shared and static test suites pass; the Python module job passes.
