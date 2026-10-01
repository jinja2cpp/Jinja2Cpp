---
status: open
priority: high
area: parity
depends: [0001, 0012]
touches: [src/internal_value.h#variant, src/value_visitors.h#ValueRenderer, src/internal_value.cpp#Subscript, src/render_context.h]
shares: [src/internal_value.h, src/internal_value.cpp, src/value_visitors.h, src/expression_evaluator.cpp, src/testers.cpp, src/value.cpp]
---
# Tell `None` apart from undefined

**Problem.** Jinja2 has two different "nothing" values: `None`, a real value that prints
as `None`, is `defined` and can be passed around, and `Undefined`, what a missing
variable or attribute evaluates to, which prints as empty and fails on most uses.
Jinja2C++ represents both as `EmptyValue`, so `{{ none_var }}` cannot print `None`
without also printing `None` for every typo, `n is defined` is false for a context
value that is `None`, and `default` cannot tell them apart. Four tasks need the
distinction (0012 printing, 0017 `none` test, 0019 `default`, 0026 undefined policies);
done separately they would each invent it in the value model at once.

**Proposal.** Keep the public `jinja2::Value` unchanged: `EmptyValue` there is `None`
(what a JSON `null` or a default-constructed `Value` means to the user). Inside, add an
undefined alternative to `InternalValueData` that only lookups produce (a missing name in
`RenderContext`, a missing attribute or item in `Subscript`), carrying the name for
later error messages. Then: undefined prints empty and `None` prints `None`; `is
defined`/`is undefined` test for the undefined kind; `none` literals (0013) and `None`
from the context are the None value; conversion back to `Value` maps undefined to
`EmptyValue`. Visitors that today special-case `EmptyValue` get the same case for
undefined unless Python treats them differently. The policies (strict, chainable) stay
in 0026. Start with an architect plan: the variant is used by every visitor in
`value_visitors.h`.

Cases: `output.none_var`, `output.none_in_concat`, the `tests.none*`/`defined` cases.

**Scheduling.** Runs after 0012, which rewrites the value printer this task extends.

**Done when.** No line of `test/parity/divergences/` names task 0034, and `ctest -R parity` passes.

**Next.** 0026 adds the undefined policies on top of the undefined value.
