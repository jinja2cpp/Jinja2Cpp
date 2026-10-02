---
status: in-progress
priority: high
area: parity
depends: [0001, 0012, 0014, 0015]
touches: [src/expression_evaluator.cpp#postfix, src/internal_value.h#variant, src/value_visitors.h#BinaryMathOperation, src/value_methods.cpp, src/value_methods.h, src/render_context.h#FindValueSlot, src/generic_adapters.h#IndexedEnumeratorImpl]
shares: [src/expression_evaluator.cpp, src/internal_value.cpp, src/internal_value.h, src/value_visitors.h]
---
# Python methods on str, list and dict values

**Problem.** Jinja2 does not sandbox by default, so templates call Python methods:
`s.strip()`, `s.split(',')`, `s.startswith('x')`, `'{}'.format(x)`, `'%s' % x`,
`d.items()`, `d.get('k', 'dflt')`, `l.append(x)`. LLM chat templates (Hugging Face
`chat_template`) depend on these heavily, and they are a main reason projects choose
another C++ engine. In Jinja2C++ none of the 41 corpus cases works: calls on variables
render empty, calls on literals do not parse.

**Proposal.** Resolve `value.name(args)` against a table of builtin methods per value
kind when the value has no attribute of that name: the commonly used subset of `str`
(case, strip family, split/rsplit, join, replace, startswith/endswith, find, count,
format, isdigit/isalpha/..., zfill, center/ljust/rjust, partition), `list` (index,
count, append, pop, extend, insert) and `dict` (items, keys, values, get, setdefault,
update), plus the `%` operator for strings. Mutating methods need lists and dicts with
reference semantics inside a render; design that with the architect, it touches the
value model. Do not expose host-object methods beyond this whitelist.

**Done when.** No line of `test/parity/divergences/` names task 0020, and `ctest -R parity` passes.

**Next.** A chat-template corpus (a handful of real `chat_template` strings) is the
natural follow-up acceptance test.

**Design (architect plan, PR for 0020).**
- *Lookup.* The parser marks `x.name` as an attribute; `x.name` finds a method before the
  item, `x[name]` the item before the method (Jinja2's `getattr`/`getitem`). Maps carry a
  `MapAttrPolicy`: dict literals, kwargs and context mappings put methods first; host
  objects (reflected structs, JSON bindings) their keys first; objects stored as maps
  (loop, cycler, self, imported namespaces) expose no dict methods. `x.name(...)` calls
  the method directly with `x` evaluated once; an unknown method on a str, list or number
  raises `'str object' has no attribute 'x'`.
- *Reference semantics.* Lists and dicts the template builds keep their items behind a
  `shared_ptr`, so copies alias (identity, `sameas`, mutation through `with`, macros and
  loops). `|list` and `copy()` make new ones. Enumerators follow the live size, `GetItem`
  is bounds-checked. Borrowed (context) containers are copied on their first mutation and
  stored back along the receiver path (`RenderContext::FindValueSlot`); the external and
  global scopes are per-render copies, so the caller's data never changes. What this
  does not cover is task 0049.
- *Formatting.* `str.format` lives in `src/value_methods.cpp`; `%` reuses 0019's
  `PythonPercentFormat` (`src/python_format.cpp`) instead of a second formatter.
