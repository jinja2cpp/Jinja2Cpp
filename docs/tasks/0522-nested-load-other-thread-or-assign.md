---
status: open
pr:
priority: low
area: robustness
depends: [0517]
touches: [src/render_workspace.h, src/render_workspace.cpp, src/template_module.cpp, include/jinja2cpp/template.h]
---
# Host code inside a first Load that reloads on another thread, or assigns the template, still frees it

**Problem.** Left by 0517 (verifier r1 on its PR). 0517 keeps a template whose first Load runs
while nested Loads replace it, in the keep list of the thread that replaced it
(`RenderWorkspace::KeepWhileLoading`). Two host-code patterns still free the template the outer
Load parses into (heap-use-after-free reading `m_settings` in `TemplateImpl::Load`, on master and
after 0517):

- The autoescape selector runs the two nested Loads on a worker thread and joins it. The kept
  template sits in the worker's thread-local workspace, and `~RenderWorkspace` frees it when the
  worker exits, while the outer Load still runs on the first thread.
- The selector assigns another template to the one being loaded (`*tpl = other;`, the implicit
  copy assignment of `BasicTemplate`), which drops the impl the outer Load runs on, as
  `tpl.reset()` would.

Only host code reaches either, not template text or data. The second is close to destroying the
object whose member runs, which the API does not support.

**Proposal.** For the first, make the loading-kept list process-wide and mutex-guarded (only a
Load that replaces a loading template pays), or have the outer Load own a reference while
`m_loading` holds without making every first Load pay an atomic count. For the second, decide
whether to support it (the same keep in `operator=`) or document it as unsupported in
template.h.

**Done when** both patterns, as tests in test/template_api_test.cpp, are clean under
address+undefined sanitizers, or the second is documented as unsupported, and no Load row is over
the bar.
