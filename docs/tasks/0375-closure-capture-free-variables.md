---
status: done
priority: low
area: perf
depends: [0326]
touches: [src/statements.cpp, src/statements.h, src/render_context.h, src/name_resolver.h, src/name_resolver.cpp, src/template_parser.cpp, src/template_parser.h, src/expression_parser.cpp, src/expression_parser.h]
---
# A kept closure copies only the names its body reads

**Problem.** Since 0326 a macro that outlives the frame it was defined in keeps a copy of that
frame: every name the frame's scopes hold, down to its template's top level
(`RenderContext::CollectNamesDownTo`). Python's closures keep only their free variables (Jinja2's
code generator closes over exactly the names the body reads: `env.compile(..., raw=True)` shows
the inner macro referencing `l_1_y` and nothing else). Copying the whole frame costs more than
Python in three ways:

- memory: a closure stored in a container of its own frame closes a cycle through the copy
  (copy, container, closure, capture) that Python does not have, freed only when the render ends
  (0373 item 4);
- time: the copy is made once per escaping frame and copied into a scope on each call through
  it (`EnterCapturedScope`), and each sibling macro in it gets its keep-alive back
  (`StrengthenCapture`): a kept closure in a frame of 40 macros pays about 40 value copies a call;
- the copy keeps values alive that no closure reads.

Found by the advisor consult of parity batch 2 group C (verifier round 3, finding d).

**Proposal.** The resolver already records each use with the frame it resolves in
(`NameResolver::AddUse`). At resolve time give each macro the hashes of the names its body (unit
subtree) reads from enclosing frames, kept in the arena; a superset is safe. The capture shared by
a frame's macros holds the union of their lists, and `CollectNamesDownTo` copies only those names.
A `self` site and a body that reaches the context another way (`m_bodyReachesContext`: `caller`,
`varargs`, an include or import with context in the body) keep the whole frame. Later, optional:
enter the copy as a read-only scope instead of copying it per call.

**Done when.** The corpus and the 0326 rows still match; a frame of 40 macros of which one is kept
makes one value copy a call through it (allocation count in a probe, as in 0326's measurements);
the 0373 item 4 template stays in constant memory when the stored closure does not read the
container; `bench/count.py` shows no regression.

**Resolution.** The resolver records the hash of every name read (`NameResolver::AddUse`, the
namespace of a `set ns.attr`, a filter or test that falls back to a value of its name, the
gettext function of a `trans`). A macro or call block keeps the sorted hashes its parameters and
body read, nested closures included (`MacroStatement::SetBodyReads`). `AddReadOfAll` stands for
every name: an include or import with context, a block, `applymacro`, and a `map`, `select`,
`reject`, `selectattr` or `rejectattr` whose filter or test is not a constant string. Each
`ClosureCapture` holds the union of the reads of the closures that share it, passes it on to the
frame below when its frame ends, and `CollectNamesDownTo` copies only those names. A `self` site
and an included template's top level keep every name (Jinja2's include context holds them all).

Measured (Debug probe): a frame of 40 macros of which one is kept costs 35k instructions a call
through it instead of 283k; the 0373 item 4 loop stays at 13 MB for 10k and 40k calls when the
stored closure reads another name than the container (21 and 58 MB before). A sibling in the
same frame that reads the container keeps it in the shared copy: still 0386.

A host callable taking `*context` in a kept closure now sees, of its ended frames, only the
names the closure reads; Python's `pass_context` sees none of a macro's locals.
Corpus: `statements.macro_closure_sets_attr_of_frame_namespace` and five more rows,
`loader.macro_closure_includes_with_frame_names`, `loader.macro_closure_imports_with_context`;
unit test `MacroTest.KeptClosureAppliesMacroByName`.
