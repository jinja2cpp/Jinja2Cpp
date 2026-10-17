---
status: open
priority: low
area: parity
depends: [0197]
touches: [src/internal_value.h, src/internal_value.cpp, include/jinja2cpp/value.h]
---
# Objects as dict keys and through user callables

**Problem.** 0197 left the two places where namespace(), cycler() and joiner() objects
still lose their identity, because both go through a representation that has no object
kind:
- A dict key is stored as a string (0036), so `{{ {namespace(): 1} }}` renders `{'': 1}`;
  Python hashes the object by identity and prints `{<Namespace {}>: 1}`
  (`globals.object_dict_key`). The 0197 brief expected a TypeError; Jinja2 3.1.6 accepts it.
- An object passed through a user callable comes back as a plain map: `ident(c) == c` is
  false and `ident(namespace(a=1))|tojson` is `{"a": 1}`, because the public `Value` has no
  object kind (no corpus case: the corpus has no user callables that return their argument).

**Proposal.** Either give non-string keys a representation (a key that keeps the
InternalValue alongside its printed form), or reject an object key with a TypeError as a
deliberate divergence. For callables, let `Value` carry the original InternalValue of an
object it was converted from, so that converting back restores it; this is a public-API
question for the 2.0 Value design (0072).

**Done when** `globals.object_dict_key` matches or is marked deliberate, and an object
returned from a user callable compares equal to the one passed in.
