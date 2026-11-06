---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/778
priority: low
area: parity
depends: [0197]
touches: [src/template_dict.h, src/template_dict.cpp, src/internal_value.h, src/internal_value.cpp, src/value_visitors.h, include/jinja2cpp/user_callable.h, test/user_callable_test.cpp, test/binding/conformance.h, test/parity/cases/globals.py, test/parity/divergences/globals.txt, test/parity/divergences/reflect.txt]
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
- A JSON-bound object (boost/nlohmann/rapid bindings) is a GenericMap like a reflected
  struct, so `{% set x, y = j %}` keeps 0197's by-name extension for host objects and
  takes the values, where Python unpacks the keys of the dict it would be. Telling the two
  apart needs a flag on the public `IMapItemAccessor` (found by the 0197 verifier).

**Proposal.** Either give non-string keys a representation (a key that keeps the
InternalValue alongside its printed form), or reject an object key with a TypeError as a
deliberate divergence. For callables, let `Value` carry the original InternalValue of an
object it was converted from, so that converting back restores it; this is a public-API
question for the 2.0 Value design (0072).

**Done when** `globals.object_dict_key` matches or is marked deliberate, and an object
returned from a user callable compares equal to the one passed in, and a JSON object
unpacks its keys.

**Also owns 0049 P5 (2026-11-03).** 0049 closed and moved the JSON accessor identity here:
a JSON value viewed through `jinja2::Reflect` and read twice gives two accessors, so a write
through one read misses the other. The six `0212` lines in
`test/parity/divergences/reflect.txt` (the 0322 pass) are these cases; they go when a view has
a stable identity.

**Plan (approved 2026-11-05, all recommended decisions).** Architect plan in the project files
(release-2.0/0212-plan.md); three PRs:
- P1, object keys: every engine object is a key by its identity (`ObjectKey` in
  template_dict.cpp): namespace(), cycler(), joiner() and reflected structs by the identity their
  copies share, functions and macros by `Callable::GetIdentity` (also what `==` compares, so
  `l.append == l.append` holds as in Python), every undefined value as one key (StrictUndefined
  raises). Corpus rows `globals.object_key_*`, `macro_key`, `class_key`, `undefined_key*`,
  `bound_method_key`, `cycler_key`, `loop_key`. A string receiver of a method stands for itself
  by value (Python compares `__self__` by identity), a deliberate approximation.
- P2, round trip: an object a user callable is handed comes back as itself within that call (a
  per-call export table). "A macro arrives as None" was fixed by 0300. Built as `HostCallScope`
  in internal_value.cpp: each map or list argument is recorded by the accessor of the GenericMap
  or GenericList it was exported as, and a template callable by its lease; the call's result,
  a list the host built of those (one level), and the arguments of a call back to a lent
  callable come back as the template's own objects. The call's parameters hold each export until
  the call ends, so a raw accessor address suffices (the plan's `weak_ptr` is not needed).
  Limits, documented in user_callable.h: an object the host keeps and returns in a later call
  comes back as a view (copy-on-write), and Undefined comes back as None (D4).
- P3, JSON identity: `IJsonNode::NodeIdentity` (added by 0370) gives two reads of one JSON value
  one identity; the GenericList enumerator follows a cell made mid-loop; the six `0212` lines in
  reflect.txt go. PR #778; D6 (a) landed with 0370 (#752).
