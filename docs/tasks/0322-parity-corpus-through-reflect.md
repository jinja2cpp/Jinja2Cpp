---
status: open
priority: low
area: parity
depends: [0311]
touches: [test/parity/parity_test.cpp, test/parity/divergences/reflect.txt, .github/workflows/bindings.yml]
---
# Run the parity corpus with the context as a JSON view

**Problem.** Since 0311 S7 the parity corpus copies each case's context through
`jinja2::ToValue(ordered_json)`, so the whole corpus checks the copy path of the JSON
adapter. The view path (`jinja2::Reflect`) is checked only by the conformance suite in
`test/binding/conformance.h`, about a hundred rows. The corpus has a few thousand cases with
contexts, and a view answers every lookup, iteration, comparison and filter through the
generic map and list adapters, which the copy never reaches; a difference there between a
reflected and a copied document would not show up.

**Proposal.** A second pass of the corpus (an environment switch such as
`JINJA2CPP_PARITY_CONTEXT=reflect`, or a separate `ParityReflect` test) that passes each
context value as `jinja2::Reflect` of a shared `ordered_json` document instead of a copy. A
view is immutable, so cases that mutate the context (`list.append`, `dict.update`, `pop` and
the like) diverge by design; they get their own allow-list,
`test/parity/divergences/reflect.txt`, with a reason per line, and every other case must
match Python. Run it nightly in `bindings.yml` (and under ASan there, since it is a wide
lifetime check of the view's owner chain, tasks 0125 and 0311), not in the per-PR matrix.

**Done when.** The reflect pass runs nightly and is green; its allow-list holds only
mutation cases, each with a reason; any other difference found is fixed or filed.
