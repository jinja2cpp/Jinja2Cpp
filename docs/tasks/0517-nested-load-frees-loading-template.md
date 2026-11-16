---
status: open
pr:
priority: medium
area: robustness
depends: []
touches: [src/template.cpp, src/template_module.cpp, src/render_workspace.h, test/template_api_test.cpp]
---
# Two Loads from host code inside a template's first Load free the template being loaded

**Problem.** Found by the verifier on #910 (0515); master 3f66175 has the same structure. The first
Load of a template parses in place (`LoadsInPlace`, src/template.cpp). Host code called during that
Load, such as `Settings::autoescapeSelector`, may load the same `Template` again:

- The first nested Load also runs in place and succeeds, so the template now has a tree.
- A second nested Load therefore takes `LoadReplacing` (src/template_module.cpp). It replaces the
  `BasicTemplate`'s impl and hands the old one to `RenderWorkspace::Retire`, which keeps it only
  while a render runs on the thread (`s_renders != 0`, src/render_workspace.h).
- The old impl is the one the outer Load is running on. It is freed, and the outer Load reads
  `m_settings` from freed memory (heap-use-after-free under ASan).

Reproducer:

```cpp
TemplateEnv env;
std::optional<Template> tpl;
bool ran = false;
env.GetSettings().autoescapeSelector = [&](const std::string&) {
    if (!ran) { ran = true; tpl->Load("a{{ x }}"); tpl->Load("c{{ x }}"); }
    return false;
};
tpl.emplace(&env);
tpl->Load("OUT{{ x }}");
```

Only host code reaches it, not template text or data.

**Fix direction.** Keep the impl alive across an in-place Load without making every Load pay an
atomic reference count, for example by having `LoadReplacing` retire an impl that is still loading
(`TemplateImpl::m_loading`, added in 0515) until that Load returns. Count `Load/plain_text` before and
after.

**Acceptance.** The reproducer as a regression test in test/template_api_test.cpp, clean under
address+undefined sanitizers. No Load row over the bar.
