---
status: open
priority: medium
area: robustness
touches: [src/template_impl.h, test/template_api_test.cpp]
---
# A nested render does not see a global the outer render changed in place

**Problem.** Found by the 0223 scenario suite (row F6, left out of the suite). The 0211 decision
says a global the template changes in place stays changed for that render and for renders nested
inside it, and never for later renders. The second half holds
(`TemplateApiTest.GlobalChangedInPlaceForOneRender`). The first does not: a host callable that
renders another template of the environment sees the global unchanged.

```c++
env.GetSettings().extensions.doStatement = true;
env.AddGlobal("g", jinja2::ValuesList{});
auto inner = env.FromString("{{ g|length }}").value();
env.AddGlobal("nested", jinja2::MakeCallable([&] { return inner.RenderAsString({}).value(); }));
// renders "{% do g.append(1) %}{{ g|length }}|{{ nested() }}"
```

Python prints `1|1` (the globals are one object); Jinja2C++ prints `1|0`. The nested render
takes the thread's globals snapshot, which the outer render's in-place change dropped
(`GlobalScopeWritten`), and converts the globals again from the environment.

**Proposal.** A render started inside another on the same thread, with the same environment,
uses the outer render's globals scope (its written copy) instead of a new snapshot; check 0049's
borrowed cells (P2), which change how the in-place copy is made.

**Done when.** The reproducer prints `1|1`, the next render still sees `g` empty, and row F6 joins
`test/embedding_lifecycle_test.cpp`.
