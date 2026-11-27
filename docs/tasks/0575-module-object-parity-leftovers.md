---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1025
priority: low
area: parity
depends: [0411]
touches: [python/src/template_module.cpp, python/src/convert.cpp]
shares: [python/tests/test_module.py, docs/python.md]
---
# What a live module object still lacks of Jinja2's, short of shared mutable data

**Problem.** After 0411 an exported `namespace`, `cycler` or `joiner` of `make_module` and `Template.module` is a live
view of the module's object, but some reads still differ from Jinja2:
- `isinstance(m.ns, jinja2.utils.Namespace)` (and `Cycler`, `Joiner`) is false: every one is a `ModuleObject`, and
  `callable()` is true of all of them.
- Each read of a nested namespace is a new Python object (`m.ns.inner is m.ns.inner` is false).
- A `ModuleObject` passed to a render prints as `{'count': 0}` where Jinja2 prints `<Namespace {'count': 0}>`, and
  `m.cyc.next` prints as a module macro.
- A cycler or joiner inside a list or dict argument of a macro (`m.f([m.cyc])`) reaches the macro as a lent map whose
  `next` is undefined; Jinja2 passes the working object.
- A C++ host's map of a dict with keys that spell alike (`{1: 'a', '1': 'b'}`): `GetItems()` has both entries but
  `GetSize()` and `GetKeys()` follow the `str()`-spelled lookup snapshot (1), so `{{ d|length }}` of such an export
  handed back to a render gives 1 where Jinja2 gives 2. Both `OwnedKeyedMap` (0411) and the render's `DictExportView`
  (src/internal_value.cpp) keep size equal to the keys, as `TemplateDictTest.PublicMapSpellsKeys` pins; fixing it means
  deciding that contract (Codex on #993). Python is not affected: it builds a dict from `GetItems()`.

**Not in scope, by the design principle (Ruslan, 2026-10-26: no Python-style shared mutable objects across renders; host
data immutable, renders independent; parity yields where it conflicts).** These stay deliberate divergences unless
Ruslan revisits the principle: Python setting an attribute of a module object (`m.ns.count = 10`, refused with
AttributeError), and exported lists and dicts shared with the module (a macro's `{% do l.append(1) %}` not showing in
`m.l`, Python's `m.l.append(2)` not reaching the macros). A module's own namespaces are live because the module is one
context whose macros already share them; the host's data stays copied. List them under deliberate divergences in
docs/python.md.

**Done when** pytest rows compare the reads above with Jinja2 (Python subclasses or `__class__` registered with
`jinja2.utils` types if that keeps `isinstance` true without importing Jinja2 at module load, a per-module cache of
nested views, the render repr), and docs/python.md lists the two out-of-scope cases as deliberate divergences.

## Outcome

- Fixed: the objects are `Namespace`, `Cycler` and `Joiner` classes of the module whose `__class__` is Jinja2's
  (`isinstance` holds; `jinja2.utils` is imported only when `__class__` is read), only a joiner is callable, nested
  namespaces and aliases are one Python object (a weak per-process cache by the engine object's identity),
  `m.cyc.next` and `m.cyc.reset` print as Jinja2's bound methods, a self-holding namespace prints `{...}`, and a module
  object passed to a render prints as `<Namespace {...}>` or `<jinja2.utils.Cycler object at ...>`
  (`IMapAccessor::GetHostObjectClass`, printing only).
- Recorded in docs/python.md: a cycler or joiner inside a list or dict argument of a macro stays a copy. Making it the
  module's object needs the host's list to hold an engine object where the module's cycle collector cannot walk it
  (`ns.l = [cyc]` with `cyc` holding `ns` would leak); `module.j.__call__` is the module class's own method.
- Kept: the C++ size contract of a map with keys that spell alike (`GetSize()` and `GetKeys()` follow the `str()`-spelled
  keys, `GetItems()` has every entry, `TemplateDictTest.PublicMapSpellsKeys`). Python builds its dict from `GetItems()`
  and is not affected; changing what `GetSize()` means for every C++ host is a public contract change for no Python gain.
- Found and filed: 0594 (a module object passed to another render is read there as a dict).
