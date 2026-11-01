---
status: open
priority: medium
area: parity
depends: [0049]
touches: [src/internal_value.cpp#GenericMapAdapter, include/jinja2cpp/json_adapter.h, test/parity/divergences/reflect.txt]
---
# A JSON object seen through Reflect behaves as a reflected struct, not as a dict

**Problem.** Found by the ParityReflect pass (0322). A JSON object reaches the template as a
dict when it is copied (`jinja2::ToValue`: an engine map, `MapAttrPolicy::MethodsFirst`), and
as a host object when it is viewed (`jinja2::Reflect`: a `GenericMapAdapter`, whose policy is
`KeysFirst` for every generic map, docs/tasks/0020). Python sees a dict either way. So the
same document renders differently depending on how the host passed it:

| Case | Python and the copy | The view |
|---|---|---|
| `subscripts.dot_shadows_method` `{{ d.items is callable }}` | `True` | `False`: the key `items` wins |
| `methods.dict_method_before_key` `{{ dd.items()\|list }}` with a key `items` | `[('items', 'own')]` | `'str' object is not callable` |
| `methods.attr_dict_method_before_key` `dd\|attr('items') is callable` | `T` | `F` |
| `filters.attr` `{{ users[0]\|attr('name') }}` | empty (getattr does not see keys) | `bob` |
| `filters.map_attr_filter` `map('attr', 'name')\|select('defined')` | `[0]` | `[4]` |
| `sequences.mapping_is_sequence` `{{ m is sequence }}` | `T` | `F` (only `MethodsFirst` maps are dicts) |

The policy is right for a reflected struct (`HasAttributes`), whose fields are attributes;
it is wrong for a JSON object, which the generic map adapter cannot tell apart today.

**Proposal.** Give the JSON adapter's object accessor (`json::ObjectAccessor`) a way to say it
is a dict, and let `GenericMapAdapter` answer `MethodsFirst` (and `HasAttributes() == false`)
for it. 0049's plan (D7 option c, section 9) adds exactly that: the exported marker interface
`IJsonNode` that P5 introduces for unpacking and identity; reuse it rather than adding a
second marker or an `IMapItemAccessor` virtual. If P5 is not taken up before the 2.0 freeze,
land the marker here with only the dict answer.

**Done when.** The six `reflect.*` lines naming 0370 in `test/parity/divergences/reflect.txt`
are gone, the conformance suite (`test/binding/conformance.h`) has a row for a key shadowing
a dict method on every binding, and a reflected struct keeps its keys-first lookup.
