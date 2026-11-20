---
status: open
priority: low
area: parity
depends: [0538]
touches: [python/src/, src/value_methods.cpp#SetMethods, src/filters.cpp#Indent]
shares: [test/parity/cases/operators.py, test/parity/divergences/operators.txt, python/tests/parity/module-divergences.txt]
---
# Sets, module host views and the indent message after 0538

**Problem.** 0538 made a dict's walks live and closed the edges of the engine's sets and mappingproxies the corpus
pinned. Three items of it stayed out of that PR, because each is its own piece of work and none of them blocks a
template that works in Jinja2 from working here (Jinja2 3.1.6):

1. `d.keys() | indent` fails with `'dict_keys' object has no attribute 'splitlines'`; Python's message is
   `unsupported operand type(s) for +=: 'dict_keys' and 'str'`. Both fail; message only.
2. In the Python module an engine view goes to Python as a list (CreateGenericList gives the host a snapshot of the
   view's kind), so a host function sees a list; a set (`d.keys() - []`) goes as a list too. A host dict's own views
   (the module's context is Python dicts) have no `-` in the engine, and their `mapping` is Python's mappingproxy as a
   host object, which `pprint` and `tojson` treat as a mapping (python/tests/parity/module-divergences.txt lines
   `operators.dict_view_set_*`, `operators.dict_view_mapping_proxy_*`). Ordering of two host views is 0381.
3. A `set` (0523) has only `isdisjoint`, `-` with another set or view, `in`, `len` and comparison; none of Python's
   other set methods (`union`, `intersection`, `add`, `|`, `&`, `^`, ...). `pprint` of a long set breaks it over lines
   as a list would, unsorted (Python sorts it).
4. (From 0538's item 9 work.) `{{ d | random }}` of a non-empty dict gives a key; Python's `random.choice` indexes it
   and raises `KeyError: 0` unless the dict has a key 0 (0538 made a mappingproxy do so, not a dict). And `pprint` of a
   mappingproxy wider than 80 columns still walks its dict for the line breaks, so a cycle through such a proxy hits
   the recursion limit where Python prints `{...}`.
5. (From 0540.) The engine's `range` is a class global: `{{ range.mro() }}` renders a list, where Jinja2's `range` is a
   function and the call is UndefinedError (in the module's sandbox `range` is Jinja2's `safe_range`). And
   `Markup.format` of a host object with `__html__` and a non-empty spec keeps the engine's formatting, where
   markupsafe raises that the object does not define `__html_format__` (not probed further).

6. (From 0538's verifier.) Live dict walks: a `loop` kept past a `break` (`set ns.l = loop`) collects the rest of
   the walk when the loop ends, so a dict changed in the body makes the loop fail with `changed size` where Python
   fails only if the kept loop is iterated; a kept loop's `length` after the dict was cleared reads the old size. An
   entry erased and added again under the walk reuses its node address on glibc, so the walk goes on silently
   (Python raises `keys changed`), or, as a new node, raises where CPython continues (its answer depends on compaction).
   An entry `loop.last`/`nextitem` peeked that the body erases and replaces fails the next step; Python goes on. Lazy
   pipelines (`e.keys() | map`, `e | items`, `e | reverse`) and loops over a mappingproxy still walk a snapshot.
7. (From 0538's verifier.) `{% set v = e.values() %}{% do e.update({'v': v}) %}{{ e | pprint }}` hits the recursion
   limit; Python prints `{'a': 1, 'v': dict_values([1, ...])}`. `Markup('x', encoding='utf-8')` (through
   `('a'|safe).__class__`) renders where Python raises `decoding str is not supported`. `(1).__class__.mro.__name__` is
   empty (Python `mro`). An `int()`/`float()` error quotes the whole text, as Python does; a cap would be kinder to logs.

**Done so far** (the Python module): item 2 (engine views and sets reach Python as their types, snapshots; a host dict's
views have the engine's set operators; a host mappingproxy prints and refuses `tojson` as Python's), the host dict in
item 4 (`d | random`), and the sandbox `range.mro()` of item 5.

**Proposal.** 1: the indent filter reports a non-string the way Python's `+=` does. 2: a host view type in the module
(python/src) that hands Python a real view or set of the engine's items, and the engine's set operators for a host
view, read through its items. 4: the random filter of a dict as of its mappingproxy; the wide pprint path takes the
container stack. 3: the set methods and operators over the engine's set (value_methods.cpp), with corpus
cases for each.

6-7: the kept loop collects what it can and fails only when iterated; pprint takes the container stack for views;
Markup's `encoding` refuses a str base.

**Done when** the module-divergences lines above are gone, items 4-7 have corpus cases that match or are recorded deliberate, the set methods have corpus cases that match in narrow and
wide runs, and `d.keys() | indent` reports Python's message.

**Decisions (2026-11-20, batch 11 C).** Public API: `IListItemAccessor::GetViewClass()` (virtual, default null)
and `GenericList::GetViewClass()` in jinja2cpp/generic_list.h, so a host learns that a lent list is a
`dict_keys`, `dict_values`, `dict_items` or `set` (as `IsTuple`, `GetObjectClass` do). Additive; adding a virtual
to `IListItemAccessor` changes its vtable, accepted while the G1 freeze is postponed. Item 1 already matched
(cases added). Item 5's `range.mro()` matches in a plain environment (the oracle's `range` is the builtin class);
in the module's sandbox it is `safe_range`'s UndefinedError. Deliberate lines (tag 0545): sets changed in a loop
over them (follow-up 0561), an entry erased and re-added on every pass, and the lazy pipelines and mappingproxy
loops that walk a snapshot.

**Verifier round 1 (2026-11-20).** Public API: `IHostObject::HtmlFormat(spec)` (virtual, default nullopt), what
`Markup.format` makes of a field with `__html_format__` or `__html__` as markupsafe's EscapeFormatter has it; the
Python module asks markupsafe's own formatter, so the order and the ValueError for a spec are Jinja2's. Additive
(a vtable change, as `GetViewClass`). Set methods count the hash tables they build against `maxRenderMemory` for the
call. Not fixed: `{{ l|pprint }}` of a list that holds itself fails with the recursion limit, where Python prints
`[<Recursion on list with id=...>]` (pprint's `_safe_repr`, not the `[...]` of `repr`); the output carries the
object's address, so no corpus case can match it.
