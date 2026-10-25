---
status: in-progress
priority: high
area: robustness
touches: [src/template.cpp, src/template_impl.h, src/template_module.cpp, include/jinja2cpp/template.h, include/jinja2cpp/error_info.h, test/template_api_test.cpp, test/template_module_test.cpp]
---
# Load replaces the template for its own object only

**Problem.** Found by the 0223 plan (probes 3, 4 and 13 in
/mnt/project-files/release-2.0/0223-probe/probe.cpp). Copies of a `Template`, the environment's
cache entry and the modules made from it share one `TemplateImpl`, and `Load` rebuilt that
object in place:
- a host callable that calls `Load` on the template that is rendering frees the tree under the
  running render (heap-use-after-free under ASan);
- `Load` on a template `LoadTemplate` returned changes the cache: later `LoadTemplate` calls and
  every `include` of that name render the host's source;
- `Load` on any copy makes the macros of every module made from any copy fail with
  `TemplateExpired`.

**Decision.** 0223 D1(a), approved by Ruslan 2026-10-25: copy on load, and a render pins its
root. Python's `Template` objects are immutable, so this is also the parity behaviour. It
reverses the second half of 0284 D1a: a module keeps its tree instead of expiring.

**Done when.** `Load` parses into a new template and puts it in place for that object only when
the parse succeeds (the first Load of a template no other object holds parses in place, as
`LoadTemplate`'s does); each render holds the template it renders; the module load generation
is gone. Rows: `TemplateApiTest.LoadReplacesOnlyThisObject` (0223 A5),
`TemplateApiTest.LoadOfTheRenderingTemplateFromACallable` (0223 D5),
`TemplateModuleTest.LoadKeepsModule`. `ErrorCode::TemplateExpired` stays for 0300 and 0301.
The `bench/count.py` Render gate holds (the pin is one `shared_ptr` copy per render).
