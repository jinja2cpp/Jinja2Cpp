---
status: open
priority: low
area: parity
touches: [src/value_visitors.h#ValueRenderer, src/global_functions.cpp, src/serialize_filters.cpp#XmlAttrFilter]
shares: [test/parity/divergences/calls.txt, test/parity/divergences/statements.txt]
---
# The builtin classes print as `<class 'range'>`

**Problem.** Found doing 0214. Jinja2's globals `range`, `dict`, `cycler`, `joiner` and
`namespace` are Python classes, and `str()` of a class is deterministic: `{{ range }}` renders
`<class 'range'>`, `{{ cycler }}` `<class 'jinja2.utils.Cycler'>`, and `xmlattr` prints the
escaped repr. Jinja2C++ prints nothing for any callable but a macro (`ValueRendererBase`
leaves them out because a Python function's repr names its address), and `xmlattr` drops them.
The same gap covers a bound method: `{{ loop.cycle|string }}` prints nothing where Jinja2 prints
`<bound method LoopContext.cycle of <LoopContext 1/2>>` (`statements.loop_cycle_string`, moved
here from 0214, which is done).
Pinned by `calls.global_class_str` and `calls.global_class_xmlattr`. `lipsum` is a function
in Python, whose repr has an address, so it stays as it is.

**Proposal.** Give the global callables a static repr text (a `const char*` beside the kind,
not an attribute, so `range.name` stays undefined), print it in `ValueRendererBase` and keep
it in `xmlattr`.

**Done when.** No line of `test/parity/divergences/` names 0305.

**Done.** `Callable::SetRepr` gives range, dict, cycler, joiner and namespace their class text
(global_functions.cpp `MakeClass`); `ValueRendererBase` prints it, and so do `xmlattr`,
`pprint`, `~`, `%` and `format`, which print through it. `loop.cycle` and `loop.changed` print
as bound methods through `Callable::SetBoundMethod(self, method, receiver)`: the receiver is the
`LoopCycleCell` the callables share with their loop (statements.cpp), which reads the live loop
while its frame holds it and keeps the last position and length after, so a kept callable does
not keep the loop (that would collect a filtered loop's rest at its end). The length after a
`break` out of a lazily filtered loop is unknown then: 0366.
