---
status: open
priority: low
area: parity
touches: [src/value_visitors.h#ValueRenderer, src/global_functions.cpp, src/serialize_filters.cpp#XmlAttrFilter]
shares: [test/parity/divergences/calls.txt]
---
# The builtin classes print as `<class 'range'>`

**Problem.** Found doing 0214. Jinja2's globals `range`, `dict`, `cycler`, `joiner` and
`namespace` are Python classes, and `str()` of a class is deterministic: `{{ range }}` renders
`<class 'range'>`, `{{ cycler }}` `<class 'jinja2.utils.Cycler'>`, and `xmlattr` prints the
escaped repr. Jinja2C++ prints nothing for any callable but a macro (`ValueRendererBase`
leaves them out because a Python function's repr names its address), and `xmlattr` drops them.
Pinned by `calls.global_class_str` and `calls.global_class_xmlattr`. `lipsum` is a function
in Python, whose repr has an address, so it stays as it is.

**Proposal.** Give the global callables a static repr text (a `const char*` beside the kind,
not an attribute, so `range.name` stays undefined), print it in `ValueRendererBase` and keep
it in `xmlattr`.

**Done when.** No line of `test/parity/divergences/` names 0305.
